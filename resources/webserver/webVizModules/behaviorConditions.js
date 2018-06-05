/*  
 * Displays IBEIConditon info that prevents behavior transitions
 */

(function(myMethods, sendData) {

  // myMethods.devShouldDumpData = true;

  var instructions = 'Arrows &#8678 and &#8680 move to the next time the stack changed.<br/>'
    + 'Arrows &#8676; and &#8677; move backwards/forwards any change in factors, condition activation, or the stack.<br/>'
    + 'If nothing changes with &#8676; and &#8677, it probably means a condition was activated for a behavior that is in the '
    + 'process of activating, so continue clicking.<br/>'
    + 'Note that conditions are only updated if they are evaluated.'

  var history = [];
  var stacks = [];
  var inScopeBehaviors = []; // shares same index with stacks
  var condChanges = [];

  var condsByBehavior = {};
  var changesByCond = {};

  var autoScroll = true;
  var scrollToChange = true;
  var firstDraw = true;
  var drawIndex = 0;

  var parentsByBehavior = {};

  function GetMaxEntryIndex() {
    return history.length-1;
  }
  function GetEntry(index) {
    return history[index];
  }
  function GetLatestEntry() {
    return GetEntry( GetMaxEntryIndex() );
  }
  function GetCurrentBehavior(index) {
    var stack = stacks[GetEntry(index).stackIndex];
    return stack[stack.length-1];
  }

  function MergeTree(newData) {
    newData.forEach(function(elem){
      var behaviorID = elem['behaviorID'];
      var parent = elem['parent'];
      if( !(behaviorID in parentsByBehavior) ) {
        parentsByBehavior[behaviorID] = [];
      }
      parentsByBehavior[behaviorID].push(parent);
    });
  }

  function AddActiveCondition(parent, condition, newChange) {
    var name = condition['conditionLabel'];
    var areConditionsMet = condition['areConditionsMet'];
    var metStr = (areConditionsMet ? '[TRUE]' : '[FALSE]') + ' ' + name;
    var newDiv = $('<div id="cond">' + metStr + '</div>').appendTo(parent);
    if( newChange ) {
      newDiv.addClass('newChange');
    }
  
    var children = []; // cache children theyre added after factors
    Object.keys(condition).forEach( function(key){
      if( key != 'areConditionsMet' && key != 'conditionLabel' && key != 'ownerDebugLabel' ) {
        if( typeof condition[key] === 'object' ) {
          children.push( condition[key] );
        } else {
          var factorStr = key + ': ' + condition[key];
          $('<div class="factor">' + factorStr + '</div>').appendTo( newDiv );
        }
      }
    });
    // now add children
    children.forEach(function(child){
      // assume active
      AddActiveCondition( newDiv, child );
    });
  }

  function AddInactiveCondition(parent, name, newChange) {
    var inactiveStr = 'inactive ' + name;
    var newDiv = $('<div id="cond">' + inactiveStr + '</div>').appendTo(parent);
    if( newChange ) {
      newDiv.addClass('newChange');
    }
  }

  function Redraw() {
    $('#conditionsContainer').empty();
    var entry = history[drawIndex];
    var condChangedThisTick = entry.whatChanged == 'c' || entry.whatChanged == 'i';
    var condChangeIndex = entry.condChangeIndex;
    var stack = stacks[entry.stackIndex];
    var inScope = inScopeBehaviors[entry.stackIndex];
    if( typeof inScope === 'undefined' ) { // no behaviors yet
      return;
    }
    var behaviorDivs = {}
    inScope.forEach(function(behaviorID){
      var behaviorDiv = $('<div class="behavior">' + behaviorID + '</div>');
      if( (!condChangedThisTick) && (GetCurrentBehavior(drawIndex) == behaviorID) ) {
        behaviorDiv.addClass('currentBehavior')
      }
      var conds = condsByBehavior[behaviorID];
      var setInactiveForThisBehavior = new Set();
      if( typeof conds !== 'undefined' ) {
        conds.forEach(function(cond) {
          var areConditionsMet = false;
          var changes = changesByCond[cond];
          // find the last change with index <= condChangeIndex
          var lastMatch;
          var newChange = false;
          changes.forEach(function(change){
            if( change <= condChangeIndex ) {
              newChange = condChangedThisTick && (change == condChangeIndex);
              lastMatch = change;
            }
          });
          if( typeof lastMatch !== 'undefined' ) {
            var matchingChange = condChanges[lastMatch];
            // behaviorID owns cond, which has matchingChange as the last entry
            if( typeof matchingChange === 'object' && !Array.isArray(matchingChange) && 'areConditionsMet' in matchingChange ) {
              // it's a factor change
              AddActiveCondition(behaviorDiv, matchingChange, newChange);
            } else if( typeof matchingChange === 'object' && Array.isArray(matchingChange) ) {
              for( var i=0; i<matchingChange.length; ++i ) {
                if( !setInactiveForThisBehavior.has( matchingChange[i] ) ) {
                  AddInactiveCondition(behaviorDiv, matchingChange[i], newChange)
                  setInactiveForThisBehavior.add( matchingChange[i] );
                }
              }
              
            }
          }
        });
      }
      behaviorDivs[behaviorID] = behaviorDiv;
    });
    // now use parentsByBehavior to determine the hierarchy of behaviorDivs
    Object.keys(behaviorDivs).forEach(function(behaviorID){
      var parents = parentsByBehavior[behaviorID];
      var div = behaviorDivs[behaviorID];
      parents.forEach(function(parent){
        if( typeof parent !== 'undefined' && parent == null ) {
          $('#conditionsContainer').append( div );
        } else {
          behaviorDivs[parent].append( div );
        }
      });
    });
    // and finally toggle white/grey classes
    $('#conditionsContainer').find('*').each(function(){
      var parentWhite = $(this).parent().hasClass('whiteBox');
      $(this).toggleClass('whiteBox', !parentWhite);
    });

    $('input[type=range]').val(drawIndex);
    UpdateScrollPos();
    UpdateDisabledState();

  }
  function RedrawIfNeeded() {
    if( autoScroll || firstDraw ) {
      firstDraw = false;
      drawIndex = history.length-1;
      Redraw();
    }
  }

  function UpdateSlider() {
    $('#timeSlider').attr('max', history.length-1);
    var time = GetEntry( $('#timeSlider').val() ).time;
    var time = Math.round( time*1000 )/1000;
    $('#timeSliderLabel').text('time = ' + time);
  }
  function UpdateScrollPos() {
    if( !scrollToChange ) {
      return;
    }
    var changed = GetEntry(drawIndex).whatChanged;
    var highlightCond = (changed == 'c' || changed == 'i'); // else highlight behavior
    var scrollTo = highlightCond ? $('div.newChange') : $('div.currentBehavior');
    if( scrollTo.length ) {
      var scrollTo = scrollTo[0].offsetTop - 150;
      $('#conditionsContainer').stop().animate({
        scrollTop: scrollTo
      }, 100);
    }
  }
  function AddEntry(time, change, stackIndex, condChangeIndex) {
    history.push({
      time: time,
      whatChanged: change,
      stackIndex: stackIndex,
      condChangeIndex: condChangeIndex
    });
    UpdateSlider();
  }

  function AddToCondsByBehavior(name, owner) {
    if( !(name in changesByCond) ) {
      changesByCond[name] = new Set();
    }
    changesByCond[ name ].add( condChanges.length-1 );
    if( !(owner in condsByBehavior) ) {
      condsByBehavior[owner] = new Set();
    }
    condsByBehavior[owner].add( name );
  }

  function OnFactorsChanged(data) {
    // todo: when the stack changes, we'll receive new conditions before
    // receiving the stack. So when adding the stack, it should be inserted
    // before the first 'c' entry of the same time. 
    var time = data['time'];
    var factors = data['factors'];
    var name = factors['conditionLabel'];
    var owner = factors['ownerDebugLabel'];
    condChanges.push( factors );
    AddEntry( time, 'c', stacks.length-1, condChanges.length-1 );
    AddToCondsByBehavior(name, owner);

    RedrawIfNeeded();
  }

  function OnInactive( data ) {
    var time = data['time'];
    var name = data['inactive'];
    var owner = data['owner'];
    if( (history.length > 0)
        && (history[history.length-1].whatChanged == 'i') 
        && (history[history.length-1].time == time) ) 
    {
      // the current tick deactivated another condition(s) so add it to that list
      condChanges[condChanges.length-1].push( name );
    } else {
      condChanges.push( [name] );
      AddEntry( time, 'i', stacks.length-1, condChanges.length-1 );
    }
    
    AddToCondsByBehavior(name, owner);

    RedrawIfNeeded();
  }

  function OnStackChanged( data ) {
    var stack = data['stack'];
    var tree = data['tree'];
    var time = data['time'];
    MergeTree(tree);
    var inScope = [];
    tree.forEach(function(e) {
      inScope.push( e['behaviorID'] );
    });
    if( history.length && GetLatestEntry().whatChanged == 's' && GetLatestEntry().time == time ) {
      stacks[ GetLatestEntry().stackIndex ] = stack;
      inScopeBehaviors[ GetLatestEntry().stackIndex ] = inScope;
    } else {
      stacks.push( stack );
      inScopeBehaviors.push( inScope );
      AddEntry( time, 's', stacks.length-1, condChanges.length-1 );
    }
    RedrawIfNeeded();
  }

  function UpdateDisabledState() {
    var stackIndex = GetEntry(drawIndex).stackIndex;
    var condIndex = GetEntry(drawIndex).condChangeIndex;

    var noMoreStacks = (stackIndex >= stacks.length - 1 );
    var noMoreFactors = (condIndex >= condChanges.length - 1 );
    $('#nextButton').prop('disabled', noMoreFactors );
    $('#nextStack').prop('disabled', noMoreStacks );
    var noPrevStacks = (stackIndex <= 0);
    var noPrevFactors = (condIndex <= 0);
    $('#backButton').prop('disabled', noPrevFactors );
    $('#backStack').prop('disabled', noPrevStacks );
  }

  myMethods.init = function(elem) {
    $('<div>' + instructions + '</div>').appendTo(elem);
    var controls = $('<div id="controls"></div>').appendTo(elem);
    $('<div id="conditionsContainer"></div>').appendTo(elem);

    controls.append('<button class="button" id="backStack">&#8678;</button>');
    controls.append('<button class="button" id="backButton">&#8676;</button>');
    controls.append('<input id="timeSlider" type="range" min="0" max="1" value="1" step="1" />');
    controls.append('<button class="button" id="nextButton">&#8677;</button>');
    controls.append('<button class="button" id="nextStack">&#8680;</button>');
    controls.append('<label id="timeSliderLabel">0</label>');
    controls.append('<div id="timeSliderTooltip"></div>');
    controls.append('<label for="chkScroll">Scroll to modified</label>');
    controls.append('<input id="chkScroll" type="checkbox" checked/>');
    

    $('#timeSlider').on('input', function() { // sliding, but not finished sliding
      var slider = $(this);
      var val = slider.val();
      var width = slider.width();
      var ballPct = (val - slider.attr("min")) / (slider.attr("max") - slider.attr("min"));
      var left = width * ballPct + slider.offset().left
      var top = slider.offset().top;
      var currBehavior = GetCurrentBehavior(val);
      $('#timeSliderTooltip')
        .text(currBehavior)
        .css({
          left: left,
          top:top
        })
        .show();
    });
    $('#timeSlider').change(function() { // finished sliding
      $('#timeSliderTooltip').hide();
      var val = $(this).val();
      var maxIdx = GetMaxEntryIndex();
      if( val <= maxIdx ) {
        var entry = GetEntry(val);
        var time = Math.round( entry.time*1000 )/1000;
        $('#timeSliderLabel').text( 'time = ' + time );
        if( val != drawIndex ) {
          drawIndex = val;
          Redraw();
        }
      }
      if( val == maxIdx ) {
        autoScroll = true;
      } else {
        autoScroll = false;
      }
    });

    $('#chkScroll').change(function() {
      scrollToChange = $(this).is(':checked');
    })

    // todo: buttons should really just apply the change instead of redraw
    // if the current sketch is the previous tick
    $('#nextButton').click(function() {
      if( drawIndex == GetMaxEntryIndex() ) {
        return;
      }
      ++drawIndex;
      if( drawIndex == GetMaxEntryIndex() ) {
        autoScroll = true;
      }
      Redraw();
    });
    $('#nextStack').click(function() {
      var nextStack = 1 + GetEntry(drawIndex).stackIndex;
      if( nextStack <= stacks.length - 1 ) {
        for( var idx=drawIndex; idx<history.length; ++idx ) {
          if( GetEntry(idx).stackIndex == nextStack ) {
            drawIndex = idx;
            if( drawIndex == GetMaxEntryIndex() ) {
              autoScroll = true;
            }
            Redraw();
            break;
          }
        }
      }
    });
    $('#backButton').click(function() {
      if( drawIndex == 0 ) {
        return;
      }
      autoScroll = false;
      --drawIndex;
      Redraw();
    });
    $('#backStack').click(function() {
      var currStack = GetEntry(drawIndex).stackIndex;
      if( currStack > 0 ) {
        var prevStack = currStack - 1;
        for( var idx=drawIndex; idx>=0; --idx ) {
          if( GetEntry(idx).stackIndex == prevStack ) {
            drawIndex = idx;
          }
          if( idx == 0 || GetEntry(idx).stackIndex < prevStack ) {
            autoScroll = false;
            Redraw();
            break;
          }
        }
      }
    });
  };

  myMethods.onData = function(data, elem) {
    if( typeof data['factors'] !== 'undefined' ) {
      OnFactorsChanged( data );
    } else if( typeof data['inactive'] !== 'undefined' ) {
      OnInactive( data );
    } else if( typeof data['stack'] !== 'undefined' ) {
      OnStackChanged(data);
    }
  };

  myMethods.update = function(dt, elem) {};

  myMethods.getStyles = function() {
    return `
      #conditionsContainer {
        overflow-y: scroll; 
        min-height:300px;
        max-height:600px;
        width:100%;
        padding:10px;
        margin-top:10px;
        border:1px solid #aaa;
      }
      #conditionsContainer div {
        margin-top:5px;
        padding: 5px 10px;
        font-family: monospace;
      }
      div {
        background-color:#ededed;
      }
      div.whiteBox {
        background-color:white;
      }
      div {
        font-weight:normal;
        border:1px solid transparent;
      }
      div.currentBehavior {
        font-weight:bold;
        border: 1px solid red;
      }
      #controls {
        padding-left:10px;
        padding-top:5px;
      }
      #controls > * {
        margin-left:5px;
      }
      #timeSliderLabel {
        margin-left:10px;
      }
      input[type=range],
      button {
        vertical-align:middle;
      }
      .newChange {
        border: 1px solid red;
      }
      #timeSliderTooltip {
        display:none;
        position:fixed;
        top:0;
        left:0;
        width:auto;
        height:20px;
        border:1px solid black;
        padding:5px 10px 10px; 5px;
        vertical-align:middle;
        background-color:white;
      }
      #chkScroll, label[for=chkScroll] {
        float:right;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
