(function(myMethods, sendData) {

  // max width of the chart
  var maxWidth_s = 60.0;
  var yLabelPose = -1.0;
  var overlappingWidth_s = 1.3;

  var defaultDisplayedEmotions = ['Stimulated'];

  var gridMarkings = []
  var gridMarkingLabels = []
  var numLabelDivs = 0;
  
  var chartOptions = {
    legend: {
      show: true,
      position: "sw",
      labelFormatter: GetLegendLabel
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
  var dumpedData = [];
  var chart;
  var ignoreOnChange = false;
  var draggingSlider;

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

  function CreateControls( elem, info ) {
    var container  = $( '#bottomContainer' );
    container.remove( '#slidersCntr,#buttonCntr' );
    var slidersCntr = $( '<div></div>', {id: 'slidersCntr'} ).appendTo( container );
    var buttonCntr =  $( '<div></div>', {id: 'buttonCntr'} ).appendTo( container );

    var emotions = info.emotions;
    for( var idx=0; idx<emotions.length; ++idx ) {
      var emotionInfo = emotions[idx];
      var emotionName = emotionInfo.emotionType;

      var onChange = function(value, emotionName) {
        if( ignoreOnChange ) {
          return;
        }
        SetControlsValue( emotionName, value);
      };

      // create a label
      var label = $( '<div class="sliderLabel">' + emotionName + '</div>' ).appendTo( slidersCntr );

      // create a slider
      var slider = $( '<input type="range" class="slider"  data-emotion="' + emotionName + '" step="0.05" ' 
          + 'min="' + emotionInfo.min + '" '
          + 'max="' + emotionInfo.max + '" ' 
          + 'value="' + emotionInfo.min + '">' ).appendTo( slidersCntr );
      slider.on( 'input', function() {
              onChange( this.value, $(this).attr('data-emotion') );
            })
            .on( 'mousedown', function(e){
              draggingSlider = this;
              $( this ).attr( 'data-prev-value', $(this).val() );
            })
            .on( 'mouseup' , function() {
              draggingSlider = undefined;
              var prevValue = $( this ).attr( 'data-prev-value' );
              var newVal = $( this ).val();
              if( newVal !== prevValue ) {
                SendCurrentControlData();
              }
            });

      //create a textbox within a div
      var valueTextDiv = $( '<div class="sliderValueCntr"></div>' ).appendTo( slidersCntr );
      var currentValueText = $( '<input type="text" class="sliderValue"  data-emotion="' + emotionName + '" '
                                 + 'value="' + FloatToString( emotionInfo.min ) + '"></input>' ).appendTo( valueTextDiv );
      currentValueText.change( function() {
                        onChange( parseFloat(this.value.trim()), $( this ).attr( 'data-emotion' ) );
                        SendCurrentControlData();
                      })
                      .keydown( function (e) {
                        if( $.inArray( e.keyCode, [46, 8, 9, 27, 13, 110, 189, 190] ) !== -1 // backspace, delete, tab, escape, enter, -, and .
                            || (e.keyCode == 65 && (e.ctrlKey === true || e.metaKey === true)) // ctrl/cmd+A
                            || (e.keyCode == 67 && (e.ctrlKey === true || e.metaKey === true)) // ctrl/cmd+C
                            || (e.keyCode == 88 && (e.ctrlKey === true || e.metaKey === true)) // ctrl/cmd+X
                            || (e.keyCode >= 35 && e.keyCode <= 39) ) // home, end, left, right
                        {
                          return; // allowed
                        }
                        // ensure that it is a number and stop the keypress
                        if( (e.shiftKey || (e.keyCode < 48 || e.keyCode > 57)) && (e.keyCode < 96 || e.keyCode > 105) ) {
                          e.preventDefault();
                        }
                      })
                      .keyup( (function( min, max ) {
                                return function() {
                                  var val = parseFloat( this.value );
                                  if( !isNaN( val ) ) {
                                    if( val < min ) {
                                      this.value = FloatToString( min );
                                    }
                                    if( val > max ) {
                                      this.value = FloatToString( max );
                                    }
                                  }
                                }
                              })( emotionInfo.min, emotionInfo.max ) );
    }

    var simpleMoods = info.simpleMoods;
    for( var simpleMood in simpleMoods ) {
      if( simpleMoods.hasOwnProperty( simpleMood ) ) {
        var values = simpleMoods[simpleMood];
        var btn = $('<button/>', {
          text: simpleMood,
          click: (function( values ) {
            return function() {
              sendData( values );
            };
          })( values )
        });
        btn.appendTo( buttonCntr );
      }
    }
  }

  function FloatToString( val, prec ) {
    prec = prec || 2;
    var p = Math.pow( 10, prec );
    var strVal = parseFloat( Math.round( val * p ) / p ).toFixed( prec );
    return (val < 0) ? strVal : (' ' + strVal);
  }

  function SetControlsValue( emoName, emoVal ) {
    ignoreOnChange = true;
    // update text
    var textDiv = $( '.sliderValue[data-emotion="' + emoName + '"]' );
    textDiv.val( FloatToString( emoVal ) );
    // update slider
    var slider = $( '.slider[data-emotion="' + emoName + '"]' );
    slider.val( emoVal );
    ignoreOnChange = false;
  }

  function UpdateControlsData() {
    for( var emoName in emoToIdxMap ) {
      if( emoToIdxMap.hasOwnProperty( emoName ) ) {
        var idx = emoToIdxMap[ emoName ];
        var value = moodData[idx][moodData[idx].length - 1][1];

        if( typeof (draggingSlider !== 'undefined') && ($(draggingSlider).attr('data-emotion') == emoName) ) {
          // don't update if it's being dragged
          return;
        }
        if( $('.sliderValue[data-emotion="' + emoName + '"]').is(':focus') ) {
          // don't update if the text is being changed
          return;
        }

        SetControlsValue( emoName, value );
      }
    }
  }

  function SendCurrentControlData() {
    var toSend = {};
    $('.slider').each( function() {
      var emoName = $( this ).attr( 'data-emotion' );
      var emoVal = $( this ).val();
      toSend[emoName] = parseFloat( emoVal );
    });
    sendData( toSend );
  }

  function PruneOverlapingEvents() {
    if( gridMarkings.length <= 1 ) {
      return;
    }

    var toRemove = []

    var last = gridMarkings[0]['xaxis']['from'];
    for( var i=1; i<gridMarkings.length; ++i ) {
      var curr = gridMarkings[i]['xaxis']['from'];
      if( curr - last <= overlappingWidth_s ) {
        // just remove the earlier one (not ideal....)
        toRemove.push(i-1);
        console.log("removing event '" + gridMarkingLabels[i-1] + "' at index " +
                    (i-1) + " at t=" + last + " next t=" + curr);
      }
      else {
        last = curr;
      }
    }

    toRemove.forEach( function(i) {
      gridMarkings.splice(i, 1);
      gridMarkingLabels.splice(i, 1);
    });
  }

  myMethods.init = function(elem) {
    $(elem).append('<div id="chartContainer"></div>');
    var bottomContainer = $('<div id="bottomContainer"></div>').appendTo(elem);
    var leftControls = $('<div id="leftControls"></div>').appendTo(bottomContainer);
    leftControls.append('<div id="simpleMoodDisplay"></div>');
    leftControls.append('<div>' +
                        '<input type="checkbox" id="showEvents" checked/>' +
                        '<label for="showEvents">Show events</label>' +
                        '</div>');
    leftControls.append('<div>' +
                        '<input type="checkbox" id="hideOverlappingEvents"/>' +
                        '<label for="hideOverlappingEvents">Hide overlapping events</label>' +
                        '</div>');
    leftControls.append('<div>' +
                        '<input type="checkbox" id="showLegend" checked/>' +
                        '<label for="showLegend">Show chart legend</label>' +
                        '</div>');
    leftControls.append('<div id="periodControl"' +
                        '<label for="sendPeriod">Update period (seconds)</label>' +
                        '<input type="text" id="sendPeriod" min="0" max="10.0" value="1.0" size="4"/>' +
                        '</div');
    leftControls.append('<div>' +
                        '<input type="checkbox" id="dumpData"/>' +
                        '<label for="dumpData">Dump raw data</label>' +
                        '</div>');
    leftControls.append('<div id="downloadDataDump"></div>');
    leftControls.append('</div>');

    $('#showEvents').change(function() {
      var isChecked =  $(this).is(':checked');
      $('.verticalLabel').toggle( isChecked );
      chart.getOptions().grid.markings = isChecked ? gridMarkings : undefined;
      chart.setupGrid();
      chart.draw();
    });

    $('#showLegend').change(function() {
      var isChecked =  $(this).is(':checked');
      chart.getOptions().legend.show = isChecked;
    });

    $('#hideOverlappingEvents').change(function() {
      if( $(this).is(':checked') ) {
        PruneOverlapingEvents();
      }
    });

    $('#dumpData').change(function() {
      var isChecked =  $(this).is(':checked');
      if( isChecked ) {
        $('#downloadDataDump').text('dumping... (stop to enable download)');
        var link = $('#downloadDataDumpLink');
        if( link ) {
          link.remove();
        }
      }
      else {
        $('#downloadDataDump').empty();
        var url = "data:text/plain;charset=utf-8," + encodeURIComponent(JSON.stringify(dumpedData, null, 2));
        var link = $('#downloadDataDump').append('<a id="downloadDataDumpLink" download="mood_data.json"'+
                                                 ' href=' + url + '>download json</a>');
      }
    });

    $('#sendPeriod').change(function() {
      var value = this.value;
      $.post('consolevarset', {key: 'MoodManager_WebVizPeriod_s', value: value}, function(result){});
    });

    $('body').on('click', '.legendLabel', function () {
      var emo = this.innerText;
      var idx = emoToIdxMap[emo];
      plotData[idx].lines.show = !plotData[idx].lines.show;
      chart.setData(plotData);
      chart.setupGrid();
      chart.draw();
    });
  };

  myMethods.onData = function(data, elem) {

    if( first && (typeof data.moods !== 'undefined') ) {

      for( var i=0; i<data.moods.length; ++i ) {
        var emo = data.moods[i].emotion;
        emoToIdxMap[emo] = i;
        moodData.push( [] );
        var newData = { label: emo,
                        data: moodData[i],
                        lines: {show: true} };
        if( defaultDisplayedEmotions.indexOf( emo ) < 0 ) {
          newData.lines.show = false;
        }
        plotData.push( newData );
      }
      chart = $.plot("#chartContainer", plotData, chartOptions);

      first = false;
    }

    if( typeof data.info !== 'undefined' ) {
      CreateControls( elem, data.info );
      return;
    }

    if( $('#dumpData').is(':checked') ) {
      dumpedData.push( data );
    }

    if( "simpleMood" in data ) {
      $('#simpleMoodDisplay').text( 'SimpleMood: ' + data.simpleMood );
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

    UpdateControlsData();

    if( "emotionEvent" in data ) {
      gridMarkings.push( { xaxis: {from: data["time"], to: data["time"]},
                           color: "#000",
                           lineWidth: 2
                         });
      gridMarkingLabels.push(data["emotionEvent"]);

      if( $('#hideOverlappingEvents').is(':checked') ) {
        PruneOverlapingEvents()
      }
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

      chart.getOptions().grid.markings = $('#showEvents').is(':checked') ? gridMarkings : undefined;
    }


    // fixed width data
    var xMin = data["time"] - maxWidth_s;
    chart.getAxes().xaxis.options.min = xMin;
    chart.getAxes().xaxis.options.max = xMin + maxWidth_s + 0.1;

    chart.setData(plotData);
    chart.setupGrid();
    chart.draw();

    if( gridMarkingLabels.length > 0 ) {
      // add label divs if we need them
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
        div.toggle( $('#showEvents').is(':checked') );
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
      .sliderLabel:not(:first-child) {
        margin-top:10px;
      }
      .sliderValue, .slider {
        vertical-align:middle;
      }
      .sliderValue {
        width:40px;
        padding-left:5px;
        color:white;
        background-color:#6c90d8;
        border:0px;
        height:14px;
      }
      .sliderValueCntr {
        display:inline;
        padding-left:10px;

      }
      .sliderValueCntr::before {
        content: "";
        width: 0;
        height: 0;
        display:inline-block;
        border-top: 7px solid transparent;
        border-bottom: 7px solid transparent;
        vertical-align:middle;
        border-right:7px solid #6c90d8;
      }
      button {
        padding: 5px 10px;
        margin: 5px 5px 10px 40px;
        display:block;
        width:100px;
      }
      #buttonCntr:before {
        content: "Quick-set SimpleMood";
        margin-left:40px;
      }
      #bottomContainer {
        height:160px;
        padding-top:10px;
      }
      #bottomContainer > div {
        float:left;
        width:250px;
        padding-left:20px;
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
      #leftControls > * {
        margin: 0px 3px 10px 0;
      }
      #sendPeriod {
        margin: 0px 3px 10px 0;
      }
      #simpleMoodDisplay {
        font-weight: bold
      }

      `;
  };
})(moduleMethods, moduleSendDataFunc);
