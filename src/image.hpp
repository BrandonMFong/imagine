
#ifndef IMAGE_H
#define IMAGE_H

#include "imagetypes.h"
#include "file.hpp"

class Image : public File {
public:
	static Image * createImage(const char * path, int * err);

protected:
	Image(const char * path, int * err);
	virtual ~Image();

	virtual ImageType type() = 0;
};

#endif

