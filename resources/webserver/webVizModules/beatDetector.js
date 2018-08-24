/*
 *  Lists details about detected beats
 */
(function(myMethods, sendData) {

  var table;
  var autoScroll = true;
  var userScrolling = false;
  var tableDirty = true;
  var paused = false;

  // expected columns from the animProcess
  var dataColumns = ['timeSinceBeat', 'tempo_bpm', 'conf', 'aboveThresh'];
  // pretty versions of that
  var prettyColumns = ['Time Since Beat (sec)', 'Tempo (bpm)', 'Confidence', 'AboveThresh'];
  // should it be displayed by default
  var enabledColumns = [true, true, true, true];

  function CreateTable( elem ) {
    // create the DataTable with prettyColumns as headers

    var tableElem = $( '<table class="display" style="width:100%"></table>' ).appendTo( elem );
    var thead = $( '<thead></thead>' ).appendTo( tableElem );
    $( '<tbody id="list"></tbody>' ).appendTo( tableElem );

    var colsToCreate = '<tr>';
    prettyColumns.forEach( function( col ){
      colsToCreate += '<th>' + col + '</th>';
    });
    colsToCreate += '</tr>';
    $( colsToCreate ).appendTo( thead );

    table = tableElem.DataTable({
      "ordering":          false,
      "scrollY":           "600px",
      "scrollX":           false,
      "scrollCollapse":    true,
      "paging":            false,
      "search": {
        "caseInsensitive": true
      }
    });
    enabledColumns.forEach( function( isVisible, idx ){
      table.column( idx ).visible( isVisible );
    });
    // todo: if this gets slow for long runs, consider the Scroller plugin for DataTables
  }

  function AddTableEntry( data ) {
    // add a row but don't draw

    var columnValues = [];
    for( var i=0; i<dataColumns.length; ++i ) {
      if( data.hasOwnProperty( dataColumns[i] ) ) {
        columnValues.push( data[dataColumns[i]] );
      } else {
        columnValues.push( '' );
      }
    }
    table.row.add( columnValues ); // don't draw now, or it's slow. wait for the update() tick to draw it
  }
  
  function ClearTable() {
    // clear table but don't draw
    table.clear()
  }

  function DrawTable() {

    // disable the effect of scrolling the table setting autoscroll because it wasn't the user scolling
    userScrolling = false;
    // draw with no paging
    table.draw( false );
    if( autoScroll ) {
      // scroll to bottom
      $( '.dataTables_scrollBody' ).scrollTop( $( '.dataTables_scrollBody' )[0].scrollHeight );
    }
  }

  function UserScrolling() {
    userScrolling = true;  // user is scrolling instead of a dynamic page updates
  }

  myMethods.init = function( elem ) {

    $('<p>Beat detector component information</p>').appendTo( elem );

    // add column toggles
    $( '<b>Column toggles:</b>' ).appendTo( elem );
    var ul = $( '<ul class="colToggles"></ul>' ).appendTo( elem );
    prettyColumns.forEach( function( col, idx ){
      var shouldDisplay = enabledColumns[idx] ? 'colEnabled' : '';
      ul.append( '<li class="toggleViz ' + shouldDisplay + '" data-column="' + idx + '">' + col + '</div>' );
    })
    $( '.toggleViz' ).on( 'click', function( e ) {
        e.preventDefault();
        var column = table.column( $( this ).attr( 'data-column' ) );
        column.visible( !column.visible() );
        $( this ).toggleClass( 'colEnabled' );
    });

    var chkPaused = $('<input />', { type: 'checkbox', id: 'chkPaused'}).appendTo( elem ).prop('checked', paused);
    $('<label />', { for: 'chkPaused', text: 'Pause' }).appendTo( elem );

    chkPaused.change( function() {
      paused = $(this).is(':checked');
    });

    beatDetectorInfoDiv = $('<h3 id="detectorInfo"></h3>').appendTo( elem );

    // create table
    CreateTable( elem );

    // toggle autoscroll based on user scroll actions
    $( '.dataTables_scrollBody' ).on( 'scroll', function() {
      if( !userScrolling ) {
        return;
      }
      // enable autoscroll when the user scrolls to the bottom, and disable when scrolling elsewhere
      autoScroll = ($( this ).scrollTop() + $( this ).innerHeight() >= $( this )[0].scrollHeight)
    });

    // call UserScrolling on any mouse scroll, which enable the effect of scrolling the table to set autoscroll
    if( document.addEventListener ) {
      document.addEventListener( 'mousewheel', UserScrolling, false );
      document.addEventListener( 'DOMMouseScroll', UserScrolling, false );
    } else {
      document.attachEvent( 'onmousewheel', UserScrolling );
    }
  };

  myMethods.onData = function( data, elem ) {
    if (paused) {
      return;
    }
    
    detectorInfo = data["detectorInfo"];
    beatDetectorInfoDiv.empty(); // remove old
    beatDetectorInfoDiv.append('<div>' + "Possible beat detected: " + detectorInfo["possibleBeatDetected"] + '</div>');
    beatDetectorInfoDiv.append('<div>' + "Definite beat detected: " + detectorInfo["beatDetected"] + '</div>');
    beatDetectorInfoDiv.append('<div>' + "Latest tempo (bpm): " + detectorInfo["latestTempo_bpm"] + '</div>');
    beatDetectorInfoDiv.append('<div>' + "Latest confidence: " + detectorInfo["latestConf"] + '</div>');
  
    ClearTable()
    data["beatInfo"].reverse().forEach(function(d) {
      AddTableEntry( d );
    });
    tableDirty = true;
  };

  myMethods.update = function( dt, elem ) {
    if( tableDirty ) {
      tableDirty = false;
      DrawTable();
    }
  };

  myMethods.getStyles = function() {
    return `
      li{
        list-style-type:none;
        font-size:1em;
        display:inline-block;
        cursor:pointer;
        margin-right:20px;
      }

      li:before {
        content:' ';
        display:inline-block;
        color:blue;
        width:10px;
        height:12px;
        padding:0 6px 0 0;
      }

      li.colEnabled:before {
        content:'\\2713';
      }

      th {
        font-size: 11px;
      }

      td {
        font-size: 10px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
