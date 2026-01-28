#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include<QImage>

//使用c去读取头文件
extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
}

class videoPlayer : public QThread
{
    Q_OBJECT

signals:
    void SIG_getoneImage(QImage img);//不能使用&引用
    //qt的多线程通信——使用信号槽进行
public:
    videoPlayer();

    void run();//虚函数

    void setFileName(const QString &newFileName);

private:
    QString m_fileName;
};

#endif // VIDEOPLAYER_H
