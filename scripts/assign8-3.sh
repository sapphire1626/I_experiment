#!/bin/bash
# $1 = サーバアドレス
./build/i1i2i3_phone $1 | play -t raw -b 16 -c 1 -e s -r 44100 -