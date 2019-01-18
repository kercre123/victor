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

    $('<div class="image-stat-container">' +
      '<div class="vision-stat-label">Local Images</div>' +
      '<span class="vision-stat" id="locallyProcessed">0</span>' +
      '</div>').appendTo( elem );

    $('<div class="image-stat-container">' +
      '<div for="cloudProcessed" class="vision-stat-label">Cloud Images</div>' +
      '<span class="vision-stat" id="cloudProcessed">0</span>' +
      '</div>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.

    if(data == null) {
      // TODO:(bn) figure out why this ever happens....
      return;
    }

    // console.log(data)

    function numberWithCommas(x) {
      return x.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",");
    }

    if(data.hasOwnProperty('local_images')) {
      $('#locallyProcessed').text( numberWithCommas(data['local_images']) );
    }

    if(data.hasOwnProperty('cloud_images')) {
      $('#cloudProcessed').text( numberWithCommas(data['cloud_images']) );
    }
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
  };

})(moduleMethods, moduleSendDataFunc);

  
