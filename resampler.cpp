#include "resampler.h"

Resampler::Resampler()
{

}

Resampler::~Resampler()
{
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
    }

    if (audio_fifo_) {
        av_audio_fifo_free(audio_fifo_);
    }

    if (resample_data_) {
        av_freep(&resample_data_[0]);
    }
    av_freep(&resample_data_);
}

bool Resampler::Init(uint64_t src_channel_layout,
                     enum AVSampleFormat src_sample_fmt,
                     int src_sample_rate,
                     uint64_t dst_channel_layout,
                     enum AVSampleFormat dst_sample_fmt,
                     int dst_sample_rate)
{
    src_channel_layout_ = src_channel_layout;
    src_sample_fmt_ = src_sample_fmt;
    src_sample_rate_ = src_sample_rate;
    dst_channel_layout_ = dst_channel_layout;
    dst_sample_fmt_ = dst_sample_fmt;
    dst_sample_rate_ = dst_sample_rate;

    //设置通道数
    src_channels_ = av_get_channel_layout_nb_channels(src_channel_layout_);
    dst_channels_ = av_get_channel_layout_nb_channels(dst_channel_layout_);

    audio_fifo_ = av_audio_fifo_alloc(dst_sample_fmt_, dst_channels_, 1);
    if (!audio_fifo_) {
        DEBUG("av_audio_fifo_alloc failed");
        return false;
    }

    //检查输入输出格式
    if (src_sample_fmt_ == dst_sample_fmt_ &&
        src_channel_layout_ == dst_channel_layout_ &&
            src_sample_rate_ == dst_sample_rate_) {
        DEBUG("no resample need, just use audio fifo");
        is_fifo_only_ = true;
        return true;
    }

    is_fifo_only_ = false;

    //初始化重采样器
    swr_ctx_ = swr_alloc();
    if (!swr_ctx_) {
        DEBUG("swr_alloc failed");
        return false;
    }

    /* set options */
    av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt",      src_sample_fmt_, 0);
    av_opt_set_int(swr_ctx_,        "in_channel_layout",  src_channel_layout_, 0);
    av_opt_set_int(swr_ctx_,        "in_sample_rate",     src_sample_rate_, 0);
    av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt",     dst_sample_fmt_, 0);
    av_opt_set_int(swr_ctx_,        "out_channel_layout", dst_channel_layout_, 0);
    av_opt_set_int(swr_ctx_,        "out_sample_rate",    dst_sample_rate_, 0);

    /* initialize the resampling context */
    int ret = swr_init(swr_ctx_);
    if (ret < 0) {
        DEBUG("swr_init failed");
        return false;
    }

    resample_data_size_ = 8096;
    return AllocResampleData();
}

int Resampler::SendFrame(AVFrame* frame)
{
    int src_nb_samples = 0;
    uint8_t** src_data = NULL;
    if (frame) {
        src_nb_samples = frame->nb_samples;
        src_data = frame->extended_data;
        if (in_start_pts_ == AV_NOPTS_VALUE &&
            frame->pts != AV_NOPTS_VALUE) {
            ResetPts(frame->pts);
        }
        if (frame->pts - in_last_pts_ > (int64_t)(1.5*src_nb_samples)) {
            ResetPts(frame->pts);
        }
        in_last_pts_ = frame->pts;
    } else {
        is_flushed_ = true;
    }

    if (is_fifo_only_) {
        // 如果不需要做重采样，原封不动写入fifo
        return src_data ?  av_audio_fifo_write(audio_fifo_, (void**)src_data, src_nb_samples) : 0;
    }

    // 计算这次做重采样能够获取到的重采样后的点数
    int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx_, src_sample_rate_) + src_nb_samples,
                   dst_sample_rate_,
                   dst_sample_rate_,
                   AV_ROUND_UP);

    if (dst_nb_samples > resample_data_size_) {
        resample_data_size_ = dst_nb_samples;
        if (!AllocResampleData())
            return -1;
    }

    int nb_samples = swr_convert(swr_ctx_, resample_data_, dst_nb_samples, (const uint8_t**)src_data, src_nb_samples);
    int ret_size = av_audio_fifo_write(audio_fifo_, (void**)resample_data_, nb_samples);
    if (ret_size != nb_samples) {
        DEBUG("Warn：av_audio_fifo_write failed, expected_write:%d, actual_write:%d\n", nb_samples, ret_size)
    }
    return ret_size;
}

AVFrame *Resampler::RecvFrame(int nb_samples)
{
    nb_samples = nb_samples == 0 ? av_audio_fifo_size(audio_fifo_) : nb_samples;
    if (av_audio_fifo_size(audio_fifo_) < nb_samples || nb_samples == 0)
        return NULL;
    return GetOneFrame(nb_samples);
}

bool Resampler::AllocResampleData()
{
    if (resample_data_) {
        av_freep(&resample_data_[0]);
    }
    //av_freep即使传入NULL也是安全的
    av_freep(&resample_data_);
    int linesize = 0;
    int ret = av_samples_alloc_array_and_samples(&resample_data_,
                                       &linesize,
                                       dst_channels_,
                                       resample_data_size_,
                                       dst_sample_fmt_,
                                       0);
    if (ret < 0) {
        DEBUG("av_samples_alloc_array_and_samples failed");
        return false;
    }
    return true;
}

void Resampler::ResetPts(int64_t pts)
{
    DEBUG("ResetPts in_start_pts:%lld", pts);
    in_start_pts_ = pts;
    in_last_pts_ = pts;
    float scale = 1.0 * dst_sample_rate_ / src_sample_rate_;
    int64_t tmp_out_pts = (int64_t)(scale * pts);
    out_start_pts_ = tmp_out_pts;
    out_cur_pts_ = tmp_out_pts;
    DEBUG("ResetPts out_cur_pts:%lld", tmp_out_pts);
}

AVFrame *Resampler::GetOneFrame(int nb_samples)
{
    //alloc frame
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        DEBUG("av_frame_alloc failed");
        return NULL;
    }

    frame->nb_samples = nb_samples;
    frame->channels = dst_channels_;
    frame->channel_layout = dst_channel_layout_;
    frame->format = dst_sample_fmt_;
    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        DEBUG("av_frame_get_buffer failed");
        return NULL;
    }

    //read
    av_audio_fifo_read(audio_fifo_, (void**)frame->data, nb_samples);
    frame->pts = out_cur_pts_;
    out_cur_pts_ += nb_samples;
    return frame;
}
