#include <api_handler/api_handler_instrument/api_handler_instrument_list.h>
#include <spdlog/spdlog.h>
#include <mongo_db/mongo_db.h>
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
            Json instruments= MongoDB::instance()
                .set_db_and_collection("instrument", collection_name)
                .find_many();

            instruments.for_each([&data](Json& instrument)
            {
                instrument.remove_field("_id");
                data.push_back(instrument);
            });
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
