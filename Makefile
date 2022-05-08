BUILD_DIR=build
APT_INSTALL=sudo apt install -y --no-install-recommends

install_ubuntu_test_deps:
	${APT_INSTALL} cmake libboost-all-dev

test: clean
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; cmake -Dthread_supervisor_TESTS=ON ..
	cd ${BUILD_DIR}; ${MAKE}
	cd ${BUILD_DIR}; ${MAKE} test

clean:
	rm -Rf ${BUILD_DIR}

.PHONY: build test
