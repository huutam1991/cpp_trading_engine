#include <order/simulator_order.h>

SavableObject<SimulatorOrder::SimulatorConfig>& SimulatorOrder::get_config()
{
    static SavableObject<SimulatorConfig> m_config{SavableObject<SimulatorConfig>::load_single_object("simulator_order", "config")};
    return m_config;
}

void SimulatorOrder::init()
{
    get_config();
    spdlog::info("SimulatorOrder initialized");
}