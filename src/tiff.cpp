/**
 * author: Brando
 * date: 2/23/23
 */

#include "tiff.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cpplib.hpp>

extern "C" {
#include <png.h>
}

bool Tiff::isType(const char * path) {
	char buf[10];
	int error = GetFileExtensionForPath(path, buf);

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

int Tiff::toPNG() {
	int result = 0;
	int width = this->width(), height = this->height();
	png_byte color_type = PNG_COLOR_TYPE_RGB;
	png_byte bit_depth = this->bitsPerComponent();
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	int number_of_passes = 0;
	char file_name[PATH_MAX];
	uint32_t imagelength = 0;
	tdata_t buf = 0;
	uint32_t row = 0;

	sprintf(file_name, "%s/%s.png", this->conversionOutputPath(), this->name());

	FILE * pngFile = fopen(file_name, "wb");
	if (!pngFile) {
		Error("[write_png_file] File %s could not be opened for writing", file_name);
		result = 1;
	}

	if (result == 0) {
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

		if (!png_ptr) {
			Error("Could not create png struct");
			result = 2;
		}
	}

	if (result == 0) {	
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			Error("[write_png_file] png_create_info_struct failed");
			result = 3;
		}
	}

	if (result == 0) {
		if (setjmp(png_jmpbuf(png_ptr))) {
			Error("[write_png_file] Error during init_io");
			result = 4;
		}
	}

	if (result == 0) {
		png_init_io(png_ptr, pngFile);

		if (setjmp(png_jmpbuf(png_ptr))) {
			Error("[write_png_file] Error during writing header");
			result = 5;
		}
	}

	if (result == 0) {
		png_set_IHDR(
			png_ptr,
			info_ptr,
			width,
			height,
			bit_depth,
			color_type,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE
		);

		png_write_info(png_ptr, info_ptr);

		if (setjmp(png_jmpbuf(png_ptr))) {
			Error("[write_png_file] Error during writing bytes");
			result = 6;
		}
	}
	
	TIFFGetField(this->_tiff, TIFFTAG_IMAGELENGTH, &imagelength);
	if (result == 0) {
		buf = _TIFFmalloc(TIFFScanlineSize(this->_tiff));
		if (!buf) {
			Error("Could not create buffer");
			result = 7;
		}
	}

	if (result == 0) {
		for (row = 0; row < imagelength; row++) {
			TIFFReadScanline(this->_tiff, buf, row);
			png_write_row(png_ptr, (png_const_bytep) buf);
		}
		_TIFFfree(buf);

		if (setjmp(png_jmpbuf(png_ptr))) {
			Error("[write_png_file] Error during end of write");
			result = 8;
		}

		png_write_end(png_ptr, NULL);
	}

	if (pngFile) fclose(pngFile);

	return 0;
}

