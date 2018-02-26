/*  
 * Displays IBEIConditon info that prevents behavior transitions
 */

(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    if ( location.port != '8888' ) { 
      $('<h3>You must use this tab with the engine process (port 8888)</h3>').appendTo(elem);
    }
  };

  myMethods.onData = function(data, elem) {
    var time = data.time;
    var objects = data.objects;
    var s = '';
    for( var i=0; i<objects.length; ++i) {
      s += objects[i];
      if( i < objects.length-1 ) {
        s += ',';
      }
    }

    var res = Math.round(time*100)/100 + ' ' + s;
    $('<p>' + res + '</p>').appendTo(elem);

  };

  myMethods.update = function(dt, elem) {
    
  };

  myMethods.getStyles = function() {
    return '';
  };

})(moduleMethods, moduleSendDataFunc);
