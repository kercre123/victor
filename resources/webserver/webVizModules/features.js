/*  
 * Shows modifiable feature flag status
 */

(function(myMethods, sendData) {

  var tableBody;

  var kNoneString  = 'none';
  var kEnabledString  = 'enabled';
  var kDisabledString = 'disabled';

  var updateEngineOnDropdownChange = true;
  
  function MakeOpt(currValue, optStr) {
    return '<option class="' + optStr +  '"' 
      + (currValue == optStr  ? ' selected' : '') + '>' 
      + optStr + '</option>';
  }

  function MakeOverrideDropdown(currValue, featureName, defaultValue) {
    var str = '<select class="override" name="' + featureName + '">';
    str += MakeOpt(currValue, kNoneString);
    // remove enabled/disabled from the list if the default is the same, _except_ if 
    // the console var overrides are set otherwise
    if( (defaultValue != kEnabledString) || (currValue == kEnabledString) ) {
      str += MakeOpt(currValue, kEnabledString);
    }
    if( (defaultValue != kDisabledString) || (currValue == kDisabledString) ) {
      str += MakeOpt(currValue, kDisabledString);
    }
    str += '</select>';
    return str;
  }

  function ChangeDropdownFonts() {
    $('select.override').each(function(idx, elem){
      var selectedOpt = $(elem).find('option:selected').text();
      if( selectedOpt == kEnabledString ) {
        $(elem).removeClass('disabled');
        $(elem).addClass('enabled');
      } else if( selectedOpt == kDisabledString ) {
        $(elem).addClass('disabled')
        $(elem).removeClass('enabled')
      } else {
        $(elem).removeClass('disabled')
        $(elem).removeClass('enabled')
      }
    });
  }

  myMethods.init = function(elem) {
    
    $(elem).append($('<div>Note that feature overrides will persist across robot reboots! Some features may not take effect until the next reboot.</div>'));
    $(elem).append($('<div>Also, if you modify feature flags using console vars, this tab may not be updated until you refresh this page.</div>'));
    

    $(`<div class="table">
        <div class="thead">
          <div class="table-row">
            <div class="table-heading">Feature name</div>
            <div class="table-heading">Build default</div>
            <div class="table-heading">Override</div>
          </div>
        </div>
        <div class="tbody"></div>
      </div>
    `).appendTo(elem);
    tableBody = $('div.tbody');

    // not created yet
    $(elem).bind('change', 'select', function(evt) {
      if( !updateEngineOnDropdownChange ) {
        return;
      }
      var toSend = {"type": "override"};
      toSend["name"] = $(evt.target).attr("name");
      toSend["override"] = $(evt.target).find('option:selected').text();
      sendData(toSend);
      ChangeDropdownFonts();
    });
    $('<input type="button" value="Reset feature overrides" />').click(function(){
      // don't alert the engine until all features have changed
      updateEngineOnDropdownChange = false;
      // select the first option (default) for every dropdown
      $("select.override").each(function(idx, dropdown) { 
        dropdown.selectedIndex = 0; 
      });
      updateEngineOnDropdownChange = true;
      var toSend = {"type": "reset"};
      sendData(toSend);
      ChangeDropdownFonts();
    }).appendTo(elem);
  };

  myMethods.onData = function(data, elem) {
    tableBody.empty();
    if( !Array.isArray(data) ) {
      alert('There was a problem understanding your request. Try reloading');
      return;
    }

    // sort data by feature name
    data.sort(function(a, b){
      var nameA=a.name.toLowerCase(), nameB=b.name.toLowerCase()
      if( nameA < nameB ) {
        return -1;
      } else if( nameA > nameB ) { 
        return 1; 
      }
      return 0;
    });
    $.each( data, function( idx, entry ) {
      var newRow = $('<div class="table-row"></div>').appendTo(tableBody);
      newRow.append($('<div class="table-cell">' + entry.name + '</div>'));
      newRow.append($('<div class="table-cell ' + entry.default + '">' + entry.default + '</div>'));
      newRow.append($('<div class="table-cell">' + MakeOverrideDropdown(entry.override, entry.name, entry.default) + '</div>'));
    });
    ChangeDropdownFonts();
  };

  myMethods.update = function(dt, elem) {};

  myMethods.getStyles = function() {
    return `
      div.table {
        display: table;
        width:100%;
        margin-top:20px;
        margin-bottom:10px;
      }
       
      div.table > div.thead {
        display: table-header-group;
      }
       
      div.table > div.tbody {
        display: table-row-group;
      }
       
      div.table > div.thead > div.table-row,
      div.table > div.tbody > div.table-row {
        display: table-row;
      }
       
      div.table > div.thead > div.table-row > div.table-heading {
        display: table-cell;
        font-weight: bold;
        padding-bottom: 5px;
      }
       
      div.table > div.tbody > div.table-row > div.table-cell {
        display: table-cell;
        padding-bottom: 5px;
      }

      select {
        width: 90px;
      }

      input {
        height:20px;
        width:190px;
      }

      /* 
      colorblind friendly "enabled/disabled" colors below  :D 
      these class names should match the constant strings at the top of this file
      */

      div.enabled {
        color: #2C7BB6;
      }
      div.disabled {
        color: #D7191C;
      }
      select.enabled {
        color: white;
        background-color: #2C7BB6;
      }
      select.disabled {
        color: white;
        background-color: #D7191C; 
      }
      select.none {
        color: #C8C8C8;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
