(function(myMethods, sendData) {

  // max width of the chart
  var maxWidth_s = 60.0;
  var yLabelPose = -1.0;

  var gridMarkings = []
  var gridMarkingLabels = []
  var numLabelDivs = 0;
  
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
    grid: {
      show: true,
    }
  }      
  
  var nameToIdxMap = {};
  var meanData = [];
  var maxData = [];
  var minData = [];
  var plotData = [];
  var chart = null;

  function DoToggling() {
    var noColumns = 0;
    if ($("#mean").is(":checked")) {
      noColumns++;
    }
    if ($("#min").is(":checked")) {
      noColumns++;
    }
    if ($("#max").is(":checked")) {
      noColumns++;
    }
    chart.getOptions().legend.noColumns = noColumns;

    plotData = []
    var idx = 0
    for (var name in nameToIdxMap) {
      if ($("#mean").is(":checked")) {
        plotData.push({label: name, data: meanData[idx]});
      }
      if ($("#min").is(":checked")) {
        plotData.push({label: name+" [min]", data: minData[idx]});
      }
      if ($("#max").is(":checked")) {
        plotData.push({label: name+" [max]", data: maxData[idx]});
      }
      idx++;
    }
  }

  myMethods.init = function(elem) {
    $(elem).append('<div id="chartContainer"></div>');
    $(elem).append('<div id="chartOptions">' +
                   '<input id="mean" type="checkbox" checked="checked" />Mean</li>' +
                   '<input id="min" type="checkbox" />Min</li>' +
                   '<input id="max" type="checkbox" />Max</li>' +
                   '</div>');

    $("#chartOptions").find("input[type='checkbox']").click(function () {
      DoToggling();
    });
  };

  myMethods.onData = function(data, elem) {
    if (!chart) {
      chart = $.plot("#chartContainer", plotData, chartOptions);
    }

    var len = data.sample.length;
    for (var i=0; i<len; ++i) {
      var sample = data.sample[i]
      var name = sample.name;

      var idx = nameToIdxMap[ name ];
      if (idx === undefined) {
        // add new name to the plot
        idx = meanData.length
        nameToIdxMap[name] = idx;
        meanData[idx] = []
        minData[idx] = []
        maxData[idx] = []

        if ($("#mean").is(":checked")) {
          plotData.push({ label: name, data: meanData[idx] });
        }
        if ($("#min").is(":checked")) {
          plotData.push({ label: name+" [min]", data: minData[idx] });
        }
        if ($("#max").is(":checked")) {
          plotData.push({ label: name+" [max]", data: maxData[idx] });
        }
      }

      meanData[idx].push([data.time, sample.mean]);
      minData[idx].push([data.time, sample.min]);
      maxData[idx].push([data.time, sample.max]);

      var dt = data.time - meanData[idx][0][0];
      while (dt > maxWidth_s && meanData[idx].length > 0) {
        meanData[idx].shift();
        minData[idx].shift();
        maxData[idx].shift();
        dt = data.time - meanData[idx][0][0];
      }
    }

    // fixed width data
    var xMin = data.time - maxWidth_s;
    chart.getAxes().xaxis.options.min = xMin;
    chart.getAxes().xaxis.options.max = xMin + maxWidth_s + 0.1;

    chart.setData(plotData);
    chart.setupGrid();
    chart.draw();

    if (gridMarkingLabels.length > 0) {
      // add lable divs if we need them
      while (numLabelDivs < gridMarkingLabels.length) {
        $("#chartContainer").append("<div class='verticalLabel' id='vl" + numLabelDivs + "'></div>");
        numLabelDivs++;
      }

      var i;
      for (i=0; i<gridMarkings.length; ++i) {
        var div = $("#vl" + i);

        div.text(gridMarkingLabels[i]);

        var chartPos = { x: gridMarkings[i].xaxis.from, y: yLabelPose};
        var pos = chart.pointOffset(chartPos);
        div.show();
        div.css({position: "absolute",
                 left: (pos.left - div.height() + 2) + "px",
                 top: (pos.top - 2) + "px"});

      }

      // hide remaining divs
      for (; i<numLabelDivs; ++i) {
        $("#vl" + i).hide();
      }
    }
    
  };

  myMethods.update = function(dt, elem) {
  };
  
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
      `;
  };

})(moduleMethods, moduleSendDataFunc);
