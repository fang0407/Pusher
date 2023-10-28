#include "video_encoder.h"

VideoEncoder::VideoEncoder()
{

}

VideoEncoder::~VideoEncoder()
{
    if (codec_ctx_) {
        avcodec_close(codec_ctx_);
    }
    DEBUG("!VideoEncoder() finish");
}

bool VideoEncoder::Init()
{
    DEBUG("VideoEncoder Init()");

    int ret = -1;
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        DEBUG("avcodec_find_encoder AV_CODEC_ID_H264 failed");
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        DEBUG("avcodec_alloc_context3 failed");
        return false;
    }

    //set codec arg
    codec_ctx_->bit_rate = bit_rate_;
    codec_ctx_->width = width_;
    codec_ctx_->height = height_;
    codec_ctx_->framerate.num = fps_;
    codec_ctx_->framerate.den = 1;
    codec_ctx_->time_base = {1, 1000};
    codec_ctx_->gop_size = fps_;
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx_->max_b_frames = max_b_frames_;

    ret = avcodec_open2(codec_ctx_, codec, NULL);
    if (ret < 0) {
        DEBUG("avcodec_open2 failed :%s", AVStrError(ret).c_str());
        return false;
    }

    return true;
}

bool VideoEncoder::Encode(AVFrame* frame, std::vector<AVPacket*>& pkts, int stream_index, int64_t time_base)
{
    int ret = -1;

    //转timebase
    if (frame) {
        frame->pts = av_rescale_q(frame->pts, AVRational{1, (int)time_base}, codec_ctx_->time_base);
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

AVCodecContext *VideoEncoder::GetAVCodecContext()
{
    return codec_ctx_;
}
