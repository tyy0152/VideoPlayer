#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include<QImage>
#include<thread>
#include "packetqueue.h"

//使用c去读取头文件
extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
    #include "libavutil/time.h"
}

class videoPlayer : public QThread
{
    Q_OBJECT

signals:
    void SIG_getoneImage(QImage img);//不能使用&引用
    //qt的多线程通信——使用信号槽进行
public:
    videoPlayer();
    ~videoPlayer();

    void run();//虚函数

    void setFileName(const QString &newFileName);

    //生产者线程
   //用于读取视频流的函数，运行在子线程里
    void readStream();

private:
    QString m_fileName;

    PacketQueue m_queue;
    std::thread* m_readThread;
    bool m_isStop;
};

#endif // VIDEOPLAYER_H
