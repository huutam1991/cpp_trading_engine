#!/bin/bash

# Build + print log to file build.log
docker compose build --progress=plain > build.log 2>&1