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

deb-cloudsmith: deb
	ls thread-supervisor-*-any.deb | xargs --no-run-if-empty -I {} cloudsmith push deb asherikov-aV7/thread_supervisor/any-distro/any-version {}

format:
	${FIND_SOURCES} | xargs ${CLANG_FORMAT} -verbose -i

