
#include "h264codec.h"

h264codec::h264codec() {}
h264codec::~h264codec() {}

//Video encoder
int h264codec::fencoder( AVFormatContext* vfctx, unsigned int vsIndex)
{
	int vfrcnt = 0;
	AVCodecContext* cctx;
	AVFrame* vfr;
	AVPacket pkt;
	AVStream* vst;

	cout << "Flushing stream #%u encoder." << vsIndex << endl;

	//ret = encode_write_frame(nullptr,vsIndex,&got_frame);
	pkt.data = nullptr;
	pkt.size = 0;

	av_init_packet(&pkt);
	//Encode
	int ret = avcodec_send_frame(cctx, vfr);
	if (ret < 0) {
		cout << "Failed to sending a h264 frame to the encode!" << endl;
		return -1;
	}
	while (1) {
		ret = avcodec_receive_packet(cctx, &pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return true;

		else if (ret < 0) {
			cout << "Failed to encoding a h264 frame!" << endl;
			return -1;
		}
		printf("Succeed to encode h264 frame:%5d\tsize:%5d.\n", vfrcnt, pkt.size);
		//vfrcnt++;
		//Scale the PTS onto the time_base of the output stream.
		av_packet_rescale_ts(&pkt, cctx->time_base, vst->time_base);
		pkt.stream_index = vst->index;
		//Writte the Data onto output stream.
		ret = av_write_frame(vfctx, &pkt);
		//	ret = av_interleaved_write_frame(fctx, &pkt);
		av_packet_unref(&pkt);
		if (ret < 0) {
			cout << "Error while writing output packet!" << endl;
			return -1;
		}
		cout << "Succeed to encode 1 h264 frame." << endl;
	}
	return ret;
}

//YUV to H264:ffmpeg -s 1280x720 -i test.yuv -vcodec libx264 test.h264
int h264codec::h264encodec(const char* ifname, const char* ofile)
{
	AVFormatContext* fctx;
	AVOutputFormat* vofmt;
	AVCodecContext* cctx;
	const AVCodec* c;
	AVStream* vst;
	AVPacket pkt;
	AVFrame* vfr;
	uint8_t* buf;
	int vsize, ysize;
	int vfrcnt = 0;

	FILE* vifile = fopen(ifname, "rb");
	int iw = 640, ih = 480;
	int vfrnum = 1000000;

	//av_register_all();
	avformat_network_init();

#if 0 //Method1:
	fctx = avformat_alloc_context();
	//Guess format
	vofmt = av_guess_format(nullptr, ofile, nullptr);
	fctx->oformat = vofmt;

#else //Method2:
	avformat_alloc_output_context2(&fctx, nullptr, nullptr, ofile);
	//vofmt = fctx->oformat;
#endif

	//Open output URL
	if (avio_open(&fctx->pb, ofile, AVIO_FLAG_READ_WRITE) < 0) {
		cout << "Failed to open output h264 file!" << endl;
		return -1;
	}

	//Add video stream
	vst = avformat_new_stream(fctx, 0);
//	vst->id = 0;
	vst->time_base.num = 1;
	vst->time_base.den = 25;
	if (vst == nullptr) {
		cout << "Format new video stream failed!" << endl;
		return -1;
	}
	vst->codecpar->codec_tag = 0;
	avcodec_parameters_from_context(vst->codecpar, cctx);

	c = avcodec_find_encoder(cctx->codec_id);
	if (!c) {
		cout << "Can't find h264 encoder!" << endl;
		return -1;
	}
	cctx = avcodec_alloc_context3(c);
	if (!cctx) {
		cout << "Failed to alloc context for encoder context!" << endl;
		return -1;
	}

	//Param that must set
	//cctx->codec_id = AV_CODEC_ID_HEVC;
	cctx->codec_id = vofmt->video_codec;
	cctx->codec_type = AVMEDIA_TYPE_VIDEO;
	cctx->pix_fmt = AV_PIX_FMT_YUV420P;
	cctx->width = iw;
	cctx->height = ih;
	cctx->bit_rate = 400000; //4*1024*1024
	//    cctx->rc_max_rate = 400000; //cctx->bit_rate
	//    cctx->bit_rate_tolerance = 400000;
	//    cctx->rc_buffer_size = 400000;
	//    cctx->rc_initial_buffer_occupancy = 400000 * 3 / 4;
	cctx->gop_size = 250;
	cctx->time_base.num = 1;
	cctx->time_base.den = 25;
#if 1 //H264
	cctx->me_range = 16;
	cctx->max_qdiff = 4;
	cctx->qcompress = 0.6f;
#endif
	cctx->qmin = 10;
	cctx->qmax = 51;
	//Optional param
	cctx->max_b_frames = 3;
//	cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	//Set Option
	AVDictionary* par = 0;
	//H.264
	if (cctx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&par, "preset", "slow", 0);
		av_dict_set(&par, "tune", "zerolatency", 0);
		//av_dict_set(&par, "profile", "main", 0);
		//av_opt_set(cctx->priv_data, "tune", "zerolatency", 0);
	}
	//H.265
	if (cctx->codec_id == AV_CODEC_ID_HEVC) {
		av_dict_set(&par, "x265-params", "qp=20", 0);
		av_dict_set(&par, "preset", "ultrafast", 0);
		av_dict_set(&par, "tune", "zero-latency", 0);
	}

	//Show same information
	av_dump_format(fctx, 0, ofile, 1);

	
	if (avcodec_open2(cctx, c, &par) < 0) {
		cout << "Failed to open h264 encoder!" << endl;
		return -1;
	}

	//Creat YUV Format Frame 
	vfr = av_frame_alloc();
	vfr->format = cctx->pix_fmt; //AV_PIX_FMT_YUV420P;
	vfr->width = iw;  //cct->width;
	vfr->height = ih;  //cct->height
	vfr->pts = 0;
	//alloc buffer for YUV Frame
	if (av_frame_get_buffer(vfr, 0) < 0) {
		av_frame_free(&vfr);
		vfr = nullptr;
		cout << "Frame get buffer failed!" << endl;
		return -1;
	}
	//Write file header
	if (avformat_write_header(fctx, nullptr) < 0) {
		cout << "Write header failed!" << endl;
		return -1;
	}

	av_new_packet(&pkt, vsize);

	ysize = cctx->width * cctx->height;
	for (int i = 0; i < vfrnum; i++) {
		//Read raw YUV data;
		if (fread(buf, 1, ysize * 3 / 2, vifile) <= 0) {
			cout << "Failed to read YUV raw data!" << endl;
			return -1;
		}
		else if (feof(vifile)) {

			break;
		}
		vfr->data[0] = buf;             //Y
		vfr->data[1] = buf + ysize;     //U
		vfr->data[2] = buf + ysize * 5 / 4; //V
		//PTS
		//vfr->pts = i;
		vfr->pts = i * (vst->time_base.den) / ((vst->time_base.num) * 25);
		int got_pic = 0;
		//Encode
		int ret = avcodec_send_frame(cctx, vfr);
		if (ret < 0) {
			cout << "Failed to sending a h264 frame to the encode!" << endl;
			return -1;
		}
		while (1) {
			ret = avcodec_receive_packet(cctx, &pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				return 0;
			}
			else if(ret < 0) {
				cout << "Failed to encoding a h264 frame!" << endl;
				return -1;
			}
			printf("Succeed to encode h264 frame:%5d\tsize:%5d.\n", vfrcnt, pkt.size);
		//	vfrcnt++;
			//Scale the PTS onto the time_base of the output stream.
			av_packet_rescale_ts(&pkt, cctx->time_base, vst->time_base);
			pkt.stream_index = vst->index;
			//Writte the Data onto output stream.
			ret = av_write_frame(fctx, &pkt);
		//	ret = av_interleaved_write_frame(fctx, &pkt);
			av_packet_unref(&pkt);
			if (ret < 0) {
				cout << "Error while writing output packet!" << endl;
				return -1;
			}
		}
	}

	//Flush Encoder
	int fret = fencoder(fctx, 0);
	if (fret < 0) {
		cout << "Flushing h264 encoder failed!" << endl;
		return -1;
	}

	//Write file trailer
	av_write_trailer(fctx);

	//Clean
	/*if (vst) {
		avcodec_close(vst->codec);
		av_free(vfr);
		av_free(buf);
	}*/
	avio_close(fctx->pb);
	avformat_free_context(fctx);
//	av_packet_free(&pkt);
	av_frame_free(&vfr);
	avcodec_free_context(&cctx);

	fclose(vifile);

	return 0;
}

//Decoder h264:ffmpeg -i text.h264 -vcodec libx264 -an test.yuv
int h264codec::h264decedec(const char* ifname, const char* ofile)
{
	AVFormatContext* fctx = NULL;
	int i = 0, vindex, obufSize;
	AVCodecContext* cctx = NULL;
	AVCodecParameters* cpar = NULL;
	const AVCodec* c = NULL;
	AVFrame* fr = NULL;
	unsigned char* obuf = NULL;
	AVPacket* pkt;
//	int y_size, vst;
	int ret, wbts;
//	struct SwsContext* img_convert_ctx;

//	char filepath[] = "Titanic.mkv";
	FILE* fp_yuv = fopen(ofile, "wb+");

//	av_register_all();
	avformat_network_init();
	fctx = avformat_alloc_context();

	if (avformat_open_input(&fctx, ifname, NULL, NULL) != 0) {
		cout << "Couldn't open input h264 stream!" << endl;
		return -1;
	}
	if (avformat_find_stream_info(fctx, NULL) < 0) {
		cout << "Couldn't find h264 stream information!" << endl;
		return -1;
	}
	vindex = -1;
	for (i = 0; i < fctx->nb_streams; i++)
		if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			vindex = i;
			break;
		}

	if (vindex == -1) {
		cout << "Didn't find a h264 stream!" << endl;
		return -1;
	}

/*	vindex = av_find_best_stream(fctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (vindex < 0) {
		cout << "Can't find H264 stream in input file!" << endl;
		return -1;
	}*/
	cpar = fctx->streams[vindex]->codecpar;

	c = avcodec_find_decoder(cpar->codec_id);
	if (!c) {
		cout << "Can't find H264 Decoder!" << endl;
		return -1;
	}
	cctx = avcodec_alloc_context3(c);
	if (!cctx) {
		cout << "Can't allocate decoder context!" << endl;
		return AVERROR(ENOMEM);
	}
	//Init parser
/*	cps = av_parser_init(c->id);
	if (!cps) {
		cout << "Parser not found!" << endl;
		return -1;
	}
*/
	if (avcodec_parameters_to_context(cctx, cpar) < 0)
	{
		cout << "Failed to copy H264decoder context!" << endl;
		return -1;
	}
	if (avcodec_open2(cctx, c, NULL) < 0) {
		cout << "Could not open H264 Decoder!" << endl;
		return -1;
	}

	fr = av_frame_alloc();
	//frYUV = av_frame_alloc();
	//pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
	pkt = av_packet_alloc();
	obufSize = av_image_get_buffer_size(cctx->pix_fmt, cctx->width, cctx->height, 16);
	obuf = (unsigned char*)av_malloc(obufSize);
	if (!obuf) {
		av_log(NULL, AV_LOG_ERROR, "Can't allocate buffer!\n");
		return AVERROR(ENOMEM);
	}
	//av_image_fill_arrays(frYUV->data, frYUV->linesize, obuf,cctx->pix_fmt, cctx->width, cctx->height, 1);

	
	//Output Info-----------------------------
	av_dump_format(fctx, 0, ifname, 0);
//	printf("#tb %d: %d/%d\n", vindex, fctx->streams[vindex]->time_base.num, fctx->streams[vindex]->time_base.den);
	i = 0;
	ret = 0;
	while (ret >= 0) {
		ret = av_read_frame(fctx, pkt);
		if (ret >= 0 && pkt->stream_index == vindex) {
			av_packet_unref(pkt);
			continue;
		}
		if (ret < 0)
			ret = avcodec_send_packet(cctx, NULL);
		else {
			if (pkt->pts == AV_NOPTS_VALUE)
				pkt->pts = pkt->dts = i;
			ret = avcodec_send_packet(cctx, pkt);
		}
		av_packet_unref(pkt);
		while (ret >= 0) {
			ret = avcodec_receive_frame(cctx, fr);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				cout << "Succeed Decoder H264 frame." << endl;
				break;
			}
			else if (ret < 0) {
				cout << "Failed to receive_frame!" << endl;
				return ret;
			}

			/*y_size = cctx->width * cctx->height;
			fwrite(frYUV->data[0], 1, y_size, fp_yuv);    //Y
			fwrite(frYUV->data[1], 1, y_size / 4, fp_yuv);  //U
			fwrite(frYUV->data[2], 1, y_size / 4, fp_yuv);  //V
			cout << "Succeed to decode 1 h264 frame!" << endl;*/
			wbts = av_image_copy_to_buffer(obuf, obufSize,
				(const uint8_t* const*)fr->data, (const int*)fr->linesize,
				cctx->pix_fmt, cctx->width, cctx->height, 1);
			if (wbts < 0) {
				av_log(NULL, AV_LOG_ERROR, "Can't copy image to buffer!\n");
				av_frame_unref(fr);
				return wbts;
			}
			av_frame_unref(fr);
		}
		i++;	
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
/*	while (1) {
		ret = av_read_frame(fctx, pkt);
		if (ret >= 0 && pkt->stream_index == vindex) {
			av_packet_unref(pkt);
			continue;
		}
		if (ret < 0)
			ret = avcodec_send_packet(cctx, NULL);
		else {
			if (pkt->pts == AV_NOPTS_VALUE)
				pkt->pts = pkt->dts = i;
			ret = avcodec_send_packet(cctx, pkt);
		}
		av_packet_unref(pkt);
		while (ret >= 0) {
			ret = avcodec_receive_frame(cctx, fr);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				cout << "Succeed Decoder H264 frame." << endl;
				break;
			}
			else if (ret < 0) {
				cout << "Failed to receive_frame!" << endl;
				return ret;
			}

			
			wbts = av_image_copy_to_buffer(obuf, obufSize,
				(const uint8_t* const*)fr->data, (const int*)fr->linesize,
				cctx->pix_fmt, cctx->width, cctx->height, 1);
			if (wbts < 0) {
				av_log(NULL, AV_LOG_ERROR, "Can't copy image to buffer!\n");
				av_frame_unref(fr);
				return wbts;
			}
			av_frame_unref(fr);
		}
		i++
	}*/

//	fclose(fp_yuv);
	av_packet_free(&pkt);
	av_frame_free(&fr);
	avcodec_free_context(&cctx);
	av_freep(&obuf);
	avformat_close_input(&fctx);

	return 0;
}