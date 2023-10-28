#include "pusher.h"


Pusher::Pusher()
{

}

Pusher::~Pusher()
{
    Stop();
}

bool Pusher::Init(const Properties& properties)
{
    avdevice_register_all();

    ShowDevices();

    bool ret = false;

    //init capture
    ret = video_capture_.Init();
    if (!ret) {
        DEBUG("video_capture_.Init() failed");
        return false;
    }

    video_capture_.SetFrameCob(std::bind(&Pusher::VideoFrameHandler, this, std::placeholders::_1));


    ret = audio_capture_.Init();
    if (!ret) {
        DEBUG("audio_capture_.Init() failed");
        return false;
    }

    audio_capture_.SetFrameCob(std::bind(&Pusher::AudioFrameHandler, this, std::placeholders::_1));

    //////////////////////////

    //init encoder
    ret = video_encoder_.Init();
    if (!ret) {
        DEBUG("video_encoder_.Init() failed");
        return false;
    }

    ret = audio_encoder_.Init();
    if (!ret) {
        DEBUG("audio_encoder_.Init() failed");
        return false;
    }

    //////////////////////////

    //init muxer
    ret = muxer_.Init(properties,
                      video_encoder_.GetAVCodecContext(),
                      audio_encoder_.GetAVCodecContext());
    if (!ret) {
        DEBUG("muxer_.Init() failed");
        return false;
    }

    return true;
}

bool Pusher::Run()
{
    int ret = -1;
    timer.Rest();
    //run
    ret = video_capture_.Run();
    if (!ret) {
        DEBUG("video_capture_.Run() failed");
        return false;
    }

    ret = audio_capture_.Run();
    if (!ret) {
        DEBUG("audio_capture_.Run() failed");
        return false;
    }

    ret = muxer_.Run();
    if (!ret) {
        DEBUG("muxer_.Run() failed");
        return false;
    }
    return true;
}

void Pusher::Stop()
{
    audio_capture_.Stop();
    video_capture_.Stop();
    muxer_.Stop();
}

void Pusher::VideoFrameHandler(AVFrame* frame)
{
    bool ret = false;
    int time_base = timer.GetTimeBase();
    int video_index = muxer_.GetVideoIndex();
    std::vector<AVPacket*> pkts;

//    DEBUG("video capture frame pts:%lld", frame->pts);
    ret = video_encoder_.Encode(frame, pkts, video_index, time_base);
    if (!ret) {
        DEBUG("video_encoder_.Encode failed");
        return;
    }

    auto delete_packet = [](AVPacket* pkt) {
        av_packet_free(&pkt);
    };

    //push to queue
    for (auto pkt: pkts) {
//        DEBUG("video encode packet pts:%lld", pkt->pts);
        std::shared_ptr<AVPacket> pkt_ptr(pkt, delete_packet);
        ret = muxer_.PushQueue(pkt_ptr, MediaType::VIDEO);
        if (!ret)
            DEBUG("muxer_.PushQueue pkt failed");
    }
}

void Pusher::AudioFrameHandler(AVFrame *frame)
{
    bool ret = false;
    int time_base = timer.GetTimeBase();
    int audio_index = muxer_.GetAudioIndex();
    std::vector<AVPacket*> pkts;

//    DEBUG("audio capture frame pts:%lld", frame->pts);
    ret = audio_encoder_.Encode(frame, pkts, audio_index, time_base);
    if (!ret) {
        DEBUG("audio_encoder_.Encode failed");
        return;
    }

    auto delete_packet = [](AVPacket* pkt) {
        av_packet_free(&pkt);
    };

    //push to queue
    for (auto pkt: pkts) {
//        DEBUG("audio encode packet pts:%lld", pkt->pts);
        std::shared_ptr<AVPacket> pkt_ptr(pkt, delete_packet);
        ret = muxer_.PushQueue(pkt_ptr, MediaType::AUDIO);
        if (!ret)
            DEBUG("muxer_.PushQueue pkt failed");
    }
}
