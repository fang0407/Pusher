#ifndef MUXER_H
#define MUXER_H

#include <thread>
#include <atomic>

#include "util.h"
#include "properties.h"
#include "packet_queue.hpp"
#include "av_publish_time.h"

class Muxer
{
public:
    Muxer();
    ~Muxer();

    bool Init(const Properties& properties,
              AVCodecContext* codec_ctx1,
              AVCodecContext* codec_ctx2);
    bool Run();
    void Stop();

    bool PushQueue(std::shared_ptr<AVPacket> pkt, MediaType media_type);
    int GetVideoIndex();
    int GetAudioIndex();

private:
    bool Open();
    bool Close();
    bool AddStream(AVCodecContext* codec_ctx);
    bool Send(AVPacket* pkt);
    void DebugPacketStats();

    void Loop();

private:
    std::string url_;
    std::string proto_type_;

    AVCodecContext* video_codec_ctx_ = NULL;
    AVCodecContext* audio_codec_ctx_ = NULL;

    AVStream* video_st_ = NULL;
    AVStream* audio_st_ = NULL;

    int video_index_ = -1;
    int audio_index_ = -1;

    AVFormatContext* fmt_ctx_ = NULL;
    std::thread looper_;
    std::atomic<bool> exit_ = true;

    int64_t pre_debug_time_ = 0;
    PacketQueue pkt_queue_;
};

#endif // MUXER_H
