
#include "aaccodec.h"

aaccodec::aaccodec() {}

aaccodec::~aaccodec() {}

int fencoder(AVFormatContext* fctx, unsigned int idx)
{
    AVCodecContext* cctx;
    AVFrame* ifr;
    AVStream* str;
    int ret;
    AVPacket pkt;
    if (!(fctx->streams[idx]->codecpar->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;
    while (1) {
        pkt.data = NULL;
        pkt.size = 0;
        av_init_packet(&pkt);
        //Encode
        //Send the frame for encoding
        ret = avcodec_send_frame(cctx, ifr);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error sending the frame to the encoder!\n");
            return -1;
        }
        //Read all the available output packets
        while (ret >= 0) {
            ret = avcodec_receive_packet(cctx, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return 0;
            }
            else if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error encoding audio frame!\n");
                return -1;
            }
            printf("Succeed to encode 1 frame! \tsize:%5d\n", pkt.size);
            pkt.stream_index = str->index;
            ret = av_write_frame(fctx, &pkt);
        }
        //av_packet_unref(&pkt);

        av_frame_free(NULL);
        if (ret < 0)
            break;
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fctx, &pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

int aaccodec::aacencodec(const char* ifname, const char* ofile)
{
    const AVCodec* c = NULL;
    AVCodecContext* cctx = NULL;
    AVCodecParameters* par = NULL;
    AVFormatContext* fctx = NULL;
    AVOutputFormat* ofmt = NULL;
    AVPacket pkt;
    AVFrame* ifr;
    AVStream* str;
    int idx = 0, ret = 0;
    int i,size = 0;
    uint8_t* buf;

    avformat_network_init();

    FILE* ifile = NULL;
    ifile = fopen(ifname, "rb"); 

    fctx = avformat_alloc_context();
    ofmt = av_guess_format(NULL, ofile, NULL);
    fctx->oformat = ofmt;

    //Open output URL
    if (avio_open(&fctx->pb, ofile, AVIO_FLAG_READ_WRITE) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open output file!\n");
        return -1;
    }

    str = avformat_new_stream(fctx, 0);
    if (str == NULL) {
        return -1;
    }

    //c = avcodec_find_encoder_by_name("libfdk_aac");
    c = avcodec_find_encoder(AV_CODEC_ID_AAC); 
    if (!c) {
        av_log(NULL, AV_LOG_ERROR, "Can not find AAC encoder!\n");
        return -1;
    }
    cctx = avcodec_alloc_context3(c);
    if (!cctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate audio codec context\n");
        return -1;
    }
    cctx->codec_id = ofmt->audio_codec;
    cctx->codec_type = AVMEDIA_TYPE_AUDIO;
    cctx->sample_fmt = AV_SAMPLE_FMT_S16;
    cctx->sample_rate = 44100; //48000
    cctx->channel_layout = AV_CH_LAYOUT_STEREO;
    cctx->channels = av_get_channel_layout_nb_channels(cctx->channel_layout);
    cctx->bit_rate = 64000;

    //Show some information
    av_dump_format(fctx, 0, ofile, 1);

    if (avcodec_open2(cctx, c, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open encoder!\n");
        return -1;
    }
    ifr = av_frame_alloc();
    ifr->nb_samples = cctx->frame_size;
    ifr->format = cctx->sample_fmt;

    size = av_samples_get_buffer_size(NULL, cctx->channels, cctx->frame_size, cctx->sample_fmt, 1);
    buf = (uint8_t*)av_malloc(size);
    avcodec_fill_audio_frame(ifr, cctx->channels, cctx->sample_fmt, (const uint8_t*)buf, size, 1);

    //Write Header
    avformat_write_header(fctx, NULL);

    av_new_packet(&pkt, size);
    for (i = 0; i < 1000; i++) {
        //Read PCM
        if (fread(buf, 1, size, ifile) <= 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to read PCM raw data!\n");
            return -1;
        }
        else if (feof(ifile)) {
            break;
        }
        ifr->data[0] = buf; //PCM Data
        ifr->pts = i * 100;
        
        //Encode
        //Send the frame for encoding
        ret = avcodec_send_frame(cctx, ifr);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error sending the frame to the encoder!\n");
            return -1;
        }
        //Read all the available output packets
        while (ret >= 0) {
            ret = avcodec_receive_packet(cctx, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return 0;
            }
            else if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error encoding audio frame!\n");
                return -1;
            }
            printf("Succeed to encode 1 frame! \tsize:%5d\n", pkt.size);
            pkt.stream_index = str->index;
            ret = av_write_frame(fctx, &pkt);
        }
        av_packet_unref(&pkt);
    }
    //Flush Encoder
    ret = fencoder(fctx, 0);
    if (ret < 0) {
        printf("Flushing encoder failed\n");
        return -1;
    }

    //Write Trailer
    av_write_trailer(fctx);

    av_frame_free(&ifr);
    avcodec_free_context(&cctx);
    free(buf);
    fclose(ifile);

    return 0;
}

//ffmpeg -i 111.aac -ar 48000 -ac 2 -f s16le 48000_2_s16le.pcm
int aaccodec::aacdecodec(const char* ifname, const char* ofile)
{
	const AVCodec* c = NULL;
	AVCodecContext* cctx = NULL;
    AVCodecParameters* par = NULL;
    AVFormatContext* fctx = NULL;
    AVPacket* pkt;
    AVFrame* ifr;
    int idx = 0, ret = 0;
    int str, i = 0;
    /*uint8_t* iraw = NULL, * oraw = NULL;
	const AVChannelLayout* ch_layout;
    int ifr_bytes, ofr_bytes;
	char buf[NAME_BUFF_SIZE];
    int sample_rates[] = { 8000, 44100, 48000, 192000 };*/

    avformat_network_init();
    
    fctx = avformat_alloc_context();
    //Open
    if (avformat_open_input(&fctx, ifname, NULL, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    // Retrieve stream information
    if (avformat_find_stream_info(fctx,NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    // Dump valid information onto standard error
    av_dump_format(fctx, 0, ifname, false);

    // Find the first audio stream
    str = -1;
    for (i = 0; i < fctx->nb_streams; i++)
        if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            str = i;
            break;
        }

    if (str == -1) {
        printf("Didn't find a audio stream.\n");
        return -1;
    }

    par = fctx->streams[str]->codecpar;

    c = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (c == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Can't find AAC decoder!\n");
        return 1;
    }
    cctx = avcodec_alloc_context3(c);
    if (!cctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate AAC decoder context!\n");
        return AVERROR(ENOMEM);
    }
/*	av_channel_layout_describe(ch_layout, buf, NAME_BUFF_SIZE);
	av_log(NULL, AV_LOG_INFO, "channel layout: %s, sample rate: %i\n", buf, srate);

    cctx->sample_fmt = AV_SAMPLE_FMT_S16;
    cctx->sample_rate = srate;
    av_channel_layout_copy(&cctx->ch_layout, ch_layout);*/

    if (avcodec_parameters_to_context(cctx, par) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy AACdecoder context!\n");
        return -1;
    }

    ret = avcodec_open2(cctx, c, NULL);
    if (ret < 0) {
        av_log(cctx, AV_LOG_ERROR, "Can't open encoder\n");
        return ret;
    }

    FILE* fpcm = NULL;
#if OUTPUT_PCM
    fpcm = fopen(ofile, "wb+");
#endif

    pkt = (AVPacket*)malloc(sizeof(AVPacket)); //av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate output packet\n");
        return AVERROR(ENOMEM);
    }
    av_init_packet(pkt);

    //Out Audio Param
    uint64_t ocl = AV_CH_LAYOUT_STEREO; //out_channel_layout
    int ospe = cctx->frame_size; //out_nb_samples: AAC:1024,MP3:1152
    AVSampleFormat osfmt = AV_SAMPLE_FMT_S16; //out_sample_fmt
    int osrate = 44100; //out_sample_rate
    int och = av_get_channel_layout_nb_channels(ocl); //out_channels
    int obufsize = av_samples_get_buffer_size(NULL, och, ospe, osfmt, 1);
    uint8_t* obuf = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

    ifr = av_frame_alloc();
    if (!ifr) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate input frame\n");
        return AVERROR(ENOMEM);
    }
    /*ifr->nb_samples = cctx->frame_size;
    ifr->format = cctx->sample_fmt;
    ret = av_channel_layout_copy(&ifr->ch_layout, &cctx->ch_layout);
    if (ret < 0)
        return ret;
    if (av_frame_get_buffer(ifr, 0) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate a buffer for input frame\n");
        return AVERROR(ENOMEM);
    }

    ofr = av_frame_alloc();
    if (!ofr) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate output frame\n");
        return AVERROR(ENOMEM);
    }

    iraw = av_malloc(ofr->linesize[0] * NUMBER_OF_AUDIO_FRAMES);
    if (!iraw) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate memory for raw_in\n");
        return AVERROR(ENOMEM);
    }

    oraw = av_malloc(ifr->linesize[0] * NUMBER_OF_AUDIO_FRAMES);
    if (!oraw) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate memory for raw_out\n");
        return AVERROR(ENOMEM);
    }

    for (i = 0; i < NUMBER_OF_AUDIO_FRAMES; i++) {
        ret = av_frame_make_writable(ifr);
        if (ret < 0)
            return ret;

        generate_raw_frame((uint16_t*)(ifr->data[0]), i, cctx->sample_rate,
            cctx->ch_layout.nb_channels, cctx->frame_size);
        ifr_bytes = ifr->nb_samples * ifr->ch_layout.nb_channels * sizeof(uint16_t);
        if (ifr_bytes > ifr->linesize[0]) {
            av_log(NULL, AV_LOG_ERROR, "Incorrect value of input frame linesize\n");
            return 1;
        }
        memcpy(iraw + in_offset, ifr->data[0], ifr_bytes);
        in_offset += ifr_bytes;
        ret = avcodec_send_frame(cctx, ifr);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error submitting a frame for encoding\n");
            return ret;
        }*/
    printf("Bitrate:\t %3d\n", fctx->bit_rate);
    printf("Decoder Name:\t %s\n", cctx->codec->long_name);
    printf("Channels:\t %d\n", cctx->channels);
    printf("Sample per Second\t %d \n", cctx->sample_rate);

    //FIX:Some Codec's Context Information is missing: in_channel_layout
    int64_t icl = av_get_default_channel_layout(cctx->channels);

    //Swr
    struct SwrContext* sctx;
    sctx = swr_alloc();
    sctx = swr_alloc_set_opts(sctx, ocl, osfmt, osrate,
        icl, cctx->sample_fmt, cctx->sample_rate, 0, NULL);
    swr_init(sctx);

    while (av_read_frame(fctx, pkt) >= 0) {
        if (pkt->stream_index == str) {
            ret = avcodec_send_packet(cctx, pkt);
            if (ret < 0 && ret != AVERROR_EOF) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding AAC audio frame!\n");
                return ret;
            }
            while (true) {
                ret = avcodec_receive_frame(cctx, ifr);
                if (ret >= 0) {
                    swr_convert(sctx, &obuf, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)ifr->data, ifr->nb_samples);

                    printf("index:%5d\t pts:%lld\t packet size:%d\n", idx, pkt->pts, pkt->size);

#if OUTPUT_PCM
                    //Write PCM
                    fwrite(obuf, 1, obufsize, fpcm);
#endif
                    idx++;
                    av_log(NULL, AV_LOG_ERROR, "Succeed decoding AAC audio frame.\n");
                    
                }
            }
        }
        av_packet_unref(pkt);
    }
    avformat_close_input(&fctx);
    avformat_free_context(fctx);
    avcodec_close(cctx);
    avcodec_free_context(&cctx);
    swr_free(&sctx);
    fclose(fpcm);
    return 0;
}

