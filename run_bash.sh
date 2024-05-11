#!/bin/bash

#export GLOG_log_dir=path
export GLOG_logbufsecs=0
#export GLOG_logbuflevel=-1

./http_server_cpp 8080 web_data
