Callgraph
====

トレース情報(trace.dat)からcall graphを作成する。トレース情報は`libtracer.so`, `libtracer.so`から出力されたものが対象です。

## Description

カレントディレクトのトレースデータファイル(trace.dat)を読み込み、Graphvizのdot データを標準出力に出力する。
ソースコードはJavasciprtで記述されて、node.jsで実行する。

## Requirement

Graphviz, Node.js

## Usage
### Create Callgraph

```
$ node bin/index.js -f trace.dat | dot -Tpng -Gdpi=200 > g.png
```

### Create json file

	$ node bin/index.js -j -f trace.dat | trace.json

## Licence

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details

## TODO

- [ ] Handle EI, EO, UE, UEI, UEO event
+ [ ] Customize [dot style](https://graphviz.org/doc/info/attrs.html)
  + [ ] default dot style file
  + [ ] user dot style file
+ [ ] Detailed description

## Author

[hkawa90](https://github.com/hkawa90)