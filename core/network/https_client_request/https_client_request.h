#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <functional>

#include <coroutine/future.h>
#include <network/tls_wrapper/tls_wrapper.h>

class HttpsClientRequest
{
    TlsWrapper* m_tls_wrapper = nullptr;
    std::string m_hostname;
    int m_port;
    std::string m_ip;

public:
    HttpsClientRequest(const std::string& hostname, int port);

private:
    static TlsContext* get_tls_context();
    std::string resolve_hostname();
};