# Cladder

A proof of concept utility that takes two arguments, a source directy and a destination directory.

The src directory is turned into a lz4-squashfs archive and then mounted onto a destination directory
with tmpfs overlayed ontop of it to make it read-write.

A possible use is to download a source code repo and then build it, and throw away the tempoary build files.

## Builing

currently requires:
  libmount (usually provided by util-linux)
  squashfs-tools
  liblz4

compile:
  ./bootstrap.sh
  ./configure
  make
