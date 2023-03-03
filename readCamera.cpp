#include "collect.h"

collect::collect() {}
collect::~collect() {}

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
	const char* ifn = "dshow"; //input_format_name
	AVFormatContext* fctx = avformat_alloc_context();
	AVDictionary* d = NULL;
	av_dict_set(&d, "list_devices", "true", 0);
	const AVInputFormat* ifmt = av_find_input_format(ifn);
	printf("========Device Info=============\n");
	avformat_open_input(&fctx, "video=dummy", ifmt, &d);
	printf("================================\n");
}

//Show Dshow Device Option
int collect::show_dshow_device_option() {
	AVFormatContext* fctx = avformat_alloc_context();
	AVDictionary* d = NULL;
	av_dict_set(&d, "list_options", "true", 0);
	const AVInputFormat* ifmt = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&fctx, "video=Integrated Camera", ifmt, &d);
	printf("================================\n");
}

//Show VFW Device
int collect::show_vfw_device() {
	AVFormatContext* fctx = avformat_alloc_context();
	const AVInputFormat* ifmt = av_find_input_format("vfwcap");
	printf("========VFW Device Info======\n");
	avformat_open_input(&fctx, "list", ifmt, NULL);
	printf("=============================\n");
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

int collect::readCamera()
{
	AVFormatContext* fctx;
	int				i, idx;
	AVCodecContext* cctx;
	AVCodecParameters* par;
	const AVCodec* c;

	avformat_network_init();
	fctx = avformat_alloc_context();

	//Open File
	//char filepath[]="src01_480x272_22.h265";
	//avformat_open_input(&pFormatCtx,filepath,NULL,NULL)

	//Register Device
	avdevice_register_all();

	//Windows
#ifdef _WIN32

	//Show Dshow Device
	show_dshow_device();
	//Show Device Options
	show_dshow_device_option();
	//Show VFW Options
	show_vfw_device();

#if USE_DSHOW
	AVInputFormat* ifmt = av_find_input_format("dshow");
	//Set own video device's name
	if (avformat_open_input(&pFormatCtx, "video=Integrated Camera", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	const AVInputFormat* ifmt = av_find_input_format("vfwcap");
	if (avformat_open_input(&fctx, "0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif
#elif defined linux
	//Linux
	const AVInputFormat* ifmt = av_find_input_format("video4linux2");
	if (avformat_open_input(&fctx, "/dev/video0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	show_avfoundation_device();
	//Mac
	const AVInputFormat* ifmt = av_find_input_format("avfoundation");
	//Avfoundation
	//[video]:[audio]
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0) {
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
		printf("Couldn't find a video stream.\n");
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
	int screen_w = 0, screen_h = 0;
	screen_w = cctx->width;
	screen_h = cctx->height;
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
	//bmp = SDL_CreateYUVOverlay(cctx->width, cctx->height, SDL_YV12_OVERLAY, screen);
	SDL_Renderer* r = SDL_CreateRenderer(ws,-1,0);
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
	int uvpitch;
	AVPicture pict;
	pict.linesize[1] = uvpitch;
	pict.linesize[2] = uvpitch;
	size_t ysize = cctx->width * cctx->height;
	size_t uvsize = cctx->width * cctx->height / 4;
	Uint8* y = (Uint8*)malloc(ysize);
	Uint8* u = (Uint8*)malloc(uvsize);
	Uint8* v = (Uint8*)malloc(uvsize);
	if (!y || !u || !v) {
		cout << "Couldn't alloctate pixel buffer!" << endl;
		return -1;
	}
	SDL_UpdateYUVTexture(t, NULL, y,cctx->width,u,uvpitch,v,uvpitch);
	SDL_Thread* vtid = SDL_CreateThread(sfp_refresh_thread, NULL, opaque);
	//SDL_WM_SetCaption("Simplest FFmpeg Read Camera", NULL);
	SDL_SetWindowTitle(ws,"Simplest FFmpeg Read Camera");
	SDL_RenderClear(r);
	SDL_RenderCopy(r, t, NULL, NULL);
	SDL_RenderPresent(r);
	//Event Loop
	SDL_Event event;

	for (;;) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT) {
			//------------------------------
			if (av_read_frame(fctx, pkt) >= 0) {
				if (pkt->stream_index == videoindex) {
					ret = avcodec_decode_video2(cctx, fr, &got_picture, packet);
					if (ret < 0) {
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture) {
					//	SDL_LockYUVOverlay(bmp);
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

					//	SDL_UnlockYUVOverlay(bmp);
					//	SDL_DisplayYUVOverlay(bmp, &rect);

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

	SDL_DestroyTexture(t);
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(ws);
	SDL_Quit();

	//av_free(out_buffer);
	av_free(frYUV);
	avcodec_close(cctx);
	avformat_close_input(&fctx);

	return 0;

}
