#pragma once

#include <c_json/json.h>
#include <string>

class Account
{
public:
    static void save_account_to_db(const JsonNew& account);
    static JsonNew load_account_by_key(const std::string& key);

};
