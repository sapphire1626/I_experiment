#!/bin/bash
# $1 = サーバアドレス
# $2 = サーバゲートポート
./build/i1i2i3_phone $1 $2 | play -t raw -b 16 -c 1 -e s -r 44100 -