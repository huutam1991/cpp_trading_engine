#include <api_handler/api_handler_instrument/api_handler_instrument_subscribe.h>
#include <instrument/instrument.h>
#include <spdlog/spdlog.h>

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
        for (const Instrument* instrument : subscribed_instruments)
        {
            subscribed_instruments_json.push_back(instrument->to_json());
        }
        response["data"] = subscribed_instruments_json;
    }
    else if (m_request->get_request_method() == RequestMethod::POST)
    {
        std::string method = m_request->get_body_param_string("method");
        std::string exchange = m_request->get_body_param_string("exchange");
        std::string symbol = m_request->get_body_param_string("symbol");

        if (method == "subscribe")
        {
            if (Instrument::add_subscribed_instrument(enum_reflect::enum_value<ExchangeId>(exchange), symbol) == true)
            {
                response["data"] = "Subscribe instrument successfully";
            }
            else
            {
                response["data"] = "Failed to subscribe instrument, exchange: [" + exchange + "], symbol: [" + symbol + "], please check if the exchange and symbol are correct or already subscribed";
            }
        }
        else if (method == "unsubscribe")
        {
            if (Instrument::remove_subscribed_instrument(enum_reflect::enum_value<ExchangeId>(exchange), symbol) == true)
            {
                response["data"] = "Unsubscribe instrument successfully";
            }
            else
            {
                response["data"] = "Failed to unsubscribe instrument, exchange: [" + exchange + "], symbol: [" + symbol + "], please check if the exchange and symbol are correct or already unsubscribed";
            }
        }
        else
        {
            response["data"] = "Invalid field [method], only support [subscribe] or [unsubscribe]";
        }
    }

    // Response
    response["msg"] = "register account ";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
