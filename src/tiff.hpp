
#ifndef TIFF_HPP
#define TIFF_HPP

#include "imagetypes.h"
#include "image.hpp"
#include <tiffio.h>
#include "accessorspecifiers.hpp"

/**
 *  Good documentation here: https://github.com/vadz/libtiff/tree/master/libtiff
 */
class Tiff : public Image {
PUBLIC:
	static bool isType(const char * path);
	Tiff(const char * path, int * err);
	virtual ~Tiff();

	// Required
	ImaginePixels width();
	ImaginePixels height();
	int bitsPerComponent();
	ImagineColorSpace colorspace();
	int load();
	int unload();
	int compileMetadata(Dictionary<String, String> * metadata);
	ImageType type();
	const char * description();

	// Conversions
	int toPNG();

PRIVATE:

	/**
	 * Holds the tiff object
	 */
	TIFF * _tiff;

	/// Tiff version
	uint16_t _version;

	/// Tiff magic number
	uint16_t _magNum;
};

#endif // TIFF_HPP

