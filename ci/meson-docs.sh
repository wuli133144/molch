#!/bin/bash
[ ! -e dummy ] && mkdir dummy
cd dummy || exit 1

rm -r -- *

if meson .. -Denable_docs=true; then
    # This has to be done with else because with '!' it won't work on Mac OS X
    echo
else
    exit $? #abort on failure
fi

if ninja doxygen; then
    # This has to be done with else because with '!' it won't work on Mac OS X
    echo
else
    exit $? #abort on failure
fi
