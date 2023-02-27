/**
 * author: Brando
 * date: 7/27/22
 */

#ifndef JPEG_HPP
#define JPEG_HPP

#include "image.hpp"

class JPEG : public Image {
public:
	/**
	 * compares the input's file extension with 
	 * what we are supporting
	 */
	static bool isType(const char * path);

	/**
	 * Returns an value from the J_COLOR_SPACE enums
	 *
	 * Caller must cast appropriately
	 */
	static int imagineColorSpaceToJPEGColorSpace(ImagineColorSpace cs);

	JPEG(const char * path, int * err);
	virtual ~JPEG();

	//int details();
	int toPNG();
	int toJPEG();
	ImageType type();
	ImaginePixels width();
	ImaginePixels height();
	int load();
	int unload();
	ImagineColorSpace colorspace();
	int bitsPerComponent();
	const char * description();
	int compileMetadata(Dictionary<String, String> * metadata);

private:
	// Holds the jpeg decompressed data
	void * _decompressionInfo;
};

#endif

