Callgraph
====

トレース情報からcall graphを作成する。

## Description

カレントディレクトのトレースデータファイル(trace.dat)を読み込み、Graphvizのdot データを標準出力に出力する。
ソースコードはJavasciprtで記述されて、node.jsで実行する。

## Requirement

Graphviz, Node.js

## Usage

	$ node bin/index.js -e executable_file -f trace.dat | dot -Tpng -Gdpi=200 > g.png
	$ display g.png

## Install

とくに必要なし。

## Licence

Free

## Author

[hkawa90](https://github.com/hkawa90)