/*
 * This module draws the directional mic information on a clock face,
 * along with confidence values and other debug information.
 * Only useful on a Victor robot as Webots has no direction information from
 * the mic.
 */

(function(myMethods, sendData) {

  var ClockData = {}
  ClockData.center = [125, 125]
  ClockData.radius = 100;
   // max width of the chart
  var maxWidth_s = 60.0;
  var chartOptions = {
    legend: {
      show: true,
      position: "sw",
      labelFormatter: GetLegendLabel
    },
    yaxis: {
      min: 0.0,
      max: 10.0,
      ticks: [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0],
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
  var plotData = [];
  var micDataPower = [];
  var micDataPeakAverage = [];
  var micDataPeakThreshold = [];
  var micDataPeakMinThreshold = [];
  var chart;


  function GetLegendLabel(label, series) {
    if( series.lines.show ) {
      return `<div class="legendLabelBox">
                <div class="legendLabelBoxFill" style="background-color:` + series.color + `"></div>
              </div>`
              + label;
    } else {
      return `<div class="legendLabelBox">
                <div class="legendLabelBoxUnused"></div>
              </div>`
              + label;
    }
  }


  function drawClockFace() {
    var context = $('#myCanvas')[0].getContext("2d");
     // draw our clock face ...
     // larger filled circle
    context.beginPath();
    context.arc( ClockData.center[0], ClockData.center[1], ClockData.radius, 0, 2*Math.PI );
    context.lineWidth = 5;
    context.strokeStyle = '#0000FF';
    context.stroke();
    context.fillStyle = '#FFFFFF';
    context.fill();
     // center dot
    context.beginPath();
    context.arc( ClockData.center[0], ClockData.center[1], 2, 0, 2*Math.PI );
    context.fillStyle = '#000000';
    context.fill();
  }

  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    if ( location.port != "8888" )
    {
      $('<h3>You must use this tab with the engine process (port 8888)</h3>').appendTo(elem);
    }

    var angleFactorA = 0.866; // cos(30 degrees)
    var angleFactorB = 0.5; // sin(30 degrees)

    // Multiplying factors (cos/sin) for the clock directions.
    // NOTE: Needs to have the 13th value so the unknown direction dot can display properly
    ClockData.offsets =
    [
      [-0.0, -1.0], // 12 o'clock - in front of robot so point down
      [angleFactorB, -angleFactorA], // 1 o'clock
      [angleFactorA, -angleFactorB], // 2 o'clock
      [1.0, -0.0], // 3 o'clock
      [angleFactorA, angleFactorB], // 4 o'clock
      [angleFactorB, angleFactorA], // 5 o'clock
      [-0.0, 1.0], // 6 o'clock - behind robot so point up
      [-angleFactorB, angleFactorA], // 7 o'clock
      [-angleFactorA, angleFactorB], // 8 o'clock
      [-1.0, -0.0], // 9 o'clock
      [-angleFactorA, -angleFactorB], // 10 o'clock
      [-angleFactorB, -angleFactorA], // 11 o'clock
      [-0.0, -0.0] // Unknown direction
    ];

    // create our canvas
    $('<canvas></canvas>', {id: 'myCanvas'}).appendTo(elem);

    // set our canvas size to fit all of our stuff
    var canvas = $('#myCanvas')[0];
    canvas.height = 250;
    canvas.width = 700;

    // default to an empty clock face
    drawClockFace();

    // Add in the chart for displaying loudness
    $(elem).append('<div id="chartContainer"></div>');
    $('body').on('click', '.legendLabel', function () {
      var labelName = this.innerText;
      var labelIdx = -1;
      for (i=0; i < plotData.length; i++)
      {
        if (plotData[i].label == labelName)
        {
          labelIdx = i;
          break;
        }
      }
       if (labelIdx != -1)
      {
        plotData[labelIdx].lines.show = !plotData[labelIdx].lines.show;
        chart.setData(plotData);
        chart.setupGrid();
        chart.draw();
      }
    });
  };


  myMethods.onData = function(data, elem) {

    // The engine has sent a new json blob.
    var canvas = $('#myCanvas')[0];
    var context = canvas.getContext("2d");

    // clear our canvas so we can paint!
    context.clearRect(0, 0, canvas.width, canvas.height);

    // need to redraw this each time
    drawClockFace();

    var dotRadius = 7;
    var dotX = ClockData.center[0] + ( ClockData.offsets[data.dominant][0] * ClockData.radius );
    var dotY = ClockData.center[1] + ( ClockData.offsets[data.dominant][1] * ClockData.radius );

    context.beginPath();
    context.arc( dotX, dotY, dotRadius, 0, 2*Math.PI );
    context.fillStyle = '#FF0000';
    context.fill();

    // Update data used in the chart
    var micPowerValue = parseFloat(data["latestPowerValue"]);
    var micNoiseFloor = parseFloat(data["latestNoiseFloor"]);
    var micPeakValue = parseFloat(data["powerScore"]);
    var micPeakAverage = parseFloat(data["powerScoreAvg"]);
    var micPeakThreshold = parseFloat(data["powerScoreThreshold"]);
    var micPeakMinThreshold = parseFloat(data["powerScoreMinThreshold"])

    micDataPower.push( [data["time"], micPowerValue] );
    micDataPeakAverage.push( [data["time"], micNoiseFloor+micPeakAverage] );
    micDataPeakThreshold.push( [data["time"], micNoiseFloor+micPeakAverage+micPeakThreshold] );
    micDataPeakMinThreshold.push( [data["time"], micNoiseFloor+micPeakAverage+micPeakMinThreshold] );

    // add in our confidence values ...
    var labelX = 250, valueX = 375;
    var textY = 25, textHeight = 25;

    context.font="normal 18px Arial";
    context.textBaseline="top";
    context.fillStyle = '#000000';
    context.textAlign = "start";

    context.fillText( "Confidence : ", labelX, textY );
    context.fillText( data.confidence, valueX, textY );

    textY += textHeight;
    context.fillText( "Scoring Level : ", labelX, textY );
    context.fillText( micPeakValue.toFixed(3), valueX, textY );

    textY += textHeight;
    context.fillText( "Scoring Avg : ", labelX, textY );
    context.fillText( micPeakAverage.toFixed(3), valueX, textY );

    valueX = 450;
    if ( data.isTriggered )
    {
      textY += textHeight*2;
      context.fillText( "Reaction Score : ", labelX, textY );
      context.fillText( data.triggerScore.toFixed(3), valueX, textY );
       textY += textHeight;
      context.fillText( "Reaction Confidence : ", labelX, textY );
      context.fillText( data.triggerConfidence, valueX, textY );
       textY += textHeight;
      context.fillText( "Reaction Direction : ", labelX, textY );
      context.fillText( data.triggerDirection, valueX, textY );
    }

    // Set up chart the first time
    if( first ) {
      var newData = { label: "Mic Power",
                      data: micDataPower,
                      lines: {show: true} };
      plotData.push( newData );
       newData = { label: "Mic Peak Avg",
                      data: micDataPeakAverage,
                      lines: {show: true} };
      plotData.push( newData );
       newData = { label: "Mic Power Trigger",
                      data: micDataPeakThreshold,
                      lines: {show: true} };
      plotData.push( newData );
       newData = { label: "Mic Conf Trigger",
                      data: micDataPeakMinThreshold,
                      lines: {show: true} };
      plotData.push( newData );
       chart = $.plot("#chartContainer", plotData, chartOptions);
      first = false;
    }

    // Remove old data
    var dt = data["time"] - micDataPower[0][0];
    while( dt > maxWidth_s && micDataPower.length > 0 ) {
      micDataPower.shift();
      dt = data["time"] - micDataPower[0][0];
    }

    dt = data["time"] - micDataPeakAverage[0][0];
    while( dt > maxWidth_s && micDataPeakAverage.length > 0 ) {
      micDataPeakAverage.shift();
      dt = data["time"] - micDataPeakAverage[0][0];
    }

    dt = data["time"] - micDataPeakThreshold[0][0];
    while( dt > maxWidth_s && micDataPeakThreshold.length > 0 ) {
      micDataPeakThreshold.shift();
      dt = data["time"] - micDataPeakThreshold[0][0];
    }

    dt = data["time"] - micDataPeakMinThreshold[0][0];
    while( dt > maxWidth_s && micDataPeakMinThreshold.length > 0 ) {
      micDataPeakMinThreshold.shift();
      dt = data["time"] - micDataPeakMinThreshold[0][0];
    }

    // fixed width data
    var xMin = data["time"] - maxWidth_s;
    chart.getAxes().xaxis.options.min = xMin;
    chart.getAxes().xaxis.options.max = xMin + maxWidth_s + 0.1;

    chart.setData(plotData);
    chart.setupGrid();
    chart.draw();
  };


  myMethods.update = function(dt, elem) {};

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
    `;
  };

})(moduleMethods, moduleSendDataFunc);
