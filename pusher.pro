QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DemoPusher
TEMPLATE = app
CONFIG += console c++17

ICON = push.png

SOURCES += \
        audio_capture.cpp \
        audio_encoder.cpp \
        av_publish_time.cpp \
        main.cpp \
        main_window.cpp \
        muxer.cpp \
        pusher.cpp \
        resampler.cpp \
        util.cpp \
        video_capture.cpp \
        video_encoder.cpp

HEADERS += \
    audio_capture.h \
    audio_encoder.h \
    av_publish_time.h \
    main_window.h \
    muxer.h \
    packet_queue.hpp \
    properties.h \
    pusher.h \
    queue.hpp \
    resampler.h \
    util.h \
    video_capture.h \
    video_encoder.h


mac {
    INCLUDEPATH += "/usr/local/include/"
    LIBS += -L/usr/local/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswscale -lswresample
}

win64 {
    INCLUDEPATH += $$PWD/ffmpeg_install/include
    LIBS += $$PWD/ffmpeg_install/lib/avformat.lib \
    $$PWD/ffmpeg_install/lib/avcodec.lib \
    $$PWD/ffmpeg_install/lib/avdevice.lib \
    $$PWD/ffmpeg_install/lib/avfilter.lib \
    $$PWD/ffmpeg_install/lib/avutil.lib \
    $$PWD/ffmpeg_install/lib/swresample.lib \
    $$PWD/ffmpeg_install/lib/postproc.lib \
    $$PWD/ffmpeg_install/lib/swscale.lib
}

FORMS += \
    mainwindow.ui

macx: {
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_MACOSX_BUNDLE_INFO_PLIST = $QMAKE_INFO_PLIST
}

RESOURCES += \
    res.qrc
