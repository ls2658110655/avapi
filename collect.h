#pragma once

#include <stdio.h>
#include <iostream>
#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL.h>
#ifdef __cplusplus
};
#endif
#endif

using namespace std;

//Output YUV420P 
#define OUTPUT_YUV420P 0 
//'1' Use Dshow 
//'0' Use VFW
#define USE_DSHOW 0
//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

class collect
{
public:
	collect();
	~collect();

	int sfp_refresh_thread(int* opaque);
	int show_dshow_device();
	int show_dshow_device_option();
	int show_vfw_device();
	int show_avfoundation_device();
	int readCamera();
	int scapture();

private:
	int thread_exit = 0;

};