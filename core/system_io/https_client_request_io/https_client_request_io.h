#pragma once

#include <spdlog/spdlog.h>
#include <network/tls_wrapper/tls_wrapper.h>
#include <system_io/system_io_object.h>

struct HttpClientRequestIO : public SystemIOObject
{
    std::string hostname;
    std::string ip;
    int port;
    std::unique_ptr<TlsWrapper> m_tls_wrapper = nullptr;
    bool is_connected = false;

    HttpClientRequestIO(const std::string& hostname_value, int port_value);

    static TlsContext* get_tls_context();
    std::string resolve_hostname();

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int activate() override;
    virtual int handle_io_data() override;
    virtual void release() override;
};
