#include "videoplayer.h"
#include <QString>
#include<QDebug>
#include<QCoreApplication>

#include <cstdlib> // 需要包含这个头文件来使用 putenv

videoPlayer::videoPlayer()
{
    m_readThread=nullptr;
    m_isStop=false;
}

videoPlayer::~videoPlayer()
{
    //播放器被销毁时，必须停止子线程
    m_isStop=true;

    if(m_readThread&&m_readThread->joinable()){
        m_readThread->join();
        delete  m_readThread;
    }
}

//void videoPlayer::run()
//{
////    qDebug()<<"videoPlayer::  "<<__func__;
////    QString path=QCoreApplication::applicationDirPath()+"/image/";
////    qDebug()<<path;
////    //循环获取图片
////    for(int i=0;i<23;i++){
////        QString tmp=QString("%1%2.png").arg(path).arg(i);
////        qDebug()<<tmp;
////        //发送信号——获取图片
////        Q_EMIT SIG_getoneImage(QImage(tmp));
////        QThread::msleep(100);
////    }


//    av_register_all();//1.完成编码器和解码器的初始化，否则打开编解码器失败

//    //2.分配avformatcontext，ffmpeg所有操作都通过avformatcontext来进行。
//    AVFormatContext *pFormatCtx=avformat_alloc_context();

//    //3.打开视频文件
//    std::string path=m_fileName.toStdString();
//    const char* file_path=path.c_str();

//    //pFormatCtx相当与文件指针，如果文件来自本地会更快打开，如果来自网络，可能要考虑延迟
//    if(avformat_open_input(&pFormatCtx,file_path,NULL,NULL)!=0){
//        qDebug()<<"can not open file.";
//        return;
//    }
//    //获取视频文件信息
//    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
//        qDebug()<<"can not find stream infomation.";
//        return;
//    }

//    //4.查找文件中的视频流
//    //循环查找视频中包含的信息流，直到找到视频类型的流
//    //将其记录下来保存到videostream中
//    //在这里只处理视频流，不管音频流
//    int videoSrteam =-1;
//    for(int i=0;i<pFormatCtx->nb_streams/*该文件里的流的总数量*/;i++){
//        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
//                            // pFormatCtx->streams[i]  --> 拿出第 i 个流
//                            // ->codec                 --> 看看它的解码器信息
//                            // ->codec_type            --> 看看它的“类型”是什么
//                            //AVMEDIA_TYPE_VIDEO--> FFmpeg 定义的“视频类型”常量
//            videoSrteam=i;//记录视频流的索引号
//        }
//    }
//    if(videoSrteam==-1){
//        qDebug()<<"don not find a video stream.";
//        return;
//    }

//    //5.查找解码器
//    auto pCodecCtx=pFormatCtx->streams[videoSrteam]->codec;//视频流的参数信息
//    auto pCodec=avcodec_find_decoder(pCodecCtx->codec_id);//获得可以解压某种编码格式的解码器//codec_id是编码格式
//    if(pCodec == NULL){
//        qDebug()<<"Codec not found";
//        return;
//    }
//    //打开解码器
//    if(avcodec_open2(pCodecCtx,pCodec,NULL)<0){
//        qDebug()<<"can not open codec.";
//        return;
//    }

//    //6.申请解码需要的结构体avframe视频缓存的结构体
//    AVFrame *pFrameYUV,*pFrameRGB;
//    pFrameYUV=av_frame_alloc();//装解码后的原始YUV格式的视频
//    pFrameRGB=av_frame_alloc();//装RGB格式的视频
//    int y_size=pCodecCtx->width*pCodecCtx->height;
//    AVPacket *packet=(AVPacket*)malloc(sizeof(AVPacket));
//    //申请了一个指针，用来装压缩数据
//    av_new_packet(packet,y_size);//给packet申请y_size的空间

//    //7.将YUV转换为RGB
//    struct SwsContext *img_convert_ctx;
//    img_convert_ctx=sws_getContext(pCodecCtx->width,pCodecCtx->height,
//                                   pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,
//                                   AV_PIX_FMT_RGB32,SWS_BICUBIC,NULL,NULL,NULL);
//    auto numBytes=avpicture_get_size(AV_PIX_FMT_RGB32,pCodecCtx->width,pCodecCtx->height);
//    uint8_t * out_buffer=(uint8_t *) av_malloc(numBytes *sizeof (uint8_t));
//    avpicture_fill((AVPicture*)pFrameRGB,out_buffer,
//                   AV_PIX_FMT_RGB32,pCodecCtx->width,pCodecCtx->height);

//    //8.循环读取视频帧, 转换为RGB格式, 抛出信号去控件显示
//    int ret, got_picture;
//    while(1)
//    {
//        //av_read_frame读取的是一帧视频，并存入一个AVPacket的结构中
//        //av_read_frame申请堆内存
//        //如果出现网络问题，av_read_frame会卡死，无法读取数据到packet——可能导致线程阻塞——解决方式（生产者消费者模型，丢帧策略/流量窗口控制，超时重连）
//        if (av_read_frame(pFormatCtx, packet) < 0)//读取混合数据流，获取未解压的数据，可能会获得音频或视频或其他数据
//        {
//            break; //这里认为视频读取完了
//        }
//        //生成图片，先判断获得的数据是不是视频数据，数据分流
//        //这一段if的操作其实是属于前端的，后端只会把压缩包从这里转发到那里，不会进行解压缩和显示操作
//        if (packet->stream_index == videoSrteam)
//        {
//            // 解码 packet 存在pFrame里面
//            ret = avcodec_decode_video2(pCodecCtx, pFrameYUV, &got_picture,packet);
//            if (ret < 0) {
//                qDebug()<< "decode error";
//                return ;
//            }
//            //有解码器解码之后得到的图像数据都是YUV420的格式，而这里需要将其保存成图片文件
//            //因此需要将得到的YUV420数据转换成RGB格式
//            if (got_picture) {
//                sws_scale(img_convert_ctx,
//                          (uint8_t const * const *) pFrameYUV->data,
//                          pFrameYUV->linesize, 0, pCodecCtx->height, pFrameRGB->data,
//                          pFrameRGB->linesize);
//                // 将 out_buffer 里面的数据存在 QImage 里面
//                QImage tmpImg((uchar
//                               *)out_buffer,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
//                //把图像复制一份 传递给界面显示
//                //显示到控件 多线程 无法控制控件 所以要发射信号
//                emit SIG_getoneImage( tmpImg );
//            }
//        }
//        av_free_packet(packet);//释放packet，要不然会内存爆炸
//        msleep(5); //单纯显示视频的速度非常快
//    }

//    //9.回收数据
//    av_free(out_buffer);
//    av_free(pFrameYUV);
//    av_free(pFrameRGB);
//    avcodec_close(pCodecCtx);
//    avformat_close_input(&pFormatCtx);

//}



void videoPlayer::run()
{
    // 1. 初始化ffmpeg网络模块
    avformat_network_init();
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //暴力清除环境变量中的代理设置
    // 这一步会骗过 FFmpeg，让它以为系统没有设置代理，从而直连
    _putenv("HTTP_PROXY=");
    _putenv("HTTPS_PROXY=");
    _putenv("ALL_PROXY=");

    //打印路径
    qDebug() << "准备播放的 URL 是:" << m_fileName;

    avformat_network_init();

    // 打开视频文件
    std::string path = m_fileName.toStdString();
    if(avformat_open_input(&pFormatCtx, path.c_str(), NULL, NULL) != 0){
        qDebug() << "can not open file.";
        return;
    }

    // 查找流信息
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        qDebug() << "can not find stream info.";
        return;
    }

    // 找到视频流索引
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        qDebug() << "no video stream.";
        return;
    }

    // 打开解码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        qDebug() << "open codec failed.";
        return;
    }

    //int64_t realtime=av_gettime()-start_time;


    // 2. 启动生产者线程 (使用lambda表达式)
    m_isStop = false;

    m_readThread = new std::thread([=](){
        AVPacket *packet = nullptr;
        while(!m_isStop){
            // 流量控制：如果队列大于100，证明解码速度太慢了
            if(m_queue.size() > 100){
                QThread::msleep(10);
                continue;
            }

            packet = (AVPacket*)malloc(sizeof(AVPacket));

            // 读取一帧数据
            if(av_read_frame(pFormatCtx, packet) >= 0){
                // 如果是视频流就入队
                if(packet->stream_index == videoStream){
                    m_queue.push(packet);
                } else {
                    // 音频或其他流，直接释放
                    av_free_packet(packet);
                    free(packet);
                }
            } else {
                // 读取失败或文件结束
                av_free_packet(packet);
                free(packet);
                QThread::msleep(10);
                if(m_isStop) break;
            }
        }
    });

    // 3. 消费者循环 (解码与显示)
    AVFrame *pFrameYUV = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();

    // 初始化格式转换器 (YUV -> RGB32)
    struct SwsContext *img_convert_ctx = sws_getContext(
        pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB32,
        SWS_BICUBIC, NULL, NULL, NULL);

    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
    uint8_t *out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);

    int ret, got_picture;

    // 开始循环，从队列中取数据
    while(!m_isStop){
        AVPacket *packet = m_queue.pop(); // 阻塞等待

        if(!packet) continue;

        ret = avcodec_decode_video2(pCodecCtx, pFrameYUV, &got_picture, packet);

        if(ret < 0){
            qDebug() << "decode error";
            av_free_packet(packet);
            free(packet);
            continue;
        }

        if(got_picture){
            // 转码
            sws_scale(img_convert_ctx, (const uint8_t* const*)pFrameYUV->data, pFrameYUV->linesize,
                      0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

            // 发送给界面
            QImage tmpImg((uchar *)out_buffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
            emit SIG_getoneImage(tmpImg.copy()); // 发送深拷贝的图片

            // 简单的延时
            QThread::msleep(40);
        }

        // 消费完必须释放
        av_free_packet(packet);
        free(packet);
    }

    // 4. 清理工作
    // 告诉生产者线程停止
    m_isStop = true;

    // 等待生产者线程真正结束
    if (m_readThread && m_readThread->joinable()) {
        m_readThread->join();
        delete m_readThread;
        m_readThread = nullptr;
    }

    // 清空队列里剩余的包
    m_queue.clear();

    // 释放 FFmpeg 资源
    av_free(out_buffer);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}


void videoPlayer::setFileName(const QString &newFileName)
{
    m_fileName = newFileName;
}

void videoPlayer::readStream()
{

}
