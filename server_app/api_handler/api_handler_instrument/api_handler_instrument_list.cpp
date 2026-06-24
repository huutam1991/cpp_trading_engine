#include <api_handler/api_handler_instrument/api_handler_instrument_list.h>
#include <spdlog/spdlog.h>
#include <mongo_db/mongo_db.h>
#include <enum_reflect/enum_reflect.h>
#include <instrument/instrument.h>

APIHandlerInstrumentList::APIHandlerInstrumentList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerInstrumentList::child_handle()
{
    Json data;

    std::vector<std::string> collections = MongoDB::instance().get_collection_name_list("instrument");
    for (const std::string& collection_name : collections)
    {
        if (collection_name != "subscribed_instruments")
        {
            ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>(collection_name);

            // Get instrument list for this exchange, only support PERPETUAL instrument type for now
            auto instruments = Instrument::get_instrument_list(exchange_id, InstrumentType::PERPETUAL);
            for (const auto& [symbol, instrument] : instruments)
            {
                data.push_back(instrument->to_json());
            }
        }
    }

    // Response
    Json response;
    response["instruments"] = data;
    response["msg"] = "Get instrument list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
