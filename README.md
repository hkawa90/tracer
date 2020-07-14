# tracer
c/c++ tracer

## Require library

```
sudo apt-get install libiberty-dev
```

## Usage

### Build your application

Compile your source code with below options.

```
gcc your_code.c -g -finstrument-functions -ldl -rdynamic
```

### Run your application

```
LD_PRELOAD=./src/libtrace.so ./test
```

Your application crate trace file.

## Callgraph using GraphViz

### Build

```
cd js/callGraph
npm install
npm run build
```

### Run

```
node bin/index.js -f trace.data | dot -Tpng > trace.png
```

## Test

### Rquire environment

Using [Criterion](https://github.com/Snaipe/Criterion) 

Install Criterion on Ubuntu:

```
$ sudo add-apt-repository ppa:snaipewastaken/ppa
$ sudo apt-get update
$ sudo apt-get install criterion-dev
```

## Known issues

### Unknown the caller of thread.