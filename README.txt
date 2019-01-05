Main executable: main
Compile: make all
By default it will be statically linked.
This implementation does not use pthread, but uses epoll instead. May not work on some ancient kernels.

This program is supposed to work with source IP 192.168.1.10, and tested on a point-to-point network without route.

Route-table is implemented in Trie tree, but has not been fully tested.
