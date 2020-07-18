Callgraph
====

トレース情報からcall graphを作成する。

## Description

カレントディレクトのトレースデータファイル(trace.dat)を読み込み、Graphvizのdot データを標準出力に出力する。
ソースコードはJavasciprtで記述されて、node.jsで実行する。

## Requirement
Graphviz, Node.js, addr2line(binutils)

## Usage
### Create Callgraph

	$ node bin/index.js -e executable_file -f trace.dat | dot -Tpng -Gdpi=200 > g.png
	$ display g.png

### Create json file

	$ node bin/index.js -e executable_file -j -f trace.dat | trace.json

## Install

とくに必要なし。

## Licence

Free

## TODO

+ [ ] Customize [dot style](https://graphviz.org/doc/info/attrs.html)
  + [ ] default dot style file
  + [ ] user dot style file
+ [ ] Detailed description

## Author

[hkawa90](https://github.com/hkawa90)