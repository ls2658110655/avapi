#pragma once

#include <stdio.h>
#include <iostream>
#include <stdarg.h>

#ifdef _WIN32
//For Windows
extern "C"
{
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavcodec/avcodec.h>
//#include <libavfilter/avfilter.h>
#include <libavutil/samplefmt.h>
#include <libavutil/common.h>
}
#else
//For Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
};
#endif
#endif

using namespace std;

#define NUMBER_OF_AUDIO_FRAMES 200
#define NAME_BUFF_SIZE 100
#define __STDC_CONSTANT_MACROS
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define OUTPUT_PCM 1
#define USE_SDL 1

//Buffer:
static  uint8_t* audio_chunk;
static  uint32_t  audio_len;
static  uint8_t* audio_pos;

class aaccodec
{
public:
	aaccodec();
	~aaccodec();

	//H264 encodec
	int fencoder(AVFormatContext* fctx, unsigned int idx);
	int aacencodec(const char* ifname, const char* ofile);
	//Decoder h264
	void  fill_audio(void* udata, uint8_t* stream, int len);
	int aacdecodec(const char* ifname, const char* ofile);

private:

};