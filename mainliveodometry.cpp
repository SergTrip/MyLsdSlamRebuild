#include "mainliveodometry.h"
#include "ui_mainliveodometry.h"

namespace lsd_slam
{

MainLiveOdometry::MainLiveOdometry(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainLiveOdometry)
{
    ui->setupUi(this);
}

MainLiveOdometry::~MainLiveOdometry()
{
    delete ui;
}

void MainLiveOdometry::start( std::string calibFileName )
{
    // Инициализируем доступ к видеопотоку
    inputStream = new InputImageStream();

    // Устанавливаем имя колибровачного файла
    inputStream->setCalibration( calibFileName );

    // Запускаем входной поток
    inputStream->run();

    // Создаем обертку для выходной 3D модели
    Output3DWrapper* outputWrapper = new ROSOutput3DWrapper(    inputStream->width(),
                                                                inputStream->height()   );

    // Оболочка для SLAM в реальном времени
    LiveSLAMWrapper slamNode( inputStream, outputWrapper );
    // Заходим в цикл SLAM
    slamNode.Loop();

    // Очистить указатели перед выходом
    if (inputStream != nullptr)
        delete inputStream;
    if (outputWrapper != nullptr)
        delete outputWrapper;
}

}
