(function(myMethods, sendData) {

  // max width of the chart
  var maxWidth_ms = 10.0*1000.0;
  
  var chartOptions = {
    legend: {
      show: true,
      position: "sw",
      labelFormatter: GetLegendLabel
    },
    yaxis: {
      min: 0.0,
      max: 100.0,
      ticks: [0, 25, 50, 75, 100],
      tickFormatter: function(val, axis) { return val.toString()+ "%"; }
    },
    xaxis: {
      ticks: 10,
      tickLength: 10,

      tickDecimals: 0
    },
    grid: {
      show: true,
    }
  }
  
  var cpuUsed = []
  var currentTime_ms = 0
  var plotData = []
  var chart = null

  function GetLegendLabel(label, series) {
    if (series.lines.show) {
      return `<div class="legendLabelBox">
                <div class="legendLabelBoxFill" style="background-color:` + series.color + `"></div>
              </div>` 
              + label
    } else {
      return `<div class="legendLabelBox">
                <div class="legendLabelBoxUnused"></div>
              </div>` 
              + label
    }
  }

  myMethods.init = function(elem) {
    // automatically turn on CPU webviz reporting, 10ms intervals
    $.post('consolevarset', {key: 'WebvizUpdatePeriod', value: 3}, function(result){});

    $(elem).append('<div id="chartContainer"></div>')

    $('body').on('click', '.legendLabel', function () {
      var stringValues = this.innerText.split(' ')
      var idx = parseInt(stringValues[1]) - 1
      plotData[idx].lines.show = !plotData[idx].lines.show
      chart.setData(plotData)
      chart.setupGrid()
      chart.draw()
    });
  };

  const kNumCPUTimeValues = 8    // Number of time values to read per cpu
  var prevCpuTime = [];

  // http://www.linuxhowtos.org/System/procstat.htm
  function CPUTimeInfo(payload, itemIndex) {
      payload = payload.substring(5) // Strip cpu name and space(s)
      // Parse the first 8 integers out of the line:
      var stringValues = payload.split(' ')
      var values = []
      var totalTime = 0
      for (var i = 0; i < kNumCPUTimeValues; i++) {
          var v = parseInt(stringValues[i])
          values[i] = v
          totalTime += v
      }
      var idleTime = values[3] + values[4]   // 'idle' + 'iowait'
      var usedTime = totalTime - idleTime
      var prev = prevCpuTime[itemIndex]
      var deltaTotalTime = totalTime - prev.prevTotalTime
      var deltaUsedTime = usedTime - prev.prevUsedTime

      var usedPct = deltaUsedTime * 100 / deltaTotalTime

      // Save the last reading for this CPU so it can be used next call
      prev.prevUsedTime = usedTime;
      prev.prevTotalTime = totalTime;

      return usedPct
  }

  myMethods.onData = function(data, elem) {
    currentTime_ms += data.deltaTime_ms

    numCpus = data.usage.length

    if (cpuUsed.length == 0) {
      // populate data
      for (var i = 0; i < numCpus; ++i) {
        cpuUsed.push([])
        prevCpuTime.push({ prevUsedTime:0, prevTotalTime:0 })
      }

      plotData = []
      // ignore 0th entry, it's total cpu that maps to 100%
      // and populate "prev" values used to calculate delta
      for(var i = 1; i < numCpus; ++i) {
        plotData.push({label: "cpu "+i.toString(), data: cpuUsed[i], lines: {show: true}})
        CPUTimeInfo(data.usage[i], i)
      }
    }

    for(var i = 1; i < numCpus; ++i) {
      var usedPct = CPUTimeInfo(data.usage[i], i)

      cpuUsed[i].push([currentTime_ms, usedPct])
    }

    // scroll window
    var num_data = cpuUsed[1].length
    var dt = currentTime_ms - cpuUsed[1][0][0];
    while (dt > maxWidth_ms && num_data > 0) {
      for(var i = 1; i < cpuUsed.length; ++i) {
        cpuUsed[i].shift()
      }
      --num_data;
      dt = currentTime_ms - cpuUsed[1][0][0]
    }

    if (chart == null) {
      chart = $.plot("#chartContainer", plotData, chartOptions);
    }

    // fixed width data
    var xMin = currentTime_ms - maxWidth_ms
    chart.getAxes().xaxis.options.min = xMin
    chart.getAxes().xaxis.options.max = xMin + maxWidth_ms + 0.1

    chart.setData(plotData)
    chart.setupGrid()
    chart.draw()
  };

  myMethods.update = function(dt, elem) {
  }
  
  myMethods.getStyles = function() {
    return `
      #chartContainer {
        height: 370px;
        width: 100%;
      }
      .legendColorBox {
        /* we manually create the boxes below */
        display:none;
      }
      .legendLabel {
        cursor: pointer;
      }
      .legendLabelBox {
        display: inline-block;
        border: 1px solid #ccc;
        padding: 1px;
        height: 14px; 
        width: 14px; 
        vertical-align: middle;
        margin-right: 3px;
      }
      .legendLabelBoxFill {
        display:inline-block;
        width:10px;
        height:10px;
      }
      .legendLabelBoxUnused {
        width: 18px; 
        height: 18px; 
        border-bottom: 1px solid black; 
        transform: translateY(-10px) translateX(-10px) rotate(-45deg); 
        -ms-transform: translateY(-10px) translateX(-10px) rotate(-45deg); 
        -moz-transform: translateY(-10px) translateX(-10px) rotate(-45deg); 
        -webkit-transform: translateY(-10px) translateX(-10px) rotate(-45deg); 
      }
      `
  }
})(moduleMethods, moduleSendDataFunc)
