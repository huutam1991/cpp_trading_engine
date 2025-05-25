#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <constants.h>
#include <util_macros.h>
#include <https_server/http_server.h>
#include <request/http_request.h>
#include <external_request/external_request.h>
#include <util_macros.h>

HttpServer::HttpServer(int port, std::string dir_path) : m_port(port), m_dir_path(dir_path)
{
    signal(SIGPIPE, SIG_IGN);
    init_socket();
    m_epoll       = new EPollWrapper(m_server_fd);
    m_thread_pool = new ThreadPool(NUMBER_OF_HTTPS_SERVER_THREADS, "Server Pool");
}

HttpServer::~HttpServer()
{
    if (m_server_fd != -1)
    {
        shutdown(m_server_fd, SHUT_RDWR);
        LOG(INFO) << "Http server is closed" << std::endl;
    }

    delete m_epoll;
    delete m_thread_pool;
}

void HttpServer::init_socket()
{
    sockaddr_in addr;
    int reuse = 1;

    if ((m_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        LOG(INFO) << "socket: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        LOG(INFO) << "setsockopt: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    int buffer_size = 1024 * 1024; // 1 MB
    setsockopt(m_server_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(m_server_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_server_fd, (sockaddr*) &addr, sizeof(sockaddr)) == -1)
    {
        LOG(INFO) << "bind: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(m_server_fd, BACKLOG_SOCKET) == -1)
    {
        LOG(INFO) << "listen: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        LOG(INFO) << "Http server is listening on port: " << m_port << std::endl;
    }
}

void HttpServer::start()
{
    m_epoll->start_waitting
    (
        [&]()               { return accept_new_connection(); },
        [&](int client_fd)  { handle_client_request(client_fd); }
    );
}

int HttpServer::accept_new_connection()
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd;

    if ((client_fd = accept(m_server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1)
    {
        LOG(INFO) << "accept: " << std::strerror(errno) << std::endl;
    }
    else
    {
        LOG(INFO) << "Connection to " << inet_ntoa(client_addr.sin_addr) << " established (fd = " << client_fd << ")" << std::endl;

        int dwTimeout = 1000; // milliseconds
        setsockopt(client_fd, SOL_SOCKET,SO_RCVTIMEO, (void*)&dwTimeout, sizeof dwTimeout);
        setsockopt(client_fd, SOL_SOCKET,SO_SNDTIMEO, (void*)&dwTimeout, sizeof dwTimeout);

        int buffer_size = 1024 * 1024; // 1 MB
        setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
        setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

        int n;
        unsigned int m = sizeof(n);
        getsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, (void *)&n, &m);
        // ADD_LOG("client_fd = " << client_fd << ", Receive buffer = " << n);
        getsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, (void *)&n, &m);
        // ADD_LOG("client_fd = " << client_fd << ", Send    buffer = " << n);
    }
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    return client_fd;
}

void HttpServer::handle_client_request(int client_fd)
{
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    int read_bytes = 0;
    int buffer_length = 0;

    if ((read_bytes = read_buffer(client_fd, temp_buffer)) > 0)
    {
        memcpy(buffer + buffer_length, temp_buffer, read_bytes);
        buffer_length += read_bytes;

        while ((read_bytes = read_buffer(client_fd, temp_buffer)) > 0)
        {
            memcpy(buffer + buffer_length, temp_buffer, read_bytes);
            buffer_length += read_bytes;
        }

        if (read_bytes == 0)
        {
            ADD_LOG("Connection lost, fd = " << client_fd);
            // Clean save buffer
            m_save_buffer_by_socket_id[client_fd] = "";
            close_connection(client_fd);
            return;
        }

        buffer[buffer_length] = '\0';
    }
    else
    {
        ADD_LOG("Connection lost, fd = " << client_fd);
        // Clean save buffer
        m_save_buffer_by_socket_id[client_fd] = "";
        close_connection(client_fd);
        return;
    }

    std::string message = m_save_buffer_by_socket_id[client_fd] + std::string(buffer);
    HttpRequest* request = HttpRequest::CreateNewHttpRequest(message.c_str(), m_dir_path);

    // Check if request is nullptr (wrong format), return error 404
    if (request == nullptr)
    {
        std::string response = request->response_not_found_404().get_response_in_string();
        write_to_socket_io(client_fd, response.c_str(), response.size());

        // Clean save buffer
        m_save_buffer_by_socket_id[client_fd] = "";

        return;
    }

    // Post request is invalid format when it's is separated into 2 buffer, that's why we need to save the first buffer
    if (request->is_valid_format() == false)
    {
        m_save_buffer_by_socket_id[client_fd] += std::string(buffer);

        delete request;
        return;
    }

    // Otherwise, clean save buffer + and execute request
    m_save_buffer_by_socket_id[client_fd] = "";

    // Execute request on a single thread
    m_thread_pool->execute_function([this, request, client_fd]()
    {
        std::string response = RouteController::instance().handle_request_base_on_route(request);
        write_to_socket_io(client_fd, response.c_str(), response.size());

        delete request;
    });
}

int HttpServer::read_buffer(int client_fd, char* const buffer)
{
    return read(client_fd, buffer, BUFFER_SIZE);
}

void HttpServer::write_to_socket_io(int client_fd, const char* buffer, std::uint32_t size)
{
    write(client_fd, buffer, size);
}

void HttpServer::close_connection(int client_fd)
{
    m_epoll->del_client_fd(client_fd);
    close(client_fd);
}
