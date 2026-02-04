#include "videoplayer.h"
#include <QString>
#include<QDebug>
#include<QCoreApplication>

#include <cstdlib> // 需要包含这个头文件来使用 putenv

// ================== 【新增】音频解码所需的全区变量或辅助逻辑 ==================
// 我们使用一种简化的方式：把需要的数据结构定义在这里
// 在实际大厂项目中，这些应该封装到类内部，通过 userdata 传递

// 音频解码器上下文，存放解码器参数（如采样率、通道数）
static AVCodecContext *s_audioCodecCtx = nullptr; // 音频解码器上下文
// 音频包队列指针，用于从外部获取压缩的音频数据
static PacketQueue    *s_audioQueue    = nullptr; // 指向你的音频队列
// 重采样上下文，用于将各种音频格式转换为 SDL 需要的格式（44100Hz/S16/双声道）
static SwrContext     *s_audioSwrCtx   = nullptr; // 重采样上下文
// 解码后的原始音频帧，存放 FFmpeg 解码出来的 PCM 数据
static AVFrame        *s_audioFrame    = nullptr; // 解码后的音频帧
// 音频缓冲区，存放重采样后、准备喂给 SDL 的数据
static uint8_t        *s_audioBuf      = nullptr; // 重采样后的缓冲区
// 缓冲区当前有效数据的大小（字节数）
static unsigned int    s_audioBufSize  = 0;       // 缓冲区数据大小
// 缓冲区当前读取到的位置（游标），记录du到了哪里
static unsigned int    s_audioBufIndex = 0;
// 临时音频包，用于从队列取数据
static AVPacket        s_audioPacket;             // 临时的包

// 音频参数 (假设 SDL 打开的是这个参数)
// 定义 SDL 需要的缓冲区大小（单位：样本数）
#define SDL_AUDIO_BUFFER_SIZE 1024
// 定义最大音频帧大小，用于申请内存（192000 足够大，能容纳 1 秒的高采样率音频）
#define MAX_AUDIO_FRAME_SIZE 192000

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


// ================== 音频解码核心函数 ==================
// 作用：从队列取包 -> 解码 -> 重采样 -> 返回填充的字节数
// 参数 audio_buf：指向目标缓冲区的指针（在这里我们将数据填入 s_audioBuf）
// 参数 buf_size：目标缓冲区最大容量
int audio_decode_farame(uint8_t* audio_buf,int buf_size){
    //如果队列指针未初始化，直接返回
    if(!s_audioQueue)return -1;

    int data_size=0;//最终数据大小

    while(1){//成功解码或队列为空才退出
        //1.从队列里面取包
        AVPacket* pkt=s_audioQueue->pop();
        if(!pkt) return -1;//队列为空或线程停止返回

        //2.解码
        //用于标记是否成功解码出一帧完整数据
        int got_frame=0;
        //返回值ret表示解码消耗了pkt中的多少字节
        int ret=avcodec_decode_audio4(s_audioCodecCtx,s_audioFrame,&got_frame,pkt);

        if(ret<0){
            //解码出错（数据损坏等），跳过这个包，继续下一个
            //释放pkt内存
            av_free_packet(pkt);
            free(pkt);
            continue;
        }

        if(got_frame){//成功解码出一帧数据
            //核心逻辑：音频重采样
            //为什么要重采样？
            //因为源音频可能是 48000Hz、浮点型,
            //而 SDL 只能播 44100Hz、整数型。

            //3.计算重采样后的样本数量
            // swr_get_delay 获取缓冲区的延迟，加上当前帧的样本数
            // av_rescale_rnd 用于处理采样率比例转换 (如 48k -> 44.1k)
            // 44100 是假设 SDL 打开的频率
            int dst_nb_samples=av_rescale_rnd(swr_get_delay(s_audioSwrCtx,
                                                            s_audioCodecCtx->sample_rate) + s_audioFrame->nb_samples,
                                                            44100, s_audioCodecCtx->sample_rate, AV_ROUND_UP);

            //转换
            // 参数说明：上下文, 目标缓冲区(取地址), 目标样本数, 源数据, 源样本数
            int convert_ret=swr_convert(s_audioSwrCtx, &audio_buf,
                                          dst_nb_samples,
                                          (const uint8_t **)s_audioFrame->data,
                                          s_audioFrame->nb_samples);

            // 计算转换后的字节数 (双通道 * 16位(2字节) * 样本数)
            data_size = convert_ret * 2 * 2;

            av_free_packet(pkt); // 记得释放
            free(pkt);
            return data_size;
        }

    }

}


// ================== SDL 回调函数 ==================
// 作用：当声卡需要数据时，SDL 会自动调用这个函数
// 参数 stream：SDL 给我们的空缓冲区，我们需要把声音填进去
// 参数 len：SDL 这次需要多少字节的数据（比如 2048 字节）
void videoPlayer::audio_callback(void *userdata, uint8_t *stream, int len)
{
    // 这里的 userdata 其实没用到，因为我们用了上面的 static 变量
    // 标准做法是把 this 指针传进来

    //1.先将SDL缓冲区清零（静音），防止杂音
    SDL_memset(stream,0,len);

    int len1=0;//本次能搬运的数据量
    int audio_size = 0;//解码出的新数据大小

    while(len>0){
        //如果 s_audioBuf 里的数据用完了 (index 追上了 size)
        if(s_audioBufIndex>=s_audioBufSize){
            //数据不够，解码新的一帧
            audio_size=audio_decode_farame(s_audioBuf,MAX_AUDIO_FRAME_SIZE);

            if(audio_size<0){//没数据，输出静音
                s_audioBufSize=1024;
                //没数据，就填充一段静音（1024字节的0）
                memset(s_audioBuf,0,s_audioBufSize);
            }else{
                //更新缓冲区大小为实际解码出的字节数
                s_audioBufSize=audio_size;
            }
            //重置,从头开始读
            s_audioBufIndex=0;
        }

        //计算当前缓冲区还剩多少数据没读
        len1=s_audioBufSize-s_audioBufIndex;
        // 如果剩余数据 > SDL 需要的数据，就只给 SDL 想要的 (len)
        // 如果剩余数据 < SDL 需要的数据，就把剩下的全给它 (len1)
        if(len1>len)len1=len;

        // 混音/拷贝
        // SDL_MixAudio 并不仅仅是拷贝，它支持音量调整 (SDL_MIX_MAXVOLUME)
        // 参数：目标地址, 源地址(基地址+游标), 长度, 音量
        SDL_MixAudio(stream, s_audioBuf + s_audioBufIndex, len1, SDL_MIX_MAXVOLUME);

        len -= len1;//SDL还需要多少
        stream += len1;//SDL缓冲区指针后移
        s_audioBufIndex += len1; //我们的缓冲区游标后移
    }

}


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

    // 找到视频流索引//查找音频索引
    m_audioStreamIndex=-1;
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }else if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            m_audioStreamIndex=i;
        }
    }

    //既无音频也无视频
    if (videoStream == -1&&m_audioStreamIndex==-1) {
        qDebug() << "no video and audio stream.";
        return;
    }

    AVCodecContext *pVideoCodecCtx = nullptr;
    AVCodec *pVideoCodec = nullptr;
    AVCodecContext *pAudioCodecCtx = nullptr;
    AVCodec *pAudioCodec = nullptr;

    //只有视频
    if (videoStream != -1) {
        // 打开解码器
        pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;
        pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
        if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0) {
            qDebug() << "open codec failed.";
            return;
        }
    }

    //初始化音频解码器和SDL
    if(m_audioStreamIndex!=-1){
        //获得解码器上下文
        pAudioCodecCtx=pFormatCtx->streams[m_audioStreamIndex]->codec;
        pAudioCodec=avcodec_find_decoder(pAudioCodecCtx->codec_id);
        if(avcodec_open2(pAudioCodecCtx,pAudioCodec,NULL)<0){
            qDebug()<<"无法打开音频解码器";
        }else{
            //初始化全局变量 (给回调函数用)
            s_audioCodecCtx = pAudioCodecCtx;//绑定解码器
            s_audioQueue= &m_audioQueue; // 指向类的成员变量
            s_audioFrame= av_frame_alloc();//申请帧内存
            //申请缓冲区内存 (乘以2是为了保险，防止重采样后数据变大溢出)
            s_audioBuf= (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

            //初始化重采样 (SwrContext)
            s_audioSwrCtx = swr_alloc();
            swr_alloc_set_opts(s_audioSwrCtx,
                               av_get_default_channel_layout(2), // 输出通道布局
                               AV_SAMPLE_FMT_S16,                // 输出采样格式 (SDL支持这种)
                               44100,                            // 输出采样率
                               av_get_default_channel_layout(pAudioCodecCtx->channels), // 输入
                               pAudioCodecCtx->sample_fmt,
                               pAudioCodecCtx->sample_rate,
                               0, NULL);
            //正式初始化Swr
            swr_init(s_audioSwrCtx);

            //始化 SDL
            if (SDL_Init(SDL_INIT_AUDIO)) {
                qDebug() << "SDL初始化失败:" << SDL_GetError();
            }

            //配置SDL音频参数 (我们期望的参数)
            SDL_AudioSpec wanted_spec;
            wanted_spec.freq = 44100;
            wanted_spec.format = AUDIO_S16SYS;
            wanted_spec.channels = 2;
            wanted_spec.silence = 0;
            wanted_spec.samples = 1024;
            wanted_spec.callback = audio_callback;
            wanted_spec.userdata = this;

            if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
                qDebug() << "SDL打开音频失败:" << SDL_GetError();
            }

            //SDL默认是暂停的，必须设为 0 才能开始播放
            SDL_PauseAudio(0); //开始播放
        }
    }

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
                //因为存在有视频没音频和有音频没视频的情况
                //所以必须加-1判断，要不然会数组越界
                // 如果是视频流就入队
                if((videoStream != -1) &&(packet->stream_index == videoStream)){
                    m_queue.push(packet);
                } //如果是音频，塞进音频队
                else if((m_audioStreamIndex != -1)&&(packet->stream_index == m_audioStreamIndex)) {
                    m_audioQueue.push(packet);
                }
                else {
                    // 其他流，直接释放
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

    struct SwsContext *img_convert_ctx=nullptr;
    uint8_t *out_buffer=nullptr;

    AVRational timeBase;
    int ret, got_picture;
    if(videoStream!=-1){
        // 初始化格式转换器 (YUV -> RGB32)
        img_convert_ctx = sws_getContext(
                    pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
                    pVideoCodecCtx->width, pVideoCodecCtx->height, AV_PIX_FMT_RGB32,
                    SWS_BICUBIC, NULL, NULL, NULL);

        int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pVideoCodecCtx->width, pVideoCodecCtx->height);
        out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
        avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, pVideoCodecCtx->width, pVideoCodecCtx->height);

        //更新功能：视频播放速度控制/*音画同步*/
        //逻辑：解码——获得当前帧时间——比对系统时间——等待——发送信号
        // 获取时间基准 (用于把抽象的 pts 转成微秒)
        timeBase = pFormatCtx->streams[videoStream]->time_base;
    }

    //加入时间戳
    int64_t start_time=av_gettime();
    int64_t pts=0;//当前视频帧的pts

    // 开始循环，从队列中取数据
    while(!m_isStop){
        if(videoStream!=-1){
            AVPacket *packet = m_queue.pop(); // 阻塞等待

            if(!packet) continue;

            ret = avcodec_decode_video2(pVideoCodecCtx, pFrameYUV, &got_picture, packet);

            if(ret < 0){
                qDebug() << "decode error";
                av_free_packet(packet);
                free(packet);
                continue;
            }

            if(got_picture){
                // 转码
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrameYUV->data, pFrameYUV->linesize,
                          0, pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);


                //获得当前帧时间
                pts=pFrameYUV->pts=pFrameYUV->best_effort_timestamp;
                if(pts!=AV_NOPTS_VALUE){
                    pts*=1000000*av_q2d(pFormatCtx->streams[videoStream]->time_base);
                }
                //计算开始播放时长
                int64_t realtime=av_gettime()-start_time;
                //如果要求播放时间>开始播放时间，就需要等待
                while(pts>realtime&&!m_isStop){
                    msleep(5);
                    realtime=av_gettime()-start_time;
                }


                // 发送给界面
                QImage tmpImg((uchar *)out_buffer, pVideoCodecCtx->width, pVideoCodecCtx->height, QImage::Format_RGB32);
                //emit SIG_getoneImage(tmpImg.copy()); // 发送深拷贝的图片
                if(!tmpImg.isNull()) {
                    emit SIG_getoneImage(tmpImg.copy());
                }

                //            // 简单的延时,延迟播放速度
                //            QThread::msleep(40);


                //}

                // 消费完必须释放
                av_free_packet(packet);
                free(packet);
            }
        }else{
            msleep(40);
        }
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


    // 关闭 SDL
    if (m_audioStreamIndex != -1) {
        SDL_CloseAudio();
        SDL_Quit();
        av_free(s_audioBuf);
        av_frame_free(&s_audioFrame);
        swr_free(&s_audioSwrCtx);
    }
    // 释放 FFmpeg 资源
    if (videoStream != -1 && pVideoCodecCtx) {
        av_free(out_buffer);
        av_frame_free(&pFrameYUV);
        av_frame_free(&pFrameRGB);
        avcodec_close(pVideoCodecCtx);
        avformat_close_input(&pFormatCtx);
    }
}


void videoPlayer::setFileName(const QString &newFileName)
{
    m_fileName = newFileName;
}

void videoPlayer::readStream()
{

}

