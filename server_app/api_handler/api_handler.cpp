#include <api_handler/api_handler.h>
#include <jwt/jwt_manager.h>
#include <utils/utils.h>

APIHandler::APIHandler(HttpRequest* request) : m_request(request)
{
}

void APIHandler::add_mandatory_params(const std::vector<std::string>& params)
{
    m_mandatory_params = params;
}

void APIHandler::add_mandatory_body_params(const std::vector<std::string>& params)
{
    m_mandatory_body_params = params;
}

std::unordered_map<std::string, std::string> APIHandler::parse_cookie_header(std::string_view cookie_header) const
{
    std::unordered_map<std::string, std::string> cookies;
    size_t start = 0;

    while (start < cookie_header.size())
    {
        size_t end = cookie_header.find(';', start);

        if (end == std::string_view::npos)
        {
            end = cookie_header.size();
        }

        std::string_view pair = cookie_header.substr(start, end - start);
        size_t eq = pair.find('=');

        if (eq != std::string_view::npos)
        {
            std::string key = std::string(pair.substr(0, eq));
            std::string value = std::string(pair.substr(eq + 1));
            cookies[std::move(key)] = std::move(value);
        }

        start = end + 1;
    }

    return cookies;
}

std::string APIHandler::check_authentication()
{
    std::string res = VALID_TOKEN;

    std::string token = m_request->get_header_param("token");
    if (token == PARAM_NOT_FOUND)
    {
        token = m_request->get_header_param("Authorization");
        if (token == PARAM_NOT_FOUND)
        {
            std::string cookies_header = m_request->get_header_param("Cookie");
            spdlog::warn("APIHandler - Cookie header: [{}]", cookies_header);
            auto cookies = parse_cookie_header(cookies_header);
            for (const auto& [key, value] : cookies)
            {
                spdlog::warn("APIHandler - Cookie: [{}] = [{}]", key, value);
            }

            token = cookies.find("accessToken") != cookies.end() ? cookies["accessToken"] : PARAM_NOT_FOUND;

            spdlog::warn("APIHandler - Token from cookie: [{}]", token);

            if (token == PARAM_NOT_FOUND)
            {
                return "Missing token";
            }
        }
    }

    spdlog::warn("APIHandler - Token: [{}]", token);

    // Check valid token + get user_id from token
    std::string check_valid_token = JWTManager::instance().verify_token(token);
    if (check_valid_token != VALID_TOKEN)
    {
        return check_valid_token;
    }
    Json payload = JWTManager::instance().get_payload(token);
    std::string type = payload["type"];

    if (type != "user")
    {
        return "Not user token";
    }

    // Get User from user_id
    std::string user_id = payload["user_id"];
    // m_user = UserManager::instance().get_user_by_id(user_id);

    if (user_id != "root")
    {
        return "Not root user";
    }

    return res;
}

Task<HttpResponse> APIHandler::handle()
{
    // Check authentication
    std::string check_valid_token;
    if (m_need_check_authentication && (check_valid_token = check_authentication()) != VALID_TOKEN)
    {
        // LOG(ERROR) << "Authentication, " << check_valid_token;
        co_return HttpRequest::response_unauthorized_request_401(check_valid_token);
    }

    if (m_need_check_none_source)
    {

    }

    // Check missing params
    std::string missing_param = m_request->check_missing_params(m_mandatory_params);
    if (missing_param != PARAM_NO_MISSING)
    {
        co_return HttpRequest::response_bad_request_400(missing_param);
    }

    // Check missing body params
    missing_param = m_request->check_missing_body_params(m_mandatory_body_params);
    if (missing_param != PARAM_NO_MISSING)
    {
        co_return HttpRequest::response_bad_request_400(missing_param);
    }

    co_return co_await child_handle();
}