#!/bin/bash

#export GLOG_log_dir=path
export GLOG_logbufsecs=0
#export GLOG_logbuflevel=-1

chmod 777 z_util_scripts/generate_server_certificate.sh
./z_util_scripts/generate_server_certificate.sh

./http_server_cpp 8080 web_data
