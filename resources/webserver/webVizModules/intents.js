
(function(myMethods, sendData) {

  var intentTypes = ['user', 'cloud', 'app'];

  myMethods.init = function(elem) {

    $(elem).append('<div id="intent-title">Last-received intents:</div');
    // for each intent type, create a div containing:
    // (1) a label for the current intent
    // (2) a div to display the current intent
    // (3) a dropdown to select an intent
    // (4) a button to send the intent

    var dropDownList = $('<div></div>', {id:'dropDownList'}).appendTo( elem );

    $(elem).append('<div id="intent-title">Resend intents:</div');
    var recentList = $('<div></div>', {id:'repeatEntriesList'}).appendTo( elem );

    for( var i=0; i<intentTypes.length; ++i ) {
      var intent = intentTypes[i];
      
      var container = $('<div></div>', {class:'intent-row', id: ('intent-'+intent)}).appendTo( dropDownList );

      var label = $('<div>' + intent + ':</div>', {class: 'intent-label'}).appendTo( container );
      var div = $('<div></div>', {class: 'current-intent'}).appendTo( container );
      var dropdown = $('<select></select>', {class: 'intent-list'}).appendTo( container ).on('change', function() {
        var selected = $(this).val();
        $(this).next().val(selected);
        this.selectedIndex = 0;
      });
      var placeholder = '';
      if( intent == 'app' ) {
        placeholder = 'intent_name [param]';
      } else if( intent == 'cloud' ) {
        placeholder = 'intent_name, or JSON';
      } else {
        placeholder = 'intent_name';
      }
      var text = $('<input/>', {class: 'intent-text'}).appendTo( container );
      text.attr('placeholder', placeholder );
      var button = $('<input/>', {class: 'intent-submit', type:'submit', value:'Trigger'}).appendTo( container );
      button.on('click', (function(intent, dropdown) {
        return function() {
          var onClick = function(requestedIntent, textBox) {
            if( intent == 'app' ) {
              var url = 'http://' + window.location.href.split('/')[2];
              url += '/sendAppMessage?type=AppIntent&intent='
              var splitRequest = requestedIntent.split(' ');
              if( splitRequest.length == 2 ) {
                url += splitRequest[0] + '&param=' + splitRequest[1];
              } else if( splitRequest.length == 1 ) {
                url += splitRequest[0];
              } else {
                alert('Format is either one string (the intent type), or two strings separated by a space: the intent type and its param');
                return;
              }
              $.get(url);
            } else {
              requestedIntent = '{"intent":"intent_play_music", "params": {"request":"KQED"}}';
              var payload = {intentType: intent, request: requestedIntent};
              sendData( payload );
            }

            textBox.val(''); // clear text box
          }

          var textBox = $(this).prev();
          var requestedIntent = textBox.val();
          onClick( requestedIntent, textBox );

          // add repeat entry if it doesnt exist
          if( $('.repeat-entry div:contains(' + requestedIntent + ')').length == 0 ) {
            var repeatEntry = $('<div></div>', {class: 'repeat-entry'}).appendTo( recentList );
            $('<div>' + requestedIntent + '</div>').appendTo( repeatEntry );
            var repeatBtn = $('<input/>', {type:'submit', value:'Resend'}).appendTo( repeatEntry );
            repeatBtn.on('click', function() { onClick(requestedIntent, textBox); } );
          }
        }
      })(intent, dropdown) );
    }
  };

  myMethods.onData = function(data, elem) {
    for( var i=0; i<data.length; ++i ) {
      var blob = data[i];
      var intent = blob.intentType;
      if( intentTypes.indexOf(intent) >= 0 ) {
        var container = $('#intent-'+intent);
        if( container.length ) {
          if( blob.type == 'current-intent' ) {
            container.find('.current-intent').text( blob.value );
          } else if( blob.type == 'all-intents' ) {
            var list = blob.list;
            var dropdown = container.find('.intent-list');
            dropdown.empty();
            $("<option/>", {val: '', text:'Select a ' + intent + ' intent'}).appendTo(dropdown);
            $(list).each(function() {
              $("<option/>", {val: this, text: this}).appendTo(dropdown);
            });
            dropdown.css('visibility', 'visible');
          }
        }
      }
    }
  };

  myMethods.update = function(dt, elem) {
  };

  myMethods.getStyles = function() {
    return `
      #intent-title {
        font-size:16px;
        margin-bottom:20px;
      }

      .intent-row {
        display: table-row;
      }

      .intent-row * {
        height:30px;
        display: table-cell;
        margin: 0px 5px;
      }
      

      /* initially hidden */
      .intent-list {
        visibility:hidden
      }

      .current-intent,
      .intent-text {
        min-width:100px;
      }
      .intent-list {
        min-width: 200px;
      }
      .current-intent {
        padding-left:10px;
        font-weight:bold;
      }

      .repeat-entry div {
        font-family: courier, courier new, serif;
      }

      .repeat-entry {
        display: table-row;
      }
      .repeat-entry * {
        display: table-cell;
        margin: 10px 5px;
      }


    `;
  };

})(moduleMethods, moduleSendDataFunc);
