#include <account/account.h>
#include <mongo_db/mongo_db.h>
#include <app_constants.h>

void Account::save_account_to_db(const Json& account)
{
    MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "account")
        .insert_one(account);
}

Json Account::load_account_by_key(const std::string& key)
{
    return MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "account")
        .find_one("key", key);
}

Json Account::load_all_accounts()
{
    return MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "account")
        .find_many();
}