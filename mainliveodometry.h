#ifndef MAINLIVEODOMETRY_H
#define MAINLIVEODOMETRY_H

#include <QWidget>

#include "InputImageStream.h"

namespace Ui {
class MainLiveOdometry;
}

namespace lsd_slam
{

class MainLiveOdometry : public QWidget
{
    Q_OBJECT

public:
    explicit MainLiveOdometry(QWidget *parent = 0);
    ~MainLiveOdometry();

    // Запускаем LSD-SLAM
    void start(std::string calibFileName = "");

private:
    // Доступ к видео потоку
    InputImageStream* inputStream;


    Ui::MainLiveOdometry *ui;
};

}

#endif // MAINLIVEODOMETRY_H
