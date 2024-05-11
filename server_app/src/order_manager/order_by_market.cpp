#include <order_manager/order_by_market.h>

OrderByMarket::OrderByMarket(const std::string& db_name) : m_db_name(db_name)
{
}

std::string OrderByMarket::get_string_order_id(Json& order)
{
    long order_id = order["orderId"];
    return std::to_string(order_id);
}

Json OrderByMarket::transform_to_json_list()
{
    Json res = Json::create_array();

    for (int i = 0; i < m_order_list.size(); i++)
    {
        res.push_back((Json)m_order_list[i]);
    }

    res.sort([](Json& a, Json& b){
        return (long)a["transactTime"] > (long)b["transactTime"];
    });

    return res;
}

Json OrderByMarket::get_order_list(const std::string& market, const std::string& db_name, long from, long to)
{
    // Check to load order from DB
    if (m_from != from || m_to != to)
    {
        // Load order from DB
        bsoncxx::v_noabi::document::view_or_value filter = document{} <<
            "market" << market <<
            "transactTime" << open_document <<
                "$gt" << from <<
                "$lte" << to <<
            close_document << finalize;

        Json order_list = MongoDB::instance()
            .set_db_and_collection(db_name, "order")
            .find_many(filter);

        order_list.sort([](Json& a, Json& b){
            return (long)a["transactTime"] > (long)b["transactTime"];
        });

        // Erase all of available items
        m_order_list.erase(m_order_list.begin(), m_order_list.end());
        m_order_map.erase(m_order_map.begin(), m_order_map.end());

        order_list.for_each([this, &db_name](Json& order)
        {
            std::string _id = order["_id"]["$oid"];
            DataModel data_model(db_name, "order", _id);

            m_order_list.push_back(data_model);
            m_order_map[get_string_order_id(order)] = data_model;
        });

        m_from = from;
        m_to = to;
        m_db_name = db_name;
    }

    return transform_to_json_list();
}

Json OrderByMarket::get_order_by_id(long order_id)
{
    std::string order_id_str = std::to_string(order_id);

    if (m_order_map.find(order_id_str) != m_order_map.end())
    {
        return m_order_map[order_id_str];
    }

    return JsonNull();
}

void OrderByMarket::add_order(Json& order)
{
    DataModel data_model(m_db_name, "order");
    data_model = order;

    m_order_list.push_back(data_model);
    m_order_map[get_string_order_id(order)] = data_model;
}

void OrderByMarket::update_order(Json& order)
{
    std::string order_id = get_string_order_id(order);

    // Find data model in m_order_map
    if (m_order_map.find(order_id) != m_order_map.end())
    {
        m_order_map[order_id] = order;
    }
    // Otherwise, just add the order
    else
    {
        add_order(order);
    }
}