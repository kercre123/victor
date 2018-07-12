
(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    var cloudDiv = $('<div id="cloud"></div>').appendTo(elem);
    cloudDiv.append('<div id="cloudBottom"></div>' +
                    '<div id="cloudRight"></div>' +
                    '<div id="cloudLeft"></div>');
    $(elem).append('<marquee direction="down" width="400" height="110" behavior="alternate" style="border:solid">' +
                      '<marquee behavior="alternate">Cloud Intent Results</marquee>' +
                    '</marquee>');

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
    return `
      #cloud {
        width:175px;
        height:100px;
        position: relative;
        float:right;
      }

      #cloud div {
        border: solid 5px black;
      }

      #cloudBottom {
        background-color: #fff;
        border-radius: 50px;
        height: 75px;
        position: absolute; 
        top: 30px;
        width: 175px;
        z-index: 1;
      }

      #cloudRight {
        background-color: #fff;
        border-radius: 100%;
        height: 75px;
        left: 70px;
        position: absolute; 
        top: 0px; 
        width: 75px;
        z-index: 0;
      }

      #cloudLeft {
        background-color: #fff;
        border-radius: 100%;
        height: 50px;
        left: 25px;
        position: absolute; 
        top: 15px; 
        width: 50px;
        z-index: 0;
      }


      #cloud::before {
        background-color: white;
        border-radius: 50%;
        content: '';
        height: 49px;
        left: 28px;
        position: absolute; 
        top: 19px; 
        width: 44px;
        z-index: 2;
     }

      #cloud::after {
        position: absolute; top: 4px; left: 73px;
        background-color: white;
        border-radius: 50%;
        content: '';
        width: 69px;
        height: 70px;
        z-index: 2;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
