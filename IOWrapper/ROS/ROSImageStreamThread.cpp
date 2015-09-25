/**
* This file is part of LSD-SLAM.
*
* Copyright 2013 Jakob Engel <engelj at in dot tum dot de> (Technical University of Munich)
* For more information see <http://vision.in.tum.de/lsdslam> 
*
* LSD-SLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* LSD-SLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with LSD-SLAM. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ROSImageStreamThread.h"
//#include <ros/callback_queue.h>

#include <boost/thread.hpp>
#include <opencv2/highgui/highgui.hpp>

//#include "cv_bridge/cv_bridge.h"
#include "util/settings.h"

#include <iostream>
#include <fstream>


namespace lsd_slam
{


using namespace cv;

ROSImageStreamThread::ROSImageStreamThread()
{
    // subscribe ( Регистрация узла в ROS )
//    vid_channel = nh_.resolveName   (   "image" );
//    vid_sub     = nh_.subscribe     (   vid_channel                     ,
//                                        1                               ,
//                                        &ROSImageStreamThread::vidCb    ,
//                                        this                                );


	// wait for cam calib
    width_  = 0;
    height_ = 0;

	// imagebuffer
    // Новый буффер из восьми "TimestampedMat"
	imageBuffer = new NotifyBuffer<TimestampedMat>(8);

    // Обнулить счетчики ( Скрей всего )
	undistorter = 0;
    lastSEQ     = 0;

    haveCalib   = false;
}

ROSImageStreamThread::~ROSImageStreamThread()
{
    // Чистим буфер
	delete imageBuffer;
}

void ROSImageStreamThread::setCalibration(std::string file)
{
    // Если файл пустой
	if(file == "")
	{
        // Видимо, подписываемся на публикацию данных о камере
//        ros::Subscriber info_sub    = nh_.subscribe(    nh_.resolveName("camera_info"),
//                                                        1,
//                                                        &ROSImageStreamThread::infoCb,
//                                                        this                            );

        // Выдаем соответствующее предупреждение
		printf("WAITING for ROS camera calibration!\n");

        // Пока один из параметров камеры не установлен
		while(width_ == 0)
		{
            // Периодически проверяем состояние настроек
//			ros::getGlobalCallbackQueue()->callAvailable(ros::WallDuration(0.03));
		}

        // Выдать сообщение,
		printf("RECEIVED ROS camera calibration!\n");

        // Снять подписку
		info_sub.shutdown();
	}
	else
	{
        // Создаем экземпляр Undistorter с  указанными параметрами
		undistorter = Undistorter::getUndistorterForFile(file.c_str());

        // Если не удалось создать
        if( undistorter == 0 )
		{
            // Сообщить обошибке
			printf("Failed to read camera calibration from file... wrong syntax?\n");
            // Остановить процесс
			exit(0);
		}

        // Вычитываем параметры матици преобразования
		fx_ = undistorter->getK().at<double>(0, 0);
		fy_ = undistorter->getK().at<double>(1, 1);
		cx_ = undistorter->getK().at<double>(2, 0);
		cy_ = undistorter->getK().at<double>(2, 1);

        // Вычитываем размеры изображения
        width_  = undistorter->getOutputWidth();
		height_ = undistorter->getOutputHeight();
	}

    // Отмечаем инициализацию колибратора
	haveCalib = true;
}

void ROSImageStreamThread::run()
{
    // Создает отдельный поток для данного объекта
    boost::thread thread( boost::ref(*this) );
}

void ROSImageStreamThread::operator()()
{
//	ros::spin();

	exit(0);
}

void ROSImageStreamThread::vidCb(const sensor_msgs::ImageConstPtr img)
{
    // Если камера еще не откалибрована
    if(!haveCalib)
        return;

    // Специальная функция для преобразования формата изображения между
    // ROS и OpenCV
//	cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(img, sensor_msgs::image_encodings::MONO8);

    // Контроль номера изображения ????
    if( img->header.seq < (unsigned int)lastSEQ )
    {
        // Сообщаем о неполадках
        printf("Backward-Jump in SEQ detected, but ignoring for now.\n");
        // сбрасываем счетчик
        lastSEQ = 0;
        // Прерываем обработку
        return;
    }

    // Обновляем номер изображения
    lastSEQ = img->header.seq;

    // Создаем экземпляр
    TimestampedMat bufferItem;

    // Надо найти спецификацию на sensor_msgs::ImageConstPtr
    // Если у сообщения есть своя отметка
    if( img->header.stamp.toSec() != 0  )
        // Оставляем ее
        bufferItem.timestamp =  Timestamp(img->header.stamp.toSec());
    else
        // Создаем свою отметку
        bufferItem.timestamp =  Timestamp(ros::Time::now().toSec());

    // Если подключен
    if( undistorter != 0    )
    {
        // Проверить на корректность инициализации
        assert(undistorter->isValid());
        // Скорректировать изображение
        undistorter->undistort( cv_ptr->image, bufferItem.data );
    }
    else
    {
        // Сохранить как есть
        bufferItem.data = cv_ptr->image;
    }

    // Сохранить в очередь
    imageBuffer->pushBack(bufferItem);
}

void ROSImageStreamThread::infoCb(const sensor_msgs::CameraInfoConstPtr info)
{
    // Если параметры камеры еще не были получены
    if(!haveCalib)
    {
        // Устанавливаем параматры камеры
        fx_ = info->P[0];
        fy_ = info->P[5];
        cx_ = info->P[2];
        cy_ = info->P[6];

        if(fx_ == 0 || fy_== 0)
        {
            printf("camera calib from P seems wrong, trying calib from K\n");

            fx_ = info->K[0];
            fy_ = info->K[4];
            cx_ = info->K[2];
            cy_ = info->K[5];
        }

        width_  = info->width;
        height_ = info->height;

        printf("Received ROS Camera Calibration: fx: %f, fy: %f, cx: %f, cy: %f @ %dx%d\n",fx_,fy_,cx_,cy_,width_,height_);
    }
}

}
