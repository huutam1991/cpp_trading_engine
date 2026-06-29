#pragma once

#include <json/json.h>
#include <string>

class AccountDB
{
public:
    static void save_account_to_db(const Json& account);
    static void update_account_in_db(const std::string& key, const Json& account);
    static Json load_account_by_key(const std::string& key);
    static Json load_all_accounts();
};
