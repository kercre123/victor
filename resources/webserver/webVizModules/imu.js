/*  
 * imu information tracking tab
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="imu-title">IMU</div>');
    
    touchInfoDiv = $('<h3 id="imuInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    touchInfoDiv.empty();
    touchInfoDiv.append('<div class="countTitle">' + "Fall Impact Count: " + data["fall_impact_count"] + '</div>');
  };

  myMethods.update = function(dt, elem) { 

  };

  myMethods.getStyles = function() {
    return `
      #imu-title {
        font-size:16px;
        margin-bottom:20px;
      }
      
      .fallImpactCountTitle {
        margin-bottom:10px;
      }
      
    `;
  };

})(moduleMethods, moduleSendDataFunc);
