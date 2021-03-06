#!/bin/bash
TESTS=("ci/test.sh" "ci/clang-static-analysis.sh" "ci/address-sanitizer.sh" "ci/undefined-behavior-sanitizer.sh" "ci/doxygen.sh")
STATUS="OK"

for TEST in "${TESTS[@]}"; do
    echo "$TEST"
    if ! "$TEST"; then
        STATUS="FAILED"
    fi
done

case $STATUS in
    "OK")
        exit 0
        ;;
    "FAILED")
        exit 1
        ;;
    *)
        exit 1
        ;;
esac
