#include <jwt/jwt_manager.h>

std::string JWTManager::generate_token(const Json& payload)
{
    // Set issuer + type
    auto builder = jwt::create()
        .set_issuer(m_issuer)
        .set_type("JWS");

    // Add payload
    payload.for_each_with_key([&builder](const std::string& key, Json& value)
    {
        if (value.is_type<std::string>() || value.is_type<const char*>())
        {
            builder.set_payload_claim(key, jwt::claim((std::string&&)value));
        }
        else
        {
            builder.set_payload_claim(key, jwt::claim(value.get_string_value()));
        }
    });

    // Add exrired time + sign
    std::string token = builder
        .set_expires_at(jwt::date(std::chrono::system_clock::now()+ std::chrono::hours{m_expried_time}))
        .sign(jwt::algorithm::hs256{m_secret_key});

    return token;
}

std::string JWTManager::verify_token(const std::string& token)
{
    try
    {
        m_verifier.verify(jwt::decode(token));
    }
    catch (std::exception const& e)
    {
        ADD_LOG("Token verification error: " << e.what());
        return e.what();
    }

    return VALID_TOKEN;
}

Json JWTManager::get_payload(const std::string& token)
{
    Json payload;
    auto decoded = jwt::decode(token);
    std::string json_str = "{";

    int counter = 0;
    for(auto& e : decoded.get_payload_json())
    {
        if (counter++ > 0) json_str += ",";
        json_str += "\"" + e.first + "\":" + // key
            (e.second.is<std::string>() == false ? e.second.to_str() : "\"" + e.second.to_str() + "\""); // value
    }
    json_str += "}";

    return Json::parse(json_str);
}

void JWTManager::update_verifier()
{
    m_verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{m_secret_key})
        .with_issuer(m_issuer);
}

JWTManager& JWTManager::set_issuer(const std::string& issuer)
{
    m_issuer = issuer;
    update_verifier();
    return *this;
}

JWTManager& JWTManager::set_secret_key(const std::string& secret_key)
{
    m_secret_key = secret_key;
    update_verifier();
    return *this;
}

JWTManager& JWTManager::set_expried_time(size_t expried_time)
{
    m_expried_time = expried_time;
    return *this;
}