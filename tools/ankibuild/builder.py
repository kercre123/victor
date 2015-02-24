#!/usr/bin/env python

import argparse

class Settings(object):
    def __init__(self):
        self.platform = 'unity'
        self.config = 'debug'
        self.build_dir = 'build'
        self.output_dir = None
        self.log_file = None
        self.basestation_path = None
        self.asset_path = None
        self.verbose = False
        self.debug = False

    def load_options(self, options):
        self.platform = options.platform
        self.config = options.config
        self.build_dir = options.build_dir
        self.output_dir = options.output_dir
        self.log_file = options.log_file
        self.basestation_path = options.with_basestation
        self.asset_path = options.with_assets
        self.verbose = options.verbose
        self.debug = options.debug

class DefaultHelpParser(argparse.ArgumentParser):
    def error(self, message):
        sys.stderr.write('error: %s\n' % message)
        self.print_help()
        sys.exit(2)

class Builder(object):
    def __init__(self, config=None):
        self.config = config

    def argument_parser(self):
        parser = DefaultHelpParser(add_help=False)

        parser.add_argument('--verbose', action="store_true", default=False)
        parser.add_argument('--debug', action="store_true", default=False)
        parser.add_argument('--build-dir', action="store")
        parser.add_argument('--output-dir', action="store")
        parser.add_argument('--log-file', action="store")
        parser.add_argument('--platform', action="store", choices=('ios', 'android', 'mac'))
        parser.add_argument('--config', action="store", choices=('debug', 'release'))
        parser.add_argument('--with-basestation', action="store",
                            help="path to basestation source repo")
        parser.add_argument('--with-assets', action="store",
                            help="path to assets repo")

        return parser

    def parse_arguments(self, parser=None, argv=[]):
        if parser == None:
            parser = self.argument_parser()
        return parser.parse_args(argv)
