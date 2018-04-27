
(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    $(elem).append('<div><marquee direction="down" width="500" height="100" behavior="alternate" style="border:solid">' +
      '<marquee behavior="alternate">Cloud Intent Results</marquee></marquee></div>');
    $(elem).append('<div id="result-table"></div>');
  };

  myMethods.onData = function(data, elem) {
    if (!(data instanceof Array)) {
      data = [data];
    }
    var parent = $('#result-table');
    data.forEach(elem => {
      var d = new Date(0);
      d.setUTCSeconds(elem.time);
      delete elem.time;
      if (elem.type == "debugFile") {
        $('<div>' + d + ' - <a href="' + elem.file.substr('/data/data/com.anki.victor'.length)
          + '">Captured audio link</a></div>').appendTo(parent);
      }
      else {
        $('<div>' + d + ' - ' + JSON.stringify(elem) + '</div>').appendTo(parent);
      }
    });
  };

  myMethods.update = function(dt, elem) {
  };

  myMethods.getStyles = function() {
    return '';
  };

})(moduleMethods, moduleSendDataFunc);
