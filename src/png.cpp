/**
 * author: Brando
 * date: 7/22/22
 */

#include "png.hpp"
#include <bflibcpp/bflibcpp.hpp>
#include <rapidxml/rapidxml.hpp>
#include "jpeg.hpp"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <jpeglib.h>
}

using namespace BF;
using namespace rapidxml;

bool PNG::isType(const char * path) {
	char buf[10];
	int error = BFFileSystemPathGetExtension(path, buf);
	
	bool result = false;

	if (error == 0) {
		result = !strcmp(buf, "png");
	}

	return result;
}

const char * PNG::description() {
	return "PNG";
}

PNG::PNG(const char * path, int * err) : Image(path, err) {
	this->_pngStruct = NULL;
	this->_pngInfo = NULL;
	this->_xmpBuf = NULL;
}

PNG::~PNG() {

}

ImageType PNG::type() {
	return kImageTypePNG;
}

ImaginePixels PNG::width() {
	if (this->_pngStruct && this->_pngInfo)
		return png_get_image_width((png_structp) this->_pngStruct, (png_infop) this->_pngInfo);
	else return 0;
}

ImaginePixels PNG::height() {
	if (this->_pngStruct && this->_pngInfo)
		return png_get_image_height((png_structp) this->_pngStruct, (png_infop) this->_pngInfo);
	else return 0;
}

int PNG::bitsPerComponent() {
	return -1;
}

ImagineColorSpace PNG::colorspace() {
	if (this->_pngStruct && this->_pngInfo) {
		switch (png_get_color_type((png_structp) this->_pngStruct, (png_infop) this->_pngInfo)) {
			case PNG_COLOR_TYPE_RGBA:
				return kImagineColorSpaceRGBA;
			case PNG_COLOR_TYPE_RGB:
				return kImagineColorSpaceRGB;
			default:
				kImagineColorSpaceUnknown;
		}
	}
	
	return kImagineColorSpaceUnknown;
}

int PNG::toPNG() {
	BFErrorPrint("Path %s is already a PNG file!", this->path());
	return 1;
}

int ColorComponentCount(ImagineColorSpace cs) {
	switch (cs) {
		case kImagineColorSpaceRGBA:
			return 4;
			break;
		case kImagineColorSpaceRGB:
		default:
			return 3;
			break;
	}
}

int PNG::toJPEG() {
	int result = 0;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile = NULL;		/* target file */
	char filename[PATH_MAX];
	ImagineColorSpace imagineColorSpace = this->colorspace();
	png_bytep * row_pointers = NULL;
	
	strcpy(filename, this->conversionOutputPath());
	sprintf(filename, "%s/%s.png", filename, this->name());

	if ((outfile = fopen(filename, "wb")) == NULL) {
		BFErrorPrint("Could not open file %s", filename);
		result = 1;
	}

	if (result == 0) {
		cinfo.err = jpeg_std_error(&jerr);

		// Init compression tools
		jpeg_create_compress(&cinfo);

		// Init output file
		jpeg_stdio_dest(&cinfo, outfile);

		// Params
		cinfo.image_width = this->width(); 	/* image width and height, in pixels */
		cinfo.image_height = this->height();

		/* # of color components per pixel */
		cinfo.input_components = ColorComponentCount(imagineColorSpace);

		/* colorspace of input image */
		cinfo.in_color_space = (J_COLOR_SPACE) JPEG::imagineColorSpaceToJPEGColorSpace(imagineColorSpace);
		jpeg_set_defaults(&cinfo);

		// Start the compression
		jpeg_start_compress(&cinfo, TRUE);
	
		/* read file */
        png_read_update_info((png_structp) this->_pngStruct, (png_infop) this->_pngInfo);
        if (setjmp(png_jmpbuf((png_structp) this->_pngStruct))) BFErrorPrint("error with png_jmpbuf");

		int height = png_get_image_height((png_structp) this->_pngStruct, (png_infop) this->_pngInfo);
        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (int y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes((png_structp) this->_pngStruct, (png_infop) this->_pngInfo));

        png_read_image((png_structp) this->_pngStruct, row_pointers);

		// Write row by row
		while (cinfo.next_scanline < cinfo.image_height) {
			png_bytep pbyte = row_pointers[cinfo.next_scanline];
			(void) jpeg_write_scanlines(&cinfo, &pbyte, 1);
		}

		// Close everything
		jpeg_finish_compress(&cinfo);
		fclose(outfile);
		jpeg_destroy_compress(&cinfo);
		free(row_pointers);
	}

	return result;
}

int PNG::load() {
	int result = 0;
	int width = 0, height = 0;
	png_structp png = 0;
	png_infop info = 0;
	char * xmpData = NULL;

	this->_fileHandler = fopen(this->path(), "rb");
	if (!this->_fileHandler) result = 1;

	if (result == 0) {
		png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(!png) result = 1;
	}

	if (result == 0) {
		info = png_create_info_struct(png);
		if(!info) result = 2;
	}

	if (result == 0) {
		if(setjmp(png_jmpbuf(png))) result = 3;
	}

	if (result == 0) {
		png_init_io(png, this->_fileHandler);

		png_read_info(png, info);

		png_textp text = 0;
		png_uint_32 count = png_get_text(png, info, &text, NULL);
		for (int i = 0; i < count; i++) {
			if (!strcmp(text->key, "XML:com.adobe.xmp")) {
				xmpData = BFStringCopyString(text->text, &result);
			}

			if (result) break;
		}
	}

	if (result == 0) {
		this->_pngStruct = png;
		this->_pngInfo = info;
		this->_xmpBuf = xmpData;
	}
	
	return result;
}

int PNG::unload() {
	if (this->_pngStruct && this->_pngInfo) {
		png_destroy_read_struct(
			(png_structp *) &this->_pngStruct,
			(png_infop *) &this->_pngInfo,
			NULL
		);

		this->_pngStruct = NULL;
		this->_pngInfo = NULL;
	}

	if (this->_xmpBuf) {
		free(this->_xmpBuf);
		this->_xmpBuf = NULL;
	}

	if (this->_fileHandler) fclose(this->_fileHandler);

	return 0;
}

/*
int PNG::details() {
	int result = this->Image::details();
	
	if (result == 0) {
		if (this->_xmpBuf) {
			printf("XMP Data:\n");
			result = PNGPrintXMPData(this->_xmpBuf);
		} else {
			result = 1;
		}
	}

	return result;
}
*/

/**
 * Prints the xmpData xml buffer using rapidxml
 */
int PNGLoadXMPData(char * xmpData, Dictionary<String, String> * metadata) {
	int result = 0;
	xml_document<> doc;
	doc.parse<0>(xmpData);
	xml_node<> * node = NULL, * valNode = NULL;

	if (!(node = doc.first_node("x:xmpmeta"))) {
		result = 1;
	} else if (!(node = node->first_node("rdf:RDF"))) {
		result = 2;
	} else if (!(node = node->first_node("rdf:Description"))) {
		result = 3;
	} else {
		if (valNode = node->first_node("exif:GPSLongitude")) {
			printf("\tLongitude: %s\n", valNode->value());
			metadata->setValueForKey("Longitude", valNode->value());
		}

		if (valNode = node->first_node("exif:GPSLatitude")) {
			printf("\tLatidude: %s\n", valNode->value());
			metadata->setValueForKey("Latitude", valNode->value());
		}

		if (valNode = node->first_node("exifEX:LensModel")) {
			printf("\tLens model: %s\n", valNode->value());
			metadata->setValueForKey("Lens Model", valNode->value());
		}
	}

	return result;
}

int PNG::compileMetadata(Dictionary<String, String> * metadata) {
	int result = 0;
	char buf[0xff];

	sprintf(buf, "%d", this->width());
	metadata->setValueForKey("Width", buf);
	sprintf(buf, "%d", this->height());
	metadata->setValueForKey("Height", buf);

	if (this->_xmpBuf) {
		result = PNGLoadXMPData(this->_xmpBuf, metadata);
	}

	return result;
}

