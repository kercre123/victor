(function(myMethods, sendData) {

  //myMethods.devShouldDumpData = true;

  // helper methods

  var downloadAsCsv = function(){
    var data = cachedTreeData; // use latest, even if frozen
    
    var doToSelfAndChildren = function(elem, func) {
      func(elem);
      var children = elem['children'];
      if( typeof children !== 'undefined' && Array.isArray(children) ) {
        children.forEach(function(child) {
          doToSelfAndChildren(child, func);
        });
      }
    }

    // find all times where behaviors were activated or deactivated
    var changedTimes = new Set();
    var findChanged = function(elem) {
      var activeTimes = elem.data.activeTimes;
      if( typeof elem.data !== 'undefined' 
          && typeof elem.data.activeTimes !== 'undefined' 
          && Array.isArray(elem.data.activeTimes) ) 
      {
        elem.data.activeTimes.forEach(function(startStop) {
          if( typeof startStop.start !== 'undefined' ) {
            changedTimes.add(startStop.start)
          }
          if( typeof startStop.end !== 'undefined' ) {
            changedTimes.add(startStop.end)
          }
        });
      }
    };
    doToSelfAndChildren( data, findChanged );
    // transform set to sorted array
    changedTimes = Array.from(changedTimes).sort(function(a,b){return a - b});

    var outputInfo = {};
    var maxStackLength = 0;
    // for any event at that time, gather info on the stack at that time
    changedTimes.forEach(function(time){
      var behaviorsActiveAtTime = [];
      var findBehaviorsActiveAtTime = function(elem) {
        var activeTimes = elem.data.activeTimes;
        if( typeof elem.data !== 'undefined' 
          && typeof elem.data.activeTimes !== 'undefined' 
          && Array.isArray(elem.data.activeTimes) ) 
        {
          elem.data.activeTimes.forEach(function(startStop) {
            if( (startStop.start <= time) 
                && ((typeof startStop.end === 'undefined') || (startStop.end > time)) ) 
            {
              behaviorsActiveAtTime.push( elem.data.behaviorID );
            }
          });
        }
      };
      doToSelfAndChildren( data, findBehaviorsActiveAtTime );

      outputInfo[time] = behaviorsActiveAtTime;
      maxStackLength = Math.max(maxStackLength, behaviorsActiveAtTime.length);
    });

    // finally, write the info into csv format

    // create header
    var dateStr = (new Date()).toLocaleString('en-US').replace(',','');
    var outputCsv = 'time[' + dateStr + ']';
    for( var i=0; i<maxStackLength; ++i ) {
      outputCsv += ',behavior';
    }
    outputCsv += '\n';
    // write body
    changedTimes.forEach(function(time){
      var info = outputInfo[time];
      outputCsv += time;
      for( var i=0; i<maxStackLength; ++i ) {
        if( i < info.length ) {
          outputCsv += ',' + info[i];  
        } else {
          outputCsv += ',';
        }
      }
      outputCsv += '\n';
    });
    // make QA agree that this doesn't suffice for full logs
    alert('Downloading behavior stacks. To help the engineers, it\'s useful to send them this file, a screenshot of this tab, _AND_ a full log (victor_log)')
    // force download
    var elem = document.createElement('a');
    elem.setAttribute('href', 'data:text/csv;charset=utf-8,' + encodeURIComponent(outputCsv));
    elem.setAttribute('download', 'behaviorStackLog.csv');
    elem.style.display = 'none';
    document.body.appendChild(elem);
    elem.click();
    document.body.removeChild(elem);
  };

  function addControls(data, container) {
    data.sort();
    // create if needed, then populate a dropdown with behaviorIDs,
    // and when the user selects one, tell the engine to start that behavior 
    var dropDown = $('#behaviorDropdown');
    if( dropDown.length == 0 ) {
      dropDown = $('<select></select>', {id: 'behaviorDropdown'}).appendTo(container);
      dropDown.change(function() {
        if( this.value ) {
          var payload = {'behaviorName': this.value};
          sendData( payload );

          flatData = undefined;
          treeData = undefined;
          svgGroups.rowGroup.selectAll('*').remove();
          svgGroups.labelGroup.selectAll('*').remove();
          svgGroups.timeBarsGroup.selectAll('.timeBar').remove();
          svgGroups.zoomGroup.selectAll('.miniTimeBar').remove();
        }
      });
      $('<input type="checkbox" id="showActivatable" checked />').appendTo(container)
        .change( function() {
          showInactive = $(this).is(':checked');
        })
      $('<label for="showActivatable">Show activatable</label>').appendTo(container);

      $('<button type="button" id="downloadTimeBars">Download as csv</button>').click(downloadAsCsv).appendTo(container);
    }
    dropDown.empty();
    $("<option/>", {val: '', text:'Select a behaviorID to switch to immediately'}).appendTo(dropDown);
    $(data).each(function() {
      $("<option/>", {val: this, text: this}).appendTo(dropDown);
    });
  }

  var liveUpdating = true;
  function setLiveUpdates(v) {
    if( v && !liveUpdating ) {
      liveUpdating = v;

      if( hasPendingUpdates ) {
        treeData = cachedTreeData;
      }
      
      clearZoomSelection();
    } else if( liveUpdating && !v ) {
      liveUpdating = v;
      update(treeData);
    }
  }
  function hasLiveUpdates() {
    return liveUpdating;
  }
  var rightAngleBend = function(fromTo) {
    var x0 = fromTo[0].x + params.pathOffset;
    var y0 = fromTo[0].y + params.barHeight;
    var x1 = fromTo[1].x;
    var y1 = fromTo[1].y + params.barHeight/2;
    var str = 'M' + x0 + ' ' + y0 + ' v ' + (y1-y0) + ' h ' + (x1-x0);
    return str;
  };
  var zoomOrigin = {x:-1, y:-1};   
  var clearZoomSelection = function() {
    zoomSelection.attr('width', 0);
    onZoomSelectionChanged();
  };
  var miniTimeBarMouseDown = function() {
    clearZoomSelection();
    var div = d3.select(this)
        .classed('active', true);
    var mousePos = d3.mouse(div.node());
    zoomOrigin = {x: mousePos[0], y: mousePos[1]};

    var w = d3.select(window)
        .on('mousemove', mousemove)
        .on('mouseup', mouseup);

    d3.event.preventDefault(); // disable text dragging

    function mousemove() {
      var mousePos = d3.mouse(div.node());
      var zoomPos = {x: Math.min(params.frameWidth, Math.max(0,mousePos[0])), y: mousePos[1]};
      var extents = {};
      if(zoomPos.x < zoomOrigin.x) {
        extents = {min:zoomPos.x, max:zoomOrigin.x};
      } else {
        extents = {min:zoomOrigin.x, max:zoomPos.x};
      }
      zoomSelection.attr('x', extents.min);
      zoomSelection.attr('width', extents.max - extents.min);
      onZoomSelectionChanged();
      setLiveUpdates(false);
    
    }

    function mouseup() {
      div.classed('active', false);
      w.on('mousemove', null).on('mouseup', null);
    }
  };
  var node2Id = function(d) {
    return d.id + '>' + d.data.parent;
  }
  var onZoomSelectionChanged = function() {
    var x = Math.max(0, parseFloat(zoomSelection.attr('x')));
    var width = Math.min(params.frameWidth, parseFloat(zoomSelection.attr('width')));
    var startPerc = x / params.frameWidth;
    var endPerc = (x + width)/params.frameWidth;
    if( width > 0 ) {
      hasZoomWindow = true;
      minZoomTime = minTime + startPerc*(globalTime - minTime);
      maxZoomTime = minTime +   endPerc*(globalTime - minTime);
    } else {
      hasZoomWindow = false;
    }

    update(treeData);
  };
  var updateTimeCursor = function() {
    if( typeof svgGroups.timeBarsGroup === 'undefined' ) {
      return; // can happen if using this with another port
    }
    var timeBar = svgGroups.timeBarsGroup.selectAll('line.currentTimeLine');
    if( timeCursorPosition >= 0 ) {
      timeBar.attr('x1', timeCursorPosition)
             .attr('x2', timeCursorPosition);
    } else {
      timeBar.attr('x1', params.frameWidth)
             .attr('x2', params.frameWidth);
      if( hasZoomWindow ) {
        timeCursorTime = maxZoomTime;
      } else {
        timeCursorTime = globalTime;
      }
    }
    var activeIds = []; // todo: speed this up with an array of bools
    if( typeof treeData === 'undefined' ) {
      return;
    }
    var activeS = '';
    treeData.each(function(d) {
      if( typeof d.data.activeTimes !== 'undefined' ) {
        for( var i=0; i<d.data.activeTimes.length; ++i ) {
          if( d.data.activeTimes[i].start <= timeCursorTime ) {
            if( (typeof d.data.activeTimes[i].end === 'undefined') ||
                (d.data.activeTimes[i].end > timeCursorTime) )
            {
              activeS += node2Id(d) + ' ';
              activeIds.push( node2Id(d) );
            }
          }
        }
      }
    });
    svgGroups.labelGroup.selectAll('.node text').classed('activeLabel', function(d) { return (activeIds.indexOf(node2Id(d))>=0); } );
    
    // decide which time labels are displayed
    var selectedRow = svgGroups.rowGroup.selectAll('rect.row:hover');
    var selectedRowId = '';
    if( selectedRow.nodes().length ) {
      selectedRowId = node2Id(selectedRow.nodes()[0].__data__);
    }
    svgGroups.timeBarsGroup
                      .selectAll('g.timeBar')
                      .data(nodesSort, idFunc)
                      .each(function(d) {
                        var id = node2Id(d);
                        var groupText = d3.select(this).selectAll('text').data(d.data.activeTimes);
                        if( groupText.length == 0 ) {
                          return;
                        }
                        groupText
                          .attr('opacity', function(d) { 
                            var cursorIsHovering = (activeIds.indexOf(id)>=0) || (selectedRowId==id)
                            return shouldShowText(d, this, cursorIsHovering) ? 1.0 : 0.0; 
                          });
                      });   
  };
  var getTimeText = function(t) {
    var ret;
    if( t > 5.0 ) {
      ret = (Math.round(t*10)/10).toFixed(1) + ' s';
    } else {
      ret = Math.round(1000*t) + ' ms';
    }
    return ret;
  };
  var calcTimeCursorTime = function() {
    var t;
    var frac = (timeCursorPosition - maxLabelWidth)/(params.frameWidth-maxLabelWidth);
    if( hasZoomWindow ) {
      t = minZoomTime + frac * (maxZoomTime - minZoomTime);
    } else {
      t = minTime + frac * (globalTime - minTime);
    }
    return t;
  };
  var idFunc = function(d) { 
    return node2Id(d) || (d.id = ++i);   //assigning id for each node
  };
  var shouldShowText = function(d, textElem, cursorIsHovering) {
    // don't show if the text is outside of the zoom window.
    // if it's inside the zoom window, don't show if it is larger
    // than its rect, unless the cursor is over that row or time slice
    var width = textElem.getComputedTextLength();
    var rectWidth = time2Width(d, maxLabelWidth);
    var textSmallEnough = width <= rectWidth - 3; // 3==buffer
    if( hasZoomWindow ) {
      var outsideWindow = (d.start < minZoomTime && getEndTime(d) <= minZoomTime) ||
                          (d.start >= maxZoomTime && getEndTime(d) > maxZoomTime);
      return !outsideWindow && (textSmallEnough || cursorIsHovering);
    } else {

      return (textSmallEnough || cursorIsHovering);
    }
  };
  var getEndTime = function(d) {
    if( typeof d.end === 'undefined' ) {
      return globalTime;
    } else {
      return d.end;
    }
  };
  var timeBarMousemove = function(e) {
    var div = d3.select(this);
    var mousePos = d3.mouse(div.node());
    
    if( mousePos[0] >= maxLabelWidth ) {
      timeCursorPosition = mousePos[0];
      timeCursorTime = calcTimeCursorTime();
    } else {
      timeCursorPosition = -1;
      if( hasZoomWindow ) {
        timeCursorTime = maxZoomTime;
      } else {
        timeCursorTime = globalTime;
      }
    }
    updateTimeCursor();
  };
  var timeBarMouseout = function(e) {
    timeCursorPosition = -1;
    if( hasZoomWindow ) {
      timeCursorTime = maxZoomTime;
    } else {
      timeCursorTime = globalTime;
    }
    updateTimeCursor();
  };
  var time2Position = function(d, maxLabelWidth, ignoreZoom) {
    if( hasZoomWindow && !ignoreZoom ) {
      var outsideWindow = (d.start < minZoomTime && getEndTime(d) <= minZoomTime) ||
                          (d.start >= maxZoomTime && getEndTime(d) > maxZoomTime);
      if( !outsideWindow ) {
        var duration = maxZoomTime - minZoomTime;
        return maxLabelWidth + Math.max(0, d.start-minZoomTime)*(params.frameWidth - maxLabelWidth)/duration;
      } else {
        return 0;
      }
    } else {
      return maxLabelWidth + (d.start-minTime)*(params.frameWidth - maxLabelWidth)/(globalTime - minTime);
    }
  }
  var time2Width = function(d, maxLabelWidth, ignoreZoom) {
    if( hasZoomWindow && !ignoreZoom ) {
      var outsideWindow = (d.start < minZoomTime && getEndTime(d) <= minZoomTime) ||
                          (d.start >= maxZoomTime && getEndTime(d) > maxZoomTime);
      if( !outsideWindow ) {
        var duration = maxZoomTime - minZoomTime;
        var dt = (Math.min(maxZoomTime, getEndTime(d))-Math.max(minZoomTime, d.start));
        return dt*(params.frameWidth - maxLabelWidth)/duration;
      } else {
        return 0;
      }
    } else {
      return (getEndTime(d)-d.start)*(params.frameWidth - maxLabelWidth)/(globalTime - minTime);
    }
  };

  var tree = d3.tree().nodeSize([0, 30]) //Invokes tree
  var params = {};
  var svgGroups = {};
  var activeFeatureDiv;
  var currentBehaviorDiv;
  var currentBehaviorStateDiv;

  var hasZoomWindow = false;
  var minZoomTime = 0;
  var maxZoomTime = 0;
  var zoomSelection;
  var timeCursorPosition=-1;
  var timeCursorTime;
  var maxLabelWidth = 190;
  var showInactive = true;
    

  function update(source) {
    // Compute the flattened node list. TODO use d3.layout.hierarchy.
    nodes = tree(source); //returns a single node with the properties of d3.tree()
    nodesSort = [];

    d3.select('svg')
      .transition()
      .duration(params.duration); //transition to make svg looks smoother

    // returns all nodes and each descendant in pre-order traversal (sort)
    nodes.eachBefore(function (n) {
      if( showInactive || (typeof n.data.activeTimes !== 'undefined') ) {
        nodesSort.push(n); 
      }
    });

    // compute positioning
    nodesSort.forEach(function (n,i) {
      n.x = i * params.barHeight;
    });

    

    // Update rows
    var rows = svgGroups.rowGroup.selectAll('rect.row')
                       .data(nodesSort, idFunc);
    var rowsEnter = rows.enter()
                       .append('rect')
                       .attr('class', 'row')
                       .attr('height', params.barHeight)
                       .attr('width', params.frameWidth)
                       .attr('transform', function(d) {  
                         return 'translate(' + 0 + ',' + d.x + ')'; 
                       });
    // update row positionings
    rows
                       .attr('transform', function(d) {  
                         return 'translate(' + 0 + ',' + d.x + ')'; 
                       });
                       
    // Update the labels
    var i = 0;
    var node = svgGroups.labelGroup.selectAll('g.node')
                         .data(nodesSort, idFunc); 

    var nodeEnter = node.enter()
                        .append('g')
                        .attr('class', 'node')
                        .style('opacity', 1e-6);
    node
                        .attr('transform', function(d) {  
                          return 'translate(' + source.y + ',' + source.x + ')'; 
                        });

    nodeEnter.append('text')
             .attr('dy', 3.5 + params.barHeight/2)
             .attr('dx', 5.5)
             .text(function (d) {
               return (d._children || d.children) ? ('   ' + d.data.behaviorID) : d.data.behaviorID;
             });

    // Transition nodes to their new position.
    nodeEnter.attr('transform', function(d) { return 'translate(' + d.y + ',' + d.x + ')'; })
             .style('opacity', 1);

    node.attr('transform', function(d) { return 'translate(' + d.y + ',' + d.x + ')'; })
        .style('opacity', 1);

    node.exit().remove();

    // Update the links…
    var link = svgGroups.labelGroup.selectAll('.link')
                         .data(source.links(), function(d) { return d.target.id; });

    var diagonalFunc = function(d) {
      if( showInactive || typeof d.target.data.activeTimes !== 'undefined' ) {
        return rightAngleBend([{
          y: d.source.x,
          x: d.source.y
        }, {
          y: d.target.x,
          x: d.target.y
        }]);
      } else {
        return;
      }
    };
    // Enter any new links at the parent's previous position.
    link.enter().insert('path', 'g')
                .attr('class', 'link')
                .attr('d', diagonalFunc);

    // update links to their new position.
    link.attr('d', diagonalFunc);

    // remove links that no longer exist
    link.exit()
        .attr('d', diagonalFunc)
        .remove();

    // stash the old positions for transition.
    // todo: is this needed?
    nodes.eachBefore(function (d) {
      d.x0 = d.x;
      d.y0 = d.y;
    });

    maxLabelWidth = 190;
    d3.selectAll('.labelGroup text')
      .each(function(d) {
        maxLabelWidth = Math.max(maxLabelWidth, 10+ d.y + this.getComputedTextLength());
      });
    
    var timeBarHeight = params.barHeight*.5;
    var timeBarOffset = 0.5*(params.barHeight-timeBarHeight);
         
    var updateTimeBars = function(d,i){
      if( typeof d.data.activeTimes === 'undefined' ) {
        return;
      }
      var id = node2Id(d);
      var groupRect = d3.select(this)
                        .selectAll('rect')
                        .data(d.data.activeTimes);
      groupRect.enter().append('rect');
      groupRect
        .attr('x', function(d,j) { return time2Position(d, maxLabelWidth); })
        .attr('y', function() { return timeBarOffset+d.x; })
        .attr('width', function(d) { return time2Width(d, maxLabelWidth); })
        .attr('height', function() { return timeBarHeight; });
      var groupText = d3.select(this)
                        .selectAll('text')
                        .data(d.data.activeTimes);
      groupText.enter().append('text');
      groupText
        .attr('x', function(d,j) { return 2+time2Position(d, maxLabelWidth); })
        .attr('y', function() { return params.barHeight/2+d.x; })
        .attr('height', function() { return timeBarHeight; })
        .text(function(d) { return getTimeText(getEndTime(d)-d.start); });
    }

    var timeBars = svgGroups.timeBarsGroup
                            .selectAll('g.timeBar')
                            .data(nodesSort, idFunc);
    // new time bars
    timeBars 
      .enter()
      .append('g')
      .attr('class', 'timeBar')
      .each(updateTimeBars);
    // update time bars
    timeBars.each(updateTimeBars);

    // recalc positioning
    var layerOffset = nodesSort.length*params.barHeight;
    
    var timeBar = svgGroups.timeBarsGroup.selectAll('line.currentTimeLine');
    timeBar.attr('y2',layerOffset);
    
    

    var scale = d3.scaleLinear()
                  .domain(hasZoomWindow ? [minZoomTime, maxZoomTime] : [minTime, globalTime])
                  .range([maxLabelWidth, params.frameWidth]);
    var x_axis = d3.axisBottom()
                   .scale(scale);

    // set axis props
    var axisHeight = 20;
    svgGroups.axisGroup.attr('y', layerOffset)
             .attr('transform', 'translate(0,' + (layerOffset) + ')')
             .attr('height', axisHeight)
             .call(x_axis);

    // update toggle container/switch positioning (todo: put dims in css)
    var toggleWidth = 30;
    var toggleHeight = 0.75*axisHeight;
    var toggleOffset = (axisHeight - toggleHeight)/2;
    svgGroups.axisGroup.selectAll('rect.toggleContainer')
             .attr('x',0)
             .attr('y',toggleOffset)
             .attr('rx',toggleHeight/2)
             .attr('width',toggleWidth)
             .attr('height',toggleHeight)
             .attr('stroke','black')
             .attr('fill',function(){ return hasLiveUpdates() ? 'blue' : '#ccc'; });
    svgGroups.axisGroup.selectAll('rect.toggleSwitch')
             .attr('x',function(d) { return hasLiveUpdates() ? toggleWidth-(toggleHeight-2) : 1; })
             .attr('y',1+toggleOffset)
             .attr('rx',toggleHeight/2)
             .attr('width',toggleHeight-2)
             .attr('height',toggleHeight-2)
             .attr('fill','white');
  
    // move to next layer (zoom bars)
    layerOffset += axisHeight;
    var miniBarHeight = 5;
    var miniBarSpacing = miniBarHeight + 2;
    // zoom bars
    var updateZoomBar = function(d,i){
      if( typeof d.data.activeTimes === 'undefined' ) {
        return;
      }
      var rect = d3.select(this).selectAll('rect')
        .data(d.data.activeTimes);
      rect.enter().append('rect');
      rect
        .attr('x', function(d,j) { return time2Position(d, 0, true); })
        .attr('y', function() { return layerOffset+i*miniBarSpacing+1; })
        .attr('width', function(d) { return time2Width(d, 0, true); })
        .attr('height', function() { return miniBarHeight; });
    }
    // todo: use vars better to stop reuse.
    var zoomBars = svgGroups.zoomGroup
                      .selectAll('g.miniTimeBar')
                      .data(nodesSort, idFunc);
    // create and update zoom bars
    zoomBars
      .enter()
      .append('g')
      .attr('class', 'miniTimeBar')
      .on('mousedown', miniTimeBarMouseDown)
      .each(updateZoomBar);
    zoomBars.each(updateZoomBar)

    // zoom bar rects
    svgGroups.zoomGroup.select('rect')
                       .attr('y',layerOffset)
                       .attr('height', miniBarSpacing*nodesSort.length);
    zoomSelection.attr('y',layerOffset)
                 .attr('height', miniBarSpacing*nodesSort.length);

    // calc and set parent svg height
    layerOffset += miniBarSpacing*nodesSort.length + 50;
    d3.select('svg')
      .attr('height', layerOffset);

  }

  // data:
  var flatData; // passed in from the engine
  var globalTime; // the displayed time
  var minTime; // the connection time that becomes the min axis value
  var cachedTime=0.0; // the time passed in, but maybe not displayed yet
  var treeData; // the displayed hierarchy created from flat data
  var cachedTreeData; // the hierarchy created from flat data but maybe not displayed yet
  var hasPendingUpdates = false; // whether there are data that have not been displayed
    

  // // // // // // // // // // // // // // // // // // 
  // this module's methods, passed back to calling page
  // // // // // // // // // // // // // // // // // // 

  myMethods.update = function(dt, elem) {
    cachedTime += dt;
    if( hasLiveUpdates() ) {
      globalTime += cachedTime;
      cachedTime = 0.0;
      if( typeof treeData !== 'undefined' ) {
        update(treeData);
      }

      // if the time boxes all moved but the mouse didn't, we still 
      // want to change the time cursor
      timeCursorTime = calcTimeCursorTime();
      updateTimeCursor();
    }
  } // end update

  myMethods.init = function(elem) {

    params.duration = 400;
    params.margin = {top: 30, right: 20, bottom: 30, left: 20};
    params.frameWidth = $(elem).width() - params.margin.right - params.margin.left;
    params.frameHeight = 300 - params.margin.top - params.margin.bottom;
    params.pathOffset = 10;
    params.barHeight = 30;

    $('<h4>Usage: Move your mouse around the main window to see active behaviors and the times they were active. You may also drag your cursor in the bottom window to zoom in on a particular period of time. Click in the same box to zoom out. Zooming will pause live updates, so you should toggle the switch on the left when you\'re done.</h3>').appendTo( elem );
    
    activeFeatureDiv = $('<h3 id="activeFeature"></h3>').appendTo( elem );
    currentBehaviorDiv = $('<h3 id="currentBehavior"></h3>').appendTo( elem );
    currentBehaviorStateDiv = $('<h3 id="currentBehaviorDebugState"></h3>').appendTo( elem );

    var svg = d3.select(elem)
                .append('svg')
                .attr('width', params.frameWidth + params.margin.right + params.margin.left)
                .append('g')
                .attr('transform', 'translate(' + params.margin.left + ',' + params.margin.top + ')');

    svgGroups.rowGroup = svg.append('g')
                            .attr('class', 'rowGroup');
    svgGroups.labelGroup = svg.append('g')
                              .attr('class', 'labelGroup');
    svgGroups.timeBarsGroup = svg.append('g')
                                 .attr('class', 'timeBarsGroup');
    svgGroups.axisGroup = svg.append('g')
                             .attr('class', 'axisGroup');
    svgGroups.zoomGroup = svg.append('g')
                             .attr('class', 'zoomGroup');

    svgGroups.timeBarsGroup.append('line')
      .attr('class', 'currentTimeLine')
      .attr('x1', params.frameWidth)
      .attr('y1', 0)
      .attr('x2', params.frameWidth)
      .attr('y2', 0); // initial height is 0 (hidden)

    svgGroups.timeBarsGroup.on('mousemove', timeBarMousemove);
    svgGroups.rowGroup.on('mousemove', timeBarMousemove);
    svgGroups.timeBarsGroup.on('mouseout', timeBarMouseout);
    svgGroups.rowGroup.on('mouseout', timeBarMouseout);

    // create and hook up toggle for live updates
    svgGroups.axisGroup.append('rect').attr('class', 'toggleContainer').on('click',function(d){ 
      setLiveUpdates( !hasLiveUpdates() );
    });
    svgGroups.axisGroup.append('rect').attr('class', 'toggleSwitch').on('click',function(d) {
      setLiveUpdates( !hasLiveUpdates() );
    })

    // zoom bars background (mostly just for mouse events)
    svgGroups.zoomGroup.append('rect')
             .attr('width', params.frameWidth)
             .attr('height',30)
             .attr('class','miniTimeBarBackground')
             .on('mousedown', miniTimeBarMouseDown);
    // overlay for selected zoom region
    zoomSelection = svgGroups.zoomGroup.append('rect')
             .attr('width', 0)
             .attr('height',30)
             .attr('class','zoomSelection')
             // the overlay is move-able and resizeable
             .on('mousemove', function() {
               var div = d3.select(this);
               var mousePos = d3.mouse(div.node());
               var borderBuffer = 5;
               var x1 = parseFloat(div.attr('x'));
               var width = parseFloat(div.attr('width'));
               var x2 = x1 + width;
               var borderHover = (mousePos[0] < x1 + borderBuffer) || 
                                 (mousePos[0] > x2 - borderBuffer);
               div.classed('borderHover', borderHover);
             })
             .on('mousedown', function() {
               var div = d3.select(this);
               var resizing = div.classed('borderHover');
               var fixed;

               var mousePos = d3.mouse(div.node());
               var x1 = parseFloat(div.attr('x'));
               var width = parseFloat(div.attr('width'));
               var x2 = x1 + width;
               if( resizing ) {
                 // closest border tells us which is fixed
                 if( mousePos[0] < x1 + 0.5*width ) {
                   fixed = x2;
                 } else {
                   fixed = x1;
                 }
               } else {
                 // store the x distance from the left border
                 fixed = mousePos[0] - x1;
               }
               
               var w = d3.select(window)
                   .on('mousemove', mousemove)
                   .on('mouseup', mouseup);

               d3.event.preventDefault(); // disable text dragging

               function mousemove() {
                var mousePos = d3.mouse(div.node());
                 if( resizing ) {
                   if( mousePos[0] < fixed ) {
                    // clamp and set
                     var newLeft = Math.max(0,mousePos[0]);
                     zoomSelection.attr('x', newLeft )
                                  .attr('width', fixed - newLeft);
                   } else {
                     // clamp and set
                     var newRight = Math.min(mousePos[0], params.frameWidth);
                     zoomSelection.attr('x', fixed)
                                  .attr('width', newRight - fixed);
                   }
                 } else {
                   var width = parseFloat(div.attr('width'));
                   // clamp and set
                   var newLeft = Math.min(Math.max(0, mousePos[0] - fixed), params.frameWidth - width );
                   zoomSelection.attr('x', newLeft);
                 }
                 onZoomSelectionChanged();
                 setLiveUpdates(false);
  
               }

               function mouseup() {
                 w.on('mousemove', null).on('mouseup', null);
               }
             });
  }; // end init

  myMethods.onData = function(allData, elem) {

    if( (typeof allData.tree === 'undefined') &&
        (typeof allData.debugState === 'undefined') &&
        (typeof allData.activeFeature === 'undefined') ) {
      // currently the only other option a list of behaviors
      addControls( allData, elem );
      return;
    }
    else if( typeof allData.debugState !== 'undefined' &&
             typeof currentBehaviorStateDiv !== 'undefined' ) {
      currentBehaviorStateDiv.text('Latest state: ' + allData.debugState);
      return;
    }
    else if( typeof allData.activeFeature !== 'undefined' &&
             typeof activeFeatureDiv !== 'undefined' ) {
      activeFeatureDiv.text('Active feature: ' + allData.activeFeature);
      return;
    }

    cachedTime = allData.time;
    var stack = allData.stack;

    if( typeof cachedTreeData === 'undefined'
        || typeof flatData === 'undefined' ) {
      flatData = allData.tree;
    } else {
      for( var i=0; i<allData.tree.length; ++i ) {
        var elem = allData.tree[i];
        var idx = flatData.findIndex(function(d) {
          return ((d.behaviorID+d.parent) == (elem.behaviorID+elem.parent));
        });
        if( idx < 0 ) {
          flatData.push( elem );
        }
      }
    }

    cachedTreeData = d3.stratify()
                       .id(function(d) { return d.behaviorID; })
                       .parentId(function(d) { return d.parent; })
                       (flatData);
    
    // always update the current behavior, even if the toggle for live updates is off
    if( stack.length && (typeof currentBehaviorDiv !== 'undefined') ) {
      var newText = 'Current behavior: ' + stack[stack.length - 1];
      if( newText != currentBehaviorDiv.text() ) {
        currentBehaviorStateDiv.text();
      }
      currentBehaviorDiv.text( newText )
    } else if( typeof currentBehaviorDiv !== 'undefined' ) {
      currentBehaviorDiv.text( 'No running behavior' )
    }

    // always set the minTime if it hasnt been yet
    if( typeof minTime === 'undefined' ) {
      minTime = cachedTime - 1.0; // start 1 sec earlier to avoid divide by 0 checks
    }
    
    // add activeTimes to data, which also sticks it back into flatData as an added bonus
    cachedTreeData.each(function( node ) {
      if( stack.indexOf(node.data.behaviorID) >= 0 ) {
        // currently running
        if( typeof node.data.activeTimes === 'undefined' ) {
          node.data.activeTimes = [{start: cachedTime}];
        } else {
          var lastTime = node.data.activeTimes[node.data.activeTimes.length-1];
          if( typeof lastTime.end !== 'undefined' ) {
            // was inactive and is now active. add a new time
            node.data.activeTimes.push( {start: cachedTime} );
          }
        }
      } else {
        // currently inactive
        if( typeof node.data.activeTimes !== 'undefined' ) {
          var lastTime = node.data.activeTimes[node.data.activeTimes.length-1];
          if( typeof lastTime.end === 'undefined' ) {
            // was active and is now inactive. end it
            lastTime.end = cachedTime;
          }
        }
      }
    });

    if( hasLiveUpdates() ) {
      globalTime = cachedTime;
      cachedTime = 0.0;
      treeData = cachedTreeData;
      
      update(treeData);
    } else {
      hasPendingUpdates = true;
    }

  }; // end onData 

  myMethods.getStyles = function() {
    // return a string of all css styles to be injected
    return `

      #behaviorDropdown {
        margin-left: 20px;
        padding-left:20px;
      }
      .node text {
        cursor: pointer;
        font: 10px sans-serif;
      }
      .node text.activeLabel {
        fill:red;
      }

      * {
        font: 10px sans-serif;
      }

      .row:nth-child(odd) { fill: #F3F3F3; }
      .row:nth-child(even) { fill: #FFF; }

      .row:hover { fill: #C3C3C3; }

      path.link {
        fill: none;
        stroke: #9ecae1;
        stroke-width: 1.5px;
      }

      .timeBar text {
        alignment-baseline: central;
        fill:white;
        font: 10px sans-serif;
      }

      .timeBar {
        pointer-events: none;
      }

      .timeBar rect,
      .miniTimeBar rect {
        fill: #333;
        fill-opacity: .5;
      }
      .miniTimeBarBackground {
        fill:#fff;
        stroke-width:1;
        stroke:#000;
      }

      .zoomSelection {
        fill:#ff0000;
        opacity:0.5;
      }

      .borderHover:hover {
        cursor:ew-resize;
      }

      .currentTimeLine {
        stroke-width:2;
        stroke-opacity:0.5;
        stroke:red;
        pointer-events: none;
      }
      #activeFeature {
        font-size:12px;
        margin-top: 3px;
        margin-bottom:3px;
      }
      #currentBehavior {
        font-size:16px;
        margin-top: 5px;
        margin-bottom:5px;
      }
      #showActivatable {
        margin-left:10px;
        margin-right:2px;
      }
      #downloadTimeBars {
        padding: 5px 10px;
        margin-right:20px;
        float:right;
      }
      `
  }; // end getStyles

})(moduleMethods, moduleSendDataFunc);
