#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tls_context.h"

enum class TlsResult {
    OK = 0,
    WANT_IO = 1,
    ERROR = -1
};

class TlsWrapper
{
private:
    TlsContext* ctx = nullptr;

    SSL* ssl = nullptr;
    int fd = -1;
    bool handshake_done = false;

public:
    TlsWrapper(TlsContext* c);

    ~TlsWrapper()
    {
        shutdown_and_free();
    }

    SSL* get_ssl() const { return ssl; }
    bool attach_fd(int socket_fd);
    bool is_handshake_done() const { return handshake_done; }
    TlsResult handshake();
    int read(char* buf, int size);
    int write(const char* buf, int size);
    void shutdown_and_free();
};