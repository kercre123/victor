/*  
 *  Lists animations as they play
 */

(function(myMethods, sendData) {

  var list;
  myMethods.init = function(elem) {
    list = $('<div id="animationList"></div>').appendTo(elem);
  };

  myMethods.onData = function(data, elem) {
    
    var animation = data["animation"];
    if( data["type"] == "start" ) {
      var icon = '►';
      var item = $('<p>'  + icon + '&nbsp;' + animation + '</p>');
      item.attr('data-animation', animation);
      list.append(item);
    } else { // stop
      var icon = '◼';
      var entry = list.find('p[data-animation="' + animation + '"]:last-child');
      entry.text( icon + entry.text().slice(1) )
    }
    
  };

  myMethods.update = function(dt, elem) { };

  myMethods.getStyles = function() {
    return `
      #animationList p {
        line-height:15px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
