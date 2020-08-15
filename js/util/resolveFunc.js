/**
 * 次の形式(libwatchalloc.so出力)から関数名、ソースパス、行番号を得る。
 * smalloc_leak[11b3] malloc(0x5) = 0x55616343b890
 * Usage:
 * Ex)
 * node resolveFunc.js memtrace.log
 */
const fs = require('fs');
const readline = require('readline');
const sync = require('child_process').spawnSync;

rs = fs.createReadStream(process.argv[2]);

var rl = readline.createInterface(rs, {});

function addr2func(exe, addr) {
    var addr2line = sync('addr2line', ['-e', exe, '-pf', addr]);
    return addr2line.stdout.toString();
}
function replacer(match, p1, p2, offset, string) {
    // match is smalloc_leak[11b3] 
    // p1 is function(smalloc_leak)
    // p2 is address(11b3)
    // offset is matching offset(0)
    // string is target string(smalloc_leak[11b3] malloc(0x5) = 0x55616343b890)
    let r_addr2line = addr2func(p1, p2).split(/\r\n|\r|\n/)[0];
    let functionName = r_addr2line.split(' ')[0];
    let r = r_addr2line.split(' ')[2];
    let sourcePath = r.split(':')[0];
    let lineNo = r.split(':')[1];

    return [p1, '[', functionName, '(', sourcePath, ':', lineNo, ')]'].join('');
}

rl.on('line', function (line) {
    line = line.split(/\r\n|\r|\n/)[0];
    let r = line.match(/([/\w+]*)\[([0-9a-fA-F]+)\]/);
    if (r !== null) {

        var result = line.replace(/([/\w+]*)\[([0-9a-fA-F]+)\]/, replacer);
        console.log(result);
    }
});