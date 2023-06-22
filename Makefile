.PHONY: clean
clean:
	rm -rf build/
	rm -rf datasus.egg-info/
	rm -rf dist

.PHONY: build
build:
	python3 setup.py build

install: build
	python3 setup.py install