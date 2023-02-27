/**
 * author: Brando
 * date: 7/27/22
 *
 * Example: https://github.com/LuaDist/libjpeg/blob/master/example.c
 */

#include "jpeg.hpp"
#include <cpplib.hpp>

extern "C" {
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <jpeglib.h>
#include <png.h>
}

const int FILE_EXTENSION_ARRAY_SIZE = 3;
const char * const FILE_EXTENSION_ARRAY[] = {"jpeg", "jpg", "JPG"};

bool JPEG::isType(const char * path) {
	char buf[10];
	int error = GetFileExtensionForPath(path, buf);

	if (error == 0) {
		for (int i = 0; i < FILE_EXTENSION_ARRAY_SIZE; i++) {
			const char * ext = FILE_EXTENSION_ARRAY[i];

			if (!strcmp(buf, ext)) return true;
		}
	}

	return false;
}

const char * JPEG::description() {
	return "JPEG";
}

int JPEG::imagineColorSpaceToJPEGColorSpace(ImagineColorSpace cs) {
	switch (cs) {
		case kImagineColorSpaceRGB:
			return JCS_RGB;
		case kImagineColorSpaceRGBA:
			return JCS_EXT_RGBA;
		default:
			return JCS_UNKNOWN;
	}
}

JPEG::JPEG(const char * path, int * err) : Image(path, err) {
	int error = err ? *err : 1;

	this->_decompressionInfo = NULL;

	if (err) *err = error;
}

JPEG::~JPEG() {

}

ImaginePixels JPEG::width() {
	if (this->_decompressionInfo) {
		return ((struct jpeg_decompress_struct *) this->_decompressionInfo)->output_width;
	} else return 0;
}

ImaginePixels JPEG::height() {
	if (this->_decompressionInfo) {
		return ((struct jpeg_decompress_struct *) this->_decompressionInfo)->output_height;
	} else return 0;
}

int JPEG::bitsPerComponent() {
	if (this->_decompressionInfo) {
		return ((struct jpeg_decompress_struct *) this->_decompressionInfo)->data_precision;
	} else return 0;
}

ImagineColorSpace JPEG::colorspace() {
	if (this->_decompressionInfo) {
		switch (((struct jpeg_decompress_struct *) this->_decompressionInfo)->out_color_space) {
			case JCS_RGB:
			case JCS_EXT_RGB:
				return kImagineColorSpaceRGB;
			default:
				return kImagineColorSpaceUnknown;
		}
	}

	return kImagineColorSpaceUnknown;
}

void JPEGErrorExit(j_common_ptr cinfo) {
	printf("Error");
}

void JPEGErrorMessage(j_common_ptr cinfo, int msg_level) {

}

int JPEG::load() {
	int result = 0;
	struct jpeg_decompress_struct * cinfo = NULL;
	JSAMPARRAY buffer = NULL;
	int row_stride;
	struct jpeg_error_mgr pub;

	// Open the file
	if ((this->_fileHandler = fopen(this->path(), "rb")) == NULL) {
		Error("can't open %s\n", this->path());
		result = 1;
	}

	// Get memory for the jpeg structure 
	if (result == 0) {
		cinfo = (struct jpeg_decompress_struct *) malloc(sizeof(struct jpeg_decompress_struct));
		result = cinfo != NULL ? 0 : 2;
	}

	// Init the reading of the jpeg file with reading the header first
	if (result == 0) {
		cinfo->err = jpeg_std_error(&pub);
		cinfo->err->error_exit = JPEGErrorExit;
		cinfo->err->emit_message = JPEGErrorMessage;
		jpeg_create_decompress(cinfo);
		jpeg_stdio_src(cinfo, this->_fileHandler);
		if (jpeg_read_header(cinfo, true) != JPEG_HEADER_OK) {
			result = 3;
			Error("Error reading header");
		}
	}

	// Get all the image data
	if (result == 0) {
		cinfo->out_color_space = JCS_RGB;
		if (!jpeg_start_decompress(cinfo)) {
			result = 4;
			Error("Error decompressing");
		}
	}

	// Save the decompressed data
	if (result == 0) {
		this->_decompressionInfo = cinfo;
	} else {
		Error("Error loading image '%s': %d", this->path(), result);
		Free(cinfo);
	}

	return result;
}

int JPEG::unload() {
	if (this->_decompressionInfo) {
		int result = 0;
		struct jpeg_decompress_struct * cinfo = (struct jpeg_decompress_struct *) this->_decompressionInfo;

		// Idk if this will cause any memory leaks but I can't have this running unless
		// we get a segmentation fault
		//jpeg_finish_decompress(cinfo);
		
		jpeg_destroy_decompress(cinfo);

		// Close the file
		if (this->_fileHandler) fclose(this->_fileHandler);

		free(this->_decompressionInfo);
		this->_decompressionInfo = NULL;

		return result;
	} else {
		return 2;
	}
}

/*
int JPEG::details() {
	return this->Image::details();
}
*/

int JPEG::toPNG() {
	int result = 0;
	struct jpeg_decompress_struct * cinfo = NULL;
	int width = this->width(), height = this->height();
	png_byte color_type = PNG_COLOR_TYPE_RGB;
	png_byte bit_depth = this->bitsPerComponent();
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	int number_of_passes = 0;
	char file_name[PATH_MAX];
	JSAMPARRAY buffer = NULL; // char **

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

	if (result == 0) {
		cinfo = (struct jpeg_decompress_struct *) this->_decompressionInfo;
		int row_stride = cinfo->output_width * cinfo->output_components;
		buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr) cinfo, JPOOL_IMAGE, row_stride, 1);

		if (!buffer) {
			Error("Could not create buffer");
			result = 7;
		}
	}

	if (result == 0) {
		while(cinfo->output_scanline < cinfo->output_height) {
			jpeg_read_scanlines(cinfo, buffer, 1);
			png_write_row(png_ptr, buffer[0]);
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			Error("[write_png_file] Error during end of write");
			result = 8;
		}

		png_write_end(png_ptr, NULL);
	}

	if (pngFile) fclose(pngFile);

	return result;
}

int JPEG::toJPEG() {
	Error("File '%s' is already a jpeg file", this->path());
	return 1;
}

ImageType JPEG::type() {
	return kImageTypeJPEG;
}

int JPEG::compileMetadata(Dictionary<String, String> * metadata) {
	//printf("Width: %d\n", this->width());
	//printf("Height: %d\n", this->height());
	//printf("Color space: %s\n", this->colorspaceString());
	//printf("Bits per component: %d\n", this->bitsPerComponent());
	
	char buf[0xff];

	sprintf(buf, "%d", this->width());
	metadata->setValueForKey("Width", buf);

	sprintf(buf, "%d", this->height());
	metadata->setValueForKey("Height", buf);

	sprintf(buf, "%s", this->colorspaceString());
	metadata->setValueForKey("Color space", buf);

	sprintf(buf, "%d", this->bitsPerComponent());
	metadata->setValueForKey("Bits per component", buf);

	return 0;
}

