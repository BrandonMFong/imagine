
#ifndef PNG_H
#define PNG_H

#include "imagetypes.h"
#include "image.hpp"

class PNG : public Image {
public:
	PNG(const char * path, int * err);
	virtual ~PNG();
	ImageType type();
};

#endif

