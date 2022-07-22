

#include "png.hpp"

PNG::PNG(const char * path, int * err) : Image(path, err) {

}

PNG::~PNG() {

}

ImageType PNG::type() {
	return kImageTypePNG;
}

