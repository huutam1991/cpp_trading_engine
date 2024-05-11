#ifndef PRICE_MANAGER_H
#define PRICE_MANAGER_H

#include <util_macros.h>
#include <json/json.h>

class PriceManager
{
    Singleton(PriceManager)

public:
    void        init();
    void        set_price(Json& price);
    long double get_price_by_symbol(const std::string& symbol);

private:
    Json m_price_list;
};

#endif //PRICE_MANAGER_H