#include "util.h"
#include <thread>
#include <chrono>
#include <qglobal.h>

#define ERROR_STRING_SIZE 2048

bool ShowDevices()
{
    int ret = 0;
    const char* device_name = NULL;
    AVFormatContext* fmt_ctx = avformat_alloc_context();
    AVDictionary* opt = nullptr;
    AVInputFormat *in_fmt = nullptr;
    ret = av_dict_set(&opt, "list_devices", "true", 0);
    if (ret < 0) {
        DEBUG("av_dict_set field list_devices failed");
        goto failed;
    }

#ifdef Q_OS_WIN
    device_name = "dshow";
#endif

#ifdef Q_OS_MAC
    device_name = "avfoundation";
#endif

    in_fmt = (AVInputFormat*)av_find_input_format(device_name);
    if (!in_fmt) {
        DEBUG("av_find_input_format failed");
        goto failed;
    }

    //list_devices == true时,第二个参数无效
    DEBUG("======Show Devices INFO======");
    avformat_open_input(&fmt_ctx, NULL, in_fmt, &opt);
    DEBUG("======Show Devices INFO======");

    avformat_close_input(&fmt_ctx);
    return true;

 failed:
    avformat_close_input(&fmt_ctx);
    return false;
}


std::string AVStrError(int code)
{
    char errors[ERROR_STRING_SIZE];
    av_strerror(code, errors, ERROR_STRING_SIZE);
    return std::string(errors);
}
