#!/bin/bash

#export GLOG_log_dir=path
# export GLOG_logbufsecs=0
#export GLOG_logbuflevel=-1

chmod 777 z_util_scripts/generate_server_certificate.sh
./z_util_scripts/generate_server_certificate.sh

ulimit -c unlimited

# Detect port
if [[ "$PROD" == "true" ]]; then
    PORT=443
    WEBSOCKET_PORT=8443
else
    PORT=8080
    WEBSOCKET_PORT=8083
fi

echo "Starting http_server_cpp on port ${PORT} - $(date '+%F %T')"
echo "---------------------------------------------------------------"
echo "---------------------------------------------------------------"

# perf stat -e cycles,instructions,task-clock,context-switches ./http_server_cpp "$PORT" "$WEBSOCKET_PORT" web_data
exec ./http_server_cpp "$PORT" "$WEBSOCKET_PORT" web_data
