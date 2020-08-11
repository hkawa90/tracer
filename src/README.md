# Tracer for C Language.

Cプログラムの関数コール毎にトレース情報を出力します。

## Getting Started

Git cloneしてソースコードを入手してください。

```
git clone https://github.com/hkawa90/tracer.git
cd tracer
```

### Prerequisites

動作はLinuxのみで、GCC(コンパイラ)を使用します。またライブラリとしてlibconfuseを使用しています。またC++の関数名のデマングルのため、libibertyを使用しています。

```
sudo apt-get install libconfuse-dev
sudo apt-get install llibiberty-dev
```

### Installing

ライブラリとして使用するためmakeでビルドします。

```
make
```

エラーがなければ、次のファイルが作成されます。

```
$ ls -l *.so
-rwxrwxr-x 1 kawa90 kawa90 76304  8月 10 17:47 libtrace++.so
-rwxrwxr-x 1 kawa90 kawa90 92936  8月 10 17:47 libtrace.so
```

## HOW TO

次のプログラムの関数トレースを例に説明します。

```
#include <stdio.h>
#include <pthread.h>

// 新しいスレッドで実行されるタスク
void *doSomething(void* pArg) {
    int *pVal = (int*) pArg;
    printf("worker thread [%d]\n", *pVal);
    *pVal = 200;
}

int main() {
    pthread_t handle;  // Thread handle.
    int data = 100;

    pthread_create(&handle, NULL, doSomething, &data);
    pthread_join(handle, NULL);

    printf("main thread [%d]\n", data);
}
```

コンパイルします。コンパイルオプションに必ず`-g`と`-finstrument-functions`をつけてコンパイルします。

```
gcc -c -g -finstrument-functions sample.c
```

### 動的リンク

関数トレース用sharedライブラリ(libtrace.so)と動的リンクします。`-L`につづくパスで`libtrace.so`の存在するパスを指定します。

```
gcc -o sample sample.o -lpthread -ltrace -L.
./sample
worker thread [100]
main thread [200]
```

また実行時にlibtrace.soが見つからないためにエラーとなる場合は、下記のように`LD_LIBRARY_PATH`で`libtrace.so`が存在するパスを指定してください。

```
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```

プログラムを実行するとカレントディレクトリに関数トレース結果(`trace.dat`)が出力されます。

```
I,149620,main,334913,795191868,0,15145689
I,149621,doSomething,334913,795383725,0,88201
O,149621,doSomething,334913,795455322,0,160146
O,149620,main,334913,795555239,0,15301790
```

関数トレース結果は次のCSV形式となります。

第1カラム:関数突入時は`I`,関数脱出時は`O`となります。
第2カラム:プロセスID
第3カラム:関数名
第4カラム:実行時クロック(秒).`CLOCK_MONOTONIC`で取得しています。
第5カラム:実行時クロック(ナノ秒).`CLOCK_MONOTONIC`で取得しています。
第6カラム:実行時クロック(秒).`CLOCK_THREAD_CPUTIME_ID`で取得しています。
第7カラム:実行時クロック(ナノ秒).`CLOCK_THREAD_CPUTIME_ID`で取得しています。


### LD_PRELOADで実行時にsharedライブラリ指定

リンクは通常通りに行います。

```
gcc -o sample sample.o -lpthread
```

実行は環境変数LD_PRELOADで`libtrace.so`を指定して実行します。

```
LD_PRELOAD=./libtrace.so ./sample
worker thread [100]
main thread [200]
```

実行結果は動的リンクと異なり、`pthread_create`, `pthread_join`, `exit`, `fork`, `pthread_exit`をhookしてトレース情報を出力します。`pthread_join`は、`pthread_join`開始時は第1カラムは`EI`となり、`pthread_join`完了後は`EO`と表示されます。`pthread_create`,`exit`, `fork`, `pthread_exit`では第1カラムは`E`となります。第8カラムは`pthread_create`と`pthread_join`のときに詳細情報が表示されます。`pthread_create`だけは第8カラムに実行スレッド関数名とスレッドIDが表示され、`pthread_join`ではjoin待ちスレッドIDが表示されます。

```
I,150284,main,335466,673478089,0,15534782
E,150284,main,335466,673615657,0,15678340,pthread_create doSomething 140241503631104
EI,150284,main,335466,673694612,0,15737977,pthread_join 140241503631104
I,150285,doSomething,335466,673716569,0,97286
O,150285,doSomething,335466,673752107,0,132986
EO,150284,main,335466,673796047,0,15763878,pthread_join 140241503631104
O,150284,main,335466,673832331,0,15798250
```
C++は上記`libtrace.so`を`libtrace++.so`に置き換えて、実行してください。

### Configuration

`libconfuse`を使った初設定を`racer.conf`ファイルに記述して動作を変更可能です。

```
# ライン行、モジュールパスをトレース情報に追加します.無効とする場合は0を代入.
use_sourceline=1
# GCCのmtraceを有効にします.無効とする場合は0を代入.
use_mcheck=1
```

`use_mcheck=1`とした場合は、環境変数`MALLOC_TRACE`でログ出力先を指定してください。

```
export MALLOC_TRACE="/tmp/mtrace.log"
./sample
mtrace ./sample $MALLOC_TRACE
```

### 独自イベント

、動的リンクすることで`finstrument.h`に公開しているAPIでトレース情報にユーザ独自メッセージを記録できます。

```
extern int tracer_event(const char *msg);
extern int tracer_event_in(const char *msg);
extern int tracer_event_out(const char *msg);
```
またヘッダ`finstrument.h`の前に`REPLACE_GLIBC_ALLOC_FUNCS`を`define`することで、`malloc`,`free`, `calloc`,`realloc`のトレース情報を取得できます。

```
#include <stdio.h>
#include <stdlib.h>

#define REPLACE_GLIBC_ALLOC_FUNCS
#include "finstrument.h"

main()
{
    void *ptr;

    ptr = malloc(5);
}
```
上記をコンパイルして動的リンクします。

```
gcc -c -g -finstrument-functions -DFINSTRUMENT sample.c
gcc -o sample sample.o -lpthread -ltrace -L.
```

上記を実行した結果: main関数で5バイトmallocしたことがわかります。
```
I,18027,main,16681,264597108,0,10147387
E,18027,main,16681,264722077,0,10277895,(0x559f6b15fa50)malloc(5)
O,18027,main,16681,264791670,0,10346411

```

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details

## TODO

- [x] Add line number to trace.dat
- [x] Add module path to trace.dat
- [x] Add event output
- [x] Allocaton history
- [ ] Test