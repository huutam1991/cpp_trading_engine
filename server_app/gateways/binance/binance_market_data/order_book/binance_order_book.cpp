#include <gateways/binance/binance_market_data/order_book/binance_order_book.h>
#include <iomanip>

BinanceOrderBook::BinanceOrderBook(const Instrument* instrument, size_t depth_level, EpollBase* event_base)
    :   m_instrument{instrument},
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

    std::string url = BINANCE_FUTURES_WS_URL;
    int port = std::stoi(BINANCE_FUTURES_WS_PORT);

    if (m_instrument->exchange_id == ExchangeId::BINANCE_TESTNET)
    {
        url = BINANCE_TESTNET_FUTURES_WS_URL;
        port = std::stoi(BINANCE_TESTNET_FUTURES_WS_PORT);
    }

    m_websocket = std::make_shared<HttpsClientWebsocket>(m_event_base, url, port, path,
        // on_connect
        [this]() -> Task<void>
        {
            spdlog::info("BinanceOrderBook Websocket for symbol [{}] is connected", m_instrument->symbol);

            co_return;
        },
        // on_message
        [this](std::string buffer) -> Task<void>
        {
            m_trace_id = g_pipeline_trace_buffer.allocate();
            g_pipeline_trace_buffer.record_start_time<"market_data_received">(m_trace_id);

            if (m_has_received_first_update == false)
            {
                spdlog::info("Received first order book update for symbol [{}]", m_instrument->symbol);
                m_has_received_first_update = true;


                m_sync_state = SyncState::Buffering;

                // After websocket is connected, we send request to get snapshot, so we can apply the updates from websocket
                send_request_get_snapshot().start_running_on(m_event_base);
            }

            Json data = Json::parse(std::move(buffer));
            handle_order_book_update(std::move(data["data"]));

            g_pipeline_trace_buffer.record_end_time<"market_data_received">(m_trace_id);

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
    spdlog::error("Re-fetching order book snapshot for symbol [{}] due to missing update", m_instrument->symbol);
    m_sync_state = SyncState::None;
    m_has_received_first_update = false;
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

    std::string url = BINANCE_FUTURES_REST_URL;
    int port = std::stoi(BINANCE_FUTURES_REST_PORT);
    if (m_instrument->exchange_id == ExchangeId::BINANCE_TESTNET)
    {
        url = BINANCE_TESTNET_FUTURES_REST_URL;
        port = std::stoi(BINANCE_TESTNET_FUTURES_REST_PORT);
    }

    auto https_client_request = std::make_shared<HttpsClientRequest>(m_event_base, url, port);
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
        spdlog::warn("Failed to process buffered updates after snapshot for symbol [{}], re-fetching order book...", m_instrument->symbol);
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
            spdlog::warn(
                "Skipping old buffered update for symbol [{}], U={}, u={}, snapshot_last_update_id={}",
                m_instrument->symbol,
                U,
                u,
                m_snapshot_last_update_id
            );
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

    if (found_first_valid_update == false)
    {
        // no buffered updates newer than snapshot
        // snapshot itself is already latest

        // m_package_last_update_id = m_snapshot_last_update_id;

        spdlog::info(
            "Snapshot already up to date for symbol [{}], snapshot_last_update_id={}",
            m_instrument->symbol,
            m_snapshot_last_update_id
        );

        return true;
    }

    return true;
}

void BinanceOrderBook::check_apply_update(Json& update)
{
    uint64_t pu = update["pu"];
    uint64_t u  = update["u"];

    if (m_package_last_update_id != 0 && u <= m_package_last_update_id)
    {
        return; // duplicate / old update
    }

    if (m_package_last_update_id != 0 && pu != m_package_last_update_id)
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
    MeasureTime measure_time("BinanceOrderBook::apply_update");

    std::vector<OrderBookUpdate> updates;

    // Apply asks
    update["a"].for_each([this, &updates](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);

        updates.emplace_back(
            m_instrument,
            m_trace_id,
            OrderBookSideType::Ask,
            quantity == 0.0 ? OrderBookUpdateType::Remove : OrderBookUpdateType::Update,
            price,
            quantity
        );
    });

    // Apply bids
    update["b"].for_each([this, &updates](Json& level)
    {
        double price = std::stod((std::string)level[0]);
        double quantity = std::stod((std::string)level[1]);

        updates.emplace_back(
            m_instrument,
            m_trace_id,
            OrderBookSideType::Bid,
            quantity == 0.0 ? OrderBookUpdateType::Remove : OrderBookUpdateType::Update,
            price,
            quantity
        );
    });

    OrderBookManager::instance().publish_order_book_data(std::move(updates));
}