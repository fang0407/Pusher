#include "main_window.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

#include <iostream>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->StartOrPauseBtn, &QPushButton::clicked, this, [this](){
        //check
        if (pusher_) {
            QMessageBox::warning(this, "warning", "推流已经启动");
            return;
        }

        //get push url
        std::string push_addr = ui->PushAddr->text().toUtf8().data();

        std::string proto_type = "";
        if (push_addr.find("rtmp://") == 0) {
            proto_type = "rtmp";
        } else if (push_addr.find("rtsp://") == 0) {
            proto_type = "rtsp";
        } else {
            QMessageBox::critical(this, "wrong", "推流地址格式不支持");
            return;
        }

        Properties properties;
        properties.SetProperty("push_addr", push_addr);
        properties.SetProperty("proto_type", proto_type);

        //启动推流
        bool ret = false;
        auto tmp_pusher = std::make_shared<Pusher>();
        ret = tmp_pusher->Init(properties);
        if (!ret) {
            QMessageBox::critical(this, "wrong", "初始化推流器失败");
            return;
        }

        ret = tmp_pusher->Run();
        if (!ret) {
            QMessageBox::critical(this, "wrong", "启动推流器失败");
            return;
        }

        pusher_ = tmp_pusher;
    });

    connect(ui->StopBtn, &QPushButton::clicked, this, [=](){
        if (pusher_) {
            pusher_->Stop();
            pusher_.reset();
            pusher_ = nullptr;
            QMessageBox::information(this, "informatino", "推流器停止成功");
        } else {
            QMessageBox::information(this, "informatino", "当前未处于推流状态");
        }
    });

    connect(ui->ActionAbort, &QAction::triggered, this, [=](){
        QMessageBox::about(this, "abort", "Demo Pusher create by chenjf");
    });

    connect(ui->ActionSetting, &QAction::triggered, this, [=](){
        QMessageBox::information(this, "informatino", "Default Setting");
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}
