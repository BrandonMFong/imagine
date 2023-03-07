/**
 * author: Brando
 * date: 2/23/23
 */

#include "tiff.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <bflibcpp/bflibcpp.hpp>

extern "C" {
#include <png.h>
#include <tiff.h>
}

using namespace BF;

bool Tiff::isType(const char * path) {
	char buf[10];
	int error = BFFileSystemPathGetExtension(path, buf);

	return (error == 0) && !strcmp(buf, "tif");
}

Tiff::Tiff(const char * path, int * err) : Image(path, err) {

}

Tiff::~Tiff() {
}

ImaginePixels Tiff::width() {
	uint32 w;
	TIFFGetField(this->_tiff, TIFFTAG_IMAGEWIDTH, &w);
	return w;
}

ImaginePixels Tiff::height() {
	uint32 h;
	TIFFGetField(this->_tiff, TIFFTAG_IMAGELENGTH, &h);
	return h;
}

int Tiff::bitsPerComponent() {
	uint32 b;
	TIFFGetField(this->_tiff, TIFFTAG_BITSPERSAMPLE, &b);
	return b;
}

ImagineColorSpace Tiff::colorspace() {
	return kImagineColorSpaceUnknown;
}

int Tiff::load() {
	int result = 0;
	TIFFHeaderCommon header;
	int fd = open(this->path(), O_RDONLY);

	if (fd == -1) {
		result = 1;
	} else if (read(fd, &header, sizeof(header)) == -1) {
		result = 2;
	} else {
		this->_version = header.tiff_version;
		this->_magNum = header.tiff_magic;
		close(fd);
	}

	if (result == 0) {
		this->_tiff = TIFFOpen(this->path(), "r");
		if (this->_tiff == NULL) {
			result = 3;
		}
	}

	return result;
}

int Tiff::unload() {
	TIFFClose(this->_tiff);
	return 0;
}

// https://www.loc.gov/preservation/digital/formats/content/tiff_tags.shtml
int Tiff::compileMetadata(Dictionary<String, String> * metadata) {
	int result = 0;
	char buf[0xff];

	sprintf(buf, "%d", this->width());
	metadata->setValueForKey("Width", buf);
	sprintf(buf, "%d", this->height());
	metadata->setValueForKey("Height", buf);
	sprintf(buf, "%d", this->bitsPerComponent());
	metadata->setValueForKey("Bits per component", buf);
	sprintf(buf, "%04x", this->_version);
	metadata->setValueForKey("Version", buf);
	sprintf(buf, "%04x", this->_magNum);
	metadata->setValueForKey("Magic Number", buf);

	

	return result;
}

ImageType Tiff::type() {
	return kImageTypeTIFF;
}

const char * Tiff::description() {
	return "Tiff";
}

