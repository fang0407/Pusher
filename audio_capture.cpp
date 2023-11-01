#include "audio_capture.h"
#include <iostream>
#include <memory>
#include <qglobal.h>

AudioCapture::AudioCapture()
{

}

AudioCapture::~AudioCapture()
{
    //结束标志
    Stop();

    if (looper_.joinable())
        looper_.join();

    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
    }

    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }

    if (pcm_flt_fp_)
        fclose(pcm_flt_fp_);

    if (capture_frame_)
        av_frame_free(&capture_frame_);

    DEBUG("~AudioCapture() finish");
}

bool AudioCapture::Init()
{
    DEBUG("AudioCapture Init()");

    int ret = -1;
    //init format context
    fmt_ctx_ = avformat_alloc_context();
    if (!fmt_ctx_) {
        DEBUG("avformat_alloc_context failed");
        return false;
    }

    AVDictionary* opt = nullptr;
    AVInputFormat *in_fmt = nullptr;
    const char* url = nullptr;

#ifdef Q_OS_MAC
    ret = av_dict_set(&opt, "audio_device_index", "0", 0);
    if (ret < 0) {
        DEBUG("av_dict_set field audio_device_index failed");
        return false;
    }

    in_fmt = (AVInputFormat*)av_find_input_format("avfoundation");
    if (!in_fmt) {
        DEBUG("av_find_input_format failed");
        return false;
    }
#endif

#ifdef Q_OS_WIN
    url = "audio=virtual-audio-capturer";

    in_fmt = (AVInputFormat*)av_find_input_format("dshow");
    if (!in_fmt) {
        DEBUG("av_find_input_format failed");
        return false;
    }
#endif

    ret = avformat_open_input(&fmt_ctx_, url, in_fmt, &opt);
    if (ret < 0) {
        DEBUG("avformat_open_input failed");
        return false;
    }

    audio_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audio_index_ < 0) {
        DEBUG("av_find_best_stream find AVMEDIA_TYPE_AUDIO failed");
        return false;
    }

    //init avcodec
    codec_ctx_ =  avcodec_alloc_context3(NULL);
    if (!codec_ctx_) {
        DEBUG("avcodec_alloc_context3 failed");
        return false;
    }

    avcodec_parameters_to_context(codec_ctx_, fmt_ctx_->streams[audio_index_]->codecpar);
    AVCodec* codec = avcodec_find_decoder(codec_ctx_->codec_id);
    if (!codec) {
        DEBUG("avcodec_find_decoder failed");
        return false;
    }

    ret = avcodec_open2(codec_ctx_, codec, NULL);
    if (ret < 0) {
        DEBUG("avcodec_open2 failed");
        return false;
    }

    //dshow not set channel_layout
    if (codec_ctx_->channel_layout == 0) {
        codec_ctx_->channel_layout = av_get_default_channel_layout(codec_ctx_->channels);
    }

    capture_frame_ = av_frame_alloc();
    if (!capture_frame_) {
        DEBUG("audio capture frame alloc failed");
        return false;
    }

    //resampler
    return resampler_.Init(codec_ctx_->channel_layout,
                    codec_ctx_->sample_fmt,
                    codec_ctx_->sample_rate,
                    av_get_default_channel_layout(convert_channels_),
                    (enum AVSampleFormat)convert_sample_fmt_,
                    convert_sample_rate_);
}

bool AudioCapture::Run()
{
    if (!audio_frame_callback_) {
        DEBUG("audio_frame_callback_ is null");
        return false;
    }

    if (!codec_ctx_) {
        DEBUG("codec_ctx_ is null");
        return false;
    }

    exit_ = false;
    looper_ = std::thread(&AudioCapture::Loop, this);
    return true;
}

void AudioCapture::Stop()
{
    exit_ = true;
}

void AudioCapture::Loop()
{
    DEBUG("AudioCapture Loop()");
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int ret = -1;
    AVPacket pkt;
    AVPublishTime& timer = AVPublishTime::GetInstance();
    int64_t pts = 0;

    while (!exit_) {
        ret = av_read_frame(fmt_ctx_, &pkt);
        if (ret < 0) {
            DEBUG("av_read_frame failed: %s", AVStrError(ret).c_str());
            continue;
        }

        if (audio_index_ == pkt.stream_index) {
            ret = avcodec_send_packet(codec_ctx_, &pkt);
            if (ret != 0) {
                DEBUG("avcodec_send_packet failed: %s", AVStrError(ret).c_str());
                av_packet_unref(&pkt);
                continue;
            }

            while (ret == 0) {
                ret = avcodec_receive_frame(codec_ctx_, capture_frame_);
                if (ret == 0) {

                    capture_frame_->pts = pts;
                    resampler_.SendFrame(capture_frame_);
                    pts += capture_frame_->nb_samples;

                    AVFrame* convert_frame = resampler_.RecvFrame(convert_nb_samples_);
                    std::shared_ptr<AVFrame> convert_frame_ptr(convert_frame, [](AVFrame* convert_frame){
                        av_frame_free(&convert_frame);
                    });

                    if (convert_frame) {
                        // set pts
                        int64_t pts = timer.GetAudioPts();
                        convert_frame->pts = pts;
                        audio_frame_callback_(convert_frame_ptr.get());


//#define __DEBUG__
#ifdef __DEBUG__
                        if(!pcm_flt_fp_) {
                            pcm_flt_fp_ = fopen("push_dump_flt.pcm", "wb");
                        }

                        if(pcm_flt_fp_) {
//                            ffplay -ar 48000 -channels 1 -f f32le  -i push_dump_flt.pcm
//                            双声道 data[0] data[1]，对于linsize，只有linsize[0]有设置值，都为采样点数x采样格式 单位字节
//                            fwrite(convert_frame->data[1], 1, convert_frame->linesize[0], pcm_flt_fp_);
//                            std::cout << convert_frame->linesize[0] << std::endl;
//                            fflush(pcm_flt_fp_);

                            //ffplay -ar 48000 -channels 2 -f f32le  -i push_dump_flt.pcm
                            int sample_size = av_get_bytes_per_sample((enum AVSampleFormat)convert_frame->format);
                            for (int i = 0; i < convert_frame->nb_samples; ++i) {
                                for (int j = 0; j < convert_frame->channels; ++j) {
                                    fwrite(convert_frame->data[j] + i * sample_size, 1, sample_size, pcm_flt_fp_);
                                }
                            }
                            fflush(pcm_flt_fp_);
                        }
#endif
                    }

                }
            }
        }

        av_packet_unref(&pkt);
    }
}

void AudioCapture::SetFrameCob(const FrameFn &cob)
{
    audio_frame_callback_ = cob;
}
