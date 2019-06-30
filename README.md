# tracer
c/c++ tracer

## Require library

```
sudo apt-get install libiberty-dev
```

## Usage

```
LD_PRELOAD=./src/libtrace.so ./test
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

