#include <gateways/gateway.h>

void Gateway::register_price_update(std::function<void(double)> price_update_callback)
{
    m_price_update_callback = price_update_callback;
}

Json Gateway::place(Order order)
{
    return Json();
}