
default all simple big tree clean:
	@echo ""
	@echo "C $@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	$(MAKE) -C c $@
	@echo ""
	@echo "C# $@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	$(MAKE) -C csharp $@
	@echo ""
	@echo "PYTHON $@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	$(MAKE) -C python $@
	@echo ""
	@echo "C++ $@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	$(MAKE) -C cpp $@
	@echo ""
	@echo "DONE $@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo ""

help:
	@echo ""
	@echo "CLAD Tests"
	@echo "Available targets (for all platforms): default all simple big tree clean"
	@echo ""
	@echo "simple: a quick test to make sure most things are working OK"
	@echo "big: similar to simple, but with multiple compilers and valgrind, so it takes a little longer"
	@echo "tree: test out the inclusion tree commands"
	@echo ""
	@echo "default (no need to specify): both simple and tree tests"
	@echo "all: run all tests"
	@echo "clean: delete all output--you may need to clean between tests to discover certain problems or regressions"
	@echo ""
	@echo "To use python 3, just add PYTHON=python3 (or the path to the python 3 binary) to the end of your command."
	@echo ""
	@echo "You may also run these commands in a specific platform directory to only run tests on that platform."
	@echo ""
