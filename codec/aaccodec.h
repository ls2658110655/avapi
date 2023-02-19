#pragma once

#include <stdio.h>
#include <iostream>
#include <stdarg.h>

#ifdef _WIN32
//For Windows
extern "C"
{
#include <libavformat/avformat.h>
//#include <libswresample/swresample.h>
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

class aaccodec
{
public:
	aaccodec();
	~aaccodec();

	int generate_raw_frame(uint16_t* frame_data, int i, int sample_rate,
		int channels, int frame_size);
	//H264 encodec
	int fencoder(AVFormatContext* fctx, unsigned int idx);
	int aacencodec(const char* ifname, const char* ofile);
	//Decoder h264
	int aacdecedec(const char* ifname, const char* ofile);

private:

};