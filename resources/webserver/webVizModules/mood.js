(function(myMethods, sendData) {

  // max width of the chart
  var maxWidth_s = 60.0;
  var yLabelPose = -1.0;

  var gridMarkings = []
  var gridMarkingLabels = []
  var numLabelDivs = 0;
  
  var chartOptions = {
    legend: {
      show: true,
      position: "sw"
    },
    yaxis: {
      min: -1.05,
      max: 1.05,
      ticks: [-1.0, 0.0, 1.0],
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
  
  var first = true;
  var emoToIdxMap = {}
  var moodData = [];
  var plotData = [];
  var chart;

  myMethods.init = function(elem) {
    $(elem).append('<div id="chartContainer"></div>')
  };

  myMethods.onData = function(data, elem) {
    
    if( first ) {
      
      for( var i=0; i<data.moods.length; ++i ) {
        var emo = data.moods[i].emotion;
        emoToIdxMap[emo] = i;
        moodData.push( [] );
        plotData.push({ label: emo,
                        data: moodData[i] });
      }
      chart = $.plot("#chartContainer", plotData, chartOptions);
      
      first = false;
    }

    for( var i=0; i<data.moods.length; ++i ) {
      var idx = emoToIdxMap[ data.moods[i].emotion ];
      
      moodData[idx].push( [data["time"], parseFloat(data.moods[i].value) ] );

      var dt = data["time"] - moodData[idx][0][0];
      while( dt > maxWidth_s && moodData[idx].length > 0 ) {
        moodData[idx].shift();
        dt = data["time"] - moodData[idx][0][0];
      }
    }
    
    if( "emotionEvent" in data ) {
      gridMarkings.push( { xaxis: {from: data["time"], to: data["time"]},
                           color: "#000",
                           lineWidth: 2
                         });
      gridMarkingLabels.push(data["emotionEvent"]);
    }

    if( gridMarkings.length > 0 ) {

      var dt = data["time"] - gridMarkings[0].xaxis.from;
      while( dt > maxWidth_s && gridMarkings.length > 0 ) {
        gridMarkings.shift();
        gridMarkingLabels.shift();
        if( gridMarkings.length > 0 ) {
          dt = data["time"] - gridMarkings[0].xaxis.from;
        }
      }

      chart.getOptions().grid.markings = gridMarkings;
    }


    // fixed width data
    var xMin = data["time"] - maxWidth_s;
    chart.getAxes().xaxis.options.min = xMin;
    chart.getAxes().xaxis.options.max = xMin + maxWidth_s + 0.1;

    chart.setData(plotData);
    chart.setupGrid();
    chart.draw();

    if( gridMarkingLabels.length > 0 ) {
      // add lable divs if we need them
      while( numLabelDivs < gridMarkingLabels.length ) {
        $("#chartContainer").append("<div class='verticalLabel' id='vl" + numLabelDivs + "'></div>");
        numLabelDivs++;
      }

      var i;
      for( i=0; i<gridMarkings.length; ++i ) {
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
      for( ; i<numLabelDivs; ++i ) {
        $("#vl" + i).hide();
      }
    }
    
  };

  myMethods.update = function(dt, elem) {
  }
  
  myMethods.getStyles = function() {
    return `
      #myHtmlElement {
        background-color: red;
      }

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
