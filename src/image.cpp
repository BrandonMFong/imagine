/**
 * author: Brando
 * date: 7/23/22
 */

#include "image.hpp"
#include "png.hpp"
#include "jpeg.hpp"
#include "gif.hpp"
#include <cpplib.hpp>

extern "C" {
#include <string.h>
#include <libgen.h>
}

Image * Image::createImage(const char * path, int * err) {
	Image * result = 0;
	int error = 0;

	if (PNG::isType(path)) {
		result = new PNG(path, &error);
	} else if (JPEG::isType(path)) {
		result = new JPEG(path, &error);
	} else if (GIF::isType(path)) {
		result = new GIF(path, &error);
	} else {
		Error("Unsupported file type for path '%s'", path);
		error = 1;
	}

	if (err) *err = error;
	
	return result;
}

Image::Image(const char * path, int * err) : File(path, err) {
	int error = err ? *err : 1;

	if (err) *err = error;
}

Image::~Image() {

}

int Image::details() {
	printf("Width: %d\n", this->width());
	printf("Height: %d\n", this->height());
	printf("Color space: %s\n", this->colorspaceString());
	printf("Bits per component: %d\n", this->bitsPerComponent());

	return 0;
}

int Image::convertToType(ImageType type) {
	switch (type) {
		case kImageTypePNG:
			return this->toPNG();
		case kImageTypeJPEG:
			return this->toJPEG();
		case kImageTypeGIF:
			return this->toGIF();
		default:
			Error("Unknown type: %d", type);
			return 1;
	}
}

int Image::toPNG() {
	Error("Cannot convert '%s' image to PNG", this->description());
	return 1;
}

int Image::toJPEG() {
	Error("Cannot convert '%s' image to JPEG", this->description());
	return 1;
}

int Image::toGIF() {
	Error("Cannot convert '%s' image to GIF", this->description());
	return 1;
}

int Image::convertToType(ImageType type, const char * path) {
	// Saves the path to our reserved buffer
	if (path) {
		if (!realpath(path, this->_imageReserved)) {
			Error("Error with finding real path for %s", path);
			strcpy(this->_imageReserved, "");
		}
	} else {
		strcpy(this->_imageReserved, "");
	}
	
	return this->convertToType(type);
}

const char * Image::colorspaceString() {
	switch (this->colorspace()) {
		case kImagineColorSpaceRGB:
			return "RGB";
		case kImagineColorSpaceRGBA:
			return "RGBA";
		default:
			return "<unknown color space>";
	}
}

const char * Image::conversionOutputPath() {
	// If we don't have a string, then we will 
	// return our directory
	if (!strlen(this->_imageReserved)) {
		strcpy(this->_imageReserved, this->directory());
		return this->_imageReserved;
	} else {
		return this->_imageReserved;
	}
}

