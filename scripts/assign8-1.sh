#!/bin/bash
# server
rec -t raw -b 16 -c 1 -e s -r 44100 - | ./build/serv_send 50000

# client
./build/client_recv 127.0.0.1 50000 | play -t raw -b 16 -c 1 -e s -r 44100 -