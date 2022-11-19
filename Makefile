BUILD_DIR=build
APT_INSTALL=sudo apt install -y --no-install-recommends
FIND_SOURCES=find ./test/ ./include/ -iname "*.h" -or -iname "*.cpp"

install_ubuntu_test_deps:
	${APT_INSTALL} cmake libboost-all-dev
	sudo pip3 install scspell3k

test: clean
	${MAKE} spell
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; cmake -Dthread_supervisor_TESTS=ON ..
	cd ${BUILD_DIR}; ${MAKE}
	cd ${BUILD_DIR}; ${MAKE} test

clean:
	rm -Rf ${BUILD_DIR}

doxclean:
	cd doc/gh-pages; git fetch --all; git checkout gh-pages; git pull
	rm -Rf ./doc/gh-pages/doxygen

dox: clean #doxclean clean
	cd doc; doxygen

spell_interactive:
	${MAKE} spell SPELL_XARGS_ARG=-o

# https://github.com/myint/scspell
spell:
	${FIND_SOURCES} \
		| xargs ${SPELL_XARGS_ARG} scspell --use-builtin-base-dict --override-dictionary ./qa/scspell.dict


.PHONY: build test
