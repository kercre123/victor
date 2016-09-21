.PHONY: copy-clad license sdist wheel examples dist

copy-clad:
	rm -rf src/cozmo/_internal/clad/*
	rm -rf src/cozmo/_internal/msgbuffers/*
	cp -r ../../generated/cladPython/clad src/cozmo/_internal/
	cp -r ../message-buffers/support/python/msgbuffers src/cozmo/_internal/


license_targets = src/cozmo/LICENSE.txt examples/LICENSE.txt

license: $(license_targets)

$(license_targets): LICENSE.txt
	for fn in $(license_targets); do \
		cp LICENSE.txt $$fn; \
	done

sdist:
	python3 setup.py sdist

wheel:
	python3 setup.py bdist_wheel

dist/sdk-examples.zip: license examples/*.py
	rm -f dist/sdk-examples.zip
	zip -r --exclude='*.git*' dist/sdk-examples.zip examples

dist/sdk-examples.tar.gz: license examples/*.py
	tar -cvzf dist/sdk-examples.tar.gz --exclude '*.git*' examples

examples: dist/sdk-examples.tar.gz dist/sdk-examples.zip

dist/vagrant-bundle.tar.gz:
	cd dist ; tar -cvf vagrant-bundle.tar sdk-examples.tar.gz cozmo*.tar.gz
	cd ../
	cd vagrant ; tar -rvf ../dist/vagrant-bundle.tar Vagrantfile setup-vm.sh
	cd ../
	gzip dist/vagrant-bundle.tar

dist/vagrant-bundle.zip:
	zip -rj dist/vagrant-bundle.zip dist/sdk-examples.tar.gz dist/cozmo*.tar.gz
	zip -urj dist/vagrant-bundle.zip vagrant/Vagrantfile vagrant/setup-vm.sh

vagrant: dist/vagrant-bundle.tar.gz dist/vagrant-bundle.zip

dist: sdist wheel examples vagrant
