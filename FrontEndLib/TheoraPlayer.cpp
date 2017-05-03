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
 * Portions created by the Initial Developer are Copyright (C) 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "TheoraPlayer.h"
#include "Colors.h" // for MAKE_SDL_RECT
#include "Screen.h" // window stuff
#include <BackEndLib/Assert.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Types.h>

#include <SDL.h>
#include <theora/theora.h>
#include <vorbis/codec.h>

#ifdef STEAMBUILD
#  include <steam_api.h>
#endif

// Ogg and codec state for demux/decode
int              theora_p=0;
int              vorbis_p=0;
Uint32 startticks = 0;
ogg_sync_state   oy; 
ogg_page         og;
ogg_stream_state vo;
ogg_stream_state to;
theora_info      ti;
theora_comment   tc;
theora_state     td;
vorbis_info      vi;
vorbis_dsp_state vd;
vorbis_block     vb;
vorbis_comment   vc;

//
//
// Header info routines.
//
//

void dump_comments(theora_comment *tc)
// dump the theora (or vorbis) comment header
{
	printf("Encoded by %s\n",tc->vendor);
	if(tc->comments)
	{
		printf("theora comment header:\n");
		for(int i=0;i<tc->comments;++i)
		{
			if(tc->user_comments[i])
			{
				int len=tc->comment_lengths[i];
				char *value=new char[len+1];
				memcpy(value,tc->user_comments[i],len);
				value[len]='\0';
				printf("\t%s\n", value);
				delete[] value;
			}
		}
	}
}

int buffer_data(ogg_sync_state *oy, CStretchyBuffer& buffer)
// Helper; just grab some more compressed bitstream and sync it for page extraction
{
	static const UINT CHUNK_SIZE = 4096;
	char *pBuffer = ogg_sync_buffer(oy, CHUNK_SIZE);
	const UINT wBytesRead = buffer.ReadChunk(pBuffer, CHUNK_SIZE);
	ogg_sync_wrote(oy, wBytesRead);
	return wBytesRead;
}

int queue_page(ogg_page *page) {
// helper: push a page into the appropriate stream.
// this can be done blindly; a stream won't accept a page that doesn't belong to it
	if (theora_p)
		ogg_stream_pagein(&to,page);
	if (vorbis_p)
		ogg_stream_pagein(&vo,page);
	return 0;
}                                   

bool parseHeaders(CStretchyBuffer& buffer)
// Parse the headers
// Only interested in Vorbis/Theora streams
{
	// extracted from player_sample.c test file for theora alpha
	ogg_packet op;
	bool stateflag=false;
	while (!stateflag)
	{
		int ret=buffer_data(&oy, buffer);
		if (ret==0) break;
		while (ogg_sync_pageout(&oy,&og)>0)
		{
			ogg_stream_state test;

			// is this a mandated initial header? If not, stop parsing
			if(!ogg_page_bos(&og)){
				// don't leak the page; get it into the appropriate stream
				queue_page(&og);
				stateflag=true;
				break;
			}
   
			ogg_stream_init(&test,ogg_page_serialno(&og));
			ogg_stream_pagein(&test,&og);
			ogg_stream_packetout(&test,&op);
   
			// identify the codec: try theora
			if (!theora_p && theora_decode_header(&ti,&tc,&op)>=0)
			{
				// it is theora
				memcpy(&to,&test,sizeof(test));
				theora_p=1;
			} else if(!vorbis_p && vorbis_synthesis_headerin(&vi,&vc,&op)>=0) {
				// it is vorbis
				memcpy(&vo,&test,sizeof(test));
				vorbis_p=1;
			} else {
				// whatever it is, we don't care about it
				ogg_stream_clear(&test);
			}
		}
	}

	// we've now identified all the bitstreams. parse the secondary header packets.
	while((theora_p && theora_p<3) || (vorbis_p && vorbis_p<3))
	{
		int ret;

		// look for further theora headers
		while (theora_p && (theora_p<3) && (ret=ogg_stream_packetout(&to,&op)))
		{
			if (ret<0) {
				ASSERT(!"Error parsing Theora stream headers; corrupt stream?");
				return false;
			}
			if (theora_decode_header(&ti,&tc,&op)) {
				ASSERT(!"Error parsing Theora stream headers; corrupt stream?");
				return false;
			}
			++theora_p;
			if (theora_p==3)
				break;
		}

		// look for more vorbis header packets
		while (vorbis_p && (vorbis_p<3) && (ret=ogg_stream_packetout(&vo,&op)))
		{
			if (ret<0) {
				ASSERT(!"Error parsing Vorbis stream headers; corrupt stream?");
				return false;
			}
			if (vorbis_synthesis_headerin(&vi,&vc,&op)) {
				ASSERT(!"Error parsing Vorbis stream headers; corrupt stream?");
				return false;
			}
			++vorbis_p;
			if (vorbis_p==3)
				break;
		}

		// The header pages/packets will arrive before anything else we
		// care about, or the stream is not obeying spec
 
		if (ogg_sync_pageout(&oy,&og)>0) {
			queue_page(&og); // demux into the appropriate stream
		} else {
			int ret=buffer_data(&oy, buffer);
			if(ret==0){
				ASSERT(!"End of file while searching for codec headers.");
				return false;
			}
		}
	}
	return true;
}

//
//
// Audio routines. -- not implemented.  See splayer.c for code.
//
//

int open_audio() {return 0;}
int start_audio() {return 0;}
void audio_close() {}

//
//
// Video routines.
//
//

double get_time() {
	double curtime;
	if (vorbis_p) {
		curtime = 0.0;  //unimplemented
	} else {
		// initialize timer variable if not set yet
		if (!startticks)
		  startticks = SDL_GetTicks();
		curtime = 1.0e-3 * (double)(SDL_GetTicks() - startticks);
	}
	return curtime;
}

SDL_Texture* open_video(SDL_Renderer *renderer)
//Prepares an overlay for video display to screen.
{
	SDL_Texture *yuv_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
			SDL_TEXTUREACCESS_STREAMING, ti.frame_width, ti.frame_height);
	return yuv_texture;
}

bool video_write(SDL_Renderer *renderer, SDL_Texture *background, SDL_Texture *yuv_texture, const int x, const int y)
//Writes a frame of video to window
{
	yuv_buffer yuv;
	theora_decode_YUVout(&td,&yuv);

	Uint8* out_pixels_y;
	int out_pitch_y;
	UINT out_width = ti.frame_width;
	UINT out_height = ti.frame_height;

	// Lock SDL_yuv_overlay
	if (SDL_LockTexture(yuv_texture, NULL, (void**)&out_pixels_y, &out_pitch_y) < 0)
		return false;

	int out_pitch_uv = out_pitch_y / 2;
	Uint8* out_pixels_u = out_pixels_y + out_height * out_pitch_y;
	Uint8* out_pixels_v = out_pixels_u + out_height/2 * out_pitch_uv;

	// Draw the data (*yuv[3]) on an SDL screen (*screen)
	// deal with border stride
	// and crop input properly, respecting the encoded frame rect
	UINT i,j;
	int crop_offset=ti.offset_x+yuv.y_stride*ti.offset_y;
	Uint8 *pY = (Uint8*)yuv.y+crop_offset;

	//Luminance range is normalized to [16,235].  Denormalize to the [0,255] range.
	//Similarly, chromiance is in the range [16,239].  Denormalize to [0,255].
	//Prepare lookup tables of pre-converted and clamped results to minimize conversion overhead.
	static BYTE lumConversion[256], chrConversion[256];
	static bool bFirst = true;
	if (bFirst)
	{
		static const UINT MIN_LUM = 13, MAX_LUM = 235; //[13,235] works better for some reason
		static const UINT MIN_CHR = 16, MAX_CHR = 239;
		for (i=0; i<256; ++i)
		{
			lumConversion[i] = i <= MIN_LUM ? 0 : i >= MAX_LUM ? 255 : (i-MIN_LUM)*255/(MAX_LUM-MIN_LUM);
			chrConversion[i] = i <= MIN_CHR ? 0 : i >= MAX_CHR ? 255 : (i-MIN_CHR)*255/(MAX_CHR-MIN_CHR);
		}
		bFirst = false;
	}

	//Translate values instead of doing a straight copy.
	Uint8 *src, *dest;
	for (i = 0; i < out_height; ++i)
	{
		src = pY+yuv.y_stride*i;
		dest = out_pixels_y + out_pitch_y * i;
		for (j = 0; j < out_width; ++j)
			*(dest++) = lumConversion[*(src++)];
		//memcpy(dest, src, yuv_overlay->w);
	}

	crop_offset=(ti.offset_x/2)+(yuv.uv_stride)*(ti.offset_y/2);
	Uint8 *pU = (Uint8*) yuv.u+crop_offset, *pV = (Uint8*) yuv.v+crop_offset;
	const UINT hOver2 = out_height / 2;
	const UINT wOver2 = out_width / 2;
	Uint8 *src2, *dest2;
	for (i=0; i<hOver2; ++i)
	{
		src = pU+yuv.uv_stride*i;
		dest = out_pixels_u + out_pitch_uv * i;
		src2 = pV+yuv.uv_stride*i;
		dest2 = out_pixels_v + out_pitch_uv * i;
		for (j=0; j<wOver2; ++j)
		{
			*(dest++) = chrConversion[*(src++)];
			*(dest2++) = chrConversion[*(src2++)];
		}
		//memcpy(dest, src, wOver2);
		//memcpy(dest2, src2, wOver2);
	}

	// Unlock SDL_yuv_overlay
	SDL_UnlockTexture(yuv_texture);

	// Show frame to surface!
	SDL_Rect rect = MAKE_SDL_RECT(x, y, out_width, out_height);

	// Blit background first (the SDL_WINDOWEVENT_EXPOSED event isn't reliable at the moment)
	SDL_RenderClear(renderer); // clear border, if any
	if (!SDL_RenderCopy(renderer, background, NULL, NULL)
			&& !SDL_RenderCopy(renderer, yuv_texture, NULL, &rect))
	{
#ifdef STEAMBUILD
		SteamAPI_RunCallbacks();
#endif
		SDL_RenderPresent(renderer);
		return true;
	}

	//Error displaying.
	char text[512];
	text[sizeof(text)-1] = 0;
	_snprintf(text, sizeof(text)-1, "SDL: Couldn't render video (%i,%i): %s\n", rect.w, rect.h, SDL_GetError());
	LOGERR(text);
	return false;
}

//
//
// Display methods.
//
//

bool CScreen::PlayVideoBuffer(
//Plays specified OGG Theora file to window.
//If window == NULL, then this method will test that the file is playable
//by decoding it as fast as possible but not displaying anything.
//
//Returns: whether playback was successful
	CStretchyBuffer& buffer, SDL_Window *window,
	const int x, const int y) //[default=(0,0)]
{
	//init
	theora_p = vorbis_p = 0;
	startticks = 0;
	bool bSkippedLastFrame = false;

	// start up Ogg stream synchronization layer
	ogg_sync_init(&oy);

	// init supporting Vorbis structures needed in header parsing
	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);

	// init supporting Theora structures needed in header parsing
	theora_comment_init(&tc);
	theora_info_init(&ti);
	if (!window)
		ti.quick_p = 1;
	ti.quality = 63;

	if (!parseHeaders(buffer))
		return false;

	// force audio off
	vorbis_p = 0;

	// initialize decoders
	if (theora_p) {
		theora_decode_init(&td,&ti);
#if 0
		printf("Ogg logical stream %x is Theora %dx%d %.02f fps video\n"
			  "  Frame content is %dx%d with offset (%d,%d).\n",
			to.serialno,ti.width,ti.height, (double)ti.fps_numerator/ti.fps_denominator,
			ti.frame_width, ti.frame_height, ti.offset_x, ti.offset_y);
		//report_colorspace(&ti); //we're not using this info for anything
		dump_comments(&tc);
#endif
	} else {
		// tear down the partial theora setup
		theora_info_clear(&ti);
		theora_comment_clear(&tc);
	}
	if(vorbis_p) {
		vorbis_synthesis_init(&vd,&vi);
		vorbis_block_init(&vd,&vb);  
		printf("Ogg logical stream %lx is Vorbis %d channel %ld Hz audio.\n",
			vo.serialno,vi.channels,vi.rate);
	} else {
		// tear down the partial vorbis setup
		vorbis_info_clear(&vi);
		vorbis_comment_clear(&vc);
	}

	// open audio
	if (vorbis_p)
		open_audio();

	// open video
	SDL_Renderer *renderer;
	SDL_Texture *background;
	SDL_Texture *yuv_overlay = NULL;
	if (theora_p && window)
	{
		renderer = SDL_GetRenderer(window);
		background = GetWindowTexture(window);
		yuv_overlay = open_video(renderer);
	}
  
	// single frame video buffering
	ogg_packet op;
	ogg_int64_t  videobuf_granulepos=-1;
	double       videobuf_time=0;
	double last_frame_time = 0;
	bool hasdatatobuffer = true;

	// Main loop
	bool audiobuf_ready=false;
	bool videobuf_ready=false;
	bool playbackdone = (yuv_overlay == NULL);
	bool isPlaying = false;
	bool bBreakout = false;
	while (!playbackdone)
	{
		// break out on SDL quit event
		SDL_Event event;
		while (PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT: playbackdone = bBreakout = true; break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							playbackdone = bBreakout = true;
							break;
						case SDLK_RETURN:
							if (!(event.key.keysym.mod & KMOD_ALT))
								break;
							// fall through
						case SDLK_F10:
							ToggleScreenSize();
							break;
					}
				break;
				default: break;
			}
		}

		while (theora_p && !videobuf_ready) {
			// get one video packet...
			if (ogg_stream_packetout(&to,&op)>0)
			{
				theora_decode_packetin(&td,&op);

				videobuf_granulepos=td.granulepos;
				videobuf_time=theora_granule_time(&td,videobuf_granulepos);

#if 0
				//Without sound channels to synch to, don't need to worry about skipping frames when slow.
				// update the frame counter
				//++frameNum;

				// check if this frame time has not passed yet.
				//	If the frame is late we need to decode additional
				//	ones and keep looping, since theora at this stage
				//	needs to decode all frames.
				const double now=get_time();
				const double delay=videobuf_time-now;
				if(delay>=0.0){
					/// got a good frame, not late, ready to break out
					videobuf_ready=true;
				} else if(now-last_frame_time>=1.0) {
					// display at least one frame per second, regardless
					videobuf_ready=true;
				} else {
					//Need to catch up -- no time to display frame.
					if (bSkippedLastFrame) //only allow skipping one frame in a row
						videobuf_ready = true; //show anyway
					else
						bSkippedLastFrame = true;
					//printf("dropping frame %d (%.3fs behind)\n", frameNum, -delay);
				}
#else
				videobuf_ready = true; //show every frame
#endif
			} else {
				// need more data
				break;
			}
		}

		if (!hasdatatobuffer && !videobuf_ready && !audiobuf_ready) {
			isPlaying = false;
			playbackdone = true;
		}

		//If we're set for the next frame, sleep.
		//In other words, don't show frames too rapidly. 
		if((!theora_p || videobuf_ready) && 
			(!vorbis_p || audiobuf_ready))
		{
			const int ticks = (int)(1000*(videobuf_time-get_time()));
			if(ticks>0 && window) //don't need to sleep if only testing file
				SDL_Delay(ticks);
		}
 
		if (videobuf_ready)
		{
			// time to write our cached frame
			if (window)
			{
				const bool bRes = video_write(renderer, background, yuv_overlay, x, y);
				if (!bRes) //couldn't display image
					playbackdone = bBreakout = true;
			}
			videobuf_ready=false;
			last_frame_time=get_time();
			bSkippedLastFrame = false;

			// if audio has not started (first frame) then start it
			if ((!isPlaying)&&(vorbis_p)) {
				start_audio();
				isPlaying = true;
			}
		}

		// HACK: always look for more audio data
		audiobuf_ready=false;

		// buffer compressed data every loop
		if (hasdatatobuffer) {
			hasdatatobuffer = buffer_data(&oy, buffer) > 0;
			if (!hasdatatobuffer) {
				//printf("Ogg buffering stopped, end of file reached.\n");
			}
		}
    
		if (ogg_sync_pageout(&oy,&og)>0)
			queue_page(&og);

	} // playbackdone

	// show number of video frames decoded
	//printf("\nFrames decoded: %d\n", frameNum);

	// deinit
	if (vorbis_p) {
		audio_close();

		ogg_stream_clear(&vo);
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
		vorbis_comment_clear(&vc);
		vorbis_info_clear(&vi); 
	}
	if (theora_p) {
		if (yuv_overlay)
			SDL_DestroyTexture(yuv_overlay);

		ogg_stream_clear(&to);
		theora_clear(&td);
		theora_comment_clear(&tc);
		theora_info_clear(&ti);
	}
	ogg_sync_clear(&oy);

	//If broken out of testing, return false since entire file was not verified.
	return !bBreakout || window != NULL;
}
