#ifndef API_HANDLER_BINANCE_ACCOUNT_INFO_H
#define API_HANDLER_BINANCE_ACCOUNT_INFO_H

#include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h>

/*  Get current account information. User in single-asset/ multi-assets mode will see different value, see comments in response section for detail
    https://binance-docs.github.io/apidocs/futures/en/#account-information-v2-user_data
*/
class APIHandlerBinanceFuturesAccountInfo : public APIHandlerBinanceFutures
{
public:
    APIHandlerBinanceFuturesAccountInfo(HttpRequest* request);

    static Json handle_internal_request(const std::string user_id = "root");

private:
    virtual HttpResponse child_handle();

    Json send_new_request();
    Json handle_binance_request();
    void standardize_response(Json& response, Json& std_response);
};

#endif //API_HANDLER_BINANCE_ACCOUNT_INFO_H