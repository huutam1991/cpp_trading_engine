#!/bin/bash

#export GLOG_log_dir=path
# export GLOG_logbufsecs=0
#export GLOG_logbuflevel=-1

if [[ ! -f server-certificate.crt || ! -f server-private-key.pem ]]; then
    chmod 777 z_util_scripts/generate_server_certificate.sh
    ./z_util_scripts/generate_server_certificate.sh
fi

ulimit -c unlimited

if [[ "$PROD" == "true" ]]; then
    PORT=443
    WEBSOCKET_PORT=8443
else
    PORT=8080
    WEBSOCKET_PORT=8083
fi

echo "==== START http_server_cpp $(date '+%F %T') ===="
echo "PORT=${PORT}, WEBSOCKET_PORT=${WEBSOCKET_PORT}"
echo "PID(before exec)=$$"
echo "---------------------------------------------------------------"

./http_server_cpp "$PORT" "$WEBSOCKET_PORT" web_data

code=$?

echo "==== EXIT http_server_cpp $(date '+%F %T'), code=${code} ===="

if [[ $code -eq 139 ]]; then
    echo "LIKELY SIGSEGV"
elif [[ $code -eq 134 ]]; then
    echo "LIKELY SIGABRT / std::terminate / assert"
elif [[ $code -eq 137 ]]; then
    echo "LIKELY SIGKILL / OOM killer"
fi

exit $code