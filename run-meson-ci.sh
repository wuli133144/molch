#!/bin/bash
TESTS=("ci/meson.sh" "ci/meson-valgrind.sh" "ci/meson-scan-build.sh" "ci/meson-sanitizers.sh" "ci/meson-docs.sh")
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
