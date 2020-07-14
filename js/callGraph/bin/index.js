#! /usr/bin/env node

const fs = require('fs');
const readline = require('readline');
const commandLineArgs = require('command-line-args');
const ftracer_parse = require('../dist/main.js');

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
  },
  {
    name: 'output_json',
    alias: 'j',
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
  process.stdout.write('option list:\n');
  process.stdout.write(' -f,-file trace.dat   : specify trace file.\n');
  process.stdout.write(' -h,-help             : This message.\n');
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

ftracer_parse.parseTraceInfoInit();

rl.on('line', function(line) {
  ftracer_parse.parseTraceInfo(line);
});

rl.on('close', function() {
  var buffer = '';
  if (options.output_json) {
    buffer = ftracer_parse.printJSON(buffer);
  } else {
    buffer = ftracer_parse.printDot(buffer);
  }
  process.stdout.write(buffer);
});