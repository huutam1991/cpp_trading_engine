#!/bin/bash

#export GLOG_log_dir=path
# export GLOG_logbufsecs=0
#export GLOG_logbuflevel=-1

chmod 777 z_util_scripts/generate_server_certificate.sh
./z_util_scripts/generate_server_certificate.sh

# Detect port
if [[ "$PROD" == "true" ]]; then
    PORT=443
else
    PORT=8080
fi

# perf stat -e cycles,instructions,task-clock,context-switches ./http_server_cpp "$PORT" web_data
./http_server_cpp "$PORT" web_data
