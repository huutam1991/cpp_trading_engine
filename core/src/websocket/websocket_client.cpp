#include <websocket/websocket_client.h>

// #define DATA_BUFFER_SIZE 2048
#define DATA_BUFFER_SIZE 4096*4

WebsocketClient::WebsocketClient(const std::string& host, const std::string& port, std::string path = "/"):
    m_host(host), m_port(port), m_path(path)
{
}

WebsocketClient::~WebsocketClient()
{
    *m_is_shut_down = true;
}

void WebsocketClient::set_use_valid_data(bool use_valid_data)
{
    m_use_valid_data = use_valid_data;
}

void WebsocketClient::run()
{
    // Set m_is_shut_down = false;
    m_is_shut_down = std::make_shared<bool>(false);

    WebsocketClient::get_thread_pool().execute_function(
    [
        host = std::move(m_host),
        port = std::move(m_port),
        path = std::move(m_path),
        on_message = std::move(m_on_message),
        on_connect = std::move(m_on_connect),
        on_close = std::move(m_on_close),
        is_shut_down = m_is_shut_down,
        use_valid_data = m_use_valid_data
    ]() -> bool
    {
        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv12_client};

        // // This holds the root certificate used for verification
        // load_root_certificates(ctx);

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

        try
        {
            // Look up the domain name
            auto const results = resolver.resolve(host, port);

            if(SSL_set_tlsext_host_name(ws.next_layer().native_handle() ,host.c_str()) == false)
            {
                auto error_code = boost::beast::error_code(
                    static_cast<int>(::ERR_get_error())
                    ,boost::asio::error::get_ssl_category()
                );

                LOG(ERROR) << "Websocket resolve host: " << error_code << std::endl;
                on_close(websocket::close_code::protocol_error);
                return EXIT_FAILURE;
            }

            // Make the connection on the IP address we get from a lookup
            net::connect(ws.next_layer().next_layer(), results.begin(), results.end());

            // Perform the SSL handshake
            ws.next_layer().handshake(ssl::stream_base::client);

            // Set a decorator to change the User-Agent of the handshake
            ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
            }));

            // Perform the websocket handshake
            ws.handshake(host, path);

        }
        catch(std::exception const& e)
        {
            LOG(ERROR) << "Websocket error connection: " << e.what() << std::endl;
            on_close(websocket::close_code::protocol_error);
            return EXIT_FAILURE;
        }

        try
        {
            // Create WebsocketClientHandle
            WebsocketClientHandle wsh(ws);

            // Invoke on_connect
            if (on_connect != nullptr && *is_shut_down == false)
            {
                on_connect(wsh);
            }

            char data[DATA_BUFFER_SIZE];
            boost::system::error_code e;

            while (ws.is_open() && *is_shut_down == false)
            {
                beast::multi_buffer buffer;
                beast::error_code ec;
                // Read a message into our buffer
                ws.read(buffer, ec);
                if (ec == websocket::error::closed || *is_shut_down == true) {
                    break;
                }

                std::string strbuf = beast::buffers_to_string(buffer.data());
                on_message(strbuf, wsh);

                buffer.consume(buffer.size());

                // if (use_valid_data == false)
                // {
                //     int length = ws.read_some(boost::asio::buffer(data, DATA_BUFFER_SIZE), e);

                //     if(e)
                //     {
                //         ADD_LOG("WebsocketClient Error, throw exception");
                //         throw boost::system::system_error(e);
                //     }

                //     if (length > 0)
                //     {
                //         data[length] = '\0';

                //         if (*is_shut_down == false)
                //         {
                //             on_message(data, wsh);
                //         }
                //     }
                //     else
                //     {
                //         ADD_LOG("WebsocketClient Error, length <= 0");
                //         throw boost::system::system_error(e);
                //     }
                // }
                // else
                // {
                //     if (ws.next_layer().next_layer().available())
                //     {
                //         // This buffer will hold the incoming message
                //         // beast::flat_buffer buffer;
                //         beast::multi_buffer buffer;
                //         // Read a message into our buffer
                //         ws.read(buffer);

                //         if (buffer.size() > 0 && on_message != nullptr && *is_shut_down == false)
                //         {
                //             // on_message(beast::buffers_to_string(buffer.data()), wsh);

                //             // while (true)
                //             // {
                //             //     auto msgBuffer = boost::beast::buffers_to_string(buffer.data());
                //             //     size_t newlinePos = msgBuffer.find('\n');
                //             //     if (newlinePos != std::string::npos)
                //             //     {
                //             //         // Process the complete message up to the newline character
                //             //         std::string message = msgBuffer.substr(0, newlinePos);
                //             //         on_message(message, wsh);
                //             //         // Consume the processed message and the newline character
                //             //         buffer.consume(newlinePos + 1);
                //             //     }
                //             //     else
                //             //     {
                //             //         // Incomplete message, break the loop and wait for more data
                //             //         break;
                //             //     }
                //             // }

                //             std::string strbuf;
                //             for ( const auto &it: buffer.data() )
                //             {
                //                 strbuf.append(static_cast<const char *>(it.data()), it.size());
                //             }

                //             on_message(strbuf, wsh);
                //         }

                //         buffer.consume(buffer.size());

                //         // buffer.consume(buffer.size());
                //     }
                // }
            }

            // Invoke on_close
            if (on_close != nullptr && *is_shut_down == true)
            {
                ADD_LOG("Invoke on_close as is_shut_down == true");
                on_close(websocket::close_code::normal);
            }

            return EXIT_SUCCESS;
        }
        catch(std::exception const& e)
        {
            LOG(ERROR) << "Websocket error streaming: " << e.what() << std::endl;

            // Invoke on_close
            if (on_close != nullptr)
            {
                LOG(ERROR) << "Close socket on error flow " << std::endl;
                websocket::close_code code = *is_shut_down == true ? websocket::close_code::normal : websocket::close_code::internal_error;
                on_close(code);
            }

            return EXIT_FAILURE;
        }
        catch (...)
        {
            LOG(ERROR) << "Websocket Default Exception" << std::endl;
            on_close(websocket::close_code::internal_error);
            return EXIT_FAILURE;
        }
    });
}

void WebsocketClient::on_message(std::function<void(const std::string&, WebsocketClientHandle&)> func)
{
    m_on_message = func;
}

void WebsocketClient::on_connect(std::function<void(WebsocketClientHandle&)> func)
{
    m_on_connect = func;
}

void WebsocketClient::on_close(std::function<void(websocket::close_code code)> func)
{
    m_on_close = func;
}

WebsocketClientHandle::WebsocketClientHandle(websocket::stream<beast::ssl_stream<tcp::socket>>& ws) : m_ws(ws)
{}

void WebsocketClientHandle::write(const std::string& buffer)
{
    m_ws.write(net::buffer(buffer));
}

void WebsocketClientHandle::close()
{
    m_ws.close(websocket::close_code::normal);
}