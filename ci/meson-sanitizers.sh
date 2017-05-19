#!/bin/bash
[ ! -e sanitizers ] && mkdir sanitizers
cd sanitizers || exit 1

rm -r -- *

if meson .. -Denable_asan=true -Denable_ubsan=true; then
    # This has to be done with else because with '!' it won't work on Mac OS X
    echo
else
    exit $? #abort on failure
fi

ninja clean
if ninja; then
    # This has to be done with else because with '!' it won't work on Mac OS X
    echo
else
    exit $? #abort on failure
fi

mesontest --setup=sanitize
