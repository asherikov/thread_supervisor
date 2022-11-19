BUILD_ROOT=build
BUILD_DIR=${BUILD_ROOT}/${OPTIONS}
APT_INSTALL=sudo apt install -y --no-install-recommends
FIND_SOURCES=find ./test/ ./include/ -iname "*.h" -or -iname "*.cpp"
ROOT_DIR=../../

install_ubuntu_test_deps:
	${APT_INSTALL} cmake libboost-all-dev
	sudo pip3 install scspell3k

install_deb_deps:
	sudo gem install fpm

install_cloudsmith_deps: install_deb_deps
	sudo pip install --upgrade cloudsmith-cli


cmake:
	mkdir -p ${BUILD_DIR};
	cd ${BUILD_DIR}; cmake -C ${ROOT_DIR}/cmake/options_${OPTIONS}.cmake ${ROOT_DIR}
	cd ${BUILD_DIR}; ${MAKE} ${MAKE_FLAGS}

install:
	cd ${BUILD_DIR}; ${MAKE} install


test: clean
	${MAKE} spell
	${MAKE} cmake OPTIONS=test
	cd ${BUILD_DIR}; ${MAKE} test

clean:
	rm -Rf ${BUILD_DIR}

doxclean:
	cd doc/gh-pages; git fetch --all; git checkout gh-pages; git pull
	rm -Rf ./doc/gh-pages/doxygen

dox: doxclean clean
	cd doc; doxygen

spell_interactive:
	${MAKE} spell SPELL_XARGS_ARG=-o

# https://github.com/myint/scspell
spell:
	${FIND_SOURCES} \
		| xargs ${SPELL_XARGS_ARG} scspell --use-builtin-base-dict --override-dictionary ./qa/scspell.dict

deb:
	${MAKE} cmake OPTIONS=deb
	${MAKE} install OPTIONS=deb
	grep "<version>" package.xml | grep -o "[0-9]*\.[0-9]*\.[0-9]*" > build/version
	fpm -t deb --version `cat build/version` --package thread_supervisor-`cat build/version`-any.deb


deb-cloudsmith: deb
	ls thread-supervisor-*-any.deb | xargs --no-run-if-empty -I {} cloudsmith push deb asherikov-aV7/thread_supervisor/any-distro/any-version {}


.PHONY: build test cmake
