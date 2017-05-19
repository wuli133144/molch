#!/bin/bash
[ ! -e mesonbuild ] && mkdir mesonbuild
cd mesonbuild || exit 1

rm -r -- *
if meson .. -Denable_bindings=true -Denable_docs=true -Db_lto=false; then
    # This has to be done with else because with '!' it won't work on Mac OS X
    echo
else
    exit $? #abort on failure
fi

mesontest
