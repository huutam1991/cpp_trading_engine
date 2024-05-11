#ifndef API_HANDLER_BINANCE_BLVT_REDEEM_H
#define API_HANDLER_BINANCE_BLVT_REDEEM_H

#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt.h>

/*
    https://binance-docs.github.io/apidocs/spot/en/#redeem-blvt-user_data
*/
class APIHandlerBinanceBLVTRedeem : public APIHandlerBinanceBLVT
{
public:
    APIHandlerBinanceBLVTRedeem(HttpRequest* request);

    static Json handle_internal_request(Json& query_json);

protected:
    virtual HttpResponse child_handle();
    virtual Json send_new_request(Json& query_json);

    virtual std::string get_query_string(Json& query_json);

    void handle_query_json(Json& query_json);
    Json handle_binance_request(Json& query_json);
    void standardize_response(Json& response);
};

#endif //API_HANDLER_BINANCE_BLVT_REDEEM_H