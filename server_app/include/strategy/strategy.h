#ifndef STRATEGY_H
#define STRATEGY_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>

class Strategy
{
    Singleton(Strategy)

public:
    void init();
};

#endif //STRATEGY_H