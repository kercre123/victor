var currentThread = -1
var threads = []

function SwitchThread(event) {
  currentThread = this.selectedIndex
  threads[currentThread].changed = true
}

(function(myMethods, sendData) {

  // max width of the chart
  var maxWidth_s = 60.0;
  
  var chartOptions = {
    legend: {
      noColumns: 1,
      show: true,
      position: "nw"
    },
    xaxis: {
      ticks: 10,
      tickLength: 10,

      tickDecimals: 0
    },
    yaxis: {
      tickFormatter: function(val, axis) { return val < axis.max ? val.toFixed(2) : "ms"; }
    },
    grid: {
      show: true,
    }
  }      
  
  var threadNameToIdxMap = {}
  var chart = null
  var plotData = []

  function DoFieldToggling(that, event, thread) {
    var idx = thread.nameToIdxMap[event.target.id]
    thread.show[idx] = that.checked
    thread.changed = true
  }

  function DoToggling(that, event, thread) {
    if(event.target.id == "min") {
      thread.min = that.checked
    }
    else if(event.target.id == "max") {
      thread.max = that.checked
    }
    else if(event.target.id == "mean") {
      thread.mean = that.checked
    }

    thread.changed = true
  }

  myMethods.init = function(elem) {
    // automatically turn on cpu profiler to webviz
    $.post('consolevarset', {key: 'ProfilerLogOutput', value: 2}, function(result){});

    $(elem).append('<select onchange="SwitchThread.call(this, event)" id="chartSelect" name="Tick">'+
                   '<option>No active ticks</option>')
    $(elem).append('<div id="chartOptions">' +
                   '<input id="mean" type="checkbox" checked="checked" />Mean</li>' +
                   '<input id="min" type="checkbox" />Min</li>' +
                   '<input id="max" type="checkbox" />Max</li>' +
                   '</div>')

    $(elem).append('<div id="chartContainer"></div>')

    $(elem).append('<div id="chartFields">' +
                   '</div>')
  }

  myMethods.onData = function(data, elem) {
    var threadName = data.threadName
    var thread = null
    
    var threadIdx = threadNameToIdxMap[threadName]
    if (threadIdx === undefined) {
      // unknown thread name, new thread
      threadIdx = threads.length

      thread = {"nameToIdxMap": {}, "time": 0, "changed": true, "meanData": [], "maxData": [], "minData": [], "show": [], "mean": true, "min": false, "max": false}
      threads.push(thread)
      threadNameToIdxMap[threadName] = threadIdx

      if (threadIdx == 0) {
        // first thread, remove "no active ticks" option and make it current
        $('#chartSelect').html("")
        currentThread = 0
      }
      $('#chartSelect').append('<option>'+threadName+'</option>')

    } else {
      thread = threads[threadIdx]
    }

    if (data.sample) {
      thread.time = data.time

      var num_samples = data.sample.length
      for (var i=0; i<num_samples; ++i) {
        var sample = data.sample[i]
        var idx = thread.nameToIdxMap[sample.name]
        if (idx === undefined) {
          // add new name to the plot
          idx = thread.meanData.length
          thread.nameToIdxMap[sample.name] = idx
          thread.meanData[idx] = []
          thread.minData[idx] = []
          thread.maxData[idx] = []

          // only the first 10 samples default to on, keeps the graph legend manageable
          if (idx < 10) {
            thread.show.push(true)
          } else {
            thread.show.push(false)
          }

          if (currentThread == threadIdx) {
            if (thread.show[idx]) {
              $('#chartFields').append('<input id="'+sample.name+'" type="checkbox" checked="checked" />'+sample.name+'</li>&emsp;')
            }

            $("#chartOptions").find("input[type='checkbox']").click(function (event) {
              DoToggling(this, event, thread)
            })

            $("#chartFields").find("input[type='checkbox']").click(function (event) {
              DoFieldToggling(this, event, thread)
            })
          }
        }

        thread.meanData[idx].push([thread.time, sample.mean])
        thread.minData[idx].push([thread.time, sample.min])
        thread.maxData[idx].push([thread.time, sample.max])

        // scroll window
        var num_data = thread.meanData[idx].length
        var dt = thread.time - thread.meanData[idx][0][0]
        while (dt > maxWidth_s && num_data > 0) {
          thread.meanData[idx].shift()
          thread.minData[idx].shift()
          thread.maxData[idx].shift()
          --num_data
          dt = thread.time - thread.meanData[idx][0][0]
        }
      }
    }

    if (currentThread >= 0 && threads[currentThread].changed) {
      thread = threads[currentThread]
      thread.changed = false

      $('#chartFields').html('')
      for (field in thread.nameToIdxMap) {
        var idx = thread.nameToIdxMap[field]
        if (thread.show[idx]) {
          $('#chartFields').append('<input id="'+field+'" type="checkbox" checked="checked" />'+field+'</li>&emsp;')
        } else {
          $('#chartFields').append('<input id="'+field+'" type="checkbox"/>'+field+'</li>&emsp;')
        }
      }

      $("#chartOptions").find("input[type='checkbox']").click(function (event) {
         DoToggling(this, event, thread)
      })

      $("#chartFields").find("input[type='checkbox']").click(function (event) {
         DoFieldToggling(this, event, thread)
      })

      var noColumns = 0
      if (thread.mean) {
        noColumns++
      }
      if (thread.min) {
        noColumns++
      }
      if (thread.max) {
        noColumns++
      }
      chartOptions.legend.noColumns = noColumns

      plotData = []
      var t = {}
      var name
      if (thread.mean) {
        for (name in thread.nameToIdxMap) {
          var idx = thread.nameToIdxMap[name]
          if (thread.show[idx]) {
              plotData.push({label: name, data: thread.meanData[idx]})
          }
        }
      }
      if (thread.min) {
        for (name in thread.nameToIdxMap) {
          var idx = thread.nameToIdxMap[name]
          if (thread.show[idx]) {
              plotData.push({label: name+" [min]", data: thread.minData[idx]})
          }
        }
      }
      if (thread.max) {
        for (name in thread.nameToIdxMap) {
          var idx = thread.nameToIdxMap[name]
          if (thread.show[idx]) {
              plotData.push({label: name+" [max]", data: thread.maxData[idx]})
          }
        }
      }
    }

    if(chart == null) {
      chart = $.plot("#chartContainer", plotData, chartOptions)
    }

    // fixed width data
    thread = threads[currentThread]
    var xMin = thread.time - maxWidth_s
    chart.getAxes().xaxis.options.min = xMin
    chart.getAxes().xaxis.options.max = xMin + maxWidth_s + 0.1

    chart.setData(plotData)
    chart.setupGrid()
    chart.draw()
  }

  myMethods.update = function(dt, elem) {
  }
  
  myMethods.getStyles = function() {
    return `
      #chartContainer {
        height: 370px;
        width: 100%;
      }

      .verticalLabel {        
        text-align: left;
        transform: rotate(-90deg);
        transform-origin: left;
        color: #606060
      }
      `
  }

})(moduleMethods, moduleSendDataFunc)
