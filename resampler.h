#ifndef RESAMPLER_H
#define RESAMPLER_H

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/audio_fifo.h"
#include "libavutil/opt.h"
#include "libavutil/avutil.h"
#include "libavutil/fifo.h"
#include "libswresample/swresample.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

#include "util.h"

class Resampler
{
public:
    Resampler();

    ~Resampler();

    bool Init(uint64_t src_channel_layout,
              enum AVSampleFormat src_sample_fmt,
              int src_sample_rate,
              uint64_t dst_channel_layout,
              enum AVSampleFormat dst_sample_fmt,
              int dst_sample_rate);

    int SendFrame(AVFrame* frame);
    AVFrame* RecvFrame(int nb_samples);

private:
    bool AllocResampleData();
    void ResetPts(int64_t pts);
    AVFrame* GetOneFrame(int nb_samples);

private:
    uint64_t src_channel_layout_;
    enum AVSampleFormat src_sample_fmt_;
    int src_sample_rate_;
    uint64_t dst_channel_layout_;
    enum AVSampleFormat dst_sample_fmt_;
    int dst_sample_rate_;

    struct SwrContext* swr_ctx_ = NULL;

    int src_channels_;
    int dst_channels_;

    AVAudioFifo* audio_fifo_ = NULL;

    bool is_fifo_only_ = false;

    uint8_t** resample_data_ = NULL;
    int resample_data_size_;

    int64_t in_start_pts_ = AV_NOPTS_VALUE;
    int64_t in_last_pts_ = AV_NOPTS_VALUE;
    int64_t out_start_pts_ = AV_NOPTS_VALUE;
    int64_t out_cur_pts_ = AV_NOPTS_VALUE;

    bool is_flushed_ = false;
};

#endif // RESAMPLER_H
