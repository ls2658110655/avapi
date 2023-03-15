
#include "collect.h"

int collect::sfp_refresh_thread(int* opaque)
{
    thread_exit = 0;
    while (!thread_exit) {
        SDL_Event event;
        event.type = SFM_REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
    thread_exit = 0;
    //Break
    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);

    return 0;
}

//Show Dshow Device
int collect::show_dshow_device() {
    AVFormatContext* fctx = avformat_alloc_context();
    AVDictionary* d = NULL;
    av_dict_set(&d, "list_devices", "true", 0);
    const AVInputFormat* ifmt = av_find_input_format("dshow");
    printf("========Device Info=============\n");
    avformat_open_input(&fctx, "video=dummy", ifmt, &d);
    printf("================================\n");
}

//Show AVFoundation Device
int collect::show_avfoundation_device() {
    AVFormatContext* fctx = avformat_alloc_context();
    AVDictionary* d = NULL;
    av_dict_set(&d, "list_devices", "true", 0);
    const AVInputFormat* ifmt = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&fctx, "", ifmt, &d);
    printf("=============================\n");
}

int collect::scapture()
{
	AVFormatContext* fctx;
	int				i, idx;
	AVCodecContext* cctx;
	AVCodecParameters* par;
	const AVCodec* c;

	//av_register_all();
	avformat_network_init();
	fctx = avformat_alloc_context();

	//Open File
	//char filepath[]="src01_480x272_22.h265";
	//avformat_open_input(&pFormatCtx,filepath,NULL,NULL)

	//Register Device
	avdevice_register_all();
	//Windows
#ifdef _WIN32
#if USE_DSHOW
	//Use dshow
	//
	//Need to Install screen-capture-recorder
	//screen-capture-recorder
	//Website: http://sourceforge.net/projects/screencapturer/
	//
	const AVInputFormat* ifmt = av_find_input_format("dshow");
	if (avformat_open_input(&fctx, "video=screen-capture-recorder", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	//Use gdigrab
	AVDictionary* d = NULL;
	//Set some options
	//grabbing frame rate
	//av_dict_set(&options,"framerate","5",0);
	//The distance from the left edge of the screen or desktop
	//av_dict_set(&options,"offset_x","20",0);
	//The distance from the top edge of the screen or desktop
	//av_dict_set(&options,"offset_y","40",0);
	//Video frame size. The default is to capture the full screen
	//av_dict_set(&options,"video_size","640x480",0);
	const AVInputFormat* ifmt = av_find_input_format("gdigrab");
	if (avformat_open_input(&fctx, "desktop", ifmt, &d) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

#endif
#elif defined linux
	//Linux
	AVDictionary* d = NULL;
	//Set some options
	//grabbing frame rate
	//av_dict_set(&d,"framerate","5",0);
	//Make the grabbed area follow the mouse
	//av_dict_set(&d,"follow_mouse","centered",0);
	//Video frame size. The default is to capture the full screen
	//av_dict_set(&d,"video_size","640x480",0);
	AVInputFormat* ifmt = av_find_input_format("x11grab");
	//Grab at position 10,20
	if (avformat_open_input(&fctx, ":0.0+10,20", ifmt, &d) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	show_avfoundation_device();
	//Mac
	const AVInputFormat* ifmt = av_find_input_format("avfoundation");
	//Avfoundation
	//[video]:[audio]
	if (avformat_open_input(&fctx, "1", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif

	if (avformat_find_stream_info(fctx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	idx = -1;
	for (i = 0; i < fctx->nb_streams; i++)
		if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			idx = i;
			break;
		}
	if (idx == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	//cctx = fctx->streams[idx]->codecpar;
	par = fctx->streams[idx]->codecpar;
	c = avcodec_find_decoder(cctx->codec_id);
	if (c == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_parameters_to_context(cctx, par) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder context!\n");
		return -1;
	}
	if (avcodec_open2(cctx, c, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	AVFrame* fr, * frYUV;
	fr = av_frame_alloc();
	frYUV = av_frame_alloc();
	//unsigned char *out_buffer=(unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	//SDL----------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	int screen_w = 640, screen_h = 360;
	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	//Half of the Desktop's width and height.
	screen_w = vi->current_w / 2;
	screen_h = vi->current_h / 2;
	//SDL_Surface* screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);
	SDL_Window* ws = SDL_CreateWindow("read Camera",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640, 480,
		SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
	if (!ws) {
		printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
		return -1;
	}
	//SDL_Overlay* bmp;
	//bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);
	SDL_Renderer* r = SDL_CreateRenderer(ws, -1, 0);
	if (!r) {
		cout << "Failed to create SDL Readerer!" << endl;
		return -1;
	}
	SDL_Texture* t = SDL_CreateTexture(r,
		SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING,
		cctx->width, cctx->height);
	if (!t) {
		cout << "SDL: Couldn't create texture - exiting!" << endl;
		return -1;
	}
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	//SDL End------------------------
	int ret, got_picture;

	AVPacket* pkt = (AVPacket*)av_malloc(sizeof(AVPacket));

#if OUTPUT_YUV420P 
	FILE* fp_yuv = fopen("output.yuv", "wb+");
#endif  

	struct SwsContext* img_convert_ctx;
	img_convert_ctx = sws_getContext(cctx->width, cctx->height, cctx->pix_fmt, cctx->width, cctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	//------------------------------
	SDL_Thread* video_tid = SDL_CreateThread(sfp_refresh_thread, NULL);
	//SDL_WM_SetCaption("Simplest FFmpeg Grab Desktop", NULL);
	SDL_SetWindowTitle(ws, "Simplest FFmpeg screen capture");
	//Event Loop
	SDL_Event event;

	for (;;) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT) {
			//------------------------------
			if (av_read_frame(fctx, pkt) >= 0) {
				if (pkt->stream_index == idx) {
					ret = avcodec_decode_video2(cctx, fr, &got_picture, pkt);
					if (ret < 0) {
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture) {
						SDL_LockYUVOverlay(bmp);
						frYUV->data[0] = bmp->pixels[0];
						frYUV->data[1] = bmp->pixels[2];
						frYUV->data[2] = bmp->pixels[1];
						frYUV->linesize[0] = bmp->pitches[0];
						frYUV->linesize[1] = bmp->pitches[2];
						frYUV->linesize[2] = bmp->pitches[1];
						sws_scale(img_convert_ctx, (const unsigned char* const*)fr->data, fr->linesize, 0, cctx->height, frYUV->data, frYUV->linesize);

#if OUTPUT_YUV420P  
						int y_size = cctx->width * cctx->height;
						fwrite(frYUV->data[0], 1, y_size, fp_yuv);    //Y   
						fwrite(frYUV->data[1], 1, y_size / 4, fp_yuv);  //U  
						fwrite(frYUV->data[2], 1, y_size / 4, fp_yuv);  //V  
#endif  
						SDL_UnlockYUVOverlay(bmp);

						SDL_DisplayYUVOverlay(bmp, &rect);

					}
				}
				av_packet_unref(pkt);
			}
			else {
				//Exit Thread
				thread_exit = 1;
			}
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT) {
			break;
		}

	}


	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P 
	fclose(fp_yuv);
#endif 

	SDL_Quit();

	//av_free(out_buffer);
	av_free(frYUV);
	avcodec_close(cctx);
	avformat_close_input(&fctx);

	return 0;

}