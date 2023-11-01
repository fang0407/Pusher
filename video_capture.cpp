#include "video_capture.h"
#include <cstdio>
#include <iostream>
#include <qglobal.h>


VideoCapture::VideoCapture()
{
}

VideoCapture::~VideoCapture()
{
    Stop();

    if (looper_.joinable())
        looper_.join();

    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
    }

    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }

    if (img_convert_ctx_) {
        sws_freeContext(img_convert_ctx_);
    }

    if (convert_frame_) {
        av_frame_free(&convert_frame_);
    }

    if (capture_frame_) {
        av_frame_free(&capture_frame_);
    }

    DEBUG("~VideoCapture() finish");
}

bool VideoCapture::Init()
{
    DEBUG("VideoCapture Init()");

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
    ret = av_dict_set(&opt, "video_device_index", "0", 0);
    if (ret < 0) {
        DEBUG("av_dict_set field video_device_index failed");
        return false;
    }

    ret = av_dict_set(&opt, "framerate", "30", 0);
    if (ret < 0) {
        DEBUG("av_dict_set field framerate failed");
        return false;
    }

    in_fmt = (AVInputFormat*)av_find_input_format("avfoundation");
    if (!in_fmt) {
        DEBUG("av_find_input_format failed");
        return false;
    }
#endif

#ifdef Q_OS_WIN
    url = "video=screen-capture-recorder";
    ret = av_dict_set(&opt, "framerate", "25", 0);
    if (ret < 0) {
        DEBUG("av_dict_set field framerate failed");
        return false;
    }

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

    video_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_index_ < 0) {
        DEBUG("av_find_best_stream find AVMEDIA_TYPE_VIDEO failed");
        return false;
    }

    //init avcodec
    codec_ctx_ =  avcodec_alloc_context3(NULL);
    if (!codec_ctx_) {
        DEBUG("avcodec_alloc_context3 failed");
        return false;
    }

    avcodec_parameters_to_context(codec_ctx_, fmt_ctx_->streams[video_index_]->codecpar);
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

    //init SwsContext
    img_convert_ctx_ = sws_getContext(
        codec_ctx_->width, codec_ctx_->height, (enum AVPixelFormat)codec_ctx_->pix_fmt,
        convert_width_, convert_height_, (enum AVPixelFormat)convert_pix_fmt_,
        SWS_BICUBIC, NULL, NULL, NULL);
    if (!img_convert_ctx_) {
        DEBUG("sws_getContext failed");
        return false;
    }

    //alloc frame
    convert_frame_ = av_frame_alloc();
    if (!convert_frame_) {
        DEBUG("convert frame alloc failed");
        return false;
    }

    convert_frame_->width = convert_width_;
    convert_frame_->height = convert_height_;
    convert_frame_->format = convert_pix_fmt_;
    ret = av_frame_get_buffer(convert_frame_, 0);
    if (ret < 0) {
        DEBUG("av_frame_get_buffer failed");
        return false;
    }

    capture_frame_ = av_frame_alloc();
    if (!capture_frame_) {
        DEBUG("capture frame alloc failed");
        return false;
    }

    return true;
}

bool VideoCapture::Run()
{
    if (!video_frame_callback_) {
        DEBUG("video_frame_callback_ is null");
        return false;
    }

    if (!codec_ctx_) {
        DEBUG("codec_ctx_ is null");
        return false;
    }

    if (!img_convert_ctx_) {
        DEBUG("img_convert_ctx_ is null");
        return false;
    }

    exit_ = false;
    looper_ = std::thread(&VideoCapture::Loop, this);
    return true;
}

void VideoCapture::Stop()
{
    exit_ = true;
}

void VideoCapture::Loop()
{
    DEBUG("VideoCapture Loop()");
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int ret = -1;
    AVPacket pkt;
    AVPublishTime& timer = AVPublishTime::GetInstance();

    while (!exit_) {
        ret = av_read_frame(fmt_ctx_, &pkt);
        if (ret < 0) {
            DEBUG("av_read_frame failed: %s", AVStrError(ret).c_str());
            continue;
        }

        if (video_index_ == pkt.stream_index) {
            ret = avcodec_send_packet(codec_ctx_, &pkt);
            if (ret != 0) {
                DEBUG("avcodec_send_packet failed: %s", AVStrError(ret).c_str());
                av_packet_unref(&pkt);
                continue;
            }

            while (ret == 0) {
                ret = avcodec_receive_frame(codec_ctx_, capture_frame_);
                if (ret == 0) {

                    sws_scale(img_convert_ctx_,
                              (const uint8_t* const*)capture_frame_->data,
                              capture_frame_->linesize, 0, codec_ctx_->height,
                              convert_frame_->data, convert_frame_->linesize);

                    //set pts
                    int64_t pts = timer.GetVideoPts();
                    convert_frame_->pts = pts;

                    video_frame_callback_(convert_frame_);
                }
            }
        }

        av_packet_unref(&pkt);
    }
}

void VideoCapture::SetFrameCob(const FrameFn &cob)
{
    video_frame_callback_ = cob;
}
