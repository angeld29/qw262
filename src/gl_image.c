#include "quakedef.h"

/************************* PCX *****************************/

gl_image_t image;

byte* LoadPCX (FILE *f, char *name)
{
	pcx_t	*pcx, pcxbuf;
	byte	palette[768];
	byte	*pix;
	int		x, y;
	int		dataByte, runLength;
	int		count;

	Con_DPrintf("LoadPCX: %s\n", name);
	fread (&pcxbuf, 1, sizeof(pcxbuf), f);

	pcx = &pcxbuf;

	pcx->xmax = LittleShort (pcx->xmax);
	pcx->xmin = LittleShort (pcx->xmin);
	pcx->ymax = LittleShort (pcx->ymax);
	pcx->ymin = LittleShort (pcx->ymin);
	pcx->hres = LittleShort (pcx->hres);
	pcx->vres = LittleShort (pcx->vres);
	pcx->bytes_per_line = LittleShort (pcx->bytes_per_line);
	pcx->palette_type = LittleShort (pcx->palette_type);

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 320
		|| pcx->ymax >= 256)
	{
		Con_Printf ("LoadPCX: %s : Bad pcx file\n", name);
		return NULL;
	}
//	if (matchwidth && (pcx->xmax+1) != matchwidth)
//		return NULL;
//	if (matchheight && (pcx->ymax+1) != matchheight)
//		return NULL;
	// seek to palette
	fseek (f, -768, SEEK_END);
	fread (palette, 1, 768, f);

	fseek (f, sizeof(pcxbuf) - 4, SEEK_SET);

	count = (pcx->xmax+1) * (pcx->ymax+1);
	image.data = malloc ( count * 4);

	for (y=0 ; y<=pcx->ymax ; y++)
	{
		pix = image.data + 4*y*(pcx->xmax+1);
		for (x=0 ; x<=pcx->ymax ; )
		{
			dataByte = fgetc(f);

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = fgetc(f);
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				pix[0] = palette[dataByte*3];
				pix[1] = palette[dataByte*3+1];
				pix[2] = palette[dataByte*3+2];
				pix[3] = 255;
				pix += 4;
				x++;
			}
		}
	}
	fclose (f);
	image.width = pcx->xmax+1;
	image.height = pcx->ymax+1;
	return image.data;
}

/************************* TGA *****************************/

#define TGA_MAXCOLORS 16384

/* Definitions for image types. */
#define TGA_Null		0	/* no image data */
#define TGA_Map			1	/* Uncompressed, color-mapped images. */
#define TGA_RGB			2	/* Uncompressed, RGB images. */
#define TGA_Mono		3	/* Uncompressed, black and white images. */
#define TGA_RLEMap		9	/* Runlength encoded color-mapped images. */
#define TGA_RLERGB		10	/* Runlength encoded RGB images. */
#define TGA_RLEMono		11	/* Compressed, black and white images. */
#define TGA_CompMap		32	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding. */
#define TGA_CompMap4	33	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process. */

/* Definitions for interleave flag. */
#define TGA_IL_None		0	/* non-interleaved. */
#define TGA_IL_Two		1	/* two-way (even/odd) interleaving */
#define TGA_IL_Four		2	/* four way interleaving */
#define TGA_IL_Reserved	3	/* reserved */

/* Definitions for origin flag */
#define TGA_O_UPPER		0	/* Origin in lower left-hand corner. */
#define TGA_O_LOWER		1	/* Origin in upper left-hand corner. */

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

static int fgetLittleShort (FILE *f)
{
	byte	b1, b2;

	b1 = fgetc(f);
	b2 = fgetc(f);

	return (short)(b1 + b2*256);
}

/*static int fgetLittleLong (FILE *f) 
{
	byte	b1, b2, b3, b4;

	b1 = fgetc(f);
	b2 = fgetc(f);
	b3 = fgetc(f);
	b4 = fgetc(f);

	return b1 + (b2<<8) + (b3<<16) + (b4<<24);
}*/

byte *LoadTGA (FILE *fin, char *name) 
{
	int			w, h, x, y, realrow, truerow, baserow, i, temp1, temp2, pixel_size, map_idx;
	int			RLE_count, RLE_flag, size, interleave, origin;
	qboolean	mapped, rlencoded;
	byte		*dst, r, g, b, a, j, k, l, *ColorMap;
	TargaHeader header;

	header.id_length = fgetc(fin);
	header.colormap_type = fgetc(fin);
	header.image_type = fgetc(fin);
	
	header.colormap_index = fgetLittleShort(fin);
	header.colormap_length = fgetLittleShort(fin);
	header.colormap_size = fgetc(fin);
	header.x_origin = fgetLittleShort(fin);
	header.y_origin = fgetLittleShort(fin);
	header.width = fgetLittleShort(fin);
	header.height = fgetLittleShort(fin);
	header.pixel_size = fgetc(fin);
	header.attributes = fgetc(fin);

	if (header.id_length != 0)
		fseek(fin, header.id_length, SEEK_CUR);  // skip TARGA image comment

	/* validate TGA type */
	switch (header.image_type) {		
		case TGA_Map: case TGA_RGB: case TGA_Mono: case TGA_RLEMap: case TGA_RLERGB: case TGA_RLEMono:
			break;
		Con_Printf ("LoadTGA: %s : Only type 1 (map), 2 (RGB), 3 (mono), 9 (RLEmap), 10 (RLERGB), 11 (RLEmono) TGA images supported\n", name);
		fclose(fin);
		return NULL;
	}

	/* validate color depth */
	switch (header.pixel_size) {
		case 8:	case 15: case 16: case 24: case 32:
			break;
		default:
			fclose(fin);
			Con_Printf ("LoadTGA: %s : Only 8, 15, 16, 24 or 32 bit images (with colormaps) supported\n", name);
			return NULL;
	}

	r = g = b = a = l = 0;

	/* if required, read the color map information. */
	ColorMap = NULL;
	mapped = ( header.image_type == TGA_Map || header.image_type == TGA_RLEMap) && header.colormap_type == 1;
	if ( mapped ) {
		/* validate colormap size */
		switch( header.colormap_size ) {
			case 8:	case 15: case 16: case 32: case 24:
				break;
			default:
				fclose(fin);
				Con_Printf ("%s : Only 8, 15, 16, 24 or 32 bit colormaps supported\n", name);
				return NULL;
		}

		temp1 = header.colormap_index;
		temp2 = header.colormap_length;
		if ((temp1 + temp2 + 1) >= TGA_MAXCOLORS) {
			fclose(fin);
			return NULL;
		}
		ColorMap = malloc (TGA_MAXCOLORS * 4);
		map_idx = 0;
		for (i = temp1; i < temp1 + temp2; ++i, map_idx += 4 ) {
			/* read appropriate number of bytes, break into rgb & put in map. */
			switch( header.colormap_size ) {
				case 8:	/* grey scale, read and triplicate. */
					r = g = b = getc(fin);
					a = 255;
					break;
				case 15:	/* 5 bits each of red green and blue. */
							/* watch byte order. */
					j = getc(fin);
					k = getc(fin);
					l = ((unsigned int) k << 8) + j;
					r = (byte) ( ((k & 0x7C) >> 2) << 3 );
					g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
					b = (byte) ( (j & 0x1F) << 3 );
					a = 255;
					break;
				case 16:	/* 5 bits each of red green and blue, 1 alpha bit. */
							/* watch byte order. */
					j = getc(fin);
					k = getc(fin);
					l = ((unsigned int) k << 8) + j;
					r = (byte) ( ((k & 0x7C) >> 2) << 3 );
					g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
					b = (byte) ( (j & 0x1F) << 3 );
					a = (k & 0x80) ? 255 : 0;
					break;
				case 24:	/* 8 bits each of blue, green and red. */
					b = getc(fin);
					g = getc(fin);
					r = getc(fin);
					a = 255;
					l = 0;
					break;
				case 32:	/* 8 bits each of blue, green, red and alpha. */
					b = getc(fin);
					g = getc(fin);
					r = getc(fin);
					a = getc(fin);
					l = 0;
					break;
			}
			ColorMap[map_idx + 0] = r;
			ColorMap[map_idx + 1] = g;
			ColorMap[map_idx + 2] = b;
			ColorMap[map_idx + 3] = a;
		}
	}

	/* check run-length encoding. */
	rlencoded = (header.image_type == TGA_RLEMap || header.image_type == TGA_RLERGB || header.image_type == TGA_RLEMono);
	RLE_count = RLE_flag = 0;

	image.width = w = header.width;
	image.height = h = header.height;

	size = w * h * 4;
	image.data = calloc(size, 1);

	/* read the Targa file body and convert to portable format. */
	pixel_size = header.pixel_size;
	origin = (header.attributes & 0x20) >> 5;
	interleave = (header.attributes & 0xC0) >> 6;
	truerow = 0;
	baserow = 0;
	for (y = 0; y < h; y++) {
		realrow = truerow;
		if ( origin == TGA_O_UPPER )
			realrow = h - realrow - 1;

		dst = image.data + realrow * w * 4;

		for (x = 0; x < w; x++) {
			/* check if run length encoded. */
			if( rlencoded ) {
				if( !RLE_count ) {
					/* have to restart run. */
					i = getc(fin);
					RLE_flag = (i & 0x80);
					if( !RLE_flag ) {
						/* stream of unencoded pixels. */
						RLE_count = i + 1;
					} else {
						/* single pixel replicated. */
						RLE_count = i - 127;
					}
					/* decrement count & get pixel. */
					--RLE_count;
				} else {
					/* have already read count & (at least) first pixel. */
					--RLE_count;
					if( RLE_flag )
						/* replicated pixels. */
						goto PixEncode;
				}
			}

			/* read appropriate number of bytes, break into RGB. */
			switch (pixel_size) {
				case 8:	/* grey scale, read and triplicate. */
					r = g = b = l = getc(fin);
					a = 255;
					break;
				case 15:	/* 5 bits each of red green and blue. */
							/* watch byte order. */
					j = getc(fin);
					k = getc(fin);
					l = ((unsigned int) k << 8) + j;
					r = (byte) ( ((k & 0x7C) >> 2) << 3 );
					g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
					b = (byte) ( (j & 0x1F) << 3 );
					a = 255;
					break;
				case 16:	/* 5 bits each of red green and blue, 1 alpha bit. */
							/* watch byte order. */
					j = getc(fin);
					k = getc(fin);
					l = ((unsigned int) k << 8) + j;
					r = (byte) ( ((k & 0x7C) >> 2) << 3 );
					g = (byte) ( (((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3 );
					b = (byte) ( (j & 0x1F) << 3 );
					a = (k & 0x80) ? 255 : 0;
					break;
				case 24:	/* 8 bits each of blue, green and red. */
					b = getc(fin);
					g = getc(fin);
					r = getc(fin);
					a = 255;
					l = 0;
					break;
				case 32:	/* 8 bits each of blue, green, red and alpha. */
					b = getc(fin);
					g = getc(fin);
					r = getc(fin);
					a = getc(fin);
					l = 0;
					break;
				default:
					fclose(fin);
					Con_Printf ("%s : Illegal pixel_size '%d'\n", name, pixel_size);
					free(image.data);
					if (mapped)
						free(ColorMap);
					return NULL;
			}
PixEncode:
			if (mapped) {
				map_idx = l * 4;
				*dst++ = ColorMap[map_idx + 0];
				*dst++ = ColorMap[map_idx + 1];
				*dst++ = ColorMap[map_idx + 2];
				*dst++ = ColorMap[map_idx + 3];
			} else {
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
				*dst++ = a;
			}
		}

		if (interleave == TGA_IL_Four)
			truerow += 4;
		else if (interleave == TGA_IL_Two)
			truerow += 2;
		else
			truerow++;
		if (truerow >= h)
			truerow = ++baserow;
	}

	if (mapped)
		free(ColorMap);
	fclose(fin);
	return image.data;
}

/************************* JPG *****************************/

// libjpeg required!
#ifdef __USE_JPG__
byte *LoadJPG (FILE *fin, char *name) 
{
	struct jpeg_decompress_struct cinfo;
	JDIMENSION num_scanlines;
	JSAMPARRAY in;
	struct jpeg_error_mgr jerr;
	int numPixels;
	int row_stride;
	byte *out;
	int count;
	int i;

	Con_DPrintf("LoadJPG: %s\n", name);
	// set up the decompression.
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress (&cinfo);

	// inititalize the source
	jpeg_stdio_src (&cinfo, fin);

	// initialize decompression
	(void) jpeg_read_header (&cinfo, TRUE);
	cinfo.out_color_space = JCS_RGB; // BorisU: force grayscale to RGB
	(void) jpeg_start_decompress (&cinfo);

	// set up the width and height for return
//	if (matchwidth && matchwidth != cinfo.image_width) 
//		return NULL;
//	if (matchheight && matchheight != cinfo.image_height) 
//		return NULL;

	numPixels = cinfo.image_width * cinfo.image_height;

	// initialize the input buffer - we'll use the in-built memory management routines in the
	// JPEG library because it will automatically free the used memory for us when we destroy
	// the decompression structure.  cool.
	row_stride = cinfo.output_width * cinfo.output_components;
	in = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	// bit of error checking
	if (cinfo.output_components != 3)
		goto error;

	if ((numPixels * 4) != ((row_stride * cinfo.output_height) + numPixels))
		goto error;

	// read the jpeg
	count = 0;

	// initialize the return data
	image.data = malloc ((numPixels * 4));

	while (cinfo.output_scanline < cinfo.output_height) 
	{
		num_scanlines = jpeg_read_scanlines(&cinfo, in, 1);
		out = in[0];

		for (i = 0; i < row_stride;)
		{
			image.data[count++] = out[i++];
			image.data[count++] = out[i++];
			image.data[count++] = out[i++];
			image.data[count++] = 255;
		}
	}

	// finish decompression and destroy the jpeg
	(void) jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);
	fclose(fin);

	image.width = cinfo.output_width;
	image.height = cinfo.output_height;
	return image.data;

error:
	// this should rarely (if ever) happen, but just in case...
	Con_Printf ("LoadJPG: %s : Invalid JPEG Format\n", name);
	jpeg_destroy_decompress (&cinfo);
	fclose(fin);

	return NULL;
}
#endif

/************************* PNG *****************************/

// libpng and zlib required!

#ifdef __USE_PNG__
byte *LoadPNG (FILE *fin, char *name)
{
	png_structp	png; 
	png_infop	pnginfo; 

	png_uint_32	width, height;
	int		bit_depth, color_type, interlace_type, compression_type, filter_type;
	int		rowbytes, bytesperpixel;
	char	*data,*rowptrs;
	long	*cvaluep;
	int		y;

	Con_DPrintf("LoadPNG: %s\n", name);
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png) return NULL;

	pnginfo = png_create_info_struct( png );
	if (!pnginfo) {
		png_destroy_read_struct(&png,&pnginfo,png_infopp_NULL);
		return NULL;
	}

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &pnginfo, png_infopp_NULL);
		fclose(fin);
		Con_Printf("LoadPNG: %s : Invalid PNG format\n", name);
		return NULL;
	}

	png_set_sig_bytes(png,0/*sizeof( sig )*/);

	png_init_io(png, fin);

	png_read_info(png, pnginfo);
	
	png_get_IHDR(png, pnginfo, &width, &height, &bit_depth, 
				&color_type, &interlace_type, &compression_type, &filter_type);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA){
			png_set_gray_to_rgb(png);
			if (bit_depth < 8)
				png_set_gray_1_2_4_to_8(png);
	}

	// Add alpha channel if present 
	if(png_get_valid( png, pnginfo, PNG_INFO_tRNS ))
		png_set_tRNS_to_alpha(png);

	if ( !(color_type & PNG_COLOR_MASK_ALPHA) )
		png_set_filler(png, 255, PNG_FILLER_AFTER);

	if (bit_depth < 8)
		png_set_expand (png);
	if (bit_depth == 16)
		png_set_strip_16(png);

	// update the info structure 
	png_read_update_info( png, pnginfo );

	rowbytes = png_get_rowbytes( png, pnginfo );
	bytesperpixel = png_get_channels( png, pnginfo );

	data = malloc(height * rowbytes);
	rowptrs = malloc(sizeof(void*) * height);

	cvaluep = (long*)rowptrs;
	for(y=0;y<height;y++) {
		cvaluep[y] = (long)data + ( y * (long)rowbytes );
	}

	png_read_image(png, (png_bytepp)rowptrs);

	png_read_end(png, pnginfo); // read last information chunks

	png_destroy_read_struct(&png, &pnginfo, 0);

	fclose (fin);
	free (rowptrs);

	image.data = data;
	image.width = width;
	image.height = height;
	return image.data;
}
#endif

/************************* DDS *****************************/

// code from nVidia SDK 

#define DDS_FOURCC		0x00000004
#define DDS_RGB			0x00000040
#define DDS_RGBA		0x00000041
#define DDS_DEPTH		0x00800000

#define DDS_COMPLEX		0x00000008
#define DDS_CUBEMAP		0x00000200
#define DDS_VOLUME		0x00200000

#define FOURCC_DXT1		0x31545844 //(MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT3		0x33545844 //(MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT5		0x35545844 //(MAKEFOURCC('D','X','T','5'))

struct DDS_PIXELFORMAT
{
	unsigned long dwSize;
	unsigned long dwFlags;
	unsigned long dwFourCC;
	unsigned long dwRGBBitCount;
	unsigned long dwRBitMask;
	unsigned long dwGBitMask;
	unsigned long dwBBitMask;
	unsigned long dwABitMask;
};

struct DDS_HEADER
{
	unsigned long dwSize;
	unsigned long dwFlags;
	unsigned long dwHeight;
	unsigned long dwWidth;
	unsigned long dwPitchOrLinearSize;
	unsigned long dwDepth;
	unsigned long dwMipMapCount;
	unsigned long dwReserved1[11];
	struct DDS_PIXELFORMAT ddspf;
	unsigned long dwCaps1;
	unsigned long dwCaps2;
	unsigned long dwReserved2[3];
};

int size_dxtc (int width, int height)
{
	return ((width+3)/4)*((height+3)/4)*
		(image.format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
}

byte *LoadDDS (FILE *fin, char *name)
{
	struct DDS_HEADER ddsh;
	char filecode[4];
	int components;
	int miplevel,h,w,full_size;

	Con_DPrintf("LoadDDS: %s\n", name);
	// read in file marker, make sure its a DDS file
	fread(filecode, 1, 4, fin);
	if (strncmp(filecode, "DDS ", 4) != 0) {
		fclose(fin);
		Con_Printf("LoadDDS: %s : Invalid DDS format\n", name);
		return NULL;
	}

	// read in DDS header
	fread(&ddsh, sizeof(ddsh), 1, fin);

	ddsh.dwSize = LittleLong(ddsh.dwSize);
	ddsh.dwFlags = LittleLong(ddsh.dwFlags);
	ddsh.dwHeight = LittleLong(ddsh.dwHeight);
	ddsh.dwWidth = LittleLong(ddsh.dwWidth);
	ddsh.dwPitchOrLinearSize = LittleLong(ddsh.dwPitchOrLinearSize);
	ddsh.dwMipMapCount = LittleLong(ddsh.dwMipMapCount);
	ddsh.ddspf.dwSize = LittleLong(ddsh.ddspf.dwSize);
	ddsh.ddspf.dwFlags = LittleLong(ddsh.ddspf.dwFlags);
	ddsh.ddspf.dwFourCC = LittleLong(ddsh.ddspf.dwFourCC);
	ddsh.ddspf.dwRGBBitCount = LittleLong(ddsh.ddspf.dwRGBBitCount);
	ddsh.dwCaps1 = LittleLong(ddsh.dwCaps1);
	ddsh.dwCaps2 = LittleLong(ddsh.dwCaps2);

//	Con_Printf("Flags=%x\n", ddsh.dwFlags);
	if (ddsh.ddspf.dwFlags & DDS_FOURCC) {
		switch(ddsh.ddspf.dwFourCC) {
			case FOURCC_DXT1:
				image.format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				components = 3;
				break;
			case FOURCC_DXT3:
				image.format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				components = 4;
				break;
			case FOURCC_DXT5:
				image.format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				components = 4;
				break;
			default:
				Con_Printf("LoadDDS: %s : Unknown format", name);
				fclose(fin);
				return NULL;
		}
	}
	else {
		Con_Printf("LoadDDS: %s : Only compressed formats are supported", name);
		fclose(fin);
		return NULL;
	}

	image.width = ddsh.dwWidth;
	image.height = ddsh.dwHeight;

	image.size = size_dxtc(image.width, image.height);
	full_size = image.size;

	if (ddsh.dwMipMapCount) {// mipmaps are present
		image.mipmaps = ddsh.dwMipMapCount;
		w=image.width;
		h=image.height;
		for (miplevel=1; miplevel<image.mipmaps; miplevel++){
			w /= 2; if (!w) w = 1;
			h /= 2; if (!h) h = 1;
			full_size += size_dxtc(w, h);
		}
	} else {
		image.mipmaps = 0;
	}

	image.data = malloc(full_size);
	fread(image.data, 1, full_size, fin);
	fclose(fin);

	return image.data;
}
