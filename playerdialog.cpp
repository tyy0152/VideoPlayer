#include "playerdialog.h"
#include "ui_playerdialog.h"

//#define _DEF_FILE_PATH "A:/TyyVideo/mmexport1685425970543.mp4"
//#define _DEF_FILE_PATH "http://vjs.zencdn.net/v/oceans.mp4"
#define _DEF_FILE_PATH "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8"

PlayerDialog::PlayerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PlayerDialog)
{
    ui->setupUi(this);

    m_myplayer=new videoPlayer;

    connect(m_myplayer,SIGNAL(SIG_getoneImage(QImage)),this,SLOT(slot_setImage(QImage)));

    m_myplayer->setFileName(_DEF_FILE_PATH);//传入播放的视频文件
}

PlayerDialog::~PlayerDialog()
{
    delete ui;

    delete m_myplayer;
}


void PlayerDialog::on_pb_start_clicked()
{
    //点击就开始播放---从音频文件里面获取图片进行播放
    m_myplayer->start();
}

void PlayerDialog::slot_setImage(QImage image)
{
    //pixmap（用于显示，有硬件支持渲染） image(内存里数据转化）
    //image进行缩放处理
    QPixmap pixmap=QPixmap::fromImage(image.scaled(ui->lb_show->size(),Qt::KeepAspectRatio));
            //直接将image放入可能会不适配,KeepAspectRatio是保持原比例
    ui->lb_show->setPixmap(pixmap);
}

//qt线程QThread 定义子类 start() run()
