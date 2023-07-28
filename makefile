# author: Brando
# date: 7/29/22
#

include external/libs/makefiles/libpaths.mk

### Global
BUILD_PATH = build
FILES = appdriver image png jpeg gif tiff tiff2png
CXXLINKS = -lpng -ljpeg -ltiff -luuid

### Release settings
R_CXXFLAGS += -I. -Iexternal/libs/$(BF_LIB_RPATH_RELEASE) -Iexternal
R_BIN_NAME = imagine
R_BUILD_PATH = $(BUILD_PATH)/release
R_MAIN_FILE = src/main.cpp
R_LIBRARIES = external/libs/$(BF_LIB_RPATH_RELEASE_CPP)
R_OBJECTS = $(patsubst %, $(R_BUILD_PATH)/%.o, $(FILES))

### Debug settings
D_CXXFLAGS = $(R_CXXFLAGS) -DDEBUG -g
D_BIN_NAME = debug-imagine
D_BUILD_PATH = $(BUILD_PATH)/debug
D_MAIN_FILE = $(R_MAIN_FILE)
D_LIBRARIES = external/libs/$(BF_LIB_RPATH_DEBUG_CPP)
D_OBJECTS = $(patsubst %, $(D_BUILD_PATH)/%.o, $(FILES))

### Test settings
T_CXXFLAGS = $(R_CXXFLAGS) -g -DTESTING -Isrc/ -Iexternal/libs/bflibcpp/testbench/
T_BIN_NAME = test-imagine
T_BUILD_PATH = $(BUILD_PATH)/test
T_MAIN_FILE = src/testbench/tests.cpp
T_LIBRARIES = external/libs/$(BF_LIB_RPATH_DEBUG_CPP)
T_OBJECTS = $(patsubst %, $(T_BUILD_PATH)/%.o, $(FILES))

### Instructions

# Default
all: release

clean:
	rm -rfv build
	rm -rfv bin

update-dependencies:
	cd ./external/libs && git pull && make

## Release build instructions
release: release-setup bin/$(R_BIN_NAME)

release-setup:
	@mkdir -p $(R_BUILD_PATH)
	@mkdir -p bin

bin/$(R_BIN_NAME): $(R_MAIN_FILE) $(R_OBJECTS) $(R_LIBRARIES)
	g++ -o $@ $^ $(CXXLINKS) $(R_CXXFLAGS)

$(R_BUILD_PATH)/%.o: src/%.cpp src/%.hpp
	g++ -c $< -o $@ $(R_CXXFLAGS)

## Debug build instructions
debug: debug-setup bin/$(D_BIN_NAME)

debug-setup:
	@mkdir -p $(D_BUILD_PATH)
	@mkdir -p bin

bin/$(D_BIN_NAME): $(D_MAIN_FILE) $(D_OBJECTS) $(D_LIBRARIES)
	g++ -o $@ $^ $(CXXLINKS) $(D_CXXFLAGS)

$(D_BUILD_PATH)/%.o: src/%.cpp src/%.hpp
	g++ -c $< -o $@ $(D_CXXFLAGS)

## Test build instructions
test: test-setup bin/$(T_BIN_NAME)
	./bin/$(T_BIN_NAME)

test-setup:
	@mkdir -p $(T_BUILD_PATH)
	@mkdir -p bin

bin/$(T_BIN_NAME): $(T_MAIN_FILE) $(T_OBJECTS) $(T_LIBRARIES)
	g++ -o $@ $^ $(CXXLINKS) $(T_CXXFLAGS)

$(T_BUILD_PATH)/%.o: src/%.cpp src/%.hpp
	g++ -c $< -o $@ $(T_CXXFLAGS)

