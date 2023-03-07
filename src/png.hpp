
#ifndef PNG_HPP
#define PNG_HPP

#include "imagetypes.h"
#include "image.hpp"

/**
 * Dependencies: libpng-dev
 * References: http://www.libpng.org/pub/png/libpng-1.4.0-manual.pdf
 *
 */
class PNG : public Image {
public:
	static bool isType(const char * path);
	PNG(const char * path, int * err);
	virtual ~PNG();
	ImageType type();
	int toPNG();
	int toJPEG();
	ImaginePixels width();
	ImaginePixels height();
	int load();
	int unload();
	ImagineColorSpace colorspace();
	int bitsPerComponent();
	const char * description();
	int compileMetadata(BF::Dictionary<BF::String, BF::String> * metadata);

private:
	void * _pngStruct;
	void * _pngInfo;
	char * _xmpBuf;
};

#endif

