#!/bin/bash
[ ! -e scan-build ] && mkdir scan-build
cd scan-build || exit 1

rm -r -- *

if meson ..; then
    # This has to be done with else because with '!' it won't work on Mac OS X
    echo
else
    exit $? #abort on failure
fi

ninja clean
ninja scan-build
