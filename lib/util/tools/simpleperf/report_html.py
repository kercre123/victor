#!/usr/bin/python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import datetime
import json
import os
import subprocess
import sys
import tempfile

from simpleperf_report_lib import ReportLib
from utils import *


class HtmlWriter(object):

    def __init__(self, output_path):
        self.fh = open(output_path, 'w')
        self.tag_stack = []

    def close(self):
        self.fh.close()

    def open_tag(self, tag, **attrs):
        attr_str = ''
        for key in attrs.keys():
            attr_str += ' %s="%s"' % (key, attrs[key])
        self.fh.write('<%s%s>' % (tag, attr_str))
        self.tag_stack.append(tag)
        return self

    def close_tag(self, tag=None):
        if tag:
            assert tag == self.tag_stack[-1]
        self.fh.write('</%s>\n' % self.tag_stack.pop())

    def add(self, text):
        self.fh.write(text)
        return self

    def add_file(self, file_path):
        file_path = os.path.join(get_script_dir(), file_path)
        with open(file_path, 'r') as f:
            self.add(f.read())
        return self


class EventScope(object):

    def __init__(self, name):
        self.name = name
        self.processes = {}  # map from pid to ProcessScope
        self.sample_count = 0
        self.event_count = 0

    def get_process(self, pid):
        process = self.processes.get(pid)
        if not process:
            process = self.processes[pid] = ProcessScope(pid)
        return process

    def get_sample_info(self):
        result = {}
        result['eventName'] = self.name
        result['eventCount'] = self.event_count
        result['processes'] = [process.get_sample_info() for process in self.processes.values()]
        return result


class ProcessScope(object):

    def __init__(self, pid):
        self.pid = pid
        self.name = ''
        self.event_count = 0
        self.threads = {}  # map from tid to ThreadScope

    def get_thread(self, tid, thread_name):
        thread = self.threads.get(tid)
        if not thread:
            thread = self.threads[tid] = ThreadScope(tid)
        thread.name = thread_name
        if self.pid == tid:
            self.name = thread_name
        return thread

    def get_sample_info(self):
        result = {}
        result['pid'] = self.pid
        result['eventCount'] = self.event_count
        result['threads'] = [thread.get_sample_info() for thread in self.threads.values()]
        return result


class ThreadScope(object):

    def __init__(self, tid):
        self.tid = tid
        self.name = ''
        self.event_count = 0
        self.libs = {}  # map from libId to LibScope

    def add_callstack(self, event_count, callstack):
        """ callstack is a list of (lib_id, func_id) pairs.
            For each i > 0, callstack[i] calls callstack[i-1]."""
        # When a callstack contains recursive function, we should only add event count
        # and callchain for each recursive function once.
        hit_func_ids = {}
        for i in range(len(callstack)):
            lib_id, func_id = callstack[i]
            lib = self.libs.get(lib_id)
            if not lib:
                lib = self.libs[lib_id] = LibScope(lib_id)
            function = lib.get_function(func_id)
            if i == 0:
                lib.event_count += event_count
                function.sample_count += 1
            if func_id in hit_func_ids:
                continue
            hit_func_ids[func_id] = True
            function.add_reverse_callchain(callstack, i + 1, len(callstack), event_count)

        hit_func_ids = {}
        for i in range(len(callstack) - 1, -1, -1):
            lib_id, func_id = callstack[i]
            if func_id in hit_func_ids:
                continue
            hit_func_ids[func_id] = True
            lib = self.libs.get(lib_id)
            lib.get_function(func_id).add_callchain(callstack, i - 1, -1, event_count)

    def get_sample_info(self):
        result = {}
        result['tid'] = self.tid
        result['eventCount'] = self.event_count
        result['libs'] = [lib.gen_sample_info() for lib in self.libs.values()]
        return result


class LibScope(object):

    def __init__(self, lib_id):
        self.lib_id = lib_id
        self.event_count = 0
        self.functions = {}  # map from func_id to FunctionScope.

    def get_function(self, func_id):
        function = self.functions.get(func_id)
        if not function:
            function = self.functions[func_id] = FunctionScope(func_id)
        return function

    def gen_sample_info(self):
        result = {}
        result['libId'] = self.lib_id
        result['eventCount'] = self.event_count
        result['functions'] = [func.gen_sample_info() for func in self.functions.values()]
        return result


class FunctionScope(object):

    def __init__(self, func_id):
        self.sample_count = 0
        self.call_graph = CallNode(func_id)
        self.reverse_call_graph = CallNode(func_id)

    def add_callchain(self, callchain, start, end, event_count):
        node = self.call_graph
        for i in range(start, end, -1):
            node = node.get_child(callchain[i][1])
        node.event_count += event_count

    def add_reverse_callchain(self, callchain, start, end, event_count):
        node = self.reverse_call_graph
        for i in range(start, end):
            node = node.get_child(callchain[i][1])
        node.event_count += event_count

    def update_subtree_event_count(self):
        a = self.call_graph.update_subtree_event_count()
        b = self.reverse_call_graph.update_subtree_event_count()
        return max(a, b)

    def limit_callchain_percent(self, min_callchain_percent):
        min_limit = min_callchain_percent * 0.01 * self.call_graph.subtree_event_count
        self.call_graph.cut_edge(min_limit)

    def hit_function(self, func_id_set):
        self.call_graph.hit_function(func_id_set)
        self.reverse_call_graph.hit_function(func_id_set)

    def gen_sample_info(self):
        result = {}
        result['c'] = self.sample_count
        result['g'] = self.call_graph.gen_sample_info()
        result['rg'] = self.reverse_call_graph.gen_sample_info()
        return result


class CallNode(object):

    def __init__(self, func_id):
        self.event_count = 0
        self.subtree_event_count = 0
        self.func_id = func_id
        self.children = {}  # map from func_id to CallNode

    def get_child(self, func_id):
        child = self.children.get(func_id)
        if not child:
            child = self.children[func_id] = CallNode(func_id)
        return child

    def update_subtree_event_count(self):
        self.subtree_event_count = self.event_count
        for child in self.children.values():
            self.subtree_event_count += child.update_subtree_event_count()
        return self.subtree_event_count

    def cut_edge(self, min_limit):
        to_del_children = []
        for key in self.children:
            child = self.children[key]
            if child.subtree_event_count < min_limit:
                to_del_children.append(key)
            else:
                child.cut_edge(min_limit)
        for key in to_del_children:
            del self.children[key]

    def hit_function(self, func_id_set):
        func_id_set.add(self.func_id)
        for child in self.children.values():
            child.hit_function(func_id_set)

    def gen_sample_info(self):
        result = {}
        result['e'] = self.event_count
        result['s'] = self.subtree_event_count
        result['f'] = self.func_id
        result['c'] = [child.gen_sample_info() for child in self.children.values()]
        return result


class LibSet(object):

    def __init__(self):
        self.libs = {}

    def get_lib_id(self, lib_name):
        lib_id = self.libs.get(lib_name)
        if lib_id is None:
            lib_id = len(self.libs)
            self.libs[lib_name] = lib_id
        return lib_id


class FunctionSet(object):

    def __init__(self):
        self.functions = {}

    def get_func_id(self, lib_id, func_name):
        key = (lib_id, func_name)
        func_id = self.functions.get(key)
        if func_id is None:
            func_id = len(self.functions)
            self.functions[key] = func_id
        return func_id


class RecordData(object):

    """RecordData reads perf.data, and generates data used by report.js in json format.
        All generated items are listed as below:
            1. recordTime: string
            2. machineType: string
            3. androidVersion: string
            4. recordCmdline: string
            5. totalSamples: int
            6. processNames: map from pid to processName.
            7. threadNames: map from tid to threadName.
            8. libList: an array of libNames, indexed by libId.
            9. functionMap: map from functionId to [libId, functionName].
            10.  sampleInfo = [eventInfo]
                eventInfo = {
                    eventName
                    eventCount
                    processes: [processInfo]
                }
                processInfo = {
                    pid
                    eventCount
                    threads: [threadInfo]
                }
                threadInfo = {
                    tid
                    eventCount
                    libs: [libInfo],
                }
                libInfo = {
                    libId,
                    eventCount,
                    functions: [funcInfo]
                }
                funcInfo = {
                    c: sampleCount
                    g: callGraph
                    rg: reverseCallgraph
                }
                callGraph and reverseCallGraph are both of type CallNode.
                callGraph shows how a function calls other functions.
                reverseCallGraph shows how a function is called by other functions.
                CallNode {
                    e: selfEventCount
                    s: subTreeEventCount
                    f: functionId
                    c: [CallNode] # children
                }
    """

    # Anki, allow symbol_cache to be overridden
    def __init__(self, record_file, symfs, min_func_percent, min_callchain_percent):
        self._load_record_file(record_file, symfs)
        self._limit_percents(min_func_percent, min_callchain_percent)

    def _load_record_file(self, record_file, symfs):
        lib = ReportLib()
        lib.ShowIpForUnknownSymbol()
        lib.SetRecordFile(record_file)
        lib.SetSymfs(symfs)
        self.meta_info = lib.MetaInfo()
        self.cmdline = lib.GetRecordCmd()
        self.arch = lib.GetArch()
        self.events = {}
        self.libs = LibSet()
        self.functions = FunctionSet()
        self.total_samples = 0
        while True:
            raw_sample = lib.GetNextSample()
            if not raw_sample:
                lib.Close()
                break
            raw_event = lib.GetEventOfCurrentSample()
            symbol = lib.GetSymbolOfCurrentSample()
            callchain = lib.GetCallChainOfCurrentSample()
            event = self._get_event(raw_event.name)
            self.total_samples += 1
            event.sample_count += 1
            event.event_count += raw_sample.period
            process = event.get_process(raw_sample.pid)
            process.event_count += raw_sample.period
            thread = process.get_thread(raw_sample.tid, raw_sample.thread_comm)
            thread.event_count += raw_sample.period

            lib_id = self.libs.get_lib_id(symbol.dso_name)
            callstack = [(lib_id, self.functions.get_func_id(lib_id, symbol.symbol_name))]
            for i in range(callchain.nr):
                symbol = callchain.entries[i].symbol
                lib_id = self.libs.get_lib_id(symbol.dso_name)
                callstack.append((lib_id, self.functions.get_func_id(lib_id, symbol.symbol_name)))
            thread.add_callstack(raw_sample.period, callstack)

        for event in self.events.values():
            for process in event.processes.values():
                for thread in process.threads.values():
                    for lib in thread.libs.values():
                        for funcId in lib.functions.keys():
                            function = lib.functions[funcId]
                            function.update_subtree_event_count()

    def _limit_percents(self, min_func_percent, min_callchain_percent):
        for event in self.events.values():
            min_limit = event.event_count * min_func_percent * 0.01
            for process in event.processes.values():
                for thread in process.threads.values():
                    for lib in thread.libs.values():
                        for func_id in lib.functions.keys():
                            function = lib.functions[func_id]
                            if function.call_graph.subtree_event_count < min_limit:
                                del lib.functions[func_id]
                            else:
                                function.limit_callchain_percent(min_callchain_percent)

    def _get_event(self, event_name):
        if event_name not in self.events:
            self.events[event_name] = EventScope(event_name)
        return self.events[event_name]

    def gen_record_info(self):
        record_info = {}
        timestamp = self.meta_info.get('timestamp')
        if timestamp:
            t = datetime.datetime.fromtimestamp(int(timestamp))
        else:
            t = datetime.datetime.now()
        record_info['recordTime'] = t.strftime('%Y-%m-%d (%A) %H:%M:%S')

        product_props = self.meta_info.get('product_props')
        machine_type = self.arch
        if product_props:
            manufacturer, model, name = product_props.split(':')
            machine_type = '%s (%s) by %s, arch %s' % (model, name, manufacturer, self.arch)
        record_info['machineType'] = machine_type
        record_info['androidVersion'] = self.meta_info.get('android_version', '')
        record_info['recordCmdline'] = self.cmdline
        record_info['totalSamples'] = self.total_samples
        record_info['processNames'] = self._gen_process_names()
        record_info['threadNames'] = self._gen_thread_names()
        record_info['libList'] = self._gen_lib_list()
        record_info['functionMap'] = self._gen_function_map()
        record_info['sampleInfo'] = self._gen_sample_info()
        return record_info

    def _gen_process_names(self):
        process_names = {}
        for event in self.events.values():
            for process in event.processes.values():
                process_names[process.pid] = process.name
        return process_names

    def _gen_thread_names(self):
        thread_names = {}
        for event in self.events.values():
            for process in event.processes.values():
                for thread in process.threads.values():
                    thread_names[thread.tid] = thread.name
        return thread_names

    def _modify_name_for_html(self, name):
        return name # Anki, not required .replace('>', '&gt;').replace('<', '&lt;')

    def _gen_lib_list(self):
        ret = sorted(self.libs.libs.keys(), key=lambda k: self.libs.libs[k])
        return [self._modify_name_for_html(x) for x in ret]

    def _gen_function_map(self):
        func_id_set = set()
        for event in self.events.values():
            for process in event.processes.values():
                for thread in process.threads.values():
                    for lib in thread.libs.values():
                        for func_id in lib.functions.keys():
                            lib.functions[func_id].hit_function(func_id_set)

        functions = self.functions.functions
        func_map = {}
        for key in functions:
            func_id = functions[key]
            if func_id in func_id_set:
                func_map[func_id] = [key[0], self._modify_name_for_html(key[1])]
        return func_map

    def _gen_sample_info(self):
        return [event.get_sample_info() for event in self.events.values()]


class ReportGenerator(object):

    def __init__(self, html_path):
        self.hw = HtmlWriter(html_path)
        self.hw.open_tag('html')
        self.hw.open_tag('head')
        self.hw.open_tag('link', rel='stylesheet', type='text/css',
            href='https://code.jquery.com/ui/1.12.0/themes/smoothness/jquery-ui.css'
                         ).close_tag()

        self.hw.open_tag('link', rel='stylesheet', type='text/css',
             href='https://cdn.datatables.net/1.10.16/css/jquery.dataTables.min.css'
                         ).close_tag()
        self.hw.open_tag('script', src='https://www.gstatic.com/charts/loader.js').close_tag()
        self.hw.open_tag('script').add(
            "google.charts.load('current', {'packages': ['corechart']});").close_tag()
        self.hw.open_tag('script', src='https://code.jquery.com/jquery-3.2.1.js').close_tag()
        self.hw.open_tag('script', src='https://code.jquery.com/ui/1.12.1/jquery-ui.js'
                         ).close_tag()
        self.hw.open_tag('script',
            src='https://cdn.datatables.net/1.10.16/js/jquery.dataTables.min.js').close_tag()
        self.hw.open_tag('script',
            src='https://cdn.datatables.net/1.10.16/js/dataTables.jqueryui.min.js').close_tag()
        self.hw.close_tag('head')
        self.hw.open_tag('body')
        self.record_info = {}

    def write_content_div(self):
        self.hw.open_tag('div', id='report_content').close_tag()

    def write_record_data(self, record_data):
        self.hw.open_tag('script', id='record_data', type='application/json')
        self.hw.add(json.dumps(record_data))
        self.hw.close_tag()

    def write_flamegraph(self, flamegraph):
        self.hw.add(flamegraph)

    def write_script(self):
        self.hw.open_tag('script').add_file('report_html.js').close_tag()

    def finish(self):
        self.hw.close_tag('body')
        self.hw.close_tag('html')
        self.hw.close()


def gen_flamegraph(record_file, symfs):
    fd, flamegraph_path = tempfile.mkstemp()
    os.close(fd)
    inferno_script_path = os.path.join(get_script_dir(), 'inferno', 'inferno.py')
    subprocess.check_call([sys.executable, inferno_script_path, '-sc', '-o', flamegraph_path,
                           '--record_file', record_file, '--symfs', symfs, '--embedded_flamegraph', '--no_browser'])
    with open(flamegraph_path, 'r') as fh:
        data = fh.read()
    remove(flamegraph_path)
    return data


def main():
    parser = argparse.ArgumentParser(description='report profiling data')
    # Anki, optionally take symbol_cache directory from the command-line
    parser.add_argument('--symfs', help="""Set the path to find binaries with symbols and debug
                        info.""")
    parser.add_argument('-i', '--record_file', default='perf.data', help="""
                        Set profiling data file to report.""")
    parser.add_argument('-o', '--report_path', default='report.html', help="""
                        Set output html file.""")
    parser.add_argument('--min_func_percent', default=0.01, type=float, help="""
                        Set min percentage of functions shown in the report.
                        For example, when set to 0.01, only functions taking >= 0.01%% of total
                        event count are collected in the report.""")
    parser.add_argument('--min_callchain_percent', default=0.01, type=float, help="""
                        Set min percentage of callchains shown in the report.
                        It is used to limit nodes shown in the function flamegraph. For example,
                        when set to 0.01, only callchains taking >= 0.01%% of the event count of
                        the starting function are collected in the report.""")
    parser.add_argument('--no_browser', action='store_true', help="Don't open report in browser.")
    args = parser.parse_args()

    record_data = RecordData(args.record_file, args.symfs, args.min_func_percent, args.min_callchain_percent)

    report_generator = ReportGenerator(args.report_path)
    report_generator.write_content_div()
    report_generator.write_record_data(record_data.gen_record_info())
    report_generator.write_script()
    flamegraph = gen_flamegraph(args.record_file, args.symfs)
    report_generator.write_flamegraph(flamegraph)
    report_generator.finish()

    if not args.no_browser:
        open_report_in_browser(args.report_path)
    log_info("Report generated at '%s'." % args.report_path)


if __name__ == '__main__':
    main()
