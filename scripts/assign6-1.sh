#!/bin/bash
# sender
cat ./send.txt | nc -l 16260
# receiver
./build/client_recv 127.0.0.1 16260 > ./recv.txt