#include "muxer.h"

Muxer::Muxer()
{

}

Muxer::~Muxer()
{
    Stop();

    if (looper_.joinable()) {
        looper_.join();
    }

    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
    }
    DEBUG("~Muxer() finish");
}

bool Muxer::Init(
    const Properties& properties,
    AVCodecContext* codec_ctx1,
    AVCodecContext* codec_ctx2)
{
    DEBUG("Muxer Init()");

    url_ = properties.GetProperty("push_addr");
    proto_type_ = properties.GetProperty("proto_type");

    int ret = -1;
    if (proto_type_ == "rtsp") {
        ret = avformat_alloc_output_context2(&fmt_ctx_, NULL, "rtsp", url_.c_str());
    } else if (proto_type_ == "rtmp") {
        ret = avformat_alloc_output_context2(&fmt_ctx_, NULL, "flv", url_.c_str());
    } else {
        DEBUG("not support proto_type: %s", proto_type_.c_str());
        return false;
    }

    if (ret < 0) {
        DEBUG("avformat_alloc_output_context2 failed: %s", AVStrError(ret).c_str());
        return false;
    }

    if (!AddStream(codec_ctx1) || !AddStream(codec_ctx2)) {
        DEBUG("AddStream() failed");
        return false;
    }

    if (!Open()) {
        DEBUG("Open() failed");
        return false;
    }
    return true;
}

bool Muxer::Open()
{
    if (!fmt_ctx_) {
        DEBUG("AVFormatContext is null");
        return false;
    }

    int ret = -1;

    if (proto_type_ == "rtmp") {
        ret = avio_open(&fmt_ctx_->pb, url_.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            DEBUG("avio_open failed :%s", AVStrError(ret).c_str());
            return false;
        }
    }


    ret = avformat_write_header(fmt_ctx_, NULL);
    if (ret < 0) {
        DEBUG("avformat_write_header failed :%s", AVStrError(ret).c_str());
        return false;
    }

    return true;
}

bool Muxer::Close()
{
    if (!fmt_ctx_) {
        DEBUG("AVFormatContext is null");
        return false;
    }

    int ret = -1;
    ret = av_write_trailer(fmt_ctx_);
    if (ret < 0) {
        DEBUG("av_write_trailer failed :%s", AVStrError(ret).c_str());
        return false;
    }
    return true;
}

bool Muxer::Send(AVPacket *pkt)
{
    int ret = -1;
    if (!pkt || pkt->size <= 0 || !pkt->data) {
        DEBUG("AVPacket is null");
        return false;
    }

    AVRational codec_time_base;
    AVRational stream_time_base;
    if (pkt->stream_index == video_index_) {
        codec_time_base = video_codec_ctx_->time_base;
        stream_time_base = video_st_->time_base;
    } else if (pkt->stream_index == audio_index_) {
        codec_time_base = audio_codec_ctx_->time_base;
        stream_time_base = audio_st_->time_base;
    }

    av_packet_rescale_ts(pkt, codec_time_base, stream_time_base);
//    DEBUG("stream index:%d, pts%lld", pkt->stream_index, pkt->pts);

    ret = av_interleaved_write_frame(fmt_ctx_, pkt);
    if (ret < 0) {
        DEBUG("av_interleaved_write_frame failed :%s", AVStrError(ret).c_str());
        return false;
    }

    return true;
}

void Muxer::DebugPacketStats()
{
    int64_t cur_time = AVPublishTime::GetInstance().GetCurrentTimeMsec();
    if(cur_time - pre_debug_time_ > 2000) {
        // 打印信息
        PacketQueueStats stats = pkt_queue_.GetStats();
        DEBUG("{duration}:a-%lld, v-%lld; {nb}:a-%d, v-%d",
              stats.audio_duration_, stats.video_duration_,
              stats.audio_nb_pkts_, stats.video_nb_pkts_);
        pre_debug_time_ = cur_time;
    }
}

bool Muxer::Run()
{
    if (!fmt_ctx_) {
        DEBUG("AVFormatContext is null");
        return false;
    }

    if (!video_codec_ctx_ || !audio_codec_ctx_) {
        DEBUG("AVCodecContext is null");
        return false;
    }

    if (!video_st_ || !audio_st_) {
        DEBUG("AVStream is null");
        return false;
    }

    exit_ = false;
    looper_ = std::thread(&Muxer::Loop, this);
    return true;
}

void Muxer::Stop()
{
    exit_ = true;
}

bool Muxer::PushQueue(std::shared_ptr<AVPacket> pkt, MediaType media_type)
{
    return pkt_queue_.Push(pkt, media_type);
}

bool Muxer::AddStream(AVCodecContext* codec_ctx)
{
    DEBUG("Muxer AddStream()");
    if (!codec_ctx) {
        DEBUG("AVCodecContext is null");
        return false;
    }

    if (!fmt_ctx_) {
        DEBUG("AVFormatContext is null");
        return false;
    }

    AVStream* st = avformat_new_stream(fmt_ctx_, NULL);
    if (!st) {
        DEBUG("avformat_new_stream failed");
        return false;
    }

    avcodec_parameters_from_context(st->codecpar, codec_ctx);

    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_codec_ctx_ = codec_ctx;
        video_st_ = st;
        video_index_ = st->index;
    } else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_codec_ctx_ = codec_ctx;
        audio_st_ = st;
        audio_index_ = st->index;
    }

    return true;
}

int Muxer::GetVideoIndex()
{
    return video_index_;
}

int Muxer::GetAudioIndex()
{
    return audio_index_;
}

void Muxer::Loop()
{
    DEBUG("Muxer::Loop()");

    bool ret = false;
    while (!exit_) {
        std::shared_ptr<AVPacket> pkt;
        MediaType media_type;
        DebugPacketStats();
        ret = pkt_queue_.PopTimeout(pkt, media_type, 100);
        if (exit_)
            break;

        if (!ret)
            continue;

        ret = Send(pkt.get());
        if (!ret) {
            DEBUG("Muxer Send() failed");
            continue;
        }
    }

    ret = Close();
    if (!ret) {
        DEBUG("Muxer Close() failed");
    }
}
