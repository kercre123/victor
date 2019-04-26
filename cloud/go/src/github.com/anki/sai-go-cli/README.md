sai-go-cli
======================================================================

A command line utility for interacting with the SAI API data and services. 
Like `curl` for Anki APIs, but more convenient.

You can download a pre-compiled version of this tool for your platform
by checking out the
[Confluence documentation](https://ankiinc.atlassian.net/wiki/display/SAI/Using+the+SAI+Command+Line+Tool)


Build
----------------------------------------------------------------------

Make the binary

`$ make restore-deps build`


Configuration
-------------------------------------------------------------------------------

The tool is configured by a `.saiconfig`
([example](.saiconfig-example)) file which specifies one or more named
environments. Each environment config specifics the app keys and
session keys used for various roles (admin, user, support, service),
as well as the URLs for each supported service.

If there is an environment named "dev" it will be used by
default. Other environments can be used by passing the `-e <name>`
argument.

The config file is found by searching several locations in order:


* If the `-f-` or `--file` command-line argument
* The `SAI_CLI_CONFIG` environment variable
* If no config file is found via the command line or environment
  variable, it will search for a config file using a Node.js style
  algorithm, walking up the directory tree, starting at the current
  directory, and terminating when we find either find a config file or
  reach `/`. This allows a config file to be bundled with related test
  configs, such as the
  [docker test stack](https://github.com/anki/sai-docker-test-stack).
* Finally, if the directory search didn't net a config file, check in
  `$HOME`.


Use
----------------------------------------------------------------------

For full instructions on using this tool, see the
[Confluence documentation](https://ankiinc.atlassian.net/wiki/display/SAI/Using+the+SAI+Command+Line+Tool)
