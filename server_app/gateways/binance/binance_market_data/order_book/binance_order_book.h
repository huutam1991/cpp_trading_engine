#pragma once

#include <map>
#include <queue>

#include <coroutine/task.h>
#include <coroutine/task.h>
#include <json/json.h>
#include <network/https_client_websocket/https_client_websocket.h>
#include <network/https_client_request/https_client_request.h>

#include <order_book/order_book_snapshot.h>
#include <order_book/order_book_manager.h>

class BinanceOrderBook
{
public:
    BinanceOrderBook(const std::string& symbol, size_t depth_level, EpollBase* event_base);

private:
    std::string m_symbol;
    const Instrument* m_instrument = nullptr;
    size_t m_depth_level;
    EpollBase* m_event_base;

    enum class SyncState
    {
        None,
        Buffering,
        Synced
    };
    SyncState m_sync_state = SyncState::None;

    std::shared_ptr<HttpsClientWebsocket> m_websocket;
    std::queue<Json> m_buffered_updates;

    Task<void> start_fetching_order_book();
    Task<void> re_fetch_order_book();
    Task<void> send_request_get_snapshot();
    void handle_order_book_update(Json update);
    bool process_buffered_updates_after_snapshot();
    void check_apply_update(Json& update);
    void apply_update(Json& update);

    // Order book data structure
    bool m_has_received_first_update = false;
    size_t m_package_last_update_id = 0;
    size_t m_snapshot_last_update_id = 0;
};