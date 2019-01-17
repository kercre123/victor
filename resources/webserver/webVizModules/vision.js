/**
 * File: vision.js
 *
 * Author: Brad Neuman
 * Created: 2019-01-16
 *
 * Description: basic webviz display for vision stats from vision component
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

(function(myMethods, sendData) {

  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="vision-title">Vision Statistics</div>');

    $('<h3 class="vision-stat" id="locallyProcessed">0</h3>').appendTo( elem );
    $('<label for="locallyProcessed" class="vision-stat-label">Local Images</label>').appendTo( elem );

    $('<h3 class="vision-stat" id="cloudProcessed">0</h3>').appendTo( elem );
    $('<label for="cloudProcessed" class="vision-stat-label">Cloud Images</label>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.

    if(data == null) {
      // TODO:(bn) figure out why this ever happens....
      return;
    }

    // console.log(data)

    if(data.hasOwnProperty('local_images')) {
      $('#locallyProcessed').text( data['local_images'] );
    }

    if(data.hasOwnProperty('cloud_images')) {
      $('#cloudProcessed').text( data['cloud_images'] );
    }
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
  };

})(moduleMethods, moduleSendDataFunc);

  
