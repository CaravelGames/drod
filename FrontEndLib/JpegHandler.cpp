// $Id$

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "JpegHandler.h"
#include "BitmapManager.h"

//For JPEG image support.
#include <BackEndLib/Files.h>
extern "C" {
	#include "jpeglib.h"
}
#include <setjmp.h>	//used for the error recovery mechanism

struct my_error_mgr {
	struct jpeg_error_mgr pub;  // "public" fields
	jmp_buf setjmp_buffer;      // for return to caller on error
};

typedef struct my_error_mgr * my_error_ptr;

//Here's the routine that will replace the standard error_exit method.
void my_error_exit (j_common_ptr cinfo)
{
  // cinfo->err really points to a my_error_mgr struct, so coerce pointer
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  // Return control to the setjmp point.
  longjmp(myerr->setjmp_buffer, 1);
}

//**********************************************************************************
bool CJpegHandler::Compress(
//Returns: true on success, false on failure
//
//Params:
	const WCHAR *wszName,   //(in)   Full path+name for the JPEG image
	SDL_Surface *pSurface,
	const int quality)	//[default=100]
{
	ASSERT(wszName);
	ASSERT(pSurface);

	//Convert surface to byte buffer.
	BYTE *pBuffer = CBitmapManager::WriteSurfaceToPixelBuffer(pSurface);
	if (!pBuffer) return false;

	const bool bRes = Compress(wszName, pBuffer, pSurface->w, pSurface->h, quality);
	delete[] pBuffer;
	return bRes;
}

//**********************************************************************************
bool CJpegHandler::Compress(
//Returns: true on success, false on failure
//
//Params:
	const WCHAR *wszName,   //(in)   Full path+name for the JPEG image
	BYTE* imageBuffer, const UINT wWidth, const UINT wHeight,
	const int quality)	//[default=100]
{
/*
	COMPRESSION Outline:
	Allocate and initialize a JPEG compression object
	Specify the destination for the compressed data (eg, a file)
	Set parameters for compression, including image size & colorspace
	jpeg_start_compress(...);
	while (scan lines remain to be written)
		jpeg_write_scanlines(...);
	jpeg_finish_compress(...);
	Release the JPEG compression object
*/
	FILE * outfile;
	CFiles f;
	if ((outfile = f.Open(wszName, "wb")) == NULL) {
		 f.AppendErrorLog("Can't open file\n");
		 return false;
	}
	struct jpeg_compress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	// Establish the setjmp return context for my_error_exit to use (C equivalent of a "try" block).
	if (setjmp(jerr.setjmp_buffer))
	{
		//If we get here, the JPEG code has signaled an error.
		//We need to clean up the JPEG object and return failure.
		jpeg_destroy_compress(&cinfo);
		return false;
	}
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	jpeg_set_quality(&cinfo, quality, false);  //int quality, boolean force_baseline
	cinfo.image_width = wWidth;   /* image width and height, in pixels */
	cinfo.image_height = wHeight;
	cinfo.input_components = 3;   /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; /* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	/* Make optional parameter settings here */
	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];   /* pointer to a single row */
	const int row_stride =        /* physical row width in buffer */
			wWidth * 3; /* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		 row_pointer[0] = & imageBuffer[cinfo.next_scanline * row_stride];
		 jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(outfile);
	return true;
}

//**********************************************************************************
bool CJpegHandler::Decompress(
//Load a JPEG from disk and uncompress it into a byte buffer.
//
//Returns: true if successful
//
//Params:
	const WCHAR *wszName,   //(in)   Full path+name to the JPEG image
	BYTE* &imageBuffer, UINT &wWidth, UINT &wHeight)   //(out) image data
{
/*
	DECOMPRESSION Outline:
	Allocate and initialize a JPEG decompression object
	Specify the source of the compressed data (eg, a file)
	Call jpeg_read_header() to obtain image info
	Set parameters for decompression
	jpeg_start_decompress(...);
	while (scan lines remain to be read)
		jpeg_read_scanlines(...);
	jpeg_finish_decompress(...);
	Release the JPEG decompression object
*/
	//Reading the JPEG file into a buffer and decompressing it there will result
	//in many fewer disk I/O accesses, so we'll do it that way.
	CFiles f;
	CStretchyBuffer buffer;
	if (!f.ReadFileIntoBuffer(wszName, buffer,true))
		return false;
	return Decompress((BYTE*)buffer, buffer.Size(), imageBuffer, wWidth, wHeight);
}


//**********************************************************************************
//JPEGLib Source Manager to read from memory obtained from:
//http://www-2.cs.cmu.edu/afs/cs.cmu.edu/academic/class/15740-f01/public/benchmarks/spec95/benchspec/CINT95/132.ijpeg/src/spec_jmemsrc.c

// Expanded data source object for stdio input
struct jms_my_source_mgr {
	struct jpeg_source_mgr pub;
};

typedef jms_my_source_mgr* jms_my_src_ptr;

void jms_init_source (j_decompress_ptr /*dinfo*/) {    /* nothing needed */ }

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * This is an error for the memory source (jms_) routines.
 *
 * TBD: use standard libjpeg error handling?
 */
boolean jms_fill_input_buffer (j_decompress_ptr /*dinfo*/)
{
	 LOGERR("Error: jms_fill_input_buffer_called");
	 exit(1);
	 return 1;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Simply moves pointers for the in-memory routines.
 */
void jms_skip_input_data (j_decompress_ptr dinfo, long num_bytes)
{
  jms_my_src_ptr src = (jms_my_src_ptr) dinfo->src;

  if (src->pub.bytes_in_buffer < (unsigned long) num_bytes) {
		LOGERR("jms_skip_input_data skips past end");
		exit(1);
  }

  src->pub.next_input_byte += (size_t) num_bytes;
  src->pub.bytes_in_buffer -= (size_t) num_bytes;
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.
 *
 * Is a NO-OP for memory input.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
void jms_term_source (j_decompress_ptr /*dinfo*/) {  /* no work necessary here */ }

void spec_jpeg_mem_src (j_decompress_ptr dinfo, JOCTET* jms_buffer, int jms_bufsz)
// Prepare for input from a memory buffer (instead of a FILE*).
{
  jms_my_src_ptr src;

  /* The source object and input buffer are made permanent so that a series
	* of JPEG images can be read from the same file by calling jpeg_stdio_src
	* only before the first one.  (If we discarded the buffer at the end of
	* one image, we'd likely lose the start of the next one.)
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/
  if (dinfo->src == NULL) {   /* first time for this JPEG object? */
	 dinfo->src = (struct jpeg_source_mgr *)
		(*dinfo->mem->alloc_small) ((j_common_ptr) dinfo, JPOOL_PERMANENT,
				  sizeof(jms_my_source_mgr));
  }

  src = (jms_my_src_ptr) dinfo->src;

  /*
	* Methods
	*/
  src->pub.init_source = jms_init_source;
  src->pub.fill_input_buffer = jms_fill_input_buffer;
  src->pub.skip_input_data = jms_skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = jms_term_source;
  
  /*
	* Data structures
	*/
  src->pub.bytes_in_buffer = jms_bufsz; 
  src->pub.next_input_byte = jms_buffer;
}

//**********************************************************************************
bool CJpegHandler::Decompress(
//Uncompress a JPEG from a byte buffer into a byte buffer.
//
//Returns: true if successful, false on failure
//
//Params:
	BYTE* inBuffer, const UINT wInSize, //(in)   JPEG data buffer
	BYTE* &imageBuffer, UINT &wWidth, UINT &wHeight)   //(out) image data
{
	struct jpeg_decompress_struct dinfo;
	struct my_error_mgr jerr;
	dinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	// Establish the setjmp return context for my_error_exit to use (C equivalent of a "try" block).
	if (setjmp(jerr.setjmp_buffer))
	{
		//If we get here, the JPEG code has signaled an error.
		//We need to clean up the JPEG object and return failure.
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jpeg_create_decompress(&dinfo);

	// Set up "source manager" to read compressed data from memory.
	spec_jpeg_mem_src(&dinfo, (JOCTET*)inBuffer, (int)wInSize);

	if (jpeg_read_header(&dinfo, FALSE) != JPEG_HEADER_OK) return false;
	jpeg_start_decompress(&dinfo);
	wWidth = dinfo.output_width;
	wHeight = dinfo.output_height;
	ASSERT(dinfo.output_components == 3);
	const UINT wSize = dinfo.output_height * dinfo.output_width * 3;
	imageBuffer = new BYTE[wSize];
	JSAMPROW row_pointer[1];   // pointer to a single row
	const int row_stride = wWidth * 3;  // JSAMPLEs per row in image_buffer

	while (dinfo.output_scanline < dinfo.output_height)
	{
		//Read one row of the image into the buffer.
		row_pointer[0] = &imageBuffer[dinfo.output_scanline * row_stride];
		jpeg_read_scanlines(&dinfo, row_pointer, 1);
	}
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	return true;
}
