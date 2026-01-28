#include "playerdialog.h"

#include <QApplication>
#include<iostream>
using namespace std;

//// 必须加 extern "C"，因为 FFmpeg 是 C 语言写的
//extern "C" {
//    #include "libavcodec/avcodec.h"
//    #include "libavformat/avformat.h"
//    #include "libswscale/swscale.h"
//    #include "libavdevice/avdevice.h"
//}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    std::cout<<"Hello FFmpeg: "<<endl;
    av_register_all();
    unsigned version=avcodec_version();//编解码器codec
    std::cout<<"version is: "<<version<<endl;

    PlayerDialog w;
    w.show();

    return a.exec();
}
