# resolveFunc.js

libwatchalloc.soの出力から`addr2line`を使って、関数名、ソース名、行番号を得ます。

## Prerequisites

システム上に`addr2line`, もしくは[node.js](https://nodejs.org/ja/)がなければ、インストールしてください。

```
apt-get install binutils nodejs
```

## Run

実行ファイルがあるディレクトリで下記のように実行します。

Usage:

```
$ cat memtrace.log
smalloc_leak[11b3] malloc(0x5) = 0x55616343b890
allocated size = 5

$ node resolveFunc.js memtrace.log
smalloc_leak[main(/home/foo/smalloc_leak.c:8)] malloc(0x5) = 0x55616343b890
```

## License

This project is licensed under the MIT License - see the [LICENSE.md](../../LICENSE) file for details
