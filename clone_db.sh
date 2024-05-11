#!/bin/bash

cd z_docker

# Create dump files
mongodump -d binance -c current_price
mongodump -d binance -c depth
mongodump -d binance -c user_config

# Create new DB from dump files
mongorestore -d binance_real -c current_price dump/binance/current_price.bson
mongorestore -d binance_real -c depth dump/binance/depth.bson
mongorestore -d binance_real -c user_config dump/binance/user_config.bson