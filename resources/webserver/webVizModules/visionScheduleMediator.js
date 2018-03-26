/**
 * File: visionSheduleMediator.js
 *
 * Author: Sam Russell
 * Date:   2/24/2018 
 *
 * Description:  basic javascript file for displaying VisionMode Schedule info in WebViz
 * 
 * Copyright: Anki, Inc. 2016
 **/

(function(myMethods, sendData) {

  var canvasHeight = 500;
  var canvasWidth = 900;
  var labelWidth = 300;
  var rowHeight = 20

  function drawVisionScheduleGrid(rows, cols) {
    var frameWidth = (canvasWidth - labelWidth) / cols;

    var context = $('#vsmCanvas')[0].getContext("2d");

    context.beginPath();
    context.fillStyle = "white";
    context.lineWidth = 2;
    context.strokeStyle = "black";
    for (var row = 0; row < rows; row++) {
      for (var column = 0; column < cols; column++) {
        var x = column * frameWidth;
        var y = row * rowHeight;
        context.rect(x, y, frameWidth, rowHeight);
        context.fill();
        context.stroke();
      }
    }
    context.closePath();
  }

  myMethods.init = function(elem) {

    if ( location.port != '8888' ) { 
      $('<h3>You must use this tab with the engine process (port 8888)</h3>').appendTo(elem);
      return;
    }

    $('<canvas></canvas>', {id: 'vsmCanvas'}).appendTo(elem);

    var canvas = $('#vsmCanvas')[0];
    canvas.height = canvasHeight;
    canvas.width = canvasWidth;

    context = canvas.getContext("2d");
    context.font="normal 18px Arial";
    context.textBaseline="top";
    context.textAlign = "start";
    context.fillStyle = '#000000';
    context.fillText( "Waiting on Data...", 0, 0);
  };

  myMethods.onData = function(data, elem) {

    var canvas = $('#vsmCanvas')[0];
    var context = canvas.getContext("2d");

    context.clearRect(0, 0, canvas.width, canvas.height);
    drawVisionScheduleGrid(data.numActiveModes, data.patternWidth);
    
    // Draw in the frames for each mode, then add a label
    var frameWidth = (canvasWidth - labelWidth) / data.patternWidth;
    var fillSquareWidth = frameWidth * 0.8;
    var fillSquareWidthOffset = (frameWidth - fillSquareWidth) / 2.0;
    var fillSquareHeight = rowHeight * 0.8;
    var fillSquareHeightOffset = (rowHeight - fillSquareHeight) / 2.0;

    context.font="normal 18px Arial";
    context.textBaseline="top";
    context.textAlign = "start";
    var textSpacing = 10;
    var textHeight = rowHeight * 0.8;
    var textYOffset = (rowHeight - textHeight) / 2.0;
    var labelX = canvasWidth - labelWidth + textSpacing;

    for(i = 0; i < data.numActiveModes; i++)
    {
      var modeSchedule = data.fullSchedule[i];
      var label = modeSchedule.visionMode;
      var updatePeriod = modeSchedule.updatePeriod;
      var offset = modeSchedule.offset;
      var ypos = i * rowHeight;
      var labelY = ypos + textYOffset;
      // Add a visionMode label
      context.fillStyle = '#000000';
      context.fillText( modeSchedule.visionMode, labelX, labelY);

      // Fill in the frames this mode is active on
      for(j = offset; j < data.patternWidth; j += updatePeriod)
      {
        var xpos = j * frameWidth;
        // Fill in the appropriate spot
        context.fillStyle = "grey";
        context.fillRect(xpos + fillSquareWidthOffset,
                         ypos + fillSquareHeightOffset,
                         fillSquareWidth,
                         fillSquareHeight);
      }
    }

  };

  myMethods.update = function(dt, elem) {
    // Called regularly. dt is the time since the previous call.
    // Note that this update loop originates from the web page,
    // not the engine, so the times may be different.
    // Any created elements should be placed inside elem.
  };

  myMethods.getStyles = function() {
    // Return a string of all css styles to be added for this
    // module. Don't worry about conflicts with other modules;
    // your styles will only apply to the children of the elem
    // object passed into the above methods. However, the css
    // selectors you provide here should be relative to the 
    // document body, e.g., "#myHtmlElement", 
    // not "#moduleTab #myHtmlElement".
    return "";
  };

})(moduleMethods, moduleSendDataFunc);
  