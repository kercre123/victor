/*  
 * Lists observed objects
 */

(function(myMethods, sendData) {

  var kFace = '&#x1F468'
  var kCube = '&#x25A6;';

  var faceList;
  var cubeList;

  function sortListById(list) {
    var elems = list.children('div');

    elems.sort( function(a,b){
      var aId = a.getAttribute('data-id');
      var bId = b.getAttribute('data-id');

      if(aId > bId) {
        return 1;
      }
      else if(aId < bId) {
        return -1;
      } else {
        return 0;
      }
    });
    elems.detach().appendTo(list);
  }

  myMethods.init = function(elem) {
    var bigStyle = {class: 'bigUnicode'};
    // needs containers for margin style, and needs different containers so that
    // cell widths can differ
    var faceContainer = $('<div class="tableContainer"></div>').appendTo(elem);
    var cubeContainer = $('<div class="tableContainer"></div>').appendTo(elem)

    $('<div></div>', bigStyle).appendTo(faceContainer).html(kFace + "'s");
    faceList = $('<div id="faceList"></div>').appendTo(faceContainer);

    $('<div></div>', bigStyle).appendTo(cubeContainer).html(kCube + "'s");
    cubeList = $('<div id="cubeList"></div>').appendTo(cubeContainer);
  };

  myMethods.onData = function(data, elem) {
    if( data["type"] == "RobotObservedFace" ) {
      var shouldSort = false;
      var faceElem = faceList.find('div[data-id="' + data["faceID"] + '"]');
      if( faceElem.length == 0 ) {
        faceElem = $('<div></div>', {class:'faceBlock'}).appendTo(faceList);
        faceElem.attr('data-id', data["faceID"]);
        shouldSort = true;
      } else {
        faceElem.empty();
      }
      faceElem.append('<p>id: ' + data["faceID"] + '</p>')
              .append('<p>t: ' + data["timestamp"] + '</p>');
      if( typeof data.name !== 'undefined' ) {
        faceElem.append('<p>t: ' + data["name"] + '</p>');
      }
      if( shouldSort ) {
        sortListById( faceList );
      }
    } 
    else if( data["type"] == "RobotDeletedFace" ) {
      var faceElem = faceList.find('div[data-id="' + data["faceID"] + '"]');
      if( faceElem.length != 0 ) {
        faceElem.remove();
      }
    }
    else if( data["type"] == "RobotObservedObject" ) {
      var shouldSort = false;
      var cubeElem = cubeList.find('div[data-id="' + data["objectID"] + '"]');
      if( cubeElem.length == 0 ) {
        cubeElem = $('<div></div>', {class:'objectBlock'}).appendTo(cubeList);
        cubeElem.attr('data-id', data["objectID"]);
        shouldSort = true;
      } else {
        cubeElem.empty();
      }
      cubeElem.append('<p>id: ' + data["objectID"] + '</p>')
              .append('<p>t: ' + data["timestamp"] + '</p>')
              .append('<p>type: ' + data["objectFamily"] + ' ' + data["objectType"] + '</p>')
              .append('<p>active: ' + data["isActive"] + '</p>');
      if( shouldSort ) {
        sortListById( cubeList );
      }
    }
    else if( data["type"] == "RobotDeletedLocatedObject" ) {
      var cubeElem = cubeList.find('div[data-id="' + data["objectID"] + '"]');
      if( cubeElem.length != 0 ) {
        cubeElem.remove();
      }
    }
  };

  myMethods.update = function(dt, elem) {};

  myMethods.getStyles = function() {
    return `
      .bigUnicode {
        font-size:20px;
      }
      .bigUnicode:not(:first-child) {
        margin-top:20px;
      }

      .tableContainer {
        display: table;
        border-collapse: separate;
        border-spacing: 10px;
      }

      #cubeList,
      #faceList {
        display: table-row;
        margin-bottom:20px;
      }

      #cubeList div,
      #faceList div {
        border: 1px solid black;
        height:30px;
        display: table-cell;
        padding:5px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
