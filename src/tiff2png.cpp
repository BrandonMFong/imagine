/**
 * author: Brando
 * date: 2/28/23
 */

#include "tiff.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <bflibcpp/bflibcpp.hpp>

extern "C" {
#include <png.h>
#include <tiff.h>
}

int tiff2png (TIFF * tif, const char *tiffname, char *pngname, int verbose, int force,
              int interlace_type, int png_compression_level, int invert,
              int faxpect_option,
              double gamma);

int Tiff::toPNG() {
	char filename[PATH_MAX];
	
	strcpy(filename, this->conversionOutputPath());
	sprintf(filename, "%s/%s.png", filename, this->name());

	return tiff2png(this->_tiff, this->path(), filename, 0, 0, PNG_INTERLACE_NONE, -1, 0, 0, -1);
}

/// These are sources I got from tiff2png

#ifdef _MSC_VER   /* works for MSVC 5.0; need finer tuning? */
#  define strcasecmp _stricmp
#endif

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NONE
#  define NONE 0
#endif

#define MAXCOLORS 256

#ifndef PHOTOMETRIC_DEPTH
#  define PHOTOMETRIC_DEPTH 32768
#endif

#define DIR_SEP '/'		/* SJT: Unix-specific */

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

typedef struct _jmpbuf_wrapper {
  jmp_buf jmpbuf;
} jmpbuf_wrapper;

static jmpbuf_wrapper tiff2png_jmpbuf_struct;


/* macros to get and put bits out of the bytes */

#define GET_LINE_SAMPLE \
  { \
    if (bitsleft == 0) \
    { \
      p_line++; \
      bitsleft = 8; \
    } \
    bitsleft -= (bps >= 8) ? 8 : bps; \
    sample = (*p_line >> bitsleft) & maxval; \
    if (invert) \
      sample = ~sample & maxval; \
  }
#define GET_STRIP_SAMPLE \
  { \
    if (getbitsleft == 0) \
    { \
      p_strip++; \
      getbitsleft = 8; \
    } \
    getbitsleft -= (bps >= 8) ? 8 : bps; \
    sample = (*p_strip >> getbitsleft) & maxval; \
    if (invert) \
      sample = ~sample & maxval; \
  }
#define PUT_LINE_SAMPLE \
  { \
    if (putbitsleft == 0) \
    { \
      p_line++; \
      putbitsleft = 8; \
    } \
    putbitsleft -= (bps >= 8) ? 8 : bps; \
    if (invert) \
      sample = ~sample; \
    *p_line |= ((sample & maxval) << putbitsleft); \
  }

void tiff2png_error_handler (png_structp png_ptr, png_const_charp msg) {
	jmpbuf_wrapper  *jmpbuf_ptr;
	BFDLog("tiff2png:  fatal libpng error: %s\n", msg);
	longjmp (jmpbuf_ptr->jmpbuf, 1);
}

int tiff2png(
	TIFF * tif,
	const char * tiffname,
	char * pngname,
	int verbose,
	int force,
	int interlace_type,
	int png_compression_level,
	int _invert,
	int faxpect_option,
	double gamma
) {
	int result = 0;
	ush bps, spp, planar;
	ush photometric, tiff_compression_method;
	int bigendian;
	int maxval;
	static int colors = 0;
	static int halfcols = 0;
	int cols, rows;
	int row;
	register int col;
	static uch *tiffstrip = NULL;
	uch *tiffline;

	size_t stripsz;
	static size_t tilesz = 0L;
	uch *tifftile; /* FAP 20020610 - Add variables to support tiled images */
	ush tiled;
	uint32 tile_width, tile_height;   /* typedef'd in tiff.h */
	static int num_tilesX = 0;

	register uch *p_strip, *p_line;
	register uch sample;
#ifdef INVERT_MINISWHITE
	int sample16;
#endif
	register int bitsleft;
	register int getbitsleft;
	register int putbitsleft;
	float xres, yres, ratio;
#ifdef GRR_16BIT_DEBUG
	uch msb_max, lsb_max;
	uch msb_min, lsb_min;
	int s16_max, s16_min;
#endif

	static FILE *png = NULL;				/* PNG */
	png_struct *png_ptr;
	png_info *info_ptr;
	png_byte *pngline;
	png_byte *p_png;
	png_color palette[MAXCOLORS];
	static png_uint_32 width;
	int bit_depth = 0;
	int color_type = -1;
	int tiff_color_type;
	int pass;
	static png_uint_32 res_x_half=0L, res_x=0L, res_y=0L;
	static int unit_type = 0;

	unsigned short *redcolormap;
	unsigned short *greencolormap;
	unsigned short *bluecolormap;
	static int have_res = FALSE;
	static int invert;
	int faxpect;
	long i, n;


	/* first figure out whether this machine is big- or little-endian */
	{
		union { int32 i; char c[4]; } endian_tester;

		endian_tester.i = 1;
		bigendian = (endian_tester.c[0] == 0);
	}

	invert = _invert;

	if (result == 0) {
		png = fopen (pngname, "wb");
		if (png == NULL) {
			BFDLog("tiff2png error:  PNG file %s cannot be created", pngname);
			result = 1;
		}
	}

	/* start PNG preparation */

	if (result == 0) {
		png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
		&tiff2png_jmpbuf_struct, tiff2png_error_handler, NULL);
		if (!png_ptr) {
			BFDLog("tiff2png error:  cannot allocate libpng main struct (%s)\n", pngname);
			result = 4;
		}
	}

	if (result == 0) {
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			BFDLog("tiff2png error:  cannot allocate libpng info struct (%s)\n", pngname);
			result = 4;
		}
	}

	if (result == 0) {
		if (setjmp (tiff2png_jmpbuf_struct.jmpbuf)) {
			BFDLog("tiff2png error:  libpng returns error condition (%s)\n", pngname);
			result = 1;
		} else {
			png_init_io (png_ptr, png);
		}
	}

	/* get TIFF header info */

	if (result == 0) {
		if (! TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photometric)) {
			BFDLog("tiff2png error:  photometric could not be retrieved (%s)\n", tiffname);
			result = 1;
		}
	}

	if (result == 0) {
		if (! TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bps)) bps = 1;
		if (! TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp)) spp = 1;
		if (! TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar)) planar = 1;

		tiled = TIFFIsTiled(tif); /* FAP 20020610 - get tiled flag */

		(void) TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols);
		(void) TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows);
		width = cols;

		ratio = 0.0;

		if (	TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xres)
			&& 	TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yres)
			&& 	(xres != 0.0) 
			&& 	(yres != 0.0)
		) {
			uint16 resunit;   /* typedef'd in tiff.h */

			have_res = TRUE;
			ratio = xres / yres;
#if 0
			/* GRR: this should be fine and works sometimes, but occasionally it
			*  seems to cause a segfault--which may be more related to Linux 2.2.10
			*  and/or SMP and/or heavy CPU loading.  Disabled for now. */
			(void) TIFFGetFieldDefaulted (tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
#else
			if (! TIFFGetField (tif, TIFFTAG_RESOLUTIONUNIT, &resunit)) resunit = RESUNIT_INCH;  /* default (see libtiff tif_dir.c) */
#endif

			/* convert from TIFF data (floats) to PNG data (unsigned longs) */
			switch (resunit) {
				case RESUNIT_CENTIMETER:
					res_x_half = (png_uint_32)(50.0*xres + 0.5);
					res_x = (png_uint_32)(100.0*xres + 0.5);
					res_y = (png_uint_32)(100.0*yres + 0.5);
					unit_type = PNG_RESOLUTION_METER;
					break;
				case RESUNIT_INCH:
					res_x_half = (png_uint_32)(0.5*39.37*xres + 0.5);
					res_x = (png_uint_32)(39.37*xres + 0.5);
					res_y = (png_uint_32)(39.37*yres + 0.5);
					unit_type = PNG_RESOLUTION_METER;
					break;
				/*    case RESUNIT_NONE:   */
				default:
					res_x_half = (png_uint_32)(50.0*xres + 0.5);
					res_x = (png_uint_32)(100.0*xres + 0.5);
					res_y = (png_uint_32)(100.0*yres + 0.5);
					unit_type = PNG_RESOLUTION_UNKNOWN;
					break;
			}
		}

		/* detect tiff filetype */

		maxval = (1 << bps) - 1;

		switch (photometric) {
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK:
			if (spp == 1) /* no alpha */ {
				color_type = PNG_COLOR_TYPE_GRAY;
				bit_depth = bps;
			}
			else /* must be alpha */ {
				color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
				if (bps <= 8) bit_depth = 8;
				else bit_depth = bps;
			}
			break;

		case PHOTOMETRIC_PALETTE: {
			int palette_8bit; /* set iff all color values in TIFF palette are < 256 */

			color_type = PNG_COLOR_TYPE_PALETTE;

			if (!TIFFGetField(tif, TIFFTAG_COLORMAP, &redcolormap, &greencolormap, &bluecolormap)) {
				BFDLog("tiff2png error:  cannot retrieve TIFF colormaps (%s)\n", tiffname);
				result = 1;
			}

			if (result == 0) {
				colors = maxval + 1;
				if (colors > MAXCOLORS) {
					BFDLog("tiff2png error:  palette too large (%d colors) (%s)\n", colors, tiffname);
					result = 1;
				}
			}

			if (result == 0) {
				/* max PNG palette-size is 8 bits, you could convert to full-color */
				if (bps >= 8) bit_depth = 8;
				else bit_depth = bps;

				/* PLTE chunk */
				/* TIFF palettes contain 16-bit shorts, while PNG palettes are 8-bit */
				/* Some broken (??) software puts 8-bit values in the shorts, which would
				make the palette come out all zeros, which isn't good. We check... */
				palette_8bit = 1;
				for (i = 0 ; i < colors ; i++) {
					if (	redcolormap[i] > 255
						||	greencolormap[i] > 255
						||	bluecolormap[i] > 255
					) {
						palette_8bit = 0;
						break;
					}
				}

				for (i = 0 ; i < colors ; i++) {
					if (invert) {
						if (palette_8bit) {
							palette[i].red   = ~((png_byte) redcolormap[i]);
							palette[i].green = ~((png_byte) greencolormap[i]);
							palette[i].blue  = ~((png_byte) bluecolormap[i]);
						} else {
								palette[i].red   = ~((png_byte) (redcolormap[i] >> 8));
								palette[i].green = ~((png_byte) (greencolormap[i] >> 8));
								palette[i].blue  = ~((png_byte) (bluecolormap[i] >> 8));
						}
					} else {
						if (palette_8bit) {
							palette[i].red   = (png_byte) redcolormap[i];
							palette[i].green = (png_byte) greencolormap[i];
							palette[i].blue  = (png_byte) bluecolormap[i];
						} else {
							palette[i].red   = (png_byte) (redcolormap[i] >> 8);
							palette[i].green = (png_byte) (greencolormap[i] >> 8);
							palette[i].blue  = (png_byte) (bluecolormap[i] >> 8);
						}
					}
				}

				/* prevent index data (pixel values) from being inverted (-> garbage) */
				invert = FALSE;
			}
			
			break;
		}

		case PHOTOMETRIC_YCBCR:
			/* GRR 20001110:  lifted from tiff2ps in libtiff 3.5.4 */
			TIFFGetField(tif, TIFFTAG_COMPRESSION, &tiff_compression_method);
			if (tiff_compression_method == COMPRESSION_JPEG && planar == PLANARCONFIG_CONTIG) {
				/* can rely on libjpeg to convert to RGB */
				TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
				photometric = PHOTOMETRIC_RGB;
			} else {
				BFDLog(
				"tiff2png error:  don't know how to handle PHOTOMETRIC_YCBCR with\n"
				"  compression %d (%sJPEG) and planar config %d (%scontiguous)\n"
				"  (%s)\n", tiff_compression_method,
				tiff_compression_method == COMPRESSION_JPEG? "" : "not ",
				planar, planar == PLANARCONFIG_CONTIG? "" : "not ", tiffname);
				result = 1;
				break;
			}
		/* fall thru... */

		case PHOTOMETRIC_RGB:
			if (spp == 3) {
				color_type = PNG_COLOR_TYPE_RGB;
			} else {
				color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			}
			if (bps <= 8) bit_depth = 8;
			else bit_depth = bps;
			break;

		case PHOTOMETRIC_LOGL:
		case PHOTOMETRIC_LOGLUV:
			/* GRR 20001110:  lifted from tiff2ps from libtiff 3.5.4 */
			TIFFGetField(tif, TIFFTAG_COMPRESSION, &tiff_compression_method);
			if (tiff_compression_method != COMPRESSION_SGILOG && tiff_compression_method != COMPRESSION_SGILOG24) {
				BFDLog(
				"tiff2png error:  don't know how to handle PHOTOMETRIC_LOGL%s with\n"
				"  compression %d (not SGILOG) (%s)\n",
				photometric == PHOTOMETRIC_LOGLUV? "UV" : "",
				tiff_compression_method, tiffname);
				result = 1;
			}

			if (result == 0) {
				/* rely on library to convert to RGB/greyscale */
#ifdef LIBTIFF_HAS_16BIT_INTEGER_FORMAT
				if (bps > 8) {
					/* SGILOGDATAFMT_16BIT converts to a floating-point luminance value;
					*  U,V are left as such.  SGILOGDATAFMT_16BIT_INT doesn't exist. */
					TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_16BIT_INT);
					bit_depth = bps = 16;
				} else
#endif
				{
					/* SGILOGDATAFMT_8BIT converts to normal grayscale or RGB format */
					TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
					bit_depth = bps = 8;
				}
				if (photometric == PHOTOMETRIC_LOGL) {
					photometric = PHOTOMETRIC_MINISBLACK;
					color_type = PNG_COLOR_TYPE_GRAY;
				} else {
					photometric = PHOTOMETRIC_RGB;
					color_type = PNG_COLOR_TYPE_RGB;
				}
			}
			break;

		/*
		case PHOTOMETRIC_YCBCR:
		case PHOTOMETRIC_LOGL:
		case PHOTOMETRIC_LOGLUV:
		*/
		case PHOTOMETRIC_MASK:
		case PHOTOMETRIC_SEPARATED:
		case PHOTOMETRIC_CIELAB:
		case PHOTOMETRIC_DEPTH:
			BFDLog(
			"tiff2png error:  don't know how to handle %s (%s)\n",
			/*
			photometric == PHOTOMETRIC_YCBCR?     "PHOTOMETRIC_YCBCR" :
			photometric == PHOTOMETRIC_LOGL?      "PHOTOMETRIC_LOGL" :
			photometric == PHOTOMETRIC_LOGLUV?    "PHOTOMETRIC_LOGLUV" :
			*/
			photometric == PHOTOMETRIC_MASK?      "PHOTOMETRIC_MASK" :
			photometric == PHOTOMETRIC_SEPARATED? "PHOTOMETRIC_SEPARATED" :
			photometric == PHOTOMETRIC_CIELAB?    "PHOTOMETRIC_CIELAB" :
			photometric == PHOTOMETRIC_DEPTH?     "PHOTOMETRIC_DEPTH" :
									  "unknown photometric",
			tiffname);
			result = 1;

		default:
			BFDLog("tiff2png error:  unknown photometric (%d) (%s)\n",
			photometric, tiffname);
			result = 1;
		}
	}

	if (result == 0) {
		tiff_color_type = color_type;

		faxpect = faxpect_option;
		if (faxpect && (!have_res || ratio < 1.90 || ratio > 2.10)) {
			BFDLog(
			"tiff2png:  aspect ratio is out of range: skipping -faxpect conversion\n");
			faxpect = FALSE;
		}

		if (faxpect && (color_type != PNG_COLOR_TYPE_GRAY || bit_depth != 1)) {
			BFDLog(
			"tiff2png:  only B&W (1-bit grayscale) images supported for -faxpect\n");
			faxpect = FALSE;
		}

		/* reduce width of fax by 2X by converting 1-bit grayscale to 2-bit, 3-color
		* palette */
		if (faxpect) {
			width = halfcols = cols / 2;
			color_type = PNG_COLOR_TYPE_PALETTE;
			palette[0].red = palette[0].green = palette[0].blue = 0;	/* both 0 */
			palette[1].red = palette[1].green = palette[1].blue = 127;	/* 0,1 or 1,0 */
			palette[2].red = palette[2].green = palette[2].blue = 255;	/* both 1 */
			colors = 3;
			bit_depth = 2;
			res_x = res_x_half;
		}

		/* put parameter info in png-chunks */

		png_set_IHDR(png_ptr, info_ptr, width, rows, bit_depth, color_type,
		interlace_type, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		if (png_compression_level != -1)
			png_set_compression_level(png_ptr, png_compression_level);

		if (color_type == PNG_COLOR_TYPE_PALETTE) 
			png_set_PLTE(png_ptr, info_ptr, palette, colors);

		/* gAMA chunk */
		if (gamma != -1.0) {
			png_set_gAMA(png_ptr, info_ptr, gamma);
		}

		/* pHYs chunk */
		if (have_res)
			png_set_pHYs(png_ptr, info_ptr, res_x, res_y, unit_type);

		png_write_info(png_ptr, info_ptr);
		png_set_packing(png_ptr);

		/* allocate space for one line (or row of tiles) of TIFF image */

		tiffline = NULL;
		tifftile = NULL;
		tiffstrip = NULL;

		if (!tiled) /* strip-based TIFF */ {
			if (planar == 1) /* contiguous picture */
				tiffline = (uch*) malloc(TIFFScanlineSize(tif));
			else /* separated planes */
				tiffline = (uch*) malloc(TIFFScanlineSize(tif) * spp);
		} else {
			/* FAP 20020610 - tiled support - allocate space for one "row" of tiles */

			TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
			TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_height);

			num_tilesX = (width+tile_width-1)/tile_width;

			if (planar == 1) {
				tilesz = TIFFTileSize(tif);
				tifftile = (uch*) malloc(tilesz);
				if (tifftile == NULL) {
					BFDLog(
					"tiff2png error:  can't allocate memory for TIFF tile buffer (%s)\n",
					tiffname);
					result = 4;
				} else {
					stripsz = (tile_width*num_tilesX) * tile_height * spp;
					tiffstrip = (uch*) malloc(stripsz);
					tiffline = tiffstrip; /* just set the line to the top of the strip.
							 * we'll move it through below. */
				}
			} else {
				BFDLog(
				"tiff2png error: can't handle tiled separated-plane TIFF format (%s)\n",
				tiffname);
				result = 5;
			}
		}
	}

	if (result == 0) {
		if (tiffline == NULL) {
			BFDLog(
			"tiff2png error:  can't allocate memory for TIFF scanline buffer (%s)\n",
			tiffname);
			if (tiled && planar == 1) free(tifftile);
			result = 4;
		}
	}

	if (result == 0) {
		if (planar != 1) /* in case we must combine more planes into one */ {
			tiffstrip = (uch*) malloc(TIFFScanlineSize(tif));
			if (tiffstrip == NULL) {
				BFDLog(
				"tiff2png error:  can't allocate memory for TIFF strip buffer (%s)\n",
				tiffname);
				free(tiffline);
				result = 4;
			}
		}
	}

	/* allocate space for one line of PNG image */
	/* max: 3 color channels plus one alpha channel, 16 bit => 8 bytes/pixel */

	if (result == 0) {
		pngline = (uch *) malloc (cols * 8);
		if (pngline == NULL) {
			BFDLog(
			"tiff2png error:  can't allocate memory for PNG row buffer (%s)\n",
			tiffname);
			free(tiffline);
			if (tiled && planar == 1) free(tifftile);
			else if (planar != 1) free(tiffstrip);
			result = 4;
		}
	}

	if (result == 0) {
#ifdef GRR_16BIT_DEBUG
		msb_max = lsb_max = 0;
		msb_min = lsb_min = 255;
		s16_max = 0;
		s16_min = 65535;
#endif

		for (pass = 0 ; pass < png_set_interlace_handling (png_ptr) ; pass++) {
			for (row = 0; row < rows; row++) {
				if (result == 0) {
					if (planar == 1) /* contiguous picture */ {
						if (!tiled) {
							if (TIFFReadScanline (tif, tiffline, row, 0) < 0) {
								BFDLog("tiff2png error:  bad data read on line %d (%s)\n",
								row, tiffname);
								free(tiffline);
								result = 1;
							}
						} else /* tiled */ {
							int col, ok=1, r;
							int tileno;
							/* FAP 20020610 - Read in one row of tiles and hand out the data one
									scanline at a time so the code below doesn't need
									to change */
							/* Is it time for a new strip? */
							if ((row % tile_height) == 0) {
								for (col = 0; ok && col < num_tilesX; col += 1) {
									tileno = col+(row/tile_height)*num_tilesX;
									/* read the tile into an RGB array */
									if (!TIFFReadEncodedTile(tif, tileno, tifftile, tilesz)) {
										ok = 0;
										break;
									}

									/* copy this tile into the row buffer */
									for (r = 0; r < (int) tile_height; r++) {
										void* dest;
										void* src;

										dest = tiffstrip + (r * tile_width * num_tilesX * spp)
													 + (col * tile_width * spp);
										src  = tifftile + (r * tile_width * spp);
										memcpy(dest, src, (tile_width * spp));
									}
								}
								tiffline = tiffstrip; /* set tileline to top of strip */
							} else {
								tiffline = tiffstrip + ((row % tile_height) * ((tile_width * num_tilesX) * spp));
							}
						} /* end if (tiled) */
					} else /* separated planes, then combine more strips into one line */ {
						ush s;

						/* XXX:  this assumes strips; are separated-plane tiles possible? */

						p_line = tiffline;
						for (n = 0; n < (cols/8 * bps*spp); n++)
							*p_line++ = '\0';

						for (s = 0; s < spp; s++) {
							p_strip = tiffstrip;
							getbitsleft = 8;
							p_line = tiffline;
							putbitsleft = 8;

							if (TIFFReadScanline(tif, tiffstrip, row, s) < 0) {
								BFDLog("tiff2png error:  bad data read on line %d (%s)\n",
								row, tiffname);
								free(tiffline);
								free(tiffstrip);
								result = 1;
							}

							if (result == 0) {
								p_strip = (uch *) tiffstrip;
								sample = '\0';
								for (i = 0 ; i < s ; i++)
									PUT_LINE_SAMPLE
								for (n = 0; n < cols; n++) {
									GET_STRIP_SAMPLE
									PUT_LINE_SAMPLE
									sample = '\0';
									for (i = 0 ; i < (spp-1) ; i++)
										PUT_LINE_SAMPLE
								}
							}
						} /* end for-loop (s) */
					} /* end if (planar/contiguous) */
				}

				if (result == 0) {
					p_line = tiffline;
					bitsleft = 8;
					p_png = pngline;

					/* convert from tiff-line to png-line */
					switch (tiff_color_type) {
					case PNG_COLOR_TYPE_GRAY:		/* we know spp == 1 */
						for (col = cols; col > 0; --col) {
							switch (bps) {
							case 16:
#ifdef INVERT_MINISWHITE
								if (photometric == PHOTOMETRIC_MINISWHITE) {
								if (bigendian) /* same as PNG order */ {
									GET_LINE_SAMPLE
									sample16 = sample;
									sample16 <<= 8;
									GET_LINE_SAMPLE
									sample16 |= sample;
								} else /* reverse of PNG */ {
									GET_LINE_SAMPLE
									sample16 = sample;
#ifdef GRR_16BIT_DEBUG
									if (msb_max < sample)
										msb_max = sample;
									if (msb_min > sample)
										msb_min = sample;
#endif
									GET_LINE_SAMPLE
									sample16 |= (((int)sample) << 8);
#ifdef GRR_16BIT_DEBUG
									if (lsb_max < sample)
										lsb_max = sample;
									if (lsb_min > sample)
										lsb_min = sample;
#endif
								}
								sample16 = maxval - sample16;
#ifdef GRR_16BIT_DEBUG
								if (s16_max < sample16)
									s16_max = sample16;
								if (s16_min > sample16)
									s16_min = sample16;
#endif
								*p_png++ = (uch)((sample16 >> 8) & 0xff);
								*p_png++ = (uch)(sample16 & 0xff);
								} else /* not PHOTOMETRIC_MINISWHITE */
#endif /* INVERT_MINISWHITE */
								{
								if (bigendian) {
									GET_LINE_SAMPLE
									*p_png++ = sample;
									GET_LINE_SAMPLE
									*p_png++ = sample;
								} else {
									GET_LINE_SAMPLE
									p_png[1] = sample;
									GET_LINE_SAMPLE
									*p_png = sample;
									p_png += 2;
								}
								} /* ? PHOTOMETRIC_MINISWHITE */
								break;

							case 8:
							case 4:
							case 2:
							case 1:
								GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
								if (photometric == PHOTOMETRIC_MINISWHITE)
								sample = maxval - sample;
#endif
								*p_png++ = sample;
								break;

							} /* end switch (bps) */
						}

						/* note that this actually converts 1-bit grayscale to 2-bit indexed
						* data, where 0 = black, 1 = half-gray (127), and 2 = white */
						if (faxpect) {
							png_byte *p_png2;

							p_png = pngline;
							p_png2 = pngline;
							for (col = halfcols; col > 0; --col) {
								*p_png++ = p_png2[0] + p_png2[1];
								p_png2 += 2;
							}
						}
						break;

					case PNG_COLOR_TYPE_GRAY_ALPHA:
						for (col = 0; col < cols; col++) {
							for (i = 0 ; i < spp ; i++) {
								switch (bps) {
								case 16:
#ifdef INVERT_MINISWHITE	/* GRR 20000122:  XXX 16-bit case not tested */
									if (photometric == PHOTOMETRIC_MINISWHITE && i == 0) {
										if (bigendian) {
											GET_LINE_SAMPLE
											sample16 = (sample << 8);
											GET_LINE_SAMPLE
											sample16 |= sample;
										} else {
											GET_LINE_SAMPLE
											sample16 = sample;
											GET_LINE_SAMPLE
											sample16 |= (((int)sample) << 8);
										}
										sample16 = maxval - sample16;
										*p_png++ = (uch)((sample16 >> 8) & 0xff);
										*p_png++ = (uch)(sample16 & 0xff);
									} else
#endif
									{
										if (bigendian) {
											GET_LINE_SAMPLE
											*p_png++ = sample;
											GET_LINE_SAMPLE
											*p_png++ = sample;
										} else {
										  GET_LINE_SAMPLE
										  p_png[1] = sample;
										  GET_LINE_SAMPLE
										  *p_png = sample;
										  p_png += 2;
										}
									}
									break;

								case 8:
									GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
									if (photometric == PHOTOMETRIC_MINISWHITE && i == 0)
									sample = maxval - sample;
#endif
									*p_png++ = sample;
									break;

								case 4:
									GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
									if (photometric == PHOTOMETRIC_MINISWHITE && i == 0)
										sample = maxval - sample;
#endif
									*p_png++ = sample * 17;	/* was 16 */
									break;

								case 2:
									GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
									if (photometric == PHOTOMETRIC_MINISWHITE && i == 0)
										sample = maxval - sample;
#endif
									*p_png++ = sample * 85;	/* was 64 */
									break;

								case 1:
									GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
									if (photometric == PHOTOMETRIC_MINISWHITE && i == 0)
										sample = maxval - sample;
#endif
									*p_png++ = sample * 255;	/* was 128...oops */
									break;

								} /* end switch */
							}
						}
						break;

					case PNG_COLOR_TYPE_RGB:
					case PNG_COLOR_TYPE_RGB_ALPHA:
						for (col = 0; col < cols; col++) {
							/* process for red, green and blue (and when applicable alpha) */
							for (i = 0 ; i < spp ; i++) {
								switch (bps) {
								case 16:
									/* XXX:  do we need INVERT_MINISWHITE support here, too, or
									*       is that only for grayscale? */
									if (bigendian) {
										GET_LINE_SAMPLE
										*p_png++ = sample;
										GET_LINE_SAMPLE
										*p_png++ = sample;
									} else {
										GET_LINE_SAMPLE
										p_png[1] = sample;
										GET_LINE_SAMPLE
										*p_png = sample;
										p_png += 2;
									}
								break;

								case 8:
									GET_LINE_SAMPLE
									*p_png++ = sample;
									break;

								/* XXX:  how common are these three cases? */

								case 4:
									GET_LINE_SAMPLE
									*p_png++ = sample * 17;	/* was 16 */
									break;

								case 2:
									GET_LINE_SAMPLE
									*p_png++ = sample * 85;	/* was 64 */
									break;

								case 1:
									GET_LINE_SAMPLE
									*p_png++ = sample * 255;	/* was 128 */
									break;

								} /* end switch */
							}
						}
						break;

					case PNG_COLOR_TYPE_PALETTE:
						for (col = 0; col < cols; col++) {
							GET_LINE_SAMPLE
							*p_png++ = sample;
						}
						break;

					default:
						BFDLog("tiff2png error:  unknown photometric (%d) (%s)\n",
						photometric, tiffname);
						free(tiffline);
						if (tiled && planar == 1)
							free(tifftile);
						else if (planar != 1) free(tiffstrip);
						result = 1;
					} /* end switch (tiff_color_type) */
				}

				if (result == 0) {
#ifdef GRR_16BIT_DEBUG
					if (bps == 16 && row == 0) {
						BFDLog("DEBUG:  hex contents of first row sent to libpng:\n");
						p_png = pngline;
						for (col = cols; col > 0; --col, p_png += 2)
							BFDLog("   %02x %02x", p_png[0], p_png[1]);
						BFDLog("\n");
						BFDLog("DEBUG:  end of first row sent to libpng\n");
					}
#endif
					png_write_row(png_ptr, pngline);
				}
			}
		} /* end for-loop (row) */
	} /* end for-loop (pass) */

	png_write_end(png_ptr, info_ptr);
	fclose(png); // keep

	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(tiffline);
	if (tiled && planar == 1)
		free(tifftile);
	else if (planar != 1)
		free(tiffstrip);

#ifdef GRR_16BIT_DEBUG
	if (bps == 16) {
		BFDLog("tiff2png:  range of most significant bytes  = %u-%u\n",
		msb_min, msb_max);
		BFDLog("tiff2png:  range of least significant bytes = %u-%u\n",
		lsb_min, lsb_max);
		BFDLog("tiff2png:  range of 16-bit integer values   = %u-%u\n",
		s16_min, s16_max);
	}
#endif

	return result;
}

