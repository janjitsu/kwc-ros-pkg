all: build

TARBALL = build/gmapping_r39.tar.gz
TARBALL_URL = http://pr.willowgarage.com/downloads/gmapping_r39.tar.gz
SOURCE_DIR = build/gmapping_export
UNPACK_CMD = tar xzf
include $(shell rospack find mk)/download_unpack_build.mk

PATCH = gmapping-r39.patch

build: wiped install

configured: $(SOURCE_DIR) Makefile
	cd $(SOURCE_DIR) && patch -p0 < ../../$(PATCH) && ./configure
	touch configured

wiped: Makefile $(PATCH)
	make wipe
	touch wiped

.PHONY: gmapping
gmapping: configured
	cd $(SOURCE_DIR) && make

# Poor-man's install step
install: gmapping
	mkdir -p lib
	cp $(SOURCE_DIR)/lib/* lib
	mkdir -p include/gmapping/sensor/sensor_range
	mkdir -p include/gmapping/sensor/sensor_odometry
	mkdir -p include/gmapping/sensor/sensor_base
	mkdir -p include/gmapping/gridfastslam
	mkdir -p include/gmapping/utils
	mkdir -p include/gmapping/particlefilter
	mkdir -p include/gmapping/scanmatcher
	mkdir -p include/gmapping/log
	mkdir -p include/gmapping/grid
	cp $(SOURCE_DIR)/sensor/sensor_range/*.h include/gmapping/sensor/sensor_range
	cp $(SOURCE_DIR)/sensor/sensor_odometry/*.h include/gmapping/sensor/sensor_odometry
	cp $(SOURCE_DIR)/sensor/sensor_base/*.h include/gmapping/sensor/sensor_base
	cp $(SOURCE_DIR)/gridfastslam/*.h $(SOURCE_DIR)/gridfastslam/*.hxx include/gmapping/gridfastslam
	cp $(SOURCE_DIR)/utils/*.h include/gmapping/utils
	cp $(SOURCE_DIR)/particlefilter/*.h include/gmapping/particlefilter
	cp $(SOURCE_DIR)/scanmatcher/*.h include/gmapping/scanmatcher
	cp $(SOURCE_DIR)/log/*.h include/gmapping/log
	cp $(SOURCE_DIR)/grid/*.h include/gmapping/grid

clean:
	-cd $(SOURCE_DIR) && make clean
	rm -rf include lib configured
wipe: clean
	rm -rf build

