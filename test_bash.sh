#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

TEST_DIR="${ROOT_DIR}/test_cases"
BUILD_DIR="${TEST_DIR}/build"
BIN="${BUILD_DIR}/test_cases"

TARGET="${1:-}"
TEST_NAME="${2:-}"

build_all() {
    mkdir -p "${BUILD_DIR}"

    cd "${BUILD_DIR}"

    cmake .. \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_C_COMPILER=gcc \
        -DENABLE_UNITY_BUILD=OFF \
        -DBUILD_ONLY="core;sts;identitystore;s3;ec2"

    cmake --build . -j 6

    chmod +x "${BIN}"
}

run_all() {
    cd "${BUILD_DIR}"
    "${BIN}"
}

normalize_target() {
    local input="$1"

    input="${input#./}"

    if [[ "${input}" != test_cases/* ]]; then
        echo "Error: target must start with test_cases/"
        echo "Example:"
        echo "./test_bash.sh test_cases/core/coroutine"
        echo "./test_bash.sh test_cases/core/coroutine/07_lifetime_usage_test.cpp"
        exit 1
    fi

    input="${input#test_cases/}"

    echo "${input}"
}

collect_cpp_files() {
    local target="$1"

    local full_path="${TEST_DIR}/${target}"

    if [ -f "${full_path}" ]; then
        echo "${full_path}"
        return
    fi

    if [ -d "${full_path}" ]; then
        find "${full_path}" -type f -name "*.cpp" | sort
        return
    fi

    echo "Error: file or directory not found:"
    echo "test_cases/${target}"
    exit 1
}

extract_tests_from_file() {
    local file="$1"

    grep -E 'TEST(_F|_P)?\s*\(' "${file}" | \
    sed -E 's/.*TEST(_F|_P)?[[:space:]]*\([[:space:]]*([A-Za-z0-9_]+)[[:space:]]*,[[:space:]]*([A-Za-z0-9_]+).*/\2.\3/'
}

build_filter() {
    local files=("$@")

    local all_tests=""

    for file in "${files[@]}"; do
        local tests

        tests=$(extract_tests_from_file "${file}" || true)

        if [ -n "${tests}" ]; then
            all_tests="${all_tests}"$'\n'"${tests}"
        fi
    done

    all_tests=$(echo "${all_tests}" | sed '/^$/d' | sort -u)

    if [ -z "${all_tests}" ]; then
        echo "Error: no GoogleTest cases found"
        exit 1
    fi

    if [ -n "${TEST_NAME}" ]; then
        matched_tests=$(echo "${all_tests}" | grep "${TEST_NAME}" || true)

        if [ -z "${matched_tests}" ]; then
            echo "Error: no test matched:"
            echo "${TEST_NAME}"
            exit 1
        fi

        all_tests="${matched_tests}"
    fi

    FILTER=$(echo "${all_tests}" | paste -sd ':' -)

    echo "${FILTER}"
}

run_filtered() {
    local filter="$1"

    cd "${BUILD_DIR}"

    echo "Running filter:"
    echo "${filter}"

    "${BIN}" --gtest_filter="${filter}"
}

main() {
    build_all

    if [ -z "${TARGET}" ]; then
        run_all
        exit 0
    fi

    TARGET=$(normalize_target "${TARGET}")

    mapfile -t FILES < <(collect_cpp_files "${TARGET}")

    if [ "${#FILES[@]}" -eq 0 ]; then
        echo "Error: no cpp files found"
        exit 1
    fi

    FILTER=$(build_filter "${FILES[@]}")

    run_filtered "${FILTER}"
}

main