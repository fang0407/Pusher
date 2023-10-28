#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <iostream>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavdevice/avdevice.h"

#ifdef __cplusplus
}
#endif

#define	DEBUG(format, ...)						\
do {											\
    printf("function:[%s] line:[%d] ", __func__, __LINE__);		\
    printf(format, ##__VA_ARGS__);				\
    printf("\n");								\
} while (0);



bool ShowDevices();
std::string AVStrError(int code);



#endif // UTIL_H
