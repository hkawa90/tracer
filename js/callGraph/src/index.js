var currentFunc = [];
var funcSeries = [];
var call = [];
var stack = [];


function diff_timespec(start, end)
{
  var r = {};
  if ((end.tv_nsec - start.tv_nsec) < 0) {
    r.tv_sec = end.tv_sec - start.tv_sec - 1;
    r.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
  } else {
    r.tv_sec = end.tv_sec - start.tv_sec;
    r.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return r;
}

function add_timespec(v1, v2)
{
  var r = {};
  if ((v1.tv_nsec + v2.tv_nsec) > 1000000000) {
    r.tv_sec = v1.tv_sec + v2.tv_sec + 1;
    r.tv_nsec = v1.tv_nsec + v2.tv_nsec - 1000000000;
  } else {
    r.tv_sec = v1.tv_sec + v2.tv_sec;
    r.tv_nsec = v1.tv_nsec + v2.tv_nsec;
  }
  return r;
}

function max_timespec(v1, v2)
{
  var r = {};
  if (v1.tv_sec > v2.tv_sec) {
    return v1;
  } else if (v1.tv_sec === v2.tv_sec) {
    if (v1.tv_nsec > v2.tv_nsec) {
      return v1;
    } else {
      return v2;
    }
  } else {
    return v2;
  }
}

function min_timespec(v1, v2)
{
  var r = {};
  if (v1.tv_sec < v2.tv_sec) {
    return v1;
  } else if (v1.tv_sec === v2.tv_sec) {
    if (v1.tv_nsec > v2.tv_nsec) {
      return v2;
    } else {
      return v1;
    }
  } else {
    return v2;
  }
}

function div_timespec(v, divideNumber)
{
  var v1, v2;
  v1 = v2 = {};
  v1.tv_sec = v1.tv_nsec = 0;
  v2.tv_sec = v2.tv_nsec = 0;
  if (v.tv_sec !== 0)
    v1.tv_sec = v.tv_sec / divideNumber;
  if (v.tv_nsec !== 0)
    v2.tv_nsec = Math.floor(v.tv_nsec / divideNumber);
  return add_timespec(v1, v2);
}

function calc_average()
{
  for (var thread in call) {
    for (var trace in call[thread]) {
      var count = call[thread][trace].count;
      var ave_time = div_timespec(call[thread][trace].sum_time, count);
      call[thread][trace].avg_time = ave_time;
      var ave_cputime = div_timespec(call[thread][trace].sum_cputime, count);
      call[thread][trace].avg_cputime = ave_cputime;
    }
  }
}

function print_time(t)
{
  var r;
  if (t.tv_sec === 0) {
    if ((t.tv_nsec / 1000) < 0) {
      // nano sec
      r = t.tv_nsec.toString() + 'ns';
    } else {
      // ms
      if ((t.tv_nsec / 1000000) < 0) {
        r = (t.tv_nsec / 1000).toString() + 'μsec';
      } else {
        r = (t.tv_nsec / 1000).toString() + 'ms';
      }
    }
  } else {
    r = (t.tv_sec + (t.tv_nsec / 1000000)).toString() + 's';
  }
  return r;
}

function createTraceInfo()
{
  var obj = new Object();
  obj.count = 0;

  obj.time = {};
  obj.time.tv_sec = 0;
  obj.time.tv_nsec = 0;

  obj.cputime = {};
  obj.cputime.tv_sec = 0;
  obj.cputime.tv_nsec = 0;

  obj.start_time = {};
  obj.start_time.tv_sec = 0;
  obj.start_time.tv_nsec = 0;

  obj.start_cputime = {};
  obj.start_cputime.tv_sec = 0;
  obj.start_cputime.tv_nsec = 0;

  obj.sum_time = {};
  obj.sum_time.tv_sec = 0;
  obj.sum_time.tv_nsec = 0;

  obj.sum_cputime = {};
  obj.sum_cputime.tv_sec = 0;
  obj.sum_cputime.tv_nsec = 0;

  obj.min_time = {};
  obj.min_time.tv_sec = 1000000000;
  obj.min_time.tv_nsec = 999999999;
  obj.max_time = {};
  obj.max_time.tv_sec = 0;
  obj.max_time.tv_nsec = 0;
  obj.avg_time = {};
  obj.avg_time.tv_sec = 0;
  obj.avg_time.tv_nsec = 0;

  obj.min_cputime = {};
  obj.min_cputime.tv_sec = 1000000000;
  obj.min_cputime.tv_nsec = 999999999;
  obj.max_cputime = {};
  obj.max_cputime.tv_sec = 0;
  obj.max_cputime.tv_nsec = 0;
  obj.avg_cputime = {};
  obj.avg_cputime.tv_sec = 0;
  obj.avg_cputime.tv_nsec = 0;
  return obj;
}
function parseTraceInfoInit()
{
  currentFunc = [];
  call = [];
  stack = [];
  funcSeries = [];
}

function parseTraceInfo(line)
{
  var words = line.split(/\s+/);
  var func = words[2];
  var status = words[0];
  var threadID = words[1];
  var time = {};
  var cputime = {};
  var obj = new Object();
  time.tv_sec = words[3];
  time.tv_nsec = words[4];
  cputime.tv_sec = words[5];
  cputime.tv_nsec = words[6];

  obj.status = status;
  obj.time = {};
  obj.time.tv_sec = time.tv_sec;
  obj.time.tv_nsec = time.tv_nsec;
  obj.cputime = {};
  obj.cputime.tv_sec = cputime.tv_sec;
  obj.cputime.tv_nsec = cputime.tv_nsec;

  if (currentFunc[threadID] === undefined) {
    currentFunc[threadID] = '_';
  }
  if (status === 'E') {
    if (stack[threadID] === undefined) {
      stack[threadID] = [];
    }
    obj.prevFunc = currentFunc[threadID];
    stack[threadID].push(obj);
    if (call[threadID] === undefined) {
      call[threadID] = [];
    }
    if (call[threadID][currentFunc[threadID] + '->' + func] === undefined) {
      call[threadID][currentFunc[threadID] + '->' + func] = createTraceInfo();
    }

    call[threadID][currentFunc[threadID] + '->' + func].count++;
    call[threadID][currentFunc[threadID] + '->' + func].start_time.tv_sec = time.tv_sec;
    call[threadID][currentFunc[threadID] + '->' + func].start_time.tv_nsec = time.tv_nsec;
    call[threadID][currentFunc[threadID] + '->' + func].start_cputime.tv_sec = cputime.tv_sec;
    call[threadID][currentFunc[threadID] + '->' + func].start_cputime.tv_nsec = cputime.tv_nsec;
    currentFunc[threadID] = func;
  } else { // status === 'X'
    var prev = stack[threadID].pop();
    var diffTime, diffCpuTime;
    var flow = prev.prevFunc + '->' + func;
    diffTime = diff_timespec(prev.time, time);
    diffCpuTime = diff_timespec(prev.cputime, cputime);
    call[threadID][flow].depth = stack[threadID].length;
    call[threadID][flow].time = diffTime;
    call[threadID][flow].cputime = diffCpuTime;
    call[threadID][flow].sum_time = add_timespec(call[threadID][flow].sum_time, diffTime);
    call[threadID][flow].sum_cputime = add_timespec(call[threadID][flow].sum_cputime, diffCpuTime);
    call[threadID][flow].min_time = min_timespec(call[threadID][flow].min_time, diffTime);
    var s = min_timespec(call[threadID][flow].min_time, diffTime);

    call[threadID][flow].min_cputime = min_timespec(call[threadID][flow].min_cputime, diffCpuTime);
    call[threadID][flow].max_time = max_timespec(call[threadID][flow].max_time, diffTime);
    call[threadID][flow].max_cputime = max_timespec(call[threadID][flow].max_cputime, diffCpuTime);

    if (funcSeries[func] === undefined) {
      funcSeries[func] = [];
    }
    var funcInfo = {};
    funcInfo.start = prev.time;
    funcInfo.end = time;
    funcInfo.func = func;
    funcInfo.caller = prev.prevFunc;
    funcInfo.depthr = stack[threadID].length;
    funcSeries[func].push(funcInfo);
    currentFunc[threadID] = prev.prevFunc;
  }
}

function printDot(buffer)
{
  calc_average();
  if ((buffer === undefined) || (buffer === null))
    buffer = '';
  for (var thread in call) {
    buffer += 'digraph call_graph_'+ thread+' {\n';
    for (var trace in call[thread]) {
      buffer += '  '+trace+'[label="' + print_time(call[thread][trace].min_time)+','+print_time(call[thread][trace].max_time)+'/' + call[thread][trace].count + '"]\n';
    }
    buffer += '}\n';
  }
  return buffer;
}

function printJSON(buffer)
{
  calc_average();
  var data = {};
  if ((buffer === undefined) || (buffer === null))
    buffer = '';
  data.call_flow = [];
  for (var threadID in call) {
    for (var trace in call[threadID]) {
      call[threadID][trace].threadID = threadID;
      call[threadID][trace].flow = trace;
      data.call_flow.push(call[threadID][trace]);
    }
  }
  data.funcSeries = [];
  for (var f in funcSeries) {
    for (var c = 0; c < funcSeries[f].length; c++) {
      data.funcSeries.push(funcSeries[f][c]);
    }
  }
  buffer += JSON.stringify(data, null, ' ');
  return buffer;
}

module.exports.parseTraceInfo =  parseTraceInfo;
module.exports.parseTraceInfoInit =  parseTraceInfoInit;
module.exports.printDot =  printDot;
module.exports.printJSON =  printJSON;
