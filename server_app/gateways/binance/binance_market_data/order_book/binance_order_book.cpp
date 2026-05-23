#include <gateways/binance/binance_market_data/order_book/binance_order_book.h>
#include <iomanip>

BinanceOrderBook::BinanceOrderBook(const std::string& symbol, size_t depth_level, EpollBase* event_base)
    :   m_symbol{symbol},
        m_instrument{Instrument::get_instrument_by_exchange_symbol(ExchangeId::BINANCE, InstrumentType::PERPETUAL, symbol)},
        m_depth_level{depth_level},
        m_event_base{event_base}
{
    auto task = start_fetching_order_book();
    task.start_running_on(m_event_base);
}

Task<void> BinanceOrderBook::start_fetching_order_book()
{
    // wss://fstream.binance.com/public/stream?streams=btcusdt@depth
    std::string path = "/public/stream?streams=" + m_instrument->get_lower_case_exchange_symbol() + "@depth";

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_event_base, BINANCE_FUTURES_WS_URL, std::stoi(BINANCE_FUTURES_WS_PORT), path,
        // on_connect
        [this]() -> Task<void>
        {
            spdlog::info("BinanceOrderBook Websocket for symbol [{}] is connected", m_instrument->symbol);

            m_sync_state = SyncState::Buffering;

            // After websocket is connected, we send request to get snapshot, so we can apply the updates from websocket
            send_request_get_snapshot().start_running_on(m_event_base);

            co_return;
        },
        // on_message
        [this](std::string buffer) -> Task<void>
        {
            Json data = Json::parse(std::move(buffer));
            handle_order_book_update(std::move(data["data"]));

            co_return;
        },
        // on_disconnect
        [this]() -> Task<void>
        {
            spdlog::debug("BinanceOrderBook Websocket for symbol [{}] disconnected, re-starting...", m_instrument->symbol);
            re_fetch_order_book().start_running_on(m_event_base);

            co_return;
        },
        // on_close
        [this]() -> Task<void>
        {
            spdlog::debug("BinanceOrderBook Websocket for symbol [{}] closed", m_instrument->symbol);
            co_return;
        }
    );

    co_return;
}

Task<void> BinanceOrderBook::re_fetch_order_book()
{
    m_sync_state = SyncState::None;
    m_snapshot_last_update_id = 0;
    m_package_last_update_id = 0;

    // Clear the buffered updates, because we will re-fetch the snapshot and apply the new updates after that
    std::queue<Json> empty_queue;
    std::swap(m_buffered_updates, empty_queue);

    m_websocket = nullptr;

    // Re-start fetching order book
    co_await start_fetching_order_book();

    co_return;
}

Task<void> BinanceOrderBook::send_request_get_snapshot()
{
    // https://fapi.binance.com/fapi/v1/depth?symbol=BTCUSDT&limit=1000

    auto https_client_request = std::make_shared<HttpsClientRequest>(m_event_base, BINANCE_FUTURES_REST_URL, std::stoi(BINANCE_FUTURES_REST_PORT));
    HttpsClientResponse response = co_await https_client_request->get("/fapi/v1/depth?symbol=" + m_instrument->exchange_symbol.to_string() + "&limit=" + std::to_string(m_depth_level));

    Json data = Json::parse(response.body);
    m_snapshot_last_update_id = (size_t)data["lastUpdateId"];

    OrderBookSnapShotObject snapshot = OrderBookSnapShotPool::acquire();
    snapshot->update_instrument(m_instrument);

    data["asks"].for_each([snapshot](Json& level) mutable
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);

        snapshot->add_ask(price, quantity);
    });

    data["bids"].for_each([snapshot](Json& level) mutable
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);

        snapshot->add_bid(price, quantity);
    });

    spdlog::warn("snapshot for {}", m_instrument->symbol);
    snapshot->print_order_book();

    // Apply snapshot
    OrderBookManager::instance().publish_order_book_data(snapshot);

    if (process_buffered_updates_after_snapshot() == false)
    {
        re_fetch_order_book().start_running_on(m_event_base);
        co_return;
    }

    m_sync_state = SyncState::Synced;

    co_return;
}

void BinanceOrderBook::handle_order_book_update(Json update)
{
    if (m_sync_state == SyncState::None)
    {
        return;
    }

    if (m_sync_state == SyncState::Buffering)
    {
        m_buffered_updates.push(std::move(update));
        spdlog::warn("Buffering order book update for symbol [{}], buffered_updates.size()={}", m_instrument->symbol, m_buffered_updates.size());
    }
    else
    {
        // Apply the update to order book
        spdlog::debug("Applying order book update for symbol [{}]", m_instrument->symbol);
        check_apply_update(update);
    }
}

bool BinanceOrderBook::process_buffered_updates_after_snapshot()
{
    bool found_first_valid_update = false;

    while (m_buffered_updates.empty() == false)
    {
        Json update = std::move(m_buffered_updates.front());
        m_buffered_updates.pop();

        uint64_t U = update["U"];
        uint64_t u = update["u"];

        // old update, already included in snapshot
        if (u < m_snapshot_last_update_id)
        {
            continue;
        }

        // first update after snapshot must bridge snapshot lastUpdateId
        if (found_first_valid_update == false)
        {
            if (!(U <= m_snapshot_last_update_id && u >= m_snapshot_last_update_id))
            {
                spdlog::warn(
                    "Cannot find bridge update for symbol [{}], U={}, u={}, snapshot_last_update_id={}",
                    m_instrument->symbol,
                    U,
                    u,
                    m_snapshot_last_update_id
                );

                return false;
            }

            apply_update(update);
            m_package_last_update_id = u;
            found_first_valid_update = true;

            spdlog::info(
                "Found first valid buffered update for symbol [{}], U={}, u={}, snapshot_last_update_id={}",
                m_instrument->symbol,
                U,
                u,
                m_snapshot_last_update_id
            );

            continue;
        }

        uint64_t pu = update["pu"];

        if (pu != m_package_last_update_id)
        {
            spdlog::warn(
                "Buffered update chain broken for symbol [{}], pu={}, expected={}",
                m_instrument->symbol,
                pu,
                m_package_last_update_id
            );

            return false;
        }

        apply_update(update);
        m_package_last_update_id = u;
    }

    return found_first_valid_update;
}

void BinanceOrderBook::check_apply_update(Json& update)
{
    uint64_t pu = update["pu"];
    uint64_t u  = update["u"];

    if (u <= m_package_last_update_id)
    {
        return; // duplicate / old update
    }

    if (pu != m_package_last_update_id)
    {
        spdlog::warn(
            "Update chain broken for symbol [{}], pu={}, expected={}, re-fetching snapshot...",
            m_instrument->symbol,
            pu,
            m_package_last_update_id
        );

        re_fetch_order_book().start_running_on(m_event_base);
        return;
    }

    apply_update(update);
    m_package_last_update_id = u;
}

void BinanceOrderBook::apply_update(Json& update)
{
    // Apply asks
    update["a"].for_each([this](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);

        OrderBookUpdate update{
            .instrument = m_instrument,
            .side = OrderBookSideType::Ask,
            .type = quantity == 0.0 ? OrderBookUpdateType::Remove : OrderBookUpdateType::Update,
            .price = price,
            .quantity = quantity
        };

        OrderBookManager::instance().publish_order_book_data(update);
    });

    // Apply bids
    update["b"].for_each([this](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);

        OrderBookUpdate update{
            .instrument = m_instrument,
            .side = OrderBookSideType::Bid,
            .type = quantity == 0.0 ? OrderBookUpdateType::Remove : OrderBookUpdateType::Update,
            .price = price,
            .quantity = quantity
        };
        OrderBookManager::instance().publish_order_book_data(update);
    });
}