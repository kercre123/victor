/*  
 * Speech Recognition System
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="srs-title">Speech Recognizer Trigger Detections</div>');
    
    InfoDiv = $('<h3 id="recognitionInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    InfoDiv.empty();
    InfoDiv.append('<div class="dataTitle">' + "Result: \"" + data["result"] + "\"" + '</div>');
    InfoDiv.append('<div class="dataTitle">' + "Score: " + data["score"] + '</div>');
    InfoDiv.append('<div class="dataTitle">' + "Start Time: " + data["startTime_ms"] + " ms" + '</div>');
    InfoDiv.append('<div class="dataTitle">' + "End Time: " + data["endTime_ms"] + " ms" + '</div>');
    InfoDiv.append('<div class="dataTitle">' + "Start Sample Index: " + data["startSampleIndex"] + '</div>');
    InfoDiv.append('<div class="dataTitle">' + "End Sample Index: " + data["endSampleIndex"] + '</div>');
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
    return `
      #srs-title {
        font-size:16px;
        margin-bottom:20px;
      }
      
      .dataTitle {
        margin-bottom:10px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
