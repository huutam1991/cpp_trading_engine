#include <account/account_db.h>
#include <mongo_db/mongo_db.h>
#include <app_constants.h>

void AccountDB::save_account_to_db(const Json& account)
{
    MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "account")
        .insert_one(account);
}

Json AccountDB::load_account_by_key(const std::string& key)
{
    return MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "account")
        .find_one("key", key);
}

Json AccountDB::load_all_accounts()
{
    Json accounts = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "account")
        .find_many();

    accounts.for_each([&](Json& account)
    {
        account.remove_field("_id"); // Remove MongoDB internal field
    });

    return accounts;
}