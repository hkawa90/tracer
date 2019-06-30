const fs = require('fs');
const readline = require('readline');
const commandLineArgs = require('command-line-args');

const optionDefinitions = [
  {
    name: 'file',
    alias: 'f',
    type: String
  },
  {
    name: 'help',
    alias: 'h',
    type: Boolean
  }
];

function isExistFile(file) {
  try {
    fs.statSync(file);
    return true
  } catch(err) {
    if(err.code === 'ENOENT') return false
  }
}


const options = commandLineArgs(optionDefinitions);
var traceFile = './trace.dat';
// HELP
if (options.help) {
  process.stdout.write('option list:');
  process.stdout.write(' -f,-file trace.dat   : specify trace file.');
  process.stdout.write(' -h,-help             : This message.');
  process.exit(1);
}

if (options.file !== null) {
  if (isExistFile(options.file)) {
    traceFile = options.file;
  } else {
    process.exit(2);
  }
}
rs = fs.createReadStream(traceFile);

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
  //console.log("threadID:"+threadID);
  if ((currentFunc !== '') && status === 'E') {
    stack.push(currentFunc);
    //console.log("threadID::"+threadID);
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
  for (var thread in call) {
    process.stdout.write('digraph call_graph_'+ thread+' {');
    for (var trace in call[thread]) {
      process.stdout.write('  '+trace);
    }
    process.stdout.write('}');
  }
});
