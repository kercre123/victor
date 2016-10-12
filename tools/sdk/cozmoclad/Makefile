.PHONY: copy-clad license sdist wheel dist

copy-clad:
	rm -rf src/cozmoclad/clad/*
	rm -rf src/cozmoclad/msgbuffers/*
	cp -r ../../../generated/cladPython/clad src/cozmoclad/
	cp -r ../../message-buffers/support/python/msgbuffers src/cozmoclad/
	# Add license header file to each python file
	# skip msgbuffers as it already has the license in the header.
	find src/cozmoclad/clad \
		-name '*.py' \
		-exec bash -c 'cat LICENSE-header.txt {} > {}.tmp' ';' \
		-exec mv -f '{}.tmp' '{}' ';'


license_targets = src/cozmoclad/LICENSE.txt

license: $(license_targets)

$(license_targets): LICENSE.txt
	for fn in $(license_targets); do \
		cp LICENSE.txt $$fn; \
	done

sdist: license
	python3 setup.py sdist

wheel: license
	python3 setup.py bdist_wheel

dist: sdist wheel
