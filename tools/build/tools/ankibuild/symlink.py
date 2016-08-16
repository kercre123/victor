#!/usr/bin/python
import os
import os.path
import sys
import argparse
import logging

class CreateSymlink(object):
    def __init__(self):
        self.log = logging.getLogger('anki.createSymlink')
        stdout_handler = logging.StreamHandler()
        formatter = logging.Formatter('%(name)s - %(message)s')
        stdout_handler.setFormatter(formatter)
        self.log.addHandler(stdout_handler)
        self.options = None


    def getOptions(self,scriptArgs):
        version = '1.0'
        parser = argparse.ArgumentParser(
            description='creates symlink only if link_name does not already point to link_target',
            version=version)
        parser.add_argument('--debug', '--verbose', '-d', dest='verbose', action='store_true',
                            help='prints debug output')
        parser.add_argument('--link_name', dest='linkName', required=True,
                            action='store', default=None, help='link name to create')
        parser.add_argument('--link_target', dest='linkTarget', required=True,
                            action='store', default=None, help='target path of the link')
        parser.add_argument('--create_folder', dest='createLinkFolder',
                            action='store', default=None, help='folder to create before creating a link')
        (self.options, args) = parser.parse_known_args(scriptArgs)

    def run(self, scriptArgs):
        self.getOptions(scriptArgs)

        if self.options.verbose:
            self.log.setLevel(logging.DEBUG)

        # for now always run in debug
        self.log.setLevel(logging.DEBUG)

        # fix link target
        self.options.linkTarget = os.path.abspath(self.options.linkTarget)

        # fix create link folder target
        if self.options.createLinkFolder:
            self.options.createLinkFolder = os.path.abspath(self.options.createLinkFolder)
            try:
                os.makedirs(self.options.createLinkFolder)
            except OSError as e:
                if not os.path.isdir(self.options.createLinkFolder):
                    self.log.error('Failed to recursively create dir to {0}: {1}'.format(
                        self.options.createLinkFolder,
                        e.strerror))
                    return False


        #fix link name
        if self.options.linkName.endswith('/'):
            self.options.linkName = self.options.linkName + self.options.linkTarget.split('/')[-1]
        self.options.linkName = os.path.abspath(self.options.linkName)

        return self.creatSymlnk()

    def creatSymlnk(self):

        if os.path.islink(self.options.linkName):
            # we have an existing symlink that MAY need to be replaced
            existingLink = os.readlink(self.options.linkName)
            self.log.info("We currently have %s -> %s" % (self.options.linkName, existingLink))
            if existingLink == self.options.linkTarget:
                self.log.info("Symlink is already setup correctly")
                return True
            self.log.info("Removing existing symlink")
            try:
                os.unlink(self.options.linkName)
            except OSError:
                self.log.error("removing existing symlink failed")
                return False
            os.symlink(self.options.linkTarget, self.options.linkName)
            return True
        else:
            # if link name exists return false
            if os.path.exists(self.options.linkName):
                self.log.error("could not create symlink, file found at %s" % (self.options.linkName))
                return False

            # if target does nto exist, return false
            if not os.path.exists(self.options.linkTarget):
                self.log.error("could not create symlink, target not found %s" % (self.options.linkTarget))
                return False

            # if link name does not exist make link
            os.symlink(self.options.linkTarget, self.options.linkName)
            return True


if __name__ == '__main__':

    createSymlink = CreateSymlink()
    args = sys.argv

    if createSymlink.run(args):
        sys.exit(0)
    else:
        sys.exit(1)
