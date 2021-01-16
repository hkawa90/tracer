# Tracer for C/CPP Language.

C/CPPプログラムの関数コール毎にトレース情報を出力します。

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
sudo apt-get install libiberty-dev
sudo apt-get install uuid-dev
```

### Installing

ライブラリとして使用するためmakeでビルドします。

```
make
```

エラーがなければ、次のファイルが作成されます。

```
$ ls -l *.so
-rwxrwxr-x 1 kawa90 kawa90 176792  8月 14 12:02 libtrace++.so
-rwxrwxr-x 1 kawa90 kawa90 107864  8月 14 12:02 libtrace.so
-rwxrwxr-x 1 kawa90 kawa90  30544  8月 14 12:18 libwatchalloc.so
```

- libtrace.so C言語用トレーサ
- libtrace++.so CPP言語用トレーサ(libtrace.soとほぼ同一、関数名のdemangleに対応)
- libwatchalloc.so malloc/calloc/realloc/freeトレーサー

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

プログラムを実行するとカレントディレクトリに関数トレース結果(`tracer`)が出力されます。1行単位で1つのトレース情報を表しています。出力形式はJSONっぽいものとCSVです。デフォルトはJSONライク(行単位ではJSON)です。

```
{ "id" : "I", "pid": "199474", "function": "main", "time1_sec": "270959", "time1_nsec": "425916885", "time2_sec": "0", "time2_nsec": "15097166", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "I", "pid": "199475", "function": "doSomething", "time1_sec": "270959", "time1_nsec": "426193207", "time2_sec": "0", "time2_nsec": "182463", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "O", "pid": "199475", "function": "doSomething", "time1_sec": "270959", "time1_nsec": "426261468", "time2_sec": "0", "time2_nsec": "250735", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "O", "pid": "199474", "function": "main", "time1_sec": "270959", "time1_nsec": "426378706", "time2_sec": "0", "time2_nsec": "15252634", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }

```

- id: I(関数突入時)/O(関数脱出時)/E(特定関数コール時)/EI/EO
- pid: プロセスID
- coreid: CPU Core ID
- function: 関数名
- time1_sec: 実行時クロック(秒).`CLOCK_MONOTONIC`で取得しています。
- time1_nsec: 実行時クロック(ナノ秒).`CLOCK_MONOTONIC`で取得しています。
- time2_sec: 実行時クロック(秒).`CLOCK_THREAD_CPUTIME_ID`で取得しています。
- time2_nsec: 実行時クロック(ナノ秒).`CLOCK_THREAD_CPUTIME_ID`で取得しています。
- info1: 各種情報
- info2: 各種情報
- info3: 各種情報
- filename: モジュールパス
- lineno: ソース行番号

CSV形式は上記の並び順の内容となります。ログは環境変数`TRACER_LOG'がセットされていると、
そのパスに出力されます。セットされていない場合はカレントディレクトリに`tracer'として出力されます。

```
export TRACER_LOG=/tmp/tracer
```

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
{ "id" : "I", "pid": "199974", "function": "main", "time1_sec": "271471", "time1_nsec": "219501477", "time2_sec": "0", "time2_nsec": "16647626", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "E", "pid": "199974", "function": "main", "time1_sec": "271471", "time1_nsec": "219611367", "time2_sec": "0", "time2_nsec": "16760603", "info1": "139640149989120","info2": "(0)pthread_create(doSomething,139640149989120)","info3": "","filename": "","lineno": "0" }
{ "id" : "EI", "pid": "199974", "function": "main", "time1_sec": "271471", "time1_nsec": "219630977", "time2_sec": "0", "time2_nsec": "16780341", "info1": "139640149989120","info2": "pthread_join","info3": "","filename": "","lineno": "0" }
{ "id" : "I", "pid": "199975", "function": "doSomething", "time1_sec": "271471", "time1_nsec": "219707247", "time2_sec": "0", "time2_nsec": "112784", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "O", "pid": "199975", "function": "doSomething", "time1_sec": "271471", "time1_nsec": "219757420", "time2_sec": "0", "time2_nsec": "163366", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "EO", "pid": "199974", "function": "main", "time1_sec": "271471", "time1_nsec": "219808052", "time2_sec": "0", "time2_nsec": "16803639", "info1": "139640149989120","info2": "(0)pthread_join","info3": "","filename": "","lineno": "0" }
{ "id" : "O", "pid": "199974", "function": "main", "time1_sec": "271471", "time1_nsec": "219858943", "time2_sec": "0", "time2_nsec": "16854726", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
```
C++は上記`libtrace.so`を`libtrace++.so`に置き換えて、実行してください。

### Configuration

`libconfuse`を使った初設定を`racer.conf`ファイルに記述して動作を変更可能です。

設定可能項目は以下の通り。

- use_timestamp: 関数実行時のタイムスタンプを取得する場合は1,そうでない場合は0. デフォルト:1
- use_cputime:タイムスタンプ取得時にCPU TIMEも取得する場合は1,そうでない場合は0. デフォルト:1
- use_sourceline: 関数のソースファイル名、行番号を取得する場合は1,そうでない場合は0. デフォルト:0
- use_mcheck: [GCCのmtrace](https://en.wikipedia.org/wiki/Mtrace)を有効にします。する場合は1,そうでない場合は0. デフォルト:0
- use_core_id: sched_getcpu()で得たCPU Core idを取得する場合は1,そうでない場合は0. デフォルト:0
- use_fsync:トレースファイル出力時に[fsync](https://linuxjm.osdn.jp/html/LDP_man-pages/man2/fsync.2.html)を実行する場合は1,そうでない場合は0. デフォルト:0
- output_format: トレースファイル形式を指定します。LJSON or CSVとします。デフォルト:LJSON


記述例：

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
{ "id" : "I", "pid": "201009", "function": "main", "time1_sec": "272269", "time1_nsec": "679372869", "time2_sec": "0", "time2_nsec": "15640034", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
{ "id" : "E", "pid": "201009", "function": "main", "time1_sec": "272269", "time1_nsec": "679432461", "time2_sec": "0", "time2_nsec": "15702937", "info1":"","info2": "(0x5609732a2c10)malloc(5)","info3": "","filename": "","lineno": "0" }
{ "id" : "O", "pid": "201009", "function": "main", "time1_sec": "272269", "time1_nsec": "679447445", "time2_sec": "0", "time2_nsec": "15718176", "info1":"","info2": "","info3": "","filename": "","lineno": "0" }
```

`libwatchalloc.so`は動的リンクしなくとも、`dlsym`を使ったhookを行うことで、ソースコード変更なく実行できます。

```
LD_PRELOAD=./libwatchalloc.so ./s
cat memtrace.log
sample[115f] malloc(0x5) = 0x556957c24890
allocated size = 5
```
トレースファイル名はデフォルトはカレントの`memtrace.log`で環境変数`MEM_TRACE_LOG`で示されたファイル名となります。

ログ内容は次の書式で、`allocated size`はmalloc,realloc,callocした合計サイズです。
```
プロセス[関数アドレス] メモリ管理関数(引数の値,...) = 戻り値
```
関数アドレスから実際の関数名を求めるには`addr2line`コマンドを使用します。

実行例：
```
addr2line -e ./sample 
main
/home/hoge/tracer/src/./sample.c:9
```

もしくは[util](../js/util)の`resolveFunc.js`を使うと同様なことをJavascriptで実行できます。詳細は[README.md](../js/util/README.md)。

参考までにプログラムが通常終了するケースでは[LeakSanitizer](http://gavinchou.github.io/experience/summary/syntax/gcc-address-sanitizer/)(GCC 4.8 later)を使う方法もあります。

>-fsanitize=leak
>
>Enable LeakSanitizer, a memory leak detector. This option only matters for linking of executables and the executable is linked against a library that overrides malloc and other allocator functions. See https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer for more details. The run-time behavior can be influenced using the LSAN_OPTIONS environment variable. The option cannot be combined with -fsanitize=thread.

```
==250502==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 5 byte(s) in 1 object(s) allocated from:
    #0 0x7f20e55f09d1 in malloc (/usr/lib/x86_64-linux-gnu/liblsan.so.0+0xf9d1)
    #1 0x55b61010015e in main /home/foo/smalloc_leak.c:8
    #2 0x7f20e54160b2 in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x270b2)

SUMMARY: LeakSanitizer: 5 byte(s) leaked in 1 allocation(s).
```

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details

## TODO

- [x] Add line number to trace.dat
- [x] Add module path to trace.dat
- [x] Add event output
- [ ] Allocaton history
- [ ] Rotate trace log file
- [ ] Test
