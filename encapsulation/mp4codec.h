#pragma once

#include <stdio.h>
#include <iostream>
#include <stdarg.h>

using namespace std;

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
};
#endif
#endif

class mp4codec
{
public:
	//Demuxer mp4
	int mp4decodec(const char* ifname, const char* ovfile, const char* oafile);
	//Write Log txt
	int writeLog(FILE* pFile, const char* fmt, ...);

private:

};
