/**
 * author: Brando
 * date: 7/30/22
 */

#include "gif.hpp"
#include <cpplib.hpp>

extern "C" {
#include <string.h>
#include <stdio.h>
#include <math.h>
}

const char * const GIF_FILE_SIGNATURE = "GIF";
const int FILE_EXTENSION_ARRAY_SIZE = 1;
const char * const FILE_EXTENSION_ARRAY[] = {"gif"};

void GIF::imageDataFree(ImageData * obj) {
	Free(obj);
}

bool GIF::isType(const char * path) {
	char buf[10];
	if (!GetFileExtensionForPath(path, buf)) {
		for (int i = 0; i < FILE_EXTENSION_ARRAY_SIZE; i++) {
			if (!strcmp(buf, FILE_EXTENSION_ARRAY[i])) return true;
		}
	}

	return false;
}

const char * GIF::description() {
	return "GIF";
}

GIF::GIF(const char * path, int * err) : Image(path, err) {
	int error = err ? *err : 1;

	this->_header = {0};
	this->_colorTableGlobal = {0};
	this->_imageData.setObjectMemoryHandler(GIF::imageDataFree);

	if (err) *err = error;
}

GIF::~GIF() {

}

int GIF::load() {
	int result = 0;
	
	if ((this->_fileHandler = fopen(this->path(), "rb")) == NULL) {
		Error("Could not open file '%s' for reading", this->path());
		result = 1;
	}

	// Read the header
	if (result == 0) {
		size_t headerSize = sizeof(GIF::Header);
		size_t rsize = fread(&this->_header, 1, headerSize, this->_fileHandler);

		if (rsize != headerSize) {
			Error("Could not read the header properly, we read %d when we should have read %d\n", rsize, headerSize);
			result = 1;
		}
	}

	// Confirm that we are a gif file
	if (result == 0) {
		for (int i = 0; i < 3; i++) {
			if (this->_header.signature[i] != GIF_FILE_SIGNATURE[i]) {
				result = 1;
				break;
			}
		}
		
		if (result) {
			Error("Gif file signature should be '%s', but instead we have %c%c%c", 
					GIF_FILE_SIGNATURE, 
					this->_header.signature[0], 
					this->_header.signature[1], 
					this->_header.signature[2]);
		}
	}

	// Read the global color table
	if (result == 0) {
		if (this->_header.packedFields & 0x80) {
			int ctSize = pow(2, (this->_header.packedFields & 0x07) + 1);
			result = GIF::colorTableRead(this->_fileHandler, &this->_colorTableGlobal, ctSize);
		}
	}
	
	// Find all extensions load them into our structures
	if (result == 0) {
		result = this->readBlocks();
	}

	return result;
}

int GIF::colorTableRead(FILE * fs, ColorTable * table, size_t pixelSize) {
	int result = 0;
	
	if ((table->red = (unsigned char *) malloc(pixelSize)) == NULL) {
		Error("Allocating red");
		result = 1;
	} else if ((table->green = (unsigned char *) malloc(pixelSize)) == NULL) {
		Error("Allocating green");
		result = 2;
	} else if ((table->blue = (unsigned char *) malloc(pixelSize)) == NULL) {
		Error("Allocating blue");
		result = 3;
	} else {
		table->size = pixelSize;
		unsigned char buf[3];
		for (int i = 0; i < table->size; i++) {
			size_t rsize = fread(buf, 1, 3, fs);

			if (rsize != 3) {
				result = 1;
				Error("Did not read expected size for global color table");
				break;
			} else {
				table->red[i] = buf[0];
				table->green[i] = buf[1];
				table->blue[i] = buf[2];
			}
		}
	}

	return result;
}

// I am reading the image data incorrectly.  There may be multiple sub blocks after the image descriptor
int GIF::readImageData(List<ImageData *> * idList, FILE * fs, const ColorTable * colorTableGlobal) {
	int result = 0;
	bool done = false;
	ImageData * img = NULL;
	unsigned char idSep = 0x2c;
	size_t rsize = 0;

	do {
		img = (ImageData *) malloc(sizeof(ImageData));
		if (!img) result = 1;

		// Read descriptor
		else {
			size_t size = sizeof(ImageDescriptor);
			rsize = fread(&img->descriptor, 1, size, fs);

			if (size != rsize) {
				Error("Could not read img descriptor");
				result = 1;

			// Should I assume there are no more images
			// in the data stream if I find no more 
			// separators?
			} else if (img->descriptor.separator != idSep) {
				done = true;
				fseek(fs, -1 * size, SEEK_CUR);

				// Free unused memory
				Free(img);
			}
		}

		if (!done) {
			// Read color table
			// use global if none
			if (result == 0) {
				if (img->descriptor.packedFields & 0x80) {
					if ((img->colorTableLocal = (ColorTable *) malloc(sizeof(ColorTable))) == NULL) {
						result = 3;
					} else {
						int ctSize = pow(2, (img->descriptor.packedFields & 0x07) + 1);
						result = GIF::colorTableRead(fs, img->colorTableLocal, ctSize);
					}
				} else {
					img->colorTableLocal = (ColorTable *) colorTableGlobal;
				}

				if (result) {
					Error("Reading color table, %d", result);
				}
			}

			// Read the lzw data
			if (result == 0) {
				rsize = fread(&img->table.lzwMinimumCodeSize, 1, 1, fs);
				if (rsize != 1) {
					Error("Reading LZW alg data");
					result = 5;
				}
			}

			// Read the subblocks that follows the lzw code size
			if (result == 0) {
				result = GIF::readSubBlockSequence(fs, &img->table.data);
			}

			if (result == 0) {
				result = idList->add(img);
			}

			done = result;
		}
	} while (!done && !result && 0);

	return result;
}

int GIF::readSubBlockSequence(FILE * fs, DataBlock * dataSequence) {
	int result = 0;
	DataBlock subBlock = {0};
	size_t rsize = 0;
	const unsigned char blockTerm = 0x00;

	// Make sure it is initialized
	dataSequence->size = 0;
	dataSequence->buf = 0;

	do {
		// Read next subblock
		result = GIF::readSubBlocks(fs, &subBlock);
		
		if (result) {
			Error("Reading image data: %d", result);
		} 
		
		// see if we have a block terminator
		if (result == 0) {
			char t;
			rsize = fread(&t, 1, 1, fs);
			if (rsize != 1) {
				Error("Could not read");
				result = 8;
			} else if (t == blockTerm) {
				// End sequence
				break; // Get out of loop
			}
		}

		// If we are still here then we have more work to do
		if (result == 0) {
			fseek(fs, -1, SEEK_CUR); // reset reader
			size_t offset = dataSequence->size;
			dataSequence->size += subBlock.size;
			if ((dataSequence->buf = (unsigned char *) realloc(dataSequence->buf, dataSequence->size)) == NULL) {
				result = 10;
				Error("Could not get more bytes for buffer");
			} else {
				if (memcpy(dataSequence->buf + offset, subBlock.buf, subBlock.size) == NULL) {
					result = 11;
					Error("Could not copy subblock data");
				}
			}
		}
	} while (!result);

	return result;
}

int GIF::readSubBlocks(FILE * fs, DataBlock * subBlock) {
	int result = 0;
	size_t rsize = 0;

	// Get the subblock size
	if (result == 0) {
		rsize = fread(&subBlock->size, 1, 1, fs);
		
		if (rsize != 1) {
			Error("Reading sub block size");
			result = 6;
		}
	}

	// Read buffered data
	if (result == 0) {
		size_t size = subBlock->size;
		if (size > 0) {
			if ((subBlock->buf = (unsigned char *) malloc(size)) == NULL) {
				Error("Allocating memory size %d\n", size);
				result = 7;
			} else {
				rsize = fread(subBlock->buf, 1, size, fs);

				if (rsize != size) {
					Error("Could not read raw data of size %d (%x)", size, size);
					Error("Read size for data was %d", rsize);
					result = 8;
				}
			}
		} else {
			subBlock->buf = NULL; // ensuring the block is null
		}
	}

	return result;
}

int GIF::readBlocks() {
	int result = 0;
	unsigned char buf;
	size_t rsize = 0;
	size_t size = 0;
	bool done = false;
	const unsigned char extIntro = 0x21;
	const unsigned char trailer = 0x3B;
	const unsigned char idSep = 0x2c;

	do {
		if (!fread(&buf, 1, 1, this->_fileHandler)) {
			done = true;
		} else if (buf == trailer) {
			done = true; // Reached end of gif file
		} else {
			// Image
			if (buf == idSep) {
				fseek(this->_fileHandler, -1, SEEK_CUR); // TODO: get rid of the separator in image struct
				result = GIF::readImageData(&this->_imageData, this->_fileHandler, &this->_colorTableGlobal);

			// Extensions
			} else if (buf == extIntro) {
				if (fread(&buf, 1, 1, this->_fileHandler) != 1) {
					result = 1;
					Error("Reading the label error");
				} else {
					// Graphics
					if (buf == this->_extGraphics.label) {
						size = 6;
						rsize = fread(&this->_extGraphics.blockSize, 1, size, this->_fileHandler);

						if (rsize != size) {
							result = 1;
						} else if (this->_extGraphics.term != 0x00) {
							result = 1;
							Error("Terminator is incorrect: 0x%x", this->_extGraphics.term);
						}

					// Comments
					} else if (buf == this->_extComments.label) {
						result = GIF::readSubBlocks(this->_fileHandler, &this->_extComments.subBlock);

						if (result) {
							Error("Sub block error: %d", result);
						} else if (!fread(&this->_extComments.term, 1, 1, this->_fileHandler)) {
							result = 8;
							Error("Terminator data could not be read");
						} else if (this->_extComments.term != 0x00) {
							result = 9;
							Error("Terminator is incorrect: 0x%x", this->_extComments.term);
						}
					
					// Plain text
					} else if (buf == this->_extPlainText.label) {
						ExtensionPlainText * pt = &this->_extPlainText;

						rsize = fread(&pt->blockSize, 1, 1, this->_fileHandler);
						
						if (pt->blockSize != 12) {
							Error("Block size is: %d", pt->blockSize);
							result = 20;
						} else {
							size = sizeof(ExtensionPlainText::TextGrid) 
								+ sizeof(ExtensionPlainText::CharacterCell) 
								+ sizeof(ExtensionPlainText::TextColorIndex);

							// Read the next structs, stop before the sub block
							if (fread(&pt->textGrid, 1, size, this->_fileHandler) != size) {
								result = 21;

							// Read plain text data
							} else if (result = GIF::readSubBlocks(this->_fileHandler, &pt->data)) {
								Error("Reading subblocks: %d", result);

							// Check terminator
							} else if (!fread(&pt->term, 1, 1, this->_fileHandler)) {
								result = 24;
							} else if (pt->term != 0x00) {
								result = 25;
							}
						}

					// Application
					} else if (buf == this->_extApplication.label) {
						size = 12;
						rsize = fread(&this->_extApplication.blockSize, 1, size, this->_fileHandler);
						if (rsize != size) {
							result = 26;
							Error("Could not read %d bytes", size);
						} else if (this->_extApplication.blockSize != 11) {
							result = 27;
							Error("Block size is %d\n", this->_extApplication.blockSize);
						} else if (result = GIF::readSubBlocks(this->_fileHandler, &this->_extApplication.data)) {
							Error("Sub block: %d\n", result);
							result = 30;
						} else if (!fread(&this->_extApplication.term, 1, 1, this->_fileHandler)) {
							result = 28;
						} else if (this->_extApplication.term != 0x00) {
							Error("Error with terminator");
							result = 29;
						}
					}
				}
			}
		}
	} while (!done && !result);

	return result;
}

int GIF::unload() {
	if (this->_fileHandler) fclose(this->_fileHandler);

	Free(this->_colorTableGlobal.red);
	Free(this->_colorTableGlobal.green);
	Free(this->_colorTableGlobal.blue);

	return 0;
}

const char * GIF::version() {
	sprintf(this->_gifReserved, "%c%c%c", 
			this->_header.version[0],
			this->_header.version[1],
			this->_header.version[2]);

	return (const char *) this->_gifReserved;
}

ImaginePixels GIF::width() {
	return (this->_header.width[1] << 8) | this->_header.width[0];
}

ImaginePixels GIF::height() {
	return (this->_header.height[1] << 8) | this->_header.height[0];
}

int GIF::bitsPerComponent() {
	unsigned char colorResolution = (this->_header.packedFields >> 4) & 0x07;
	return colorResolution + 1;
}

ImagineColorSpace GIF::colorspace() {
	return kImagineColorSpaceUnknown;
}

ImageType GIF::type() {
	return kImageTypeGIF;
}

int GIF::toGIF() {
	Error("'%s' is already a gif file!", this->path());
	return 1;
}

int GIF::compileMetadata(Dictionary<String, String> * metadata) {
	int result = 0;
	char buf[0xff];

	sprintf(buf, "%d", this->width());
	metadata->setValueForKey("Width", buf);
	sprintf(buf, "%d", this->height());
	metadata->setValueForKey("Height", buf);
	metadata->setValueForKey("Version", this->version());
	strncpy(buf, this->_extApplication.id, 8);
	metadata->setValueForKey("App ID", buf);
	strncpy(buf, (char *) this->_extApplication.authCode, 3);
	metadata->setValueForKey("Auth Code", buf);
	sprintf(buf, "%d", this->_imageData.count());
	metadata->setValueForKey("Image Count", buf);

	return result;
}

