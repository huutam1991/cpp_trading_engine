#include <openssl/sha.h>

#include <app_utils/app_utils.h>
#include <app_constants.h>
#include <utils/utils.h>

bool AppUtils::is_long_number(const std::string& number_str)
{
    try {
        size_t pos;
        std::stol(number_str, &pos);
        return pos == number_str.length();
    }
    catch (const std::invalid_argument& ia)
    {
        return false;
    }
    catch (const std::out_of_range& oor)
    {
        return false;
    }
}

// Method to check if a string contains all of digits
// (Some orders placed manually using Iphone has [clientOrderId] like this: "ios_47d0a66fc34f421d8f56e4d4048bc8d4")
// (Which cause error when force cast to std::stoull)
bool AppUtils::is_all_digit(const std::string& str)
{
    for (char c : str)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }

    return true;
}

OrderId AppUtils::parse_order_id(const std::string& str)
{
    if (is_all_digit(str) == false)
    {
        return 0;
    }

    return std::stoull(str);
}

OrderId AppUtils::client_order_id_to_system_order_id(const std::string& client_order_id)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256(
        reinterpret_cast<const unsigned char*>(client_order_id.data()),
        client_order_id.size(),
        hash
    );

    uint64_t value = 0;

    // Get first 8 bytes of hash to create OrderId
    for (int i = 0; i < 8; ++i)
    {
        value = (value << 8) | static_cast<uint64_t>(hash[i]);
    }

    // Make sure the value is positive and fits in size_t
    value &= 0x7FFFFFFFFFFFFFFFULL;

    return value;
}

double AppUtils::round_up_quantity_by_instrument(Instrument* instrument, double quantity)
{
    std::string round_str_number = Utils::round_string_number(std::to_string(quantity), instrument->lot_size);

    return std::stod(round_str_number);
}

