#pragma once

#include <string>
#include <unordered_map>
#include <queue>

#include <epoll_wrapper.h>
#include <thread_pool.h>
#include <https_server/route/route_controller.h>
#include <util_macros.h>

class HttpServer
{
private:
    int m_port;
    std::string m_dir_path;
    std::unordered_map<int, std::string> m_save_buffer_by_socket_id;

    int m_server_fd = -1;

    EPollWrapper* m_epoll = nullptr;
    ThreadPool*   m_thread_pool = nullptr;

public:
    HttpServer(int port, std::string dir_path);
    ~HttpServer();

    void init_socket();
    void start();

    virtual int accept_new_connection();
    virtual int read_buffer(int client_fd, char* const buffer);
    virtual void handle_client_request(int client_fd);
    virtual void close_connection(int client_fd);
    virtual void write_to_socket_io(int client_fd, const char* buffer, std::uint32_t size);
};

