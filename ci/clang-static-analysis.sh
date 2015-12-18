#!/bin/sh
if ! hash scan-build; then
    echo Clang static analyzer not installed. Skipping ...
    exit 0
fi
[ ! -e static-analysis ] && mkdir static-analysis
cd static-analysis
scan-build --status-bugs cmake .. -DCMAKE_BUILD_TYPE=Debug
make clean
scan-build --status-bugs make
