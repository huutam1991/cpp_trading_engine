#pragma once

#include <spdlog/spdlog.h>
#include <network/tls_wrapper/tls_wrapper.h>
#include <system_io/system_io_object.h>

struct HttpClientRequestIO : public SystemIOObject
{
    std::string ip;
    int port;
    TlsWrapper* m_tls_wrapper = nullptr;
    bool is_connected = false;

    HttpClientRequestIO(const std::string& ip_value, int port_value, TlsWrapper* tls_wrapper);

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();
    virtual void release();
};
