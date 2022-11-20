FIND_SOURCES=find ./test/ ./include/ -iname "*.h" -or -iname "*.cpp"

install_ubuntu_test_deps: install_deps_common
	${APT_INSTALL} libboost-all-dev

test: clean
	${MAKE} cppcheck
	${MAKE} spell
	${MAKE} unit_tests OPTIONS=test

clean: clean_common

dox: doxclean clean
	cd doc; doxygen

format:
	${FIND_SOURCES} | xargs ${CLANG_FORMAT} -verbose -i

