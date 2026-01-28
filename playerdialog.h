#ifndef PLAYERDIALOG_H
#define PLAYERDIALOG_H

#include <QDialog>
#include"videoplayer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PlayerDialog; }
QT_END_NAMESPACE

class PlayerDialog : public QDialog
{
    Q_OBJECT

public:
    PlayerDialog(QWidget *parent = nullptr);
    ~PlayerDialog();

private slots:
    void on_pb_start_clicked();
    void slot_setImage(QImage image);

private:
    Ui::PlayerDialog *ui;

    videoPlayer* m_myplayer;
};
#endif // PLAYERDIALOG_H
