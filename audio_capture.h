#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <thread>
#include "util.h"
#include "av_publish_time.h"
#include "resampler.h"

class AudioCapture
{
public:
    using FrameFn = std::function<void (AVFrame*)>;

    AudioCapture();
    ~AudioCapture();

    bool Init();
    bool Run();
    void Stop();
    void SetFrameCob(const FrameFn& cob);

private:
    void Loop();

private:
    int audio_index_ = -1;
    AVFormatContext* fmt_ctx_ = NULL;
    AVCodecContext* codec_ctx_ = NULL;
    FrameFn audio_frame_callback_ = NULL;

    int convert_channels_ = 2;
    int convert_sample_rate_ = 48000;
    int convert_sample_fmt_ = (int)AV_SAMPLE_FMT_FLTP;
    int convert_nb_samples_ = 1024;

    std::atomic<bool> exit_ = true;
    std::thread looper_;
    FILE* pcm_flt_fp_ = NULL;

    Resampler resampler_;
};

#endif // AUDIOCAPTURE_H
