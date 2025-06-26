# I実験
## コンパイル方法
```sh
cmake -Bbuild -S. -GNinja
cmake --build build
```

## 実行方法
### サーバ
```sh
./build/chat_server
```

### クライアント
```sh
./scripts/listening-only.sh <サーバIP>
./scripts/speaking-only.sh <サーバIP>
./scripts/assign8-3.sh <サーバIP>
```