#include "tls_context.h"

// Server Context
TlsServerContext::TlsServerContext(const std::string& cert_file, const std::string& key_file)
    : m_cert_file{cert_file}, m_key_file{key_file}
{
    type = TlsType::SERVER;
    init();
}

bool TlsServerContext::init()
{
    // Use modern server method (TLS 1.2 / 1.3, no SSLv3)
    ctx = SSL_CTX_new(TLS_server_method());

    if (!ctx)
    {
        log_openssl_error("SSL_CTX_new(TLS_server_method) failed");
        return false;
    }

    // Load server certificate
    if (SSL_CTX_use_certificate_file(ctx, m_cert_file.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        log_openssl_error("Failed to load certificate file");
        return false;
    }

    // Load server private key
    if (SSL_CTX_use_PrivateKey_file(ctx, m_key_file.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        log_openssl_error("Failed to load private key file");
        return false;
    }

    // Ensure key matches certificate
    if (!SSL_CTX_check_private_key(ctx))
    {
        log_openssl_error("Private key does not match certificate");
        return false;
    }

    // Optional: Disable TLS1.0/1.1 (recommended for security)
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    return true;
}

// Client Context
TlsClientContext::TlsClientContext(bool verify_peer, const std::string& ca_file)
    : m_verify_peer{verify_peer}, m_ca_file{ca_file}
{
    type = TlsType::CLIENT;
    init();
}

bool TlsClientContext::init()
{
    ctx = SSL_CTX_new(TLS_client_method());

    if (!ctx)
    {
        log_openssl_error("SSL_CTX_new(TLS_client_method) failed");
        return false;
    }

    // CLIENT VERIFY MODE
    if (m_verify_peer)
    {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

        if (!m_ca_file.empty())
        {
            if (SSL_CTX_load_verify_locations(ctx, m_ca_file.c_str(), nullptr) != 1)
            {
                log_openssl_error("Failed to load CA file");
                return false;
            }
        }
        else
        {
            // Load default system CA (Linux)
            if (SSL_CTX_set_default_verify_paths(ctx) != 1)
            {
                log_openssl_error("Failed to load default CA paths");
                return false;
            }
        }
    }
    else
    {
        // No certificate validation (dev/debug mode)
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    return true;
}

