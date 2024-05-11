
#include <data_feed/data_feed.h>
#include <app_utils.h>
//#include <back_testing/back_testing.h>

DataFeed::DataFeed(const std::string& db_name): m_db_name(db_name)
{
    /*m_back_testing_callback_id = BackTesting::instance().register_callback_back_testing_mode([this](bool is_on)
    {
        back_testing_callback(is_on);
    });*/
}

DataFeed::~DataFeed()
{
    //BackTesting::instance().unregister_callback_back_testing_mode(m_back_testing_callback_id);
}

/*void DataFeed::back_testing_callback(bool is_on)
{
    start();
}*/
