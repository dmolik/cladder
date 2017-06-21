# Cladder

A proof of concept utility that takes two arguments, a source directory and a destination directory.

The src directory is turned into a lz4-squashfs archive and then mounted onto a destination directory
with tmpfs overlayed ontop of it to make it writeable.

A possible use is to download a source code repo and then build it, and throw away the tempoary build files.

## Builing

Requirements

- libmount, usually provided by util-linux
- pthread
- lz4


compiling

    ./bootstrap.sh
    ./configure
    make

