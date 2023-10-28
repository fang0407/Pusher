#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <thread>
#include "util.h"
#include "av_publish_time.h"

class VideoCapture
{
public:
    using FrameFn = std::function<void (AVFrame*)>;

    VideoCapture();
    ~VideoCapture();

    bool Init();
    bool Run();
    void Stop();
    void SetFrameCob(const FrameFn& cob);

private:
    void Loop();

private:
    int video_index_ = -1;
    AVFormatContext* fmt_ctx_ = NULL;
    AVCodecContext* codec_ctx_ = NULL;
    FrameFn video_frame_callback_ = NULL;
    struct SwsContext* img_convert_ctx_ = NULL;

    int convert_width_ = 1920;
    int convert_height_ = 1080;
    int convert_pix_fmt_ = (int)AV_PIX_FMT_YUV420P;

    /*
     * 这里遇到BUG，之前这样定义 AVFrame convert_frame;
     * 这种方式定义没有初始化AVFrame的成员值，会导致偶尔崩溃
     * 遇到的一个问题是编码avcodec_send_frame中访问AVFrame中的nb_side_data成员
     * 由于nb_side_data没有初始化，值可能不为0，可能导致崩溃
     * 对于音频的AVFrame也一样，只不过没有遇到崩溃，暂时没有修改
    */
    AVFrame* convert_frame_ = NULL;

    std::atomic<bool> exit_ = true;
    std::thread looper_;
};

#endif // VIDEOCAPTURE_H
