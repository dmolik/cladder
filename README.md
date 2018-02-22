# Cladder

A proof of concept utility that takes two arguments, a source directory and a destination directory.

The src directory is turned into a lz4-squashfs archive and then mounted onto a destination directory
with tmpfs overlayed ontop of it to make it writeable.

A possible use is to download a source code repo and then build it, and throw away the tempoary build files.

## Requirements

### System/Runtime

- Linux Kernel supporting squashfs and overlayfs, and iptables-dev installed

### Toolchain

- GCC or Clang
- libc

## Compiling

    v=$(curl 'https://api.github.com/repos/graytshirt/cladder/tags' 2>&1 | awk '/name/{print $2;}' | sed -e 's/"//g' -e 's/,$//')
    curl -L https://github.com/GrayTShirt/cladder/releases/download/${v}/cladder-${v#v}.tar.gz | tar x - 
    cd cladder-${v#v}
    ./configure --prefix=/usr CFLAGS="-march=native -mtune=native -O2"
    make
    sudo make install
