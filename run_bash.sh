#!/bin/bash

if [[ ! -f server-certificate.crt || ! -f server-private-key.pem ]]; then
    chmod 777 z_util_scripts/generate_server_certificate.sh
    ./z_util_scripts/generate_server_certificate.sh
fi

ulimit -c unlimited

# Core dump pattern: /tmp/core.%e.%p.%t
current_core_pattern=$(cat /proc/sys/kernel/core_pattern)
echo "Current core_pattern: ${current_core_pattern}"

if [[ "${current_core_pattern}" != "/tmp/core.%e.%p.%t" ]]; then
    echo "Updating core_pattern to /tmp/core.%e.%p.%t"

    sudo sysctl -w kernel.core_pattern=/tmp/core.%e.%p.%t

    echo "Updated core_pattern: $(cat /proc/sys/kernel/core_pattern)"
else
    echo "core_pattern already correct"
fi

if [[ "${PROD:-false}" == "true" ]]; then
    PORT=443
    WEBSOCKET_PORT=8443
    MONGO_URI="mongodb://172.31.9.78:27017"
    ENV_NAME="prod"
else
    PORT=8080
    WEBSOCKET_PORT=8083
    MONGO_URI="mongodb://127.0.0.1:27017"
    ENV_NAME="local"
fi

DB_NAME="system_monitoring"
COLLECTION_NAME="crash_log"

echo "==== START http_server_cpp $(date '+%F %T') ===="
echo "ENV=${ENV_NAME}"
echo "PORT=${PORT}, WEBSOCKET_PORT=${WEBSOCKET_PORT}"
echo "SCRIPT_PID=$$"
echo "---------------------------------------------------------------"

./http_server_cpp "$PORT" "$WEBSOCKET_PORT" web_data

code=$?
now=$(date '+%F_%H-%M-%S')

echo "==== EXIT http_server_cpp $(date '+%F %T'), code=${code} ===="

if [[ $code -eq 139 || $code -eq 134 || $code -eq 137 ]]; then
    signal="UNKNOWN"

    if [[ $code -eq 139 ]]; then
        signal="SIGSEGV"
        echo "LIKELY SIGSEGV"
    elif [[ $code -eq 134 ]]; then
        signal="SIGABRT"
        echo "LIKELY SIGABRT / std::terminate / assert"
    elif [[ $code -eq 137 ]]; then
        signal="SIGKILL_OR_OOM"
        echo "LIKELY SIGKILL / OOM killer"
    fi

    latest_core=$(ls -t /tmp/core.http_server_cpp.* 2>/dev/null | head -n 1)

    if [[ -n "$latest_core" ]]; then
        trace_file="/tmp/http_server_cpp_crash_${now}.log"

        gdb -batch \
            -ex "set pagination off" \
            -ex "bt" \
            ./http_server_cpp "$latest_core" \
            > "$trace_file" 2>&1

        frame0=$(grep -m 1 "^#0" "$trace_file")
        frame1=$(grep -m 1 "^#1" "$trace_file")

        crash_function=$(echo "$frame0" | sed -E 's/^#0 +0x[0-9a-f]+ in ([^(]+).*/\1/')
        caller=$(echo "$frame1" | sed -E 's/^#1 +0x[0-9a-f]+ in ([^(]+).*/\1/')

        crash_line=$(echo "$frame0" | grep -oE ':[0-9]+' | tail -n 1 | tr -d ':')
        caller_line=$(echo "$frame1" | grep -oE ':[0-9]+' | tail -n 1 | tr -d ':')

        core_size=$(du -h "$latest_core" | cut -f1)
        created_at_ns=$(date -u +%s%N)

        if command -v mongosh >/dev/null 2>&1 && command -v jq >/dev/null 2>&1; then
            mongosh "${MONGO_URI}/${DB_NAME}" --quiet --eval "
                db.${COLLECTION_NAME}.insertOne({
                    app: 'cpp_trading_engine',
                    env: '${ENV_NAME}',
                    exit_code: ${code},
                    signal: '${signal}',
                    crash_function: $(jq -Rn --arg v "$crash_function" '$v'),
                    crash_line: $(jq -Rn --arg v "$crash_line" '$v'),
                    caller: $(jq -Rn --arg v "$caller" '$v'),
                    caller_line: $(jq -Rn --arg v "$caller_line" '$v'),
                    core_file_size: '${core_size}',
                    host: '$(hostname)',
                    created_at: new Date(),
                    created_at_ns: NumberLong(\"${created_at_ns}\")
                });
            "
        else
            echo "mongosh or jq not found, skip MongoDB crash insert"
        fi

        rm -f "$latest_core"
        rm -f "$trace_file"

        if [[ ! -f "$latest_core" ]]; then
            echo "Deleted core file: $latest_core"
        fi

        if [[ ! -f "$trace_file" ]]; then
            echo "Deleted trace file: $trace_file"
        fi
    else
        echo "No core file found"
    fi
fi

exit $code