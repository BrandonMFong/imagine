
CPPFLAGS += -I. -Ilibs/bin/

all: setup sources main

clean:
	rm -rfv build
	rm -rfv bin

setup:
	mkdir -p build
	mkdir -p bin

sources:
	g++ -c src/file.cpp -o build/file.o $(CPPFLAGS)
	g++ -c src/image.cpp -o build/image.o $(CPPFLAGS)
	g++ -c src/png.cpp -o build/png.o $(CPPFLAGS)

main:
	g++ -o bin/imagine src/main.cpp build/file.o build/image.o build/png.o libs/bin/clib.o $(CPPFLAGS)

