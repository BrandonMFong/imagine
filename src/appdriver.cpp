/**
 * author: brando
 * date: 7/22/22
 */

#include "appdriver.hpp"
#include <string.h>
#include "image.hpp"
#include <bflibcpp/bflibcpp.hpp>
#include <libgen.h>

using namespace BF;

// Shared instance varaible
AppDriver * APP_DRIVER = 0;

// Main commands
const char * const DETAILS_COMMAND = "details";
const char * const AS_COMMAND = "as";

// Conversion argument types
const char * const PNG_TYPE_ARG = "png";
const char * const JPEG_TYPE_ARG = "jpeg";
const char * const GIF_TYPE_ARG = "gif";

// Sub commands
const char * const OUTPUT_ARG = "-o";

void AppDriver::help() {
	printf("usage: %s <path> <commands>\n", basename((char *) this->_args->objectAtIndex(0)));

	printf("\n");

	// Commands
	printf("Commands:\n");
	printf("\t%s: Prints details for input file\n", DETAILS_COMMAND);
	printf("\t%s <type> [ %s <output> ]: Converts image to <type>\n", AS_COMMAND, OUTPUT_ARG);

	printf("\n");
}

AppDriver::AppDriver(int argc, char * argv[], int * err) {
	int error = err ? *err : 1;
	this->_args = new Array<const char *>((const char **) argv, argc);

	if (this->_args == 0) {
		error = 1;
	} else {
		this->_args->setComparator(strcmp);
	}
	
	if (err) *err = error;

	APP_DRIVER = this;
}

AppDriver::~AppDriver() {
	delete this->_args;
	APP_DRIVER = 0;
}

const Array<const char *> * AppDriver::args() {
	return this->_args;
}

int AppDriver::run() {
	int result = 0;
	Image * img = 0;
	
	if (this->_args->count() == 1) {
		this->help();
		result = 1;
	} else {
		const char * path = this->_args->objectAtIndex(1);
		img = Image::createImage(path, &result);
	}

	if (result == 0) {
		if (this->_args->contains((char *) AS_COMMAND)) {
			result = this->handleAsCommand(img);
		} else if (this->_args->contains((char *) DETAILS_COMMAND)) {
			result = this->handleDetailsCommand(img);
		} else {
			BFErrorPrint("No known commands");
			result = 1;
		}
	}

	if (img) delete img;

	return result;
}

AppDriver * AppDriver::shared() {
	return APP_DRIVER;
}

int AppDriver::handleAsCommand(Image * img) {
	int result = 0;
	const char * arg = NULL;
	int index = this->_args->indexForObject((char *) AS_COMMAND);
	ImageType type  = kImageTypeUnknown;
	const char * outputPath = NULL;

	if ((arg = this->_args->objectAtIndex(index+1)) == NULL) {
		BFErrorPrint("Could not get arg at index %d", index+1);
		result = 1;
	}

	if (result == 0) {
		if (!strcmp(PNG_TYPE_ARG, arg)) {
			type = kImageTypePNG;
		} else if (!strcmp(JPEG_TYPE_ARG, arg)) {
			type = kImageTypeJPEG;
		} else if (!strcmp(GIF_TYPE_ARG, arg)) {
			type = kImageTypeGIF;
		} else {
			result = 3;
			BFErrorPrint("Unknown type '%s'", arg);
		}
	}

	if (result == 0) {
		if (this->_args->contains((char *) OUTPUT_ARG)) {
			index = this->_args->indexForObject((char *) OUTPUT_ARG);

			if ((outputPath = this->_args->objectAtIndex(index+1)) == NULL) {
				BFErrorPrint("Could not get arg at index %d", index+1);
				result = 4;
			}
		}
	}	

	if (result == 0) {
		if (result = img->load()) {
			BFErrorPrint("loading: %d", result);
		} else if (result = img->convertToType(type, outputPath)) {
			BFErrorPrint("converting to type %d: %d", type, result);
		} else if (result = img->unload()) {
			BFErrorPrint("unloading: %d", result);
		}
	}

	return result;
}

int AppDriver::handleDetailsCommand(Image * img) {
	int result = 0;

	if (result = img->load()) {
		return result;
	} else if (result = img->details()) {
		return result;
	} else if (result = img->unload()) {
		return result;
	}

	return 0;
}

