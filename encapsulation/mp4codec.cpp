
#include "mp4codec.h"

int mp4decodec(const char* ifname, const char* ovfile, const char* oafile)
{
	AVOutputFormat* oafmt = NULL, * ovfmt = NULL;
	//£¨Input AVFormatContext and Output AVFormatContext£©
	AVFormatContext* ifctx = NULL, * oafctx = NULL, * ovfctx = NULL;
	AVCodecParameters* cpar;
	AVPacket pkt;
	int ret, i;
	int vindex = -1, aindex = -1;
	int frindex = 0;

	//	const char* in_filename = "cuc_ieschool.ts";//Input file URL
		//char *in_filename  = "cuc_ieschool.mkv";
	//	const char* out_filename_v = "cuc_ieschool.h264";//Output file URL
		//char *out_filename_a = "cuc_ieschool.mp3";
	//	const char* out_filename_a = "cuc_ieschool.aac";

//	av_register_all();

	//Input
	if ((ret = avformat_open_input(&ifctx, ifname, 0, 0)) != 0) {
		cout << "Could not open input mp4 file!" << endl;
		return -1;
	}
	if ((ret = avformat_find_stream_info(ifctx, 0)) < 0) {
		cout << "Failed to retrieve input mp4 stream information!" << endl;
		return -1;
	}

	//Output video context
	avformat_alloc_output_context2(&ovfctx, NULL, NULL, ovfile);
	if (!ovfctx) {
		cout << "Could not create output h264 context!" << endl;
		ret = AVERROR_UNKNOWN;
		return -1;
	}
	//ovfmt = ovfctx->oformat;
	//Output Audio context
	avformat_alloc_output_context2(&oafctx, NULL, NULL, oafile);
	if (!oafctx) {
		cout << "Could not create output AAC context!" << endl;
		ret = AVERROR_UNKNOWN;
		return -1;
	}
	//	oafmt = oafctx->oformat;

	for (i = 0; i < ifctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVFormatContext* ofctx;
		AVStream* istr = ifctx->streams[i];
		AVStream* ostr = NULL;

		if (ifctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			vindex = i;
			//ostr = avformat_new_stream(ovfctx, istr->codecpar->codec);
		//	ostr = avformat_new_stream(ovfctx, istr->codecpar->codec_id);
		//	ofctx = ovfctx;
			//Calculate Video total time
			if (istr->duration != AV_NOPTS_VALUE) {
				int vdur = (istr->duration) * av_q2d(istr->time_base);
				cout << "Video duration:" << vdur / 3600 << ","
					<< (vdur % 3600) / 60 << "," << vdur % 60 << endl;
			}
			else {
				cout << "Video duration unknow!" << endl;
			}
		}
		else if (ifctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			aindex = i;
			//	ostr = avformat_new_stream(oafctx, istr->codecpar->codec);
			//	ofctx = oafctx;
				//Calculate Audio total time
			if (istr->duration != AV_NOPTS_VALUE) {
				int adur = (istr->duration) * av_q2d(istr->time_base);
				cout << "Audio duration:" << adur / 3600 << ","
					<< (adur % 3600) / 60 << "," << adur % 60 << endl;
			}
			else {
				cout << "Audio duration unknow!" << endl;
			}
		}
		else {
			break;
		}
	}

/*		if (!ostr) {
			cout << "Failed allocating output stream!" << endl;
			ret = AVERROR_UNKNOWN;
			return -1;
		}
		//Copy the settings of AVCodecContext
		if (avcodec_copy_context(ostr->codecpar, istr->codecpar) < 0) {
			cout << "Failed to copy context from input to output stream codec context!" << endl;
			return -1;
		}
		ostr->codecpar->codec_tag = 0;

		if (ofctx->oformat->flags & AVFMT_GLOBALHEADER)
			ostr->codecpar->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}*/

	//Dump Format------------------
	printf("\n==============Input Video=============\n");
	av_dump_format(ifctx, 0, ifname, 0);
	printf("\n==============Output Video============\n");
	av_dump_format(ovfctx, 0, ovfile, 1);
	printf("\n==============Output Audio============\n");
	av_dump_format(oafctx, 0, oafile, 1);

	//Open output file
	if (!(ovfmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ovfctx->pb, ovfile, AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output h264 file '%s'!", ovfile);
			return -1;
		}
	}

	if (!(oafmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&oafctx->pb, oafile, AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output aac file '%s'!", oafile);
			return -1;
		}
	}

	//Write file header
	if (avformat_write_header(ovfctx, NULL) < 0) {
		cout << "Error occurred when opening h264 output file!" << endl;
		return -1;
	}
	if (avformat_write_header(oafctx, NULL) < 0) {
		cout << "Error occurred when opening aac output file!" << endl;
		return -1;
	}

#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif

	while (1) {
		AVFormatContext* ofctx;
		AVStream* istr, * ostr;
		//Get an AVPacket
		if (av_read_frame(ifctx, &pkt) < 0) {
			cout << "failed to read frame!" << endl;
			break;
		}
		istr = ifctx->streams[pkt.stream_index];


		if (pkt.stream_index == vindex) {
			ostr = ovfctx->streams[0];
			ofctx = ovfctx;
			printf("Write h264 Packet. size:%d\tpts:%lld.\n", pkt.size, pkt.pts);
			
#if USE_H264BSF
			av_bitstream_filter_filter(h264bsfc, istr->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
		}
		else if (pkt.stream_index == aindex) {
			ostr = oafctx->streams[0];
			ofctx = oafctx;
			printf("Write aac Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);
		}
		else {
			continue;
		}


		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, istr->time_base, ostr->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, istr->time_base, ostr->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, istr->time_base, ostr->time_base);
		pkt.pos = -1;
		pkt.stream_index = 0;
		//Write
		if (av_interleaved_write_frame(ofctx, &pkt) < 0) {
			cout << "Error muxing packet!" << endl;
			
			break;
		}
		//printf("Write %8d frames to output file.\n",frame_index);
		av_packet_unref(&pkt);
		frindex++;
	}

#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);
#endif

	//Write file trailer
	av_write_trailer(oafctx);
	av_write_trailer(ovfctx);

	avformat_close_input(&ifctx);
	/* close output */
	if (oafctx && !(oafmt->flags & AVFMT_NOFILE))
		avio_close(oafctx->pb);

	if (ovfctx && !(ovfmt->flags & AVFMT_NOFILE))
		avio_close(ovfctx->pb);

	avformat_free_context(oafctx);
	avformat_free_context(ovfctx);


	if (ret < 0 && ret != AVERROR_EOF) {
		cout << "Error occurred!" << endl;
		
		return -1;
	}

	system("pause");
	return 0;
}