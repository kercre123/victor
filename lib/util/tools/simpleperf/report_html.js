/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
'use strict';

// Use IIFE to avoid leaking names to other scripts.
$(document).ready(function() {

function openHtml(name, attrs={}) {
    let s = `<${name} `;
    for (let key in attrs) {
        s += `${key}="${attrs[key]}" `;
    }
    s += '>';
    return s;
}

function closeHtml(name) {
    return `</${name}>`;
}

function getHtml(name, attrs={}) {
    let text;
    if ('text' in attrs) {
        text = attrs.text;
        delete attrs.text;
    }
    let s = openHtml(name, attrs);
    if (text) {
        s += text;
    }
    s += closeHtml(name);
    return s;
}

function getTableRow(cols, colName, attrs={}) {
    let s = openHtml('tr', attrs);
    for (let col of cols) {
        s += `<${colName}>${col}</${colName}>`;
    }
    s += '</tr>';
    return s;
}

function toPercentageStr(percentage) {
    return percentage.toFixed(2) + '%';
}

function getProcessName(pid) {
    let name = gProcesses[pid];
    return name ? `${pid} (${name})`: pid.toString();
}

function getThreadName(tid) {
    let name = gThreads[tid];
    return name ? `${tid} (${name})`: tid.toString();
}

function getLibName(libId) {
    return gLibList[libId];
}

function getFuncName(funcId) {
    return gFunctionMap[funcId][1];
}

function getLibNameOfFunction(funcId) {
    return getLibName(gFunctionMap[funcId][0]);
}

class TabManager {
    constructor(divContainer) {
        this.div = $('<div>', {id: 'tabs'});
        this.div.appendTo(divContainer);
        this.div.append(getHtml('ul'));
        this.tabs = [];
        this.isDrawCalled = false;
    }

    addTab(title, tabObj) {
        let id = 'tab_' + this.div.children().length;
        let tabDiv = $('<div>', {id: id});
        tabDiv.appendTo(this.div);
        this.div.children().first().append(
            getHtml('li', {text: getHtml('a', {href: '#' + id, text: title})}));
        tabObj.init(tabDiv);
        this.tabs.push(tabObj);
        if (this.isDrawCalled) {
            this.div.tabs('refresh');
        }
        return tabObj;
    }

    findTab(title) {
        let links = this.div.find('li a');
        for (let i = 0; i < links.length; ++i) {
            if (links.eq(i).text() == title) {
                return this.tabs[i];
            }
        }
        return null;
    }

    draw() {
        this.div.tabs({
            active: 0,
        });
        this.tabs.forEach(function(tab) {
            tab.draw();
        });
        this.isDrawCalled = true;
    }

    setActive(tabObj) {
        for (let i = 0; i < this.tabs.length; ++i) {
            if (this.tabs[i] == tabObj) {
                this.div.tabs('option', 'active', i);
                break;
            }
        }
    }
}

// Show global information retrieved from the record file, including:
//   record time
//   machine type
//   Android version
//   record cmdline
//   total samples
class RecordFileView {
    constructor(divContainer) {
        this.div = $('<div>');
        this.div.appendTo(divContainer);
    }

    draw() {
        if (gRecordInfo.recordTime) {
            this.div.append(getHtml('p', {text: 'Record Time: ' + gRecordInfo.recordTime}));
        }
        if (gRecordInfo.machineType) {
            this.div.append(getHtml('p', {text: 'Machine Type: ' + gRecordInfo.machineType}));
        }
        if (gRecordInfo.androidVersion) {
            this.div.append(getHtml('p', {text: 'Android Version: ' + gRecordInfo.androidVersion}));
        }
        if (gRecordInfo.recordCmdline) {
            this.div.append(getHtml('p', {text: 'Record Cmdline: ' + gRecordInfo.recordCmdline}));
        }
        this.div.append(getHtml('p', {text: 'Total Samples: ' + gRecordInfo.totalSamples}));
    }
}

// Show pieChart of event count percentage of each process, thread, library and function.
class ChartView {
    constructor(divContainer, eventInfo) {
        this.id = divContainer.children().length;
        this.div = $('<div>', {id: 'chartstat_' + this.id});
        this.div.appendTo(divContainer);
        this.eventInfo = eventInfo;
        this.processInfo = null;
        this.threadInfo = null;
        this.libInfo = null;
        this.states = {
            SHOW_EVENT_INFO: 1,
            SHOW_PROCESS_INFO: 2,
            SHOW_THREAD_INFO: 3,
            SHOW_LIB_INFO: 4,
        };
    }

    _getState() {
        if (this.libInfo) {
            return this.states.SHOW_LIB_INFO;
        }
        if (this.threadInfo) {
            return this.states.SHOW_THREAD_INFO;
        }
        if (this.processInfo) {
            return this.states.SHOW_PROCESS_INFO;
        }
        return this.states.SHOW_EVENT_INFO;
    }

    _drawTitle() {
        if (this.eventInfo) {
            this.div.append(getHtml('p', {text: `Event Type: ${this.eventInfo.eventName}`}));
        }
        if (this.processInfo) {
            this.div.append(getHtml('p',
                {text: `Process: ${getProcessName(this.processInfo.pid)}`}));
        }
        if (this.threadInfo) {
            this.div.append(getHtml('p',
                {text: `Thread: ${getThreadName(this.threadInfo.tid)}`}));
        }
        if (this.libInfo) {
            this.div.append(getHtml('p',
                {text: `Library: ${getLibName(this.libInfo.libId)}`}));
        }
        if (this.processInfo) {
            let button = $('<button>', {text: 'Back'});
            button.appendTo(this.div);
            button.button().click(() => this._goBack());
        }
    }

    _goBack() {
        let state = this._getState();
        if (state == this.states.SHOW_PROCESS_INFO) {
            this.processInfo = null;
        } else if (state == this.states.SHOW_THREAD_INFO) {
            this.threadInfo = null;
        } else if (state == this.states.SHOW_LIB_INFO) {
            this.libInfo = null;
        }
        this.draw();
    }

    _selectHandler(chart) {
        let selectedItem = chart.getSelection()[0];
        if (selectedItem) {
            let state = this._getState();
            if (state == this.states.SHOW_EVENT_INFO) {
                this.processInfo = this.eventInfo.processes[selectedItem.row];
            } else if (state == this.states.SHOW_PROCESS_INFO) {
                this.threadInfo = this.processInfo.threads[selectedItem.row];
            } else if (state == this.states.SHOW_THREAD_INFO) {
                this.libInfo = this.threadInfo.libs[selectedItem.row];
            }
            this.draw();
        }
    }

    draw() {
        google.charts.setOnLoadCallback(() => this.realDraw());
    }

    realDraw() {
        this.div.empty();
        this._drawTitle();
        let data = new google.visualization.DataTable();
        let title;
        let state = this._getState();
        if (state == this.states.SHOW_EVENT_INFO) {
            title = 'Processes in event type ' + this.eventInfo.eventName;
            data.addColumn('string', 'Process');
            data.addColumn('number', 'EventCount');
            let rows = [];
            for (let process of this.eventInfo.processes) {
                rows.push([getProcessName(process.pid), process.eventCount]);
            }
            data.addRows(rows);
        } else if (state == this.states.SHOW_PROCESS_INFO) {
            title = 'Threads in process ' + getProcessName(this.processInfo.pid);
            data.addColumn('string', 'Thread');
            data.addColumn('number', 'EventCount');
            let rows = [];
            for (let thread of this.processInfo.threads) {
                rows.push([getThreadName(thread.tid), thread.eventCount]);
            }
            data.addRows(rows);
        } else if (state == this.states.SHOW_THREAD_INFO) {
            title = 'Libraries in thread ' + getThreadName(this.threadInfo.tid);
            data.addColumn('string', 'Lib');
            data.addColumn('number', 'EventCount');
            let rows = [];
            for (let lib of this.threadInfo.libs) {
                rows.push([getLibName(lib.libId), lib.eventCount]);
            }
            data.addRows(rows);
        } else if (state == this.states.SHOW_LIB_INFO) {
            title = 'Functions in library ' + getLibName(this.libInfo.libId);
            data.addColumn('string', 'Function');
            data.addColumn('number', 'EventCount');
            let rows = [];
            for (let func of this.libInfo.functions) {
                rows.push([getFuncName(func.g.f), func.g.e]);
            }
            data.addRows(rows);
        }

        let options = {
            title: title,
            width: 1000,
            height: 600,
        };
        let wrapperDiv = $('<div>');
        wrapperDiv.appendTo(this.div);
        let chart = new google.visualization.PieChart(wrapperDiv.get(0));
        chart.draw(data, options);
        google.visualization.events.addListener(chart, 'select', () => this._selectHandler(chart));
    }
}


class ChartStatTab {
    constructor() {
    }

    init(div) {
        this.div = div;
        this.recordFileView = new RecordFileView(this.div);
        this.chartViews = [];
        for (let eventInfo of gSampleInfo) {
            this.chartViews.push(new ChartView(this.div, eventInfo));
        }
    }

    draw() {
        this.recordFileView.draw();
        for (let charView of this.chartViews) {
            charView.draw();
        }
    }
}


class SampleTableTab {
    constructor() {
    }

    init(div) {
        this.div = div;
    }

    draw() {
        for (let tId = 0; tId < gSampleInfo.length; tId++) {
            let eventInfo = gSampleInfo[tId];
            let eventName = eventInfo.eventName;
            this.div.append(getHtml('p', {text: 'Sample table for event ' + eventName}));
            let percentMul = 100.0 / eventInfo.eventCount;
            let tableId = 'reportTable_' + tId;
            let titles = ['Total', 'Self', 'SampleCount', 'Process', 'Thread', 'Lib', 'Function'];
            let tableStr = openHtml('table', {id: tableId, cellspacing: '0', width: '100%'}) +
                           getHtml('thead', {text: getTableRow(titles, 'th')}) +
                           getHtml('tfoot', {text: getTableRow(titles, 'th')}) +
                           openHtml('tbody');

            for (let i = 0; i < eventInfo.processes.length; ++i) {
                let processInfo = eventInfo.processes[i];
                let processName = getProcessName(processInfo.pid);
                for (let j = 0; j < processInfo.threads.length; ++j) {
                    let threadInfo = processInfo.threads[j];
                    let threadName = getThreadName(threadInfo.tid);
                    for (let k = 0; k < threadInfo.libs.length; ++k) {
                        let lib = threadInfo.libs[k];
                        for (let t = 0; t < lib.functions.length; ++t) {
                            let func = lib.functions[t];
                            let key = [i, j, k, t].join('_');
                            let treePercentage = toPercentageStr(func.g.s * percentMul);
                            let selfPercenetage = toPercentageStr(func.g.e * percentMul);
                            tableStr += getTableRow([treePercentage, selfPercenetage, func.c,
                                processName, threadName, getLibName(lib.libId),
                                getFuncName(func.g.f)], 'td', {key: key});
                        }
                    }
                }
            }
            tableStr += closeHtml('tbody') + closeHtml('table');
            this.div.append(tableStr);
            let table = this.div.find(`table#${tableId}`).dataTable({
                lengthMenu: [10, 20, 50, 100, -1],
                processing: true,
                order: [0, 'desc'],
                responsive: true,
            });

            table.find('tr').css('cursor', 'pointer');
            table.on('click', 'tr', function() {
                let key = this.getAttribute('key');
                if (!key) {
                    return;
                }
                let indexes = key.split('_');
                let processInfo = eventInfo.processes[indexes[0]];
                let threadInfo = processInfo.threads[indexes[1]];
                let lib = threadInfo.libs[indexes[2]];
                let func = lib.functions[indexes[3]];
                FunctionTab.showFunction(eventInfo, processInfo, threadInfo, lib, func);
            });
        }
    }
}


// Show embedded flamegraph generated by inferno.
class FlameGraphTab {
    constructor() {
    }

    init(div) {
        this.div = div;
    }

    draw() {
        $('div#flamegraph_id').appendTo(this.div).css('display', 'block');
        flamegraphInit();
    }
}


// FunctionTab: show information of a function.
// 1. Show the callgrpah and reverse callgraph of a function as flamegraphs.
class FunctionTab {
    static showFunction(eventInfo, processInfo, threadInfo, lib, func) {
        let title = 'Function';
        let tab = gTabs.findTab(title);
        if (!tab) {
            tab = gTabs.addTab(title, new FunctionTab());
        }
        tab.setFunction(eventInfo, processInfo, threadInfo, lib, func);
    }

    constructor() {
        this.func = null;
        this.selectPercent = 'thread';
    }

    init(div) {
        this.div = div;
    }

    setFunction(eventInfo, processInfo, threadInfo, lib, func) {
        this.eventInfo = eventInfo;
        this.processInfo = processInfo;
        this.threadInfo = threadInfo;
        this.lib = lib;
        this.func = func;
        this.selectorView = null;
        this.callgraphView = null;
        this.reverseCallgraphView = null;
        this.draw();
        gTabs.setActive(this);
    }

    draw() {
        if (!this.func) {
            return;
        }
        this.div.empty();
        let eventName = this.eventInfo.eventName;
        let processName = getProcessName(this.processInfo.pid);
        let threadName = getThreadName(this.threadInfo.tid);
        let libName = getLibName(this.lib.libId);
        let funcName = getFuncName(this.func.g.f);
        let title = getHtml('p', {text: `Event ${eventName}`}) +
                    getHtml('p', {text: `Process ${processName}`}) +
                    getHtml('p', {text: `Thread ${threadName}`}) +
                    getHtml('p', {text: `Library ${libName}`}) +
                    getHtml('p', {text: `Function ${funcName}`});
        this.div.append(title);

        this.selectorView = new FunctionSampleWeightSelectorView(this.div, this.eventInfo,
            this.processInfo, this.threadInfo, () => this.onSampleWeightChange());
        this.selectorView.draw();

        this.div.append(getHtml('hr'));
        this.div.append(getHtml('b', {text: `Functions called by ${funcName}`}) + '<br/>');
        this.callgraphView = new FlameGraphView(this.div, this.func.g, false);

        this.div.append(getHtml('hr'));
        this.div.append(getHtml('b', {text: `Functions calling ${funcName}`}) + '<br/>');
        this.reverseCallgraphView = new FlameGraphView(this.div, this.func.rg, true);
        this.onSampleWeightChange();  // Manually set sample weight function for the first time.
    }

    onSampleWeightChange() {
        let sampleWeightFunction = this.selectorView.getSampleWeightFunction();
        if (this.callgraphView) {
            this.callgraphView.draw(sampleWeightFunction);
        }
        if (this.reverseCallgraphView) {
            this.reverseCallgraphView.draw(sampleWeightFunction);
        }
    }
}


// Select the way to show sample weight in FunctionTab.
// 1. Show percentage of event count relative to all processes.
// 2. Show percentage of event count relative to the current process.
// 3. Show percentage of event count relative to the current thread.
// 4. Show absolute event count.
// 5. Show event count in milliseconds, only possible for cpu-clock or task-clock events.
class FunctionSampleWeightSelectorView {
    constructor(divContainer, eventInfo, processInfo, threadInfo, onSelectChange) {
        this.div = $('<div>');
        this.div.appendTo(divContainer);
        this.onSelectChange = onSelectChange;
        this.eventCountForAllProcesses = eventInfo.eventCount;
        this.eventCountForProcess = processInfo.eventCount;
        this.eventCountForThread = threadInfo.eventCount;
        this.options = {
            PERCENT_TO_ALL_PROCESSES: 0,
            PERCENT_TO_CUR_PROCESS: 1,
            PERCENT_TO_CUR_THREAD: 2,
            RAW_EVENT_COUNT: 3,
            EVENT_COUNT_IN_TIME: 4,
        }
        let name = eventInfo.eventName;
        this.supportEventCountInTime = name.includes('task-clock') || name.includes('cpu-clock');
        if (this.supportEventCountInTime) {
            this.curOption = this.options.EVENT_COUNT_IN_TIME;
        } else {
            this.curOption = this.options.PERCENT_TO_CUR_THREAD;
        }
    }

    draw() {
        let options = [];
        options.push('Show percentage of event count relative to all processes.');
        options.push('Show percentage of event count relative to the current process.');
        options.push('Show percentage of event count relative to the current thread.');
        options.push('Show event count.');
        if (this.supportEventCountInTime) {
            options.push('Show event count in milliseconds.');
        }
        let optionStr = '';
        for (let i = 0; i < options.length; ++i) {
            optionStr += getHtml('option', {value: i, text: options[i]});
        }
        this.div.append(getHtml('select', {text: optionStr}));
        let selectMenu = this.div.children().last();
        selectMenu.children().eq(this.curOption).attr('selected', 'selected');
        let thisObj = this;
        selectMenu.selectmenu({
            change: function() {
                thisObj.curOption = this.value;
                thisObj.onSelectChange();
            },
            width: '100%',
        });
    }

    getSampleWeightFunction() {
        let thisObj = this;
        if (this.curOption == this.options.PERCENT_TO_ALL_PROCESSES) {
            return function(eventCount) {
                let percent = eventCount * 100.0 / thisObj.eventCountForAllProcesses;
                return percent.toFixed(2) + '%';
            };
        }
        if (this.curOption == this.options.PERCENT_TO_CUR_PROCESS) {
            return function(eventCount) {
                let percent = eventCount * 100.0 / thisObj.eventCountForProcess;
                return percent.toFixed(2) + '%';
            };
        }
        if (this.curOption == this.options.PERCENT_TO_CUR_THREAD) {
            return function(eventCount) {
                let percent = eventCount * 100.0 / thisObj.eventCountForThread;
                return percent.toFixed(2) + '%';
            };
        }
        if (this.curOption == this.options.RAW_EVENT_COUNT) {
            return function(eventCount) {
                return '' + eventCount;
            };
        }
        if (this.curOption == this.options.EVENT_COUNT_IN_TIME) {
            return function(eventCount) {
                let timeInMs = eventCount / 1000000.0;
                return timeInMs.toFixed(3) + ' ms';
            };
        }
    }
}


// Given a callgraph, show the flamegraph.
class FlameGraphView {
    // If reverseOrder is false, the root of the flamegraph is at the bottom,
    // otherwise it is at the top.
    constructor(divContainer, callgraph, reverseOrder) {
        this.id = divContainer.children().length;
        this.div = $('<div>', {id: 'fg_' + this.id});
        this.div.appendTo(divContainer);
        this.callgraph = callgraph;
        this.reverseOrder = reverseOrder;
        this.sampleWeightFunction = null;
        this.svgWidth = $(window).width();
        this.svgNodeHeight = 17;
        this.fontSize = 12;

        function getMaxDepth(node) {
            let depth = 0;
            for (let child of node.c) {
                depth = Math.max(depth, getMaxDepth(child));
            }
            return depth + 1;
        }
        this.maxDepth = getMaxDepth(this.callgraph);
        this.svgHeight = this.svgNodeHeight * (this.maxDepth + 3);
    }

    draw(sampleWeightFunction) {
        this.sampleWeightFunction = sampleWeightFunction;
        this.div.empty();
        this.div.css('width', '100%').css('height', this.svgHeight + 'px');
        let svgStr = '<svg xmlns="http://www.w3.org/2000/svg" \
        xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1" \
        width="100%" height="100%" style="border: 1px solid black; font-family: Monospace;"> \
        </svg>';
        this.div.append(svgStr);
        this.svg = this.div.find('svg');
        this._renderBackground();
        this._renderSvgNodes(this.callgraph, 0, 0);
        this._renderUnzoomNode();
        this._renderInfoNode();
        this._renderPercentNode();
        // Make the added nodes in the svg visible.
        this.div.html(this.div.html());
        this.svg = this.div.find('svg');
        this._adjustTextSize();
        this._enableZoom();
        this._enableInfo();
        this._adjustTextSizeOnResize();
    }

    _renderBackground() {
        this.svg.append(`<defs > <linearGradient id="background_gradient_${this.id}"
                                  y1="0" y2="1" x1="0" x2="0" > \
                                  <stop stop-color="#eeeeee" offset="5%" /> \
                                  <stop stop-color="#efefb1" offset="90%" /> \
                                  </linearGradient> \
                         </defs> \
                         <rect x="0" y="0" width="100%" height="100%" \
                           fill="url(#background_gradient_${this.id})" />`);
    }

    _getYForDepth(depth) {
        if (this.reverseOrder) {
            return (depth + 3) * this.svgNodeHeight;
        }
        return this.svgHeight - (depth + 1) * this.svgNodeHeight;
    }

    _getWidthPercentage(eventCount) {
        return eventCount * 100.0 / this.callgraph.s;
    }

    _getHeatColor(widthPercentage) {
        return {
            r: Math.floor(245 + 10 * (1 - widthPercentage * 0.01)),
            g: Math.floor(110 + 105 * (1 - widthPercentage * 0.01)),
            b: 100,
        };
    }

    _renderSvgNodes(callNode, depth, xOffset) {
        let x = xOffset;
        let y = this._getYForDepth(depth);
        let width = this._getWidthPercentage(callNode.s);
        if (width < 0.1) {
            return xOffset;
        }
        let color = this._getHeatColor(width);
        let borderColor = {};
        for (let key in color) {
            borderColor[key] = Math.max(0, color[key] - 50);
        }
        let funcName = getFuncName(callNode.f);
        let libName = getLibNameOfFunction(callNode.f);
        let sampleWeight = this.sampleWeightFunction(callNode.s);
        let title = funcName + ' | ' + libName + ' (' + callNode.s + ' events: ' +
                    sampleWeight + ')';
        this.svg.append(`<g> <title>${title}</title> <rect x="${x}%" y="${y}" ox="${x}" \
                        depth="${depth}" width="${width}%" owidth="${width}" height="15.0" \
                        ofill="rgb(${color.r},${color.g},${color.b})" \
                        fill="rgb(${color.r},${color.g},${color.b})" \
                        style="stroke:rgb(${borderColor.r},${borderColor.g},${borderColor.b})"/> \
                        <text x="${x}%" y="${y + 12}" font-size="${this.fontSize}" \
                        font-family="Monospace"></text></g>`);

        let childXOffset = xOffset;
        for (let child of callNode.c) {
            childXOffset = this._renderSvgNodes(child, depth + 1, childXOffset);
        }
        return xOffset + width;
    }

    _renderUnzoomNode() {
        this.svg.append(`<rect id="zoom_rect_${this.id}" style="display:none;stroke:rgb(0,0,0);" \
        rx="10" ry="10" x="10" y="10" width="80" height="30" \
        fill="rgb(255,255,255)"/> \
         <text id="zoom_text_${this.id}" x="19" y="30" style="display:none">Zoom out</text>`);
    }

    _renderInfoNode() {
        this.svg.append(`<clipPath id="info_clip_path_${this.id}"> \
                         <rect style="stroke:rgb(0,0,0);" rx="10" ry="10" x="120" y="10" \
                         width="789" height="30" fill="rgb(255,255,255)"/> \
                         </clipPath> \
                         <rect style="stroke:rgb(0,0,0);" rx="10" ry="10" x="120" y="10" \
                         width="799" height="30" fill="rgb(255,255,255)"/> \
                         <text clip-path="url(#info_clip_path_${this.id})" \
                         id="info_text_${this.id}" x="128" y="30"></text>`);
    }

    _renderPercentNode() {
        this.svg.append(`<rect style="stroke:rgb(0,0,0);" rx="10" ry="10" \
                         x="934" y="10" width="100" height="30" \
                         fill="rgb(255,255,255)"/> \
                         <text id="percent_text_${this.id}" text-anchor="end" \
                         x="1024" y="30">100.00%</text>`);
    }

    _adjustTextSizeForNode(g) {
        let text = g.find('text');
        let width = parseFloat(g.find('rect').attr('width')) * this.svgWidth * 0.01;
        if (width < 28) {
            text.text('');
            return;
        }
        let methodName = g.find('title').text().split(' | ')[0];
        let numCharacters;
        for (numCharacters = methodName.length; numCharacters > 4; numCharacters--) {
            if (numCharacters * 7.5 <= width) {
                break;
            }
        }
        if (numCharacters == methodName.length) {
            text.text(methodName);
        } else {
            text.text(methodName.substring(0, numCharacters - 2) + '..');
        }
    }

    _adjustTextSize() {
        this.svgWidth = $(window).width();
        let thisObj = this;
        this.svg.find('g').each(function(_, g) {
            thisObj._adjustTextSizeForNode($(g));
        });
    }

    _enableZoom() {
        this.zoomStack = [this.svg.find('g').first().get(0)];
        this.svg.find('g').css('cursor', 'pointer').click(zoom);
        this.svg.find(`#zoom_rect_${this.id}`).css('cursor', 'pointer').click(unzoom);
        this.svg.find(`#zoom_text_${this.id}`).css('cursor', 'pointer').click(unzoom);

        let thisObj = this;
        function zoom() {
            thisObj.zoomStack.push(this);
            displayFromElement(this);
            thisObj.svg.find(`#zoom_rect_${thisObj.id}`).css('display', 'block');
            thisObj.svg.find(`#zoom_text_${thisObj.id}`).css('display', 'block');
        }

        function unzoom() {
            if (thisObj.zoomStack.length > 1) {
                thisObj.zoomStack.pop();
                displayFromElement(thisObj.zoomStack[thisObj.zoomStack.length - 1]);
                if (thisObj.zoomStack.length == 1) {
                    thisObj.svg.find(`#zoom_rect_${thisObj.id}`).css('display', 'none');
                    thisObj.svg.find(`#zoom_text_${thisObj.id}`).css('display', 'none');
                }
            }
        }

        function displayFromElement(g) {
            g = $(g);
            let clickedRect = g.find('rect');
            let clickedOriginX = parseFloat(clickedRect.attr('ox'));
            let clickedDepth = parseInt(clickedRect.attr('depth'));
            let clickedOriginWidth = parseFloat(clickedRect.attr('owidth'));
            let scaleFactor = 100.0 / clickedOriginWidth;
            thisObj.svg.find('g').each(function(_, g) {
                g = $(g);
                let text = g.find('text');
                let rect = g.find('rect');
                let depth = parseInt(rect.attr('depth'));
                let ox = parseFloat(rect.attr('ox'));
                let owidth = parseFloat(rect.attr('owidth'));
                if (depth < clickedDepth || ox < clickedOriginX - 1e-9 ||
                    ox + owidth > clickedOriginX + clickedOriginWidth + 1e-9) {
                    rect.css('display', 'none');
                    text.css('display', 'none');
                } else {
                    rect.css('display', 'block');
                    text.css('display', 'block');
                    let nx = (ox - clickedOriginX) * scaleFactor + '%';
                    let ny = thisObj._getYForDepth(depth - clickedDepth);
                    rect.attr('x', nx);
                    rect.attr('y', ny);
                    rect.attr('width', owidth * scaleFactor + '%');
                    text.attr('x', nx);
                    text.attr('y', ny + 12);
                    thisObj._adjustTextSizeForNode(g);
                }
            });
        }
    }

    _enableInfo() {
        this.selected = null;
        let thisObj = this;
        this.svg.find('g').on('mouseenter', function(e) {
            if (thisObj.selected) {
                thisObj.selected.css('stroke-width', '0');
            }
            // Mark current node.
            let g = $(this);
            thisObj.selected = g;
            g.css('stroke', 'black').css('stroke-width', '0.5');

            // Parse title.
            let title = g.find('title').text();
            let methodAndInfo = title.split(' | ');
            thisObj.svg.find(`#info_text_${thisObj.id}`).text(methodAndInfo[0]);

            // Parse percentage.
            // '/system/lib64/libhwbinder.so (4 events: 0.28%)'
            let regexp = /.* \(.*:\s+(.*)\)/g;
            let match = regexp.exec(methodAndInfo[1]);
            let percentage = '';
            if (match && match.length > 1) {
                percentage = match[1];
            }
            thisObj.svg.find(`#percent_text_${thisObj.id}`).text(percentage);
        });
    }

    _adjustTextSizeOnResize() {
        function throttle(callback) {
            let running = false;
            return function() {
                if (!running) {
                    running = true;
                    window.requestAnimationFrame(function () {
                        callback();
                        running = false;
                    });
                }
            };
        }
        $(window).resize(throttle(() => this._adjustTextSize()));
    }
}

function initGlobalObjects() {
    gTabs = new TabManager($('div#report_content'));
    let recordData = $('#record_data').text();
    gRecordInfo = JSON.parse(recordData);
    gProcesses = gRecordInfo.processNames;
    gThreads = gRecordInfo.threadNames;
    gLibList = gRecordInfo.libList;
    gFunctionMap = gRecordInfo.functionMap;
    gSampleInfo = gRecordInfo.sampleInfo;
}

function createTabs() {
    gTabs.addTab('Chart Statistics', new ChartStatTab());
    gTabs.addTab('Sample Table', new SampleTableTab());
    gTabs.addTab('Flamegraph', new FlameGraphTab());
    gTabs.draw();
}

let gTabs;
let gRecordInfo;
let gProcesses;
let gThreads;
let gLibList;
let gFunctionMap;
let gSampleInfo;

initGlobalObjects();
createTabs();

});