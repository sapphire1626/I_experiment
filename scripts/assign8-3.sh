#!/bin/bash
# $1 = サーバアドレス
# $2 = wavソースならばwavパス
./build/i1i2i3_phone $1 $2 -l -s| play -t raw -b 16 -c 1 -e s -r 44100 -