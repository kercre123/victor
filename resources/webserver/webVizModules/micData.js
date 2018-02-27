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

    if ( location.port != "8889" )
    { 
      $('<h3>You must use this tab with the anim process (port 8889)</h3>').appendTo(elem);
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
    canvas.width = 500;

    // default to an empty clock face
    drawClockFace();
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.

    var canvas = $('#myCanvas')[0];
    var context = canvas.getContext("2d");

    // clear our canvas so we can paint!
    context.clearRect(0, 0, canvas.width, canvas.height);

    // need to redraw this each time
    drawClockFace();

    // draw our direction line values ...
    for ( i = 0; i < 12; i++ )
    {
      if ( data.directions[i] > 0 )
      {
        var barFactor = ( data.directions[i] / data.maxConfidence );
        context.beginPath();
        context.moveTo( ClockData.center[0], ClockData.center[1] );
        var lineX = ClockData.center[0] + ( ClockData.offsets[i][0] * barFactor * ClockData.radius );
        var lineY = ClockData.center[1] + ( ClockData.offsets[i][1] * barFactor * ClockData.radius );
        context.lineWidth = 2;
        context.lineTo( lineX, lineY );
        context.stroke();
      }
    }

    // draw our dominant direction ...
    var dotRadius = 7;
    var dotX = ClockData.center[0] + ( ClockData.offsets[data.dominant][0] * ClockData.radius );
    var dotY = ClockData.center[1] + ( ClockData.offsets[data.dominant][1] * ClockData.radius );

    context.beginPath();
    context.arc( dotX, dotY, dotRadius, 0, 2*Math.PI );
    context.fillStyle = '#FF0000';
    context.fill();


    // add in our confidence values ...
    var labelX = 250, valueX = 425;
    var textY = 25, textHeight = 25;

    context.font="normal 18px Arial";
    context.textBaseline="top";
    context.fillStyle = '#000000';

    context.textAlign = "start";
    context.fillText( "Confidence : ", labelX, textY );
    context.fillText( data.confidence, valueX, textY );

    textY += textHeight;
    context.fillText( "Delay Time (ms) : ", labelX, textY );
    context.fillText( data.delayTime, valueX, textY );

    if ( data.triggerDetected )
    {
      textY += textHeight;
      context.fillText( "Trigger Word Detected", labelX, textY );
    }
  };

  myMethods.update = function(dt, elem) {

  };

  myMethods.getStyles = function() {
    return "";
  };

})(moduleMethods, moduleSendDataFunc);
