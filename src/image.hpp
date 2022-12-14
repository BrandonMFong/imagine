
#ifndef IMAGE_H
#define IMAGE_H

#include "imagetypes.h"
#include "file.hpp"
#include <dictionary.hpp>
#include <string.hpp>

extern "C" {
#include <filesystem.h>
}

typedef long ImaginePixels;

typedef enum {
	kImagineColorSpaceUnknown = 0,
	kImagineColorSpaceRGB = 1,
	kImagineColorSpaceRGBA = 2,
} ImagineColorSpace;

class Image : public File {
public:
	/**
	 * Creates an image object 
	 *
	 * This function will determine what dervied class
	 * will be created to support the input image
	 */
	static Image * createImage(const char * path, int * err);
	virtual ~Image();

	/** Details
	 * 	Prints the standard details.  Override if you 
	 * 	would like to print more
	 */
	virtual int details();

	// Tells the Image object to convert to a specific
	// type of image
	int convertToType(ImageType type);
	int convertToType(ImageType type, const char * path);

	// Image dimensions in pixels
	virtual ImaginePixels width() = 0;
	virtual ImaginePixels height() = 0;
	virtual int bitsPerComponent() = 0;

	// Color space of the pixels
	virtual ImagineColorSpace colorspace() = 0;

	// Processing
	virtual int load() = 0;
	virtual int unload() = 0;

	/**
	 * Requires derived classes to compile its own metadata
	 *
	 * param `metadata`: output dictionary that will have metadata
	 */
	virtual int compileMetadata(Dictionary<String, String> * metadata) = 0;

protected:
	Image(const char * path, int * err);

	// Returns type represented as an enum value
	virtual ImageType type() = 0;
	
	/**
	 * Derived classes should return a brief description of the image type
	 *
	 * Example would be spelling out the acronym
	 */
	virtual const char * description() = 0;
	
	// Specific conversions
	virtual int toPNG();
	virtual int toJPEG();
	virtual int toGIF();

	// Prints the color space type as a string
	const char * colorspaceString();

	/**
	 * Will return the path we will write to when 
	 * we are converting our image type
	 *
	 * The lifetime of the result string will be until
	 * the next time you call this function
	 */
	const char * conversionOutputPath();

private:

	/** 
	 * Used when converting image to a type
	 *
	 * This value will be set to the custom path passed 
	 * when the convertToType() function is invoked. This 
	 * variable will be reset after the function is finished.
	 */
	char _imageReserved[PATH_MAX];
};

#endif

