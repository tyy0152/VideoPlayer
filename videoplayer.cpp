#include "videoplayer.h"
#include <QString>
#include<QDebug>
#include<QCoreApplication>

videoPlayer::videoPlayer()
{

}

void videoPlayer::run()
{
//    qDebug()<<"videoPlayer::  "<<__func__;
//    QString path=QCoreApplication::applicationDirPath()+"/image/";
//    qDebug()<<path;
//    //循环获取图片
//    for(int i=0;i<23;i++){
//        QString tmp=QString("%1%2.png").arg(path).arg(i);
//        qDebug()<<tmp;
//        //发送信号——获取图片
//        Q_EMIT SIG_getoneImage(QImage(tmp));
//        QThread::msleep(100);
//    }


    av_register_all();//1.完成编码器和解码器的初始化，否则打开编解码器失败

    //2.分配avformatcontext，ffmpeg所有操作都通过avformatcontext来进行。
    AVFormatContext *pFormatCtx=avformat_alloc_context();

    //3.打开视频文件
    std::string path=m_fileName.toStdString();
    const char* file_path=path.c_str();

    //pFormatCtx相当与文件指针，如果文件来自本地会更快打开，如果来自网络，可能要考虑延迟
    if(avformat_open_input(&pFormatCtx,file_path,NULL,NULL)!=0){
        qDebug()<<"can not open file.";
        return;
    }
    //获取视频文件信息
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        qDebug()<<"can not find stream infomation.";
        return;
    }

    //4.查找文件中的视频流
    //循环查找视频中包含的信息流，直到找到视频类型的流
    //将其记录下来保存到videostream中
    //在这里只处理视频流，不管音频流
    int videoSrteam =-1;
    for(int i=0;i<pFormatCtx->nb_streams/*该文件里的流的总数量*/;i++){
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
                            // pFormatCtx->streams[i]  --> 拿出第 i 个流
                            // ->codec                 --> 看看它的解码器信息
                            // ->codec_type            --> 看看它的“类型”是什么
                            //AVMEDIA_TYPE_VIDEO--> FFmpeg 定义的“视频类型”常量
            videoSrteam=i;//记录视频流的索引号
        }
    }
    if(videoSrteam==-1){
        qDebug()<<"don not find a video stream.";
        return;
    }

    //5.查找解码器
    auto pCodecCtx=pFormatCtx->streams[videoSrteam]->codec;//视频流的参数信息
    auto pCodec=avcodec_find_decoder(pCodecCtx->codec_id);//获得可以解压某种编码格式的解码器//codec_id是编码格式
    if(pCodec == NULL){
        qDebug()<<"Codec not found";
        return;
    }
    //打开解码器
    if(avcodec_open2(pCodecCtx,pCodec,NULL)<0){
        qDebug()<<"can not open codec.";
        return;
    }

    //6.申请解码需要的结构体avframe视频缓存的结构体
    AVFrame *pFrameYUV,*pFrameRGB;
    pFrameYUV=av_frame_alloc();//装解码后的原始YUV格式的视频
    pFrameRGB=av_frame_alloc();//装RGB格式的视频
    int y_size=pCodecCtx->width*pCodecCtx->height;
    AVPacket *packet=(AVPacket*)malloc(sizeof(AVPacket));
    //申请了一个指针，用来装压缩数据
    av_new_packet(packet,y_size);//给packet申请y_size的空间

    //7.将YUV转换为RGB
    struct SwsContext *img_convert_ctx;
    img_convert_ctx=sws_getContext(pCodecCtx->width,pCodecCtx->height,
                                   pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,
                                   AV_PIX_FMT_RGB32,SWS_BICUBIC,NULL,NULL,NULL);
    auto numBytes=avpicture_get_size(AV_PIX_FMT_RGB32,pCodecCtx->width,pCodecCtx->height);
    uint8_t * out_buffer=(uint8_t *) av_malloc(numBytes *sizeof (uint8_t));
    avpicture_fill((AVPicture*)pFrameRGB,out_buffer,
                   AV_PIX_FMT_RGB32,pCodecCtx->width,pCodecCtx->height);

    //8.循环读取视频帧, 转换为 RGB 格式, 抛出信号去控件显示
    int ret, got_picture;
    while(1)
    {
        //可以看出 av_read_frame 读取的是一帧视频，并存入一个 AVPacket 的结构中
        //av_read_frame申请堆内存
        //如果出现网络问题，av_read_frame会卡死，无法读取数据到packet——可能导致线程阻塞——解决方式（生产者消费者模型，丢帧策略/流量窗口控制，超时重连）
        if (av_read_frame(pFormatCtx, packet) < 0)//读取混合数据流，获取未解压的数据，可能会获得音频或视频或其他数据
        {
            break; //这里认为视频读取完了
        }
        //生成图片，先判断获得的数据是不是视频数据，数据分流
        //这一段if的操作其实是属于前端的，后端只会把压缩包从这里转发到那里，不会进行解压缩和显示操作
        if (packet->stream_index == videoSrteam)
        {
            // 解码 packet 存在 pFrame 里面
            ret = avcodec_decode_video2(pCodecCtx, pFrameYUV, &got_picture,packet);
            if (ret < 0) {
                qDebug()<< "decode error";
                return ;
            }
            //有解码器解码之后得到的图像数据都是 YUV420 的格式，而这里需要将其保存成图片文件
            //因此需要将得到的 YUV420 数据转换成 RGB 格式
            if (got_picture) {
                sws_scale(img_convert_ctx,
                          (uint8_t const * const *) pFrameYUV->data,
                          pFrameYUV->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                          pFrameRGB->linesize);
                // 将 out_buffer 里面的数据存在 QImage 里面
                QImage tmpImg((uchar
                               *)out_buffer,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
                //把图像复制一份 传递给界面显示
                //显示到控件 多线程 无法控制控件 所以要发射信号
                emit SIG_getoneImage( tmpImg );
            }
        }
        av_free_packet(packet);//释放packet，要不然会内存爆炸
        msleep(5); //单纯显示视频的速度非常快
    }

    //9.回收数据
    av_free(out_buffer);
    av_free(pFrameYUV);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

}


void videoPlayer::setFileName(const QString &newFileName)
{
    m_fileName = newFileName;
}
