#pragma once

#include <functional>
#include <queue>

#include <spdlog/spdlog.h>
#include <network/tls_wrapper/tls_wrapper.h>
#include <system_io/system_io_object.h>

struct HttpClientRequestIO : public NamedIOObject<HttpClientRequestIO>
{
    std::string hostname;
    std::string ip;
    int port;
    std::unique_ptr<TlsWrapper> m_tls_wrapper = nullptr;
    bool is_connected = false;
    std::queue<std::string> write_queue;


    enum State
    {
        CONNECTING_AND_HANDSHAKING,
        READING,
        WRITING,
        NONE
    };
    State current_state = State::NONE;

    HttpClientRequestIO(const std::string& hostname_value, int port_value);
    ~HttpClientRequestIO();

    // Set callback on disconnect
    std::function<void()> on_disconnect_callback = nullptr;
    void set_on_disconnect_callback(std::function<void()> callback);

    // Set callback on response received
    std::function<void(const char* buffer, std::uint32_t size)> on_response_received_callback = nullptr;
    void set_on_response_received_callback(std::function<void(const char* buffer, std::uint32_t size)> callback);

    // Get TLS context
    static TlsContext* get_tls_context();
    std::string resolve_hostname();

    // Handle data methods
    int check_connect_and_handshake();
    void write(std::string data);
    int handle_read_data();
    int read_buffer(char* const buffer);
    int check_to_write();
    int write_to_socket_io(const char* buffer, std::uint32_t size);

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;
};
