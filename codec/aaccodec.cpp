
#include "aaccodec.h"

aaccodec::aaccodec() {}

aaccodec::~aaccodec() {}

//generate i-th frame of test audio
int aaccodec::generate_raw_frame(uint16_t* frame_data, int i, int sample_rate,
    int channels, int frame_size)
{
    int j, k;

    for (j = 0; j < frame_size; j++) {
        frame_data[channels * j] = 10000 * ((j / 10 * i) % 2);
        for (k = 1; k < channels; k++)
            frame_data[channels * j + k] = frame_data[channels * j] * (k + 1);
    }
    return 0;
}

int aaccodec::aacencodec(const char* ifname, const char* ofile)
{
	const AVCodec* c = NULL;
	AVCodecContext** cctx = NULL;
    AVPacket* pkt;
    AVFrame* ifr, * ofr;
    uint8_t* iraw = NULL, * oraw = NULL;
	const AVChannelLayout* ch_layout;
	int srate,ret = 0;
    int i = 0;
    int ifr_bytes, ofr_bytes;
	char buf[NAME_BUFF_SIZE];
    int sample_rates[] = { 8000, 44100, 48000, 192000 };

    c = avcodec_find_encoder(AV_CODEC_ID_FLAC);
    if (!c) {
        av_log(NULL, AV_LOG_ERROR, "Can't find encoder\n");
        return 1;
    }
	av_channel_layout_describe(ch_layout, buf, NAME_BUFF_SIZE);
	av_log(NULL, AV_LOG_INFO, "channel layout: %s, sample rate: %i\n", buf, srate);

    cctx = avcodec_alloc_context3(c);
    if (!cctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate encoder context\n");
        return AVERROR(ENOMEM);
    }

    cctx->sample_fmt = AV_SAMPLE_FMT_S16;
    cctx->sample_rate = srate;
    av_channel_layout_copy(&cctx->ch_layout, ch_layout);

    ret = avcodec_open2(cctx, c, NULL);
    if (ret < 0) {
        av_log(cctx, AV_LOG_ERROR, "Can't open encoder\n");
        return ret;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate output packet\n");
        return AVERROR(ENOMEM);
    }

    ifr = av_frame_alloc();
    if (!ifr) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate input frame\n");
        return AVERROR(ENOMEM);
    }

    ifr->nb_samples = cctx->frame_size;
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
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(cctx, pkt);
            if (ret == AVERROR(EAGAIN))
                break;
            else if (ret < 0 && ret != AVERROR_EOF) {
                av_log(NULL, AV_LOG_ERROR, "Error encoding audio frame\n");
                return ret;
            }
            
            if (ifr->format != ofr->format) {
                av_log(NULL, AV_LOG_ERROR, "Error frames before and after decoding has different sample format\n");
                return AVERROR_UNKNOWN;
            }
            ofr_bytes = ofr->nb_samples * ofr->ch_layout.nb_channels * sizeof(uint16_t);
            if (ofr_bytes > ofr->linesize[0]) {
                av_log(NULL, AV_LOG_ERROR, "Incorrect value of output frame linesize\n");
                return 1;
            }
            memcpy(oraw + out_offset, ofr->data[0], ofr_bytes);
            out_offset += ofr_bytes;
        }
    }
    if (memcmp(iraw, oraw, ofr_bytes * NUMBER_OF_AUDIO_FRAMES) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Output differs\n");
        return 1;
    }

    av_log(NULL, AV_LOG_INFO, "OK\n");

    av_freep(&iraw);
    av_freep(&oraw);
    av_packet_free(&pkt);
    av_frame_free(&ifr);
    av_frame_free(&ofr);
    return 0;
}

int aaccodec::aacdecedec(const char* ifname, const char* ofile)
{

}