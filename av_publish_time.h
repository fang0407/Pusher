#ifndef AVPUBLISHTIME_H
#define AVPUBLISHTIME_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <time.h>
#else
    #include <sys/time.h>
#endif

#include <mutex>

class AVPublishTime
{
public:
    static AVPublishTime& GetInstance();

    void Rest();
    int GetTimeBase();
    int64_t GetVideoPts();
    int64_t GetAudioPts();
    int64_t GetCurrentTimeMsec();

private:
    AVPublishTime();

private:
    int64_t start_time_;

    std::mutex mu_;
};

#endif // AVPUBLISHTIME_H
