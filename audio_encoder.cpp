#include "audio_encoder.h"

AudioEncoder::AudioEncoder()
{

}

AudioEncoder::~AudioEncoder()
{
    if (codec_ctx_) {
        avcodec_close(codec_ctx_);
    }
    DEBUG("!AudioEncoder() finish");
}

bool AudioEncoder::Init()
{
    DEBUG("AudioEncoder Init()");

    int ret = -1;
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        DEBUG("avcodec_find_encoder AV_CODEC_ID_AAC failed");
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        DEBUG("avcodec_alloc_context3 failed");
        return false;
    }

    //set codec arg
    codec_ctx_->bit_rate = bit_rate_;
    codec_ctx_->sample_rate = sample_rate_;
    codec_ctx_->channels = channels_;
    codec_ctx_->channel_layout = av_get_default_channel_layout(channels_);
    codec_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
    codec_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
    codec_ctx_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    //avcodec_open2 自动设置codec timebase
    ret = avcodec_open2(codec_ctx_, codec, NULL);
    if (ret < 0) {
        DEBUG("avcodec_open2 failed :%s", AVStrError(ret).c_str());
        return false;
    }

    return true;
}

bool AudioEncoder::Encode(AVFrame* frame, std::vector<AVPacket*>& pkts, int stream_index, int64_t time_base)
{
    int ret = -1;

    //转timebase
    if (frame) {
        frame->pts = av_rescale_q(frame->pts, AVRational{1, (int)time_base}, codec_ctx_->time_base);
//        DEBUG("before Encode frame->pts:%lld", frame->pts);
    }

    if (frame) {
        ret = avcodec_send_frame(codec_ctx_, frame);
    } else {
        ret = avcodec_send_frame(codec_ctx_, NULL);
    }
    if (ret != 0) {
        DEBUG("avcodec_send_frame failed :%s", AVStrError(ret).c_str());
        return false;
    }

    while (true) {
        AVPacket* pkt = av_packet_alloc();
        if (!pkt) {
            DEBUG("av_packet_alloc failed");
            ret = -1;
            break;
        }
        ret = avcodec_receive_packet(codec_ctx_, pkt);
        pkt->stream_index = stream_index;
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            ret = 0;
            break;
        } else if (ret != 0) {
            DEBUG("avcodec_receive_packet failed :%s", AVStrError(ret).c_str());
            break;
        }

//        DEBUG("after Encode pkt->pts:%lld", pkt->pts);
//        DEBUG("after Encode pkt->dts:%lld", pkt->dts);
        pkts.push_back(pkt);
    }

    //error check
    if (ret < 0) {
        for (auto pkt : pkts) {
            av_packet_free(&pkt);
        }
        pkts.clear();
        return false;
    }

    return true;
}

AVCodecContext *AudioEncoder::GetAVCodecContext()
{
    return codec_ctx_;
}
