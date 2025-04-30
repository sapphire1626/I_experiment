#include <stdlib.h>
#include <math.h>
#include <stdio.h>
// 秒数 => プロット個数
int secToN(double sec){
    return sec * 44100;
}

/**
 * @brief 音階バイナリ値を標準出力に出力する
 * argv[1]: 音階個数
 */
int main(int argc, char* argv[]) {
    int n = atoi(argv[1]);
    // C4から始める
    double base_freq = 261.625565; // C4の周波数
    for(int i = 0; i < n; i++){
        double freq = base_freq * pow(2.0, (double)i / 12.0);
        char command[256];
        snprintf(command, sizeof(command), "./sin 10000 %d %d", (int)freq, secToN(0.3));
        system(command);
    }
}