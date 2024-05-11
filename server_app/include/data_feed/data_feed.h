#ifndef DATA_FEED_H
#define DATA_FEED_H

#include <memory>
#include <string>
//#include <functional>
#include <websocket/websocket_client.h>
#include <mongo_db/mongo_db_header.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
//#include <util_macros.h>
//#include <utils.h>
#include <app_constants.h>

class DataFeed
{
public:
    DataFeed(const std::string& db_name);
    ~DataFeed();

    void start() { this->init(); }
    virtual void init() = 0;

protected:
    std::string m_db_name;

    std::shared_ptr<WebsocketClient> m_websocket;

    //long m_back_testing_callback_id = -1;
    //virtual void back_testing_callback(bool is_on);
    
};

#endif //DATA_FEED_H
