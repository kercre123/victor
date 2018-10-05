#!/usr/bin/env python3

"""
Invoke the configure.py script using dev or beta credentials
"""

import argparse
import importlib
import sys

conf = importlib.import_module('vector-sdk.configure')

class InternalApi(conf.Api):
    def __init__(self, env):
        self._env = env
        if env == "prod":
            sys.exit("Please use 'vector_sdk/configure.py' for production")
        elif env == "beta":
            self._handler = conf.ApiHandler(
                headers={'Anki-App-Key': 'va3guoqueiN7aedashailo'},
                url='https://accounts-beta.api.anki.com/1/sessions'
            )
        elif env == "dev":
            self._handler = conf.ApiHandler(
                headers={'Anki-App-Key': 'eiqu8ae6aesae4vi7ooYi7'},
                url='https://accounts-dev2.api.anki.com/1/sessions'
            )
        else:
            sys.exit("Invalid env '{}', please choose 'dev' or 'beta'".format(env))

    @property
    def name(self):
        return "Anki Cloud ({})".format(self._env)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-e", "--env", help="Accounts api to use [dev,beta]")
    args = parser.parse_args()
    conf.main(InternalApi(args.env if args.env is not None else 'dev'))
