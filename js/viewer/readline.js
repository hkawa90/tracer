var fs = require('fs');
var rs = fs.createReadStream('../../test/trace.dat');
var readline = require('readline');

var rl = readline.createInterface(rs, {});
var pStatus = '';
var currentFunc = '';
var call = [];
var stack = [];

rl.on('line', function(line) {
  var words = line.split(/\s+/);
  var func = words[2];
  var status = words[0];
  var threadID = words[1];
  if ((currentFunc !== '') && status === 'E') {
    stack.push(currentFunc);
    if (call[threadID] === undefined) {
      call[threadID] = [];
    }
    if (call[threadID][currentFunc + '->' + func] === undefined) {
      var obj = new Object();
      obj.count = 0;
      obj.min_time = 0;
      obj.max_time = 0;
      obj.avg_time = 0;
      obj.min_cputime = 0;
      obj.max_cputime = 0;
      obj.avg_cputime = 0;
      call[threadID][currentFunc + '->' + func] = obj;
    }
    call[threadID][currentFunc + '->' + func].count ++;
  } else if ((currentFunc !== '') && status === 'X') {
    currentFunc = stack.pop();
  }
  currentFunc = func;
});

rl.on('close', function() {
  console.log(call);
});
