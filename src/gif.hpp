/**
 * author: Brando
 * date: 7/30/22
 */

#ifndef GIF_HPP
#define GIF_HPP

#include "image.hpp"
#include "list.hpp"

/**
 *
 * docs:
 * 	https://www.w3.org/Graphics/GIF/spec-gif89a.txt
 */
class GIF : public Image {

// Types
private:

	/**
	 * Custom struct to hold the header data we read from 
	 * the gif file
	 */
	typedef struct {
		// Core header
		unsigned char signature[3]; // Should be 'GIF'
		unsigned char version[3];

		// Logical Screen Descriptor fields
		unsigned char width[2];
		unsigned char height[2];

		/**
		 * 	Global Color Table Flag 		1 Bit
		 * 	Color Resolution              	3 Bits
		 * 	Sort Flag                     	1 Bit
		 * 	Size of Global Color Table    	3 Bits
		 */
		unsigned char packedFields;
		unsigned char backgroundColorIndex;
		unsigned char aspectRatio;
	} Header;

	/**
	 * Each member is an array of size size
	 */
	typedef struct {
		unsigned char * red;
		unsigned char * green;
		unsigned char * blue;
		short size;
	} ColorTable;

	typedef struct {
		unsigned char separator;
		unsigned char leftPosition[2];
		unsigned char topPosition[2];
		unsigned char width[2];
		unsigned char height[2];
		
		/**
		 * Local Color Table Flag        1 Bit
		 * Interlace Flag                1 Bit
		 * Sort Flag                     1 Bit
		 * Reserved                      2 Bits
		 * Size of Local Color Table     3 Bits
		 */
		unsigned char packedFields;
	} ImageDescriptor;

	/**
	 * Holds sub block data or sub block sequence data
	 */
	typedef struct {
		size_t size = 0;
		unsigned char * buf = 0;
	} DataBlock;

	typedef struct {
		unsigned char lzwMinimumCodeSize;
		DataBlock data;
	} ImageTableData;

	/**
	 * this is how image data is organized
	 */
	typedef struct {
		ImageDescriptor descriptor;
		ColorTable * colorTableLocal;
		ImageTableData table;
	} ImageData;

	// The following extension structs purposely do not hold extension introducers
	// 
	// I set each extension's term to 0xff for error handling. If I read the 
	// file correctly, the term should be overridden with 0x00
	
	struct ExtensionGraphicControl {
		unsigned char label = 0xF9;
		unsigned char blockSize;
		unsigned char packedField;
		unsigned char delayTime[2];
		unsigned char transparentColorIndex;
		unsigned char term = 0xFF;
	} _extGraphics;

	struct ExtensionComment {
		unsigned char label = 0xFE;
		DataBlock subBlock;
		unsigned char term = 0xFF;
	} _extComments;

	struct ExtensionPlainText {
		unsigned char label = 0x01;
		unsigned char blockSize;

		struct TextGrid {
			unsigned short leftPosition;
			unsigned short topPosition;
			unsigned short width;
			unsigned short height;
		} textGrid;

		struct CharacterCell {
			unsigned char width;
			unsigned char height;
		} characterCell;

		struct TextColorIndex {
			unsigned char foreground;
			unsigned char background;
		} textColorIndex;

		DataBlock data;
		unsigned char term = 0xFF;

	} _extPlainText;

	struct ExtensionApplication {
		unsigned char label = 0xFF;
		unsigned char blockSize;
		char id[8];
		unsigned char authCode[3];
		DataBlock data;
		unsigned char term = 0xFF;
	} _extApplication;

public:
	static bool isType(const char * path);
	GIF(const char * path, int * err);
	virtual ~GIF();

	/**
	 * Returns the 3 bytes versioning as a null-terminated string
	 */
	const char * version();

private:
	/**
	 * Reads the file given by stream fs at the point it is at until pixelSize into table
	 */
	static int colorTableRead(FILE * fs, ColorTable * table, size_t pixelSize);

	/**
	 * Finds all the image data from fs' current stream position and adds them to idList
	 */	
	static int readImageData(List<ImageData *> * idList, FILE * fs, const ColorTable * colorTableGlobal);

	int readBlocks();

	/**
	 * Assumes the file position is where the block size is
	 */
	static int readSubBlocks(FILE * fs, DataBlock * subBlock);

	/**
	 * Reads a sequence of subblocks into 
	 */
	static int readSubBlockSequence(FILE * fs, DataBlock * blockSequence);

	/**
	 * Holds the header data from gif file
	 *
	 * initially read when load() is called
	 */
	Header _header;

	/**
	 * Holds the global color table if present
	 */
	ColorTable _colorTableGlobal;

	char _gifReserved[4];

	/**
	 * Handles memory for our _imageData list
	 */	
	static void imageDataFree(ImageData * obj);

	/**
	 * Holds all image data
	 */
	List<ImageData *> _imageData;

// required 
public:
	int details();
	ImaginePixels width();
	ImaginePixels height();
	int bitsPerComponent();
	ImagineColorSpace colorspace();
	int load();
	int unload();
	ImageType type();
	int toGIF();
	const char * description();
};

#endif // GIF_HPP

