
#include "file.hpp"
#include <cpplib.hpp>
#include <stdlib.h>

File::File(const char * path, int * err) {
	int error = err ? *err : 1;
	this->_path = CopyString(path, &error);

	if (err) *err = error;
}

File::~File() {
	free(this->_path);
}

void File::read() {

}

void File::write() {

}

