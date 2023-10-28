#ifndef PUSHER_H
#define PUSHER_H

#include "muxer.h"
#include "video_capture.h"
#include "audio_capture.h"
#include "video_encoder.h"
#include "audio_encoder.h"

#include "properties.h"

class Pusher
{
public:
    Pusher();
    ~Pusher();

    bool Init(const Properties& properties);
    bool Run();
    void Stop();

private:
    void VideoFrameHandler(AVFrame* frame);
    void AudioFrameHandler(AVFrame* frame);

private:
    AudioEncoder audio_encoder_;
    VideoEncoder video_encoder_;

    Muxer muxer_;

    AudioCapture audio_capture_;
    VideoCapture video_capture_;

    AVPublishTime& timer = AVPublishTime::GetInstance();
};

#endif // PUSHER_H
