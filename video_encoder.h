#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include "util.h"

class VideoEncoder
{
public:
    VideoEncoder();
    ~VideoEncoder();

    bool Init();
    bool Encode(AVFrame* frame, std::vector<AVPacket*>& pkts, int stream_index, int64_t time_base);

    AVCodecContext* GetAVCodecContext();

private:
    int bit_rate_ = 500 * 1024;
    int width_ = 1920;
    int height_ = 1080;
    int fps_ = 30;
    int gop_size_ = 30;
    int max_b_frames_ = 0;

    AVCodecContext* codec_ctx_ = NULL;
};

#endif // VIDEOENCODER_H
