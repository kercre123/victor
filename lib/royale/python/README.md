Royale Python Samples
=====================

To use the Python samples you have to install numpy and matplotlib,
because they are used for the visualization of the data.

Linux installation
------------------

On Ubuntu and Debian, install python3-matplotlib and its dependencies.

You also need the libpython3.x package for the same minor version of python3.x that the roypy
library was built for. In Debian and Ubuntu the libpython3.5, libpython3.6 and libpython3.7
libraries are all co-installable. For example, if running a sample fails with:

    ImportError: libpython3.5m.so.1.0: cannot open shared object file: No such file or directory

then the libpython3.5 package is required.


Windows installation
--------------------

There are several different python packages for Windows. The Royale
bindings work with the packages from www.python.org (e.g. version
3.6.5). The architecture (e.g. 32 bit vs. 64 bit) needs to match that
of the Royale installation.

If you are using cygwin on Windows, be advised that the python package
which can be installed as part of cygwin does not work with the Royale
bindings. Please make sure that the correct python binary is in the
path (e.g. by running ``which python``).

For a Windows install with python from python.org, you will need the
following additional packages:
- numpy
- matplotlib
- pywin32 (contains the pythoncom packages needed by royale)

These can be installed using "pip install &lt;package&gt;". Note that there
are issues when trying to set up virtual environments (described
below) when using the python.org python in a cygwin environment (it
doesn't seem to handle cygwin paths correctly and it produces shell
scripts (e.g. activate) which are unsuitable for the cygwin bash).


Virtual environments
--------------------

If you can't (or don't want to) modify your system python
installation, you may be able to make a local installation of numpy
and matplotlib in a virtual environment. For example, to install in
`~/bin/virtual_env_for_python`:

```
mkdir ~/bin/virtual_env_for_python
python -m venv ~/bin/virtual_env_for_python
. ~/bin/virtual_env_for_python/Scripts/activate
pip install numpy
pip install matplotlib
```

Then in any shell that you run the samples in, start with

```
. ~/bin/virtual_env_for_python/Scripts/activate
```
