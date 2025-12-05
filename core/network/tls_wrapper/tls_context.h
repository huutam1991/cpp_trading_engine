#pragma once

#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <spdlog/spdlog.h>

enum TlsType
{
    CLIENT,
    SERVER
};

class TlsContext
{
public:
    SSL_CTX* ctx = nullptr;
    TlsType type;

    TlsContext() = default;

    ~TlsContext()
    {
        cleanup();
    }

    // Disallow copying (SSL_CTX cannot be copied)
    TlsContext(const TlsContext&) = delete;
    TlsContext(TlsContext&& other) = delete;
    TlsContext& operator=(const TlsContext&) = delete;
    TlsContext& operator=(TlsContext&& other) = delete;

    // // Allow moving
    // TlsContext(TlsContext&& other) noexcept
    // {
    //     ctx = other.ctx;
    //     other.ctx = nullptr;
    // }

    // TlsContext& operator=(TlsContext&& other) noexcept
    // {
    //     if (this != &other)
    //     {
    //         cleanup();
    //         ctx = other.ctx;
    //         other.ctx = nullptr;
    //     }
    //     return *this;
    // }
protected:
    static void log_openssl_error(const std::string& msg)
    {
        spdlog::error("TLSContext: {}", msg);
        ERR_print_errors_fp(stderr);
    }

    // Init methods
    virtual bool init() = 0;

    void cleanup()
    {
        if (ctx)
        {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }
};

class TlsServerContext : public TlsContext
{
    std::string m_cert_file;
    std::string m_key_file;

public:
    TlsServerContext(const std::string& cert_file, const std::string& key_file);
    virtual bool init() override;
};

class TlsClientContext : public TlsContext
{
    bool m_verify_peer;
    std::string m_ca_file;

public:
    TlsClientContext(bool verify_peer = false, const std::string& ca_file = "");
    virtual bool init() override;
};