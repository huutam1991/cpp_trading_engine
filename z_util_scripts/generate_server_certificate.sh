#!/bin/bash

# Check if OpenSSL is installed
if ! command -v openssl &> /dev/null; then
    echo "OpenSSL is not installed. Attempting to install..."

    # Check if the system is Debian/Ubuntu-based
    if [ -f /etc/debian_version ]; then
        sudo apt update
        sudo apt install -y openssl
    else
        echo "Automatic installation is not supported on this system. Please install OpenSSL manually."
        exit 1
    fi
else
    echo "OpenSSL is already installed: $(openssl version)"
fi

# Generate a 2048-bit RSA private key and save to file
openssl genrsa -out private_key_in_pkcs1.pem 2048

# Generate file [server-private-key.pem] from [private_key_in_pkcs1.pem]
openssl pkcs8 -topk8 -in private_key_in_pkcs1.pem -outform pem -nocrypt -out server-private-key.pem

# Create a certificate signing request (CSR) using the private key
openssl req -new -key server-private-key.pem -out request.csr \
-subj "/C=SG/ST=Singapore/L=Singapore/O=TamDev/OU=IT/CN=tam.com"

# Generate [server-certificate.crt] from [request.csr] + [server-private-key.pem]
openssl x509 -req -days 365 -in request.csr -signkey server-private-key.pem -out server-certificate.crt

# Delete files: [private_key_in_pkcs1.pem] + [request.csr]
rm -rf private_key_in_pkcs1.pem
rm -rf request.csr