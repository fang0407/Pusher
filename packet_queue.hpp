#ifndef PACKET_QUEUE_HPP
#define PACKET_QUEUE_HPP

#include <memory>

#include "queue.hpp"
#include "util.h"

using namespace gcm::container;

enum MediaType
{
    AUDIO = 0,
    VIDEO,
};

struct MyAVPacket
{
    MyAVPacket(std::shared_ptr<AVPacket> pkt, MediaType media_type)
        : pkt_(pkt)
        , media_type_(media_type)
    {
    }

    std::shared_ptr<AVPacket> pkt_;
    MediaType media_type_;
};

struct PacketQueueStats
{
    int audio_nb_pkts_ = 0;
    int audio_size_ = 0;
    int64_t audio_duration_ = 0;

    int video_nb_pkts_ = 0;
    int video_size_ = 0;
    int64_t video_duration_ = 0;
};

class PacketQueue
{
public:
    using Queue = Queue<std::shared_ptr<MyAVPacket>>;
    using PredicateFn = Queue::PredicateFn;

    bool Push(std::shared_ptr<AVPacket> pkt, MediaType media_type)
    {
        if (!pkt) {
            DEBUG("push pkt is null");
            return false;
        }

        {
            //save front pts
            std::lock_guard<std::mutex> lck(mu_);
            if (media_type == MediaType::AUDIO) {
                if (is_audio_first_pkt_) {
                    is_audio_first_pkt_ = false;
                    audio_front_pts_ = pkt->pts;
                }
                audio_back_pts_ = pkt->pts;
                stats_.audio_nb_pkts_ += 1;
                stats_.audio_size_ += pkt->size;
            } else if (media_type == MediaType::VIDEO) {
                if (is_video_first_pkt_) {
                    is_video_first_pkt_ = false;
                    video_front_pts_ = pkt->pts;
                }
                video_back_pts_ = pkt->pts;
                stats_.video_nb_pkts_ += 1;
                stats_.video_size_ += pkt->size;
            } else {
                DEBUG("not support MediaType: %d", (int)media_type);
                return false;
            }

        }

        //save pkt
        std::shared_ptr<MyAVPacket> my_pkt = std::make_shared<MyAVPacket>(pkt, media_type);
        queue_.sync_emplace(my_pkt);
        return true;
    }

    bool PopTimeout(std::shared_ptr<AVPacket>& pkt, MediaType& media_type, int milliseconds)
    {
        std::shared_ptr<MyAVPacket> my_pkt;
        bool ret = queue_.sync_pop_for(my_pkt, milliseconds);
        //超时，并且队列为空
        if (!ret)
            return false;

        pkt = std::move(my_pkt->pkt_);
        media_type = my_pkt->media_type_;

        {
            std::lock_guard<std::mutex> lck(mu_);
            if (media_type == MediaType::VIDEO) {
                video_front_pts_ = pkt->pts;
                stats_.video_size_ -= pkt->size;
                stats_.video_nb_pkts_ -= 1;
            } else if (media_type == MediaType::AUDIO) {
                audio_front_pts_ = pkt->pts;
                stats_.audio_size_ -= pkt->size;
                stats_.audio_nb_pkts_ -= 1;
            } else {
                DEBUG("not support MediaType: %d", (int)media_type);
                return false;
            }
        }

        return true;
    }

    bool Empty()
    {
        return queue_.empty();
    }

    Queue::ContainerType* InsideQueue()
    {
        return queue_.queue();
    }

    PacketQueueStats GetStats()
    {
        std::lock_guard<std::mutex> lck(mu_);

        int64_t audio_duration = audio_back_pts_ - audio_front_pts_;
        if(audio_duration < 0 ||
            audio_duration >= audio_frame_duration_ * stats_.audio_nb_pkts_ * 2) {
            audio_duration = audio_frame_duration_ * stats_.audio_nb_pkts_;
        } else {
            audio_duration += audio_frame_duration_;
        }

        int64_t video_duration = video_back_pts_ - video_front_pts_;
        if(video_duration < 0 ||
            video_duration >= video_frame_duration_ * stats_.video_nb_pkts_ * 2) {
            video_duration = video_frame_duration_ * stats_.video_nb_pkts_;
        } else {
            video_duration += video_frame_duration_;
        }

        stats_.audio_duration_ = audio_duration;
        stats_.video_duration_ = video_duration;

        return stats_;
    }

private:
    Queue queue_;
    double audio_frame_duration_ = 1024.0;
    double video_frame_duration_ = 33.33 ;

    std::mutex mu_;
    bool is_audio_first_pkt_ = true;
    bool is_video_first_pkt_ = true;

    int64_t audio_front_pts_;
    int64_t video_front_pts_;

    int64_t audio_back_pts_;
    int64_t video_back_pts_;

    PacketQueueStats stats_;
};

#endif // PACKET_QUEUE_HPP
