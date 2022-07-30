# author: Brando
# date: 7/29/22
#

CPPFLAGS += -I. -Iexternal/libs/bin/ -Iexternal
CFLAGS += -I. -Iexternal/libs/bin/ -Iexternal
BIN_NAME = imagine
BUILD_PATH = build/release
MAIN_FILE = src/main.cpp
LIBRARIES = external/libs/bin/cpplib.a
all: setup sources main

debug: CPPFLAGS += -DDEBUG -g
debug: CFLAGS += -DDEBUG -g
debug: BIN_NAME = debug-imagine
debug: BUILD_PATH = build/debug
debug: LIBRARIES = external/libs/bin/debug-cpplib.a
debug: setup sources main

test: CPPFLAGS += -DTESTING -g -Isrc/ -Iexternal/libs/cpplib/testbench
test: CFLAGS += -DTESTING -g -Isrc/ -Iexternal/libs/clib/testbench
test: BIN_NAME = test-imagine
test: BUILD_PATH = build/test
test: MAIN_FILE = src/testbench/tests.cpp
test: setup sources main
	./bin/$(BIN_NAME)

clean:
	rm -rfv build
	rm -rfv bin

setup:
	mkdir -p $(BUILD_PATH)
	mkdir -p bin

sources:
	g++ -c src/image.cpp -o $(BUILD_PATH)/image.o $(CPPFLAGS)
	g++ -c src/png.cpp -o $(BUILD_PATH)/png.o $(CPPFLAGS)
	g++ -c src/jpeg.cpp -o $(BUILD_PATH)/jpeg.o $(CPPFLAGS)
	g++ -c src/appdriver.cpp -o $(BUILD_PATH)/appdriver.o $(CPPFLAGS)

main:
	g++ -o bin/$(BIN_NAME) $(MAIN_FILE) $(BUILD_PATH)/appdriver.o $(BUILD_PATH)/image.o $(BUILD_PATH)/png.o $(BUILD_PATH)/jpeg.o $(LIBRARIES) -lpng -ljpeg $(CPPFLAGS)

