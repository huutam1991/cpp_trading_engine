#pragma once

#include <json/json.h>
#include <string>

class Account
{
public:
    static void save_account_to_db(const Json& account);
    static Json load_account_by_key(const std::string& key);
    static Json load_all_accounts();
};
