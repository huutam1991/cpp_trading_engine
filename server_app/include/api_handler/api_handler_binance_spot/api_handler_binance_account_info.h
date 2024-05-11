#ifndef API_HANDLER_BINANCE_ACCOUNT_INFO_H
#define API_HANDLER_BINANCE_ACCOUNT_INFO_H

#include <api_handler/api_handler_binance_spot/api_handler_binance.h>

/*  Get current account information.
    https://binance-docs.github.io/apidocs/spot/en/#account-information-user_data
*/
class APIHandlerBinanceAccountInfo : public APIHandlerBinance
{
public:
    APIHandlerBinanceAccountInfo(HttpRequest* request);

    static Json handle_internal_request(const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_new_request();
    Json handle_binance_request();
    void standardize_response(Json& response, Json& std_response);
};

#endif //API_HANDLER_BINANCE_ACCOUNT_INFO_H