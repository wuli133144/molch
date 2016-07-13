molch
=====

[![Travis Build Status](https://travis-ci.org/1984not-GmbH/molch.svg?branch=master)](https://travis-ci.org/1984not-GmbH/molch)
[![Coverity Scan Build](https://scan.coverity.com/projects/6421/badge.svg)](https://scan.coverity.com/projects/6421)
[![Join the chat at https://gitter.im/FSMaxB/molch](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/FSMaxB/molch?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

An implementation of the axolotl ratchet (https://github.com/trevp/axolotl/wiki) based on libsodium.

**WARNING: THIS SOFTWARE ISN'T READY YET. DON'T USE IT!**

how to get the code
-------------------
After cloning the repository, the git submodules have to be initialised and updated:
```
$ git clone https://github.com/FSMaxB/molch
$ git submodule update --init --recursive
```

You might also have to run `git submodule update` when changing branches or after pulling in new changes.

how to build
------------
This has been tested on GNU/Linux and Mac OS X.

First make sure `libsodium` and `cmake` are installed.

Then do the following:
```
$ mkdir build #make build directory
$ cd build    #change into it
$ cmake ..    #run cmake (only required once)
$ make        #finally compile the software
```
or run the script `ci/build.sh`.

Run the tests (you need to have valgrind installed):
```
$ cd build
$ make test
```

Run the static analysis (you need clang and clangs static analyzer):
```
$ mkdir static-analysis
$ cd static-analysis
$ scan-build cmake ..
$ scan-build make
```
or run the script `ci/clang-static-analysis.sh`.

how to generate traces for debugging
------------------------------------
```
$ mkdir tracing
$ cd tracing
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DTRACING=On
$ make
```

Now, when you run one of the tests (those are located at `tracing/test/`), it will generate a file `trace.out` and print all function calls to stdout.

You can postprocess this tracing output with `test/trace.lua`, pass it the path of `trace.out`, or the path to a saved output of the test and it will pretty-print the trace. It can also filter out function calls to make things easier to read, see it's source code for more details.

format of a packet
----------------
Molch uses [Googles Protocol Buffers](https://developers.google.com/protocol-buffers/) via the [Protobuf-C](https://github.com/protobuf-c/protobuf-c) library. You can find the protocol descriptions in `lib/protobuf`.

cryptography
------------
This is a brief non-complete overview of the cryptographic primitives used by molch. A detailed description of what molch does cryptographically is only provided by its source code at the moment.

Molch uses only primitives implemented by [libsodium](https://github.com/jedisct1/libsodium).

**Key derivation:** Blake2b
**Header encryption:** Xsalsa20 with Poly1305 MAC
**Message encryption:** XSalsa20 with Poly1305 MAC
**Signing keys (used to sign prekeys and the identity key):** Ed25519
**Other keypairs:** X25519
**Key exchange:** ECDH with X25519

Molch allows you to mix in a low entropy random source to the creation of signing and identity keypairs. In this case, the low entropy random source is used as input to Argon2i and the output xored with high entropy random numbers provided by the operating system.

Want to help?
-------------------
Take a look at the file `CONTRIBUTING`. And look for GitHub Issues with the `help wanted` label.

license
-------
This library is licensed under the ISC license.

> ISC License
>
> Copyright (C) 2015-2016 1984not Security GmbH
>
> Author: Max Bruckner (FSMaxB)
>
> Permission to use, copy, modify, and/or distribute this software for any
> purpose with or without fee is hereby granted, provided that the above
> copyright notice and this permission notice appear in all copies.
>
> THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
> WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
> MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
> ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
> WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
> ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
> OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
