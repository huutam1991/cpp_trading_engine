#pragma once

#include <utils/util_macros.h>
#include <jwt/jwt_lib/jwt.h>
#include <json/json.h>

class JWTManager
{
    Singleton(JWTManager);

private:
    std::string m_issuer = "issuer";
    std::string m_secret_key = "JWTManager02112022";
    size_t      m_expried_time = 24;

    // Default verifier
    jwt::verifier<jwt::default_clock, jwt::traits::kazuho_picojson> m_verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{m_secret_key})
        .with_issuer(m_issuer);

public:
    std::string generate_token(const Json& payload);
    std::string verify_token(const std::string& token);
    Json get_payload(const std::string& token);
    void update_verifier();

    JWTManager& set_issuer(const std::string& issuer);
    JWTManager& set_expried_time(size_t expried_time);
    JWTManager& set_secret_key(const std::string& secret_key);

};

#define VALID_TOKEN "valid_token"