#ifndef AUDIOENCODER_H
#define AUDIOENCODER_H

#include <vector>
#include "util.h"

class AudioEncoder
{
public:
    AudioEncoder();
    ~AudioEncoder();

    bool Init();
    bool Encode(AVFrame* frame, std::vector<AVPacket*>& pkts, int stream_index, int64_t time_base);

    AVCodecContext* GetAVCodecContext();

private:
    int bit_rate_ = 128 * 1024;
    int channels_ = 2;
    int sample_rate_ = 48000;

    AVCodecContext* codec_ctx_ = NULL;
};

#endif // AUDIOENCODER_H
