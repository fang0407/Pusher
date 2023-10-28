#include "util.h"

#define ERROR_STRING_SIZE 2048

bool ShowDevices()
{
    int ret = 0;
    AVFormatContext* fmt_ctx = avformat_alloc_context();
    AVDictionary* opt = nullptr;
    AVInputFormat *in_fmt = nullptr;
    ret = av_dict_set(&opt, "list_devices", "true", 0);
    if (ret < 0) {
        std::cout << "av_dict_set field list_devices failed" << std::endl;
        goto failed;
    }

    in_fmt = (AVInputFormat*)av_find_input_format("avfoundation");
    if (!in_fmt) {
        std::cout << "av_find_input_format failed" << std::endl;
        goto failed;
    }

    //list_devices == true时,第二个参数无效
    std::cout << "======Show Devices INFO======" << std::endl;
    avformat_open_input(&fmt_ctx, NULL, in_fmt, &opt);
    std::cout << "======Show Devices INFO======" << std::endl;

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
