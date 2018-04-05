/*  
 * Displays IBEIConditon info that prevents behavior transitions
 */

(function(myMethods, sendData) {

  var scroller;
  myMethods.init = function(elem) {
    if ( location.port != '8888' ) { 
      $('<h3>You must use this tab with the engine process (port 8888)</h3>').appendTo(elem);
    }
    scroller = $('<div></div>', {id:'conditionsScroller'}).appendTo(elem);
  };

  myMethods.onData = function(data, elem) {
    var fullname = data.owner + '>' + data.name;
    if( data.changed == 'inactive' ) {
      // remove matching element
      $('div[data-name="' + fullname + + '"]').remove();
    } else {
      var text = fullname + ' ' + data.value + ' @t=' + Math.round(data.time*100)/100;
      if( data.list ) {
        text += ' [';
        $.each(data.list, function( index, value ) {
          text += value.name + '=' + value.value;
          if( index != data.list.length - 1 ) {
            text += ', ';
          }
        });
        text += ']';
      }
      var div = $('<div>' + text + '</div>').appendTo(scroller);
      div.attr( 'data-name', fullname );
      div.attr( 'data-owner', data.owner );
      div.addClass('condition-line');

      // sort all the divs so theyre grouped by owner and then condition
      $('div.condition-line').sort(function (a, b) {
        var nameA = $(a).attr('data-name');
        var nameB = $(b).attr('data-name');
        return nameA.localeCompare(nameB);
      });

      // put a space before new owners (css ignores the first space)
      var previous;
      $('div.condition-line').each( function() {
        var owner = $(this).attr('data-owner');
        if( owner != previous ) {
          previous = owner;
          $(this).addClass('first-owner');
        } else {
          $(this).removeClass('first-owner');
        }
      });
    }
  };

  myMethods.update = function(dt, elem) {
    
  };

  myMethods.getStyles = function() {
    return `
      #conditionsScoller {
        overflow-y: scroll; 
        min-height:300px;
        max-height:600px;
        width:100%;
        padding:10px;
      }
      #conditionsScroller div {
        margin-top:5px;
        font-family: monospace;
      }
      div.condition-line {
        text-indent: -10px;
        padding-left: 10px;
      }
      div.condition-line.first-owner:not(:first-child) {
        padding-top:20px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
