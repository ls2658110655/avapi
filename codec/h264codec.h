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
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
//#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#else
//For Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
//#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
};
#endif
#endif

using namespace std;

#define OUTPUT_YUV420P 0
#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)
#define H264 0

class h264codec
{
public:
	h264codec();
	~h264codec();

	//H264 encodec
	int fencoder(AVFormatContext* vfctx, unsigned int vsIndex);
	int h264encodec(const char* ifname, const char* ofile);
	//Decoder h264
	int h264decedec(const char* ifname, const char* ofile);

private:

};