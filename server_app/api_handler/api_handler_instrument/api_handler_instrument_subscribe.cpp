#include <api_handler/api_handler_instrument/api_handler_instrument_subscribe.h>
#include <instrument/instrument.h>
#include <spdlog/spdlog.h>

#include <gateways/gateway_manager.h>

APIHandlerInstrumentSubscribe::APIHandlerInstrumentSubscribe(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerInstrumentSubscribe::child_handle()
{
    Json response;

    if (m_request->get_request_method() == RequestMethod::GET)
    {
        std::vector<const Instrument*> subscribed_instruments = Instrument::get_subscribed_instruments();

        Json subscribed_instruments_json;
        subscribed_instruments_json.set_size(0);

        for (const Instrument* instrument : subscribed_instruments)
        {
            subscribed_instruments_json.push_back(instrument->to_json());
        }

        response["data"] = subscribed_instruments_json;;
        response["msg"] = "Get subscribed instruments successfully";
    }
    else if (m_request->get_request_method() == RequestMethod::POST)
    {
        std::string method = m_request->get_body_param_string("method");
        std::string exchange = m_request->get_body_param_string("exchange");
        std::string symbol = m_request->get_body_param_string("symbol");
        ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>(exchange);

        auto gateway = GatewayManager::instance().get_gateway(exchange_id);
        const Instrument* instrument = Instrument::get_instrument_by_symbol(exchange_id, symbol);

        if (method == "subscribe")
        {
            if (Instrument::add_subscribed_instrument(exchange_id, symbol) == true)
            {
                response["msg"] = "Subscribe instrument with exchange: [" + exchange + "], symbol: [" + symbol + "] successfully";
                response["data"] = nullptr;

                gateway->subscribe_instrument(instrument);
            }
            else
            {
                response["msg"] = "Failed to subscribe instrument with exchange: [" + exchange + "], symbol: [" + symbol + "], please check if the exchange and symbol are correct or already subscribed";
                response["data"] = nullptr;
            }
        }
        else if (method == "unsubscribe")
        {
            if (Instrument::remove_subscribed_instrument(exchange_id, symbol) == true)
            {
                response["msg"] = "Unsubscribe instrument with exchange: [" + exchange + "], symbol: [" + symbol + "] successfully";
                response["data"] = nullptr;

                gateway->unsubscribe_instrument(instrument);
            }
            else
            {
                response["msg"] = "Failed to unsubscribe instrument with exchange: [" + exchange + "], symbol: [" + symbol + "], please check if the exchange and symbol are correct or already unsubscribed";
                response["data"] = nullptr;
            }
        }
        else
        {
            response["msg"] = "Invalid field [method], only support [subscribe] or [unsubscribe]";
            response["data"] = nullptr;
        }
    }

    // Response
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
