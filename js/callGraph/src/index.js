var currentFunc = [];
var funcSeries = [];
var call = [];
var stack = [];
var stack_event = [];
var event = [];
var exeFile = '';
var funcID = 0;

var sync = require('child_process').spawnSync;

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
  stack_event = [];
  event = [];
  funcSeries = [];
  funcID = 0;
}

function parseTraceInfo(data)
{
  var func = data.function;
  var status = data.id;
  var pID = data.pid;
  var time = {};
  var cputime = {};
  var obj = new Object();
  time.tv_sec = data.time1_sec;
  time.tv_nsec = data.time1_nsec;
  cputime.tv_sec = data.time2_sec;
  cputime.tv_nsec = data.time2_nsec;

  obj.status = status;
  obj.time = {};
  obj.time.tv_sec = time.tv_sec;
  obj.time.tv_nsec = time.tv_nsec;
  obj.cputime = {};
  obj.cputime.tv_sec = cputime.tv_sec;
  obj.cputime.tv_nsec = cputime.tv_nsec;

  if (currentFunc[pID] === undefined) {
    currentFunc[pID] = '_';
  }
  if (status === 'I') {
    if (stack[pID] === undefined) {
      stack[pID] = [];
    }
    obj.prevFunc = currentFunc[pID];
    stack[pID].push(obj);
    if (call[pID] === undefined) {
      call[pID] = [];
    }
    if (call[pID][currentFunc[pID] + '->' + func] === undefined) {
      call[pID][currentFunc[pID] + '->' + func] = createTraceInfo();
    }

    call[pID][currentFunc[pID] + '->' + func].count++;
    call[pID][currentFunc[pID] + '->' + func].start_time.tv_sec = time.tv_sec;
    call[pID][currentFunc[pID] + '->' + func].start_time.tv_nsec = time.tv_nsec;
    call[pID][currentFunc[pID] + '->' + func].start_cputime.tv_sec = cputime.tv_sec;
    call[pID][currentFunc[pID] + '->' + func].start_cputime.tv_nsec = cputime.tv_nsec;
    currentFunc[pID] = func;
  } else if (status === 'O') { // status === 'O'
    var prev = stack[pID].pop();
    var diffTime, diffCpuTime;
    var flow = prev.prevFunc + '->' + func;
    diffTime = diff_timespec(prev.time, time);
    diffCpuTime = diff_timespec(prev.cputime, cputime);
    call[pID][flow].depth = stack[pID].length;
    call[pID][flow].time = diffTime;
    call[pID][flow].cputime = diffCpuTime;
    call[pID][flow].sum_time = add_timespec(call[pID][flow].sum_time, diffTime);
    call[pID][flow].sum_cputime = add_timespec(call[pID][flow].sum_cputime, diffCpuTime);
    call[pID][flow].min_time = min_timespec(call[pID][flow].min_time, diffTime);
    var s = min_timespec(call[pID][flow].min_time, diffTime);

    call[pID][flow].min_cputime = min_timespec(call[pID][flow].min_cputime, diffCpuTime);
    call[pID][flow].max_time = max_timespec(call[pID][flow].max_time, diffTime);
    call[pID][flow].max_cputime = max_timespec(call[pID][flow].max_cputime, diffCpuTime);

    if (funcSeries[func] === undefined) {
      funcSeries[func] = [];
    }
    var funcInfo = {};
    funcID ++;
    funcInfo.id = funcID;
    funcInfo.start = prev.time;
    funcInfo.end = time;
    funcInfo.func = func;
    funcInfo.caller = prev.prevFunc;
    funcInfo.PID = pID;
    funcInfo.depth = stack[pID].length;
    funcSeries[func].push(funcInfo);
    currentFunc[pID] = prev.prevFunc;
  } else if (status === 'E') { // event for pthread_create, pthread_exit, exit, fork, tracer_strdup, tracer_strndup, tracer_malloc, tracer_free, tracer_calloc, tracer_realloc, 
    var comment = data.info2;
    event.push({PID: pID, func, time, cputime, comment});
  } else if (status === 'EI') { // event for pthread_join
    if (stack_event[pID] === undefined) {
      stack_event[pID] = [];
    }
  } else if (status === 'EO') { // event for pthread_join
  } else if (status === 'UE') { // user event for tracer_event
  } else if (status === 'UEI') { // user range event for tracer_event_in_r, tracer_event_in
  } else if (status === 'UEO') { // user range event for tracer_event_out_r, tracer_event_out
  }
}

function printDot(buffer)
{
  const regex = /_->/g; // remove pseudo top node
  calc_average();
  if ((buffer === undefined) || (buffer === null))
    buffer = '';
  for (var thread in call) {
    buffer += 'digraph call_graph_'+ thread+' {\n';
    for (var trace in call[thread]) {
      buffer += '  '+trace.replace(regex, '')+'[label="' + print_time(call[thread][trace].min_time)+','+print_time(call[thread][trace].max_time)+'/' + call[thread][trace].count + '"]\n';
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
  for (var PID in call) {
    for (var trace in call[PID]) {
      call[PID][trace].PID = PID;
      call[PID][trace].flow = trace;
      data.call_flow.push(call[PID][trace]);
    }
  }
  data.funcSeries = [];
  for (var f in funcSeries) {
    for (var c = 0; c < funcSeries[f].length; c++) {
      data.funcSeries.push(funcSeries[f][c]);
    }
  }
  data.event = [];
  for (var cnt = 0; cnt < event.length; cnt++) {
    data.event.push(event[cnt]);
  }
  buffer += JSON.stringify(data, null, ' ');
  return buffer;
}

module.exports.parseTraceInfo =  parseTraceInfo;
module.exports.parseTraceInfoInit =  parseTraceInfoInit;
module.exports.printDot =  printDot;
module.exports.printJSON =  printJSON;
