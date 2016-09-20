.PHONY: copy-clad license sdist wheel examples dist

copy-clad:
	rm -rf src/cozmo/_internal/clad
	rm -rf src/cozmo/_internal/msgbuffers
	cp -r ../../generated/cladPython/clad src/cozmo/_internal/
	cp -r ../message-buffers/support/python/msgbuffers src/cozmo/_internal/


license_targets = src/cozmo/LICENSE.txt examples/LICENSE.txt

license: $(license_targets)

$(license_targets): LICENSE.txt
	for fn in $(license_targets); do \
		cp LICENSE.txt $$fn; \
	done

sdist:
	python setup.py sdist

wheel:
	python setup.py bdist_wheel

dist/sdk-examples.zip: license examples/*.py
	rm -f dist/sdk-examples.zip
	zip -r dist/sdk-examples.zip examples/*.py examples/*.txt

dist/sdk-examples.tar.gz: license examples/*.py
	tar -cvzf dist/sdk-examples.tar.gz examples/*.py examples/*.txt

examples: dist/sdk-examples.tar.gz dist/sdk-examples.zip

dist: sdist wheel examples
