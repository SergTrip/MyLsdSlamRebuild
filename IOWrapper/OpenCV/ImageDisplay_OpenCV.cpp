#include "IOWrapper/ImageDisplay.h"

#include <opencv2/highgui/highgui.hpp>

#include <string>
#include <unordered_set>

#include <boost/thread.hpp>

namespace lsd_slam
{

namespace Util
{
    // Флаг использования потока
	const bool useImageDisplayThread = true;

    // Неупорядоченный список открытых окон, описанных std::string
    std::unordered_set<std::string> openWindows;

    // Обеспечивает потокобезопасность данных
    boost::mutex                    openCVdisplayMutex;
    // Условная переменная ( наверное, генерирует сигнал )
    boost::condition_variable       openCVdisplaySignal;

    // Указатель на поток ( найти инициализацию!!! )
    boost::thread*                  imageDisplayThread = 0;

    // Массив изображений
    std::vector<DisplayImageObect>  displayQueue;

    bool imageThreadKeepRunning     = true;

// Функция вызывается в цикле потока
void displayThreadLoop()
{
    // Сообщает о запуске потока
	printf("started image display thread!\n");

    // Устанавливает мютекс для потока ??
	boost::unique_lock<boost::mutex> lock(openCVdisplayMutex);

    // Пока неоходим поток изображений
	while(imageThreadKeepRunning)
	{
        // Останавливает поток до получения новых данных
		openCVdisplaySignal.wait(lock);

        // Если требуется остановить поток
		if(!imageThreadKeepRunning)
            // Прервать цикл
			break;

        // Если очередь не пустая
		while(displayQueue.size() > 0)
		{
            // Если последний элемент не поддерживает autosize
			if(!displayQueue.back().autoSize)
			{
                // Если последнее изображение соответствует последними окну
				if(openWindows.find(displayQueue.back().name) == openWindows.end())
				{
                    // Создаем какоето окно ???
                    cv::namedWindow ( displayQueue.back().name, cv::WINDOW_NORMAL );
                    cv::resizeWindow( displayQueue.back().name, displayQueue.back().img.cols, displayQueue.back().img.rows);

                    // Добавить в вектор
                    openWindows.insert(displayQueue.back().name);
				}
			}
            // Показать окно
			cv::imshow(displayQueue.back().name, displayQueue.back().img);
            // добавить в очередь
			displayQueue.pop_back();
		}
	}

    // Удалить все окна
	cv::destroyAllWindows();
    // Очистить вектор
	openWindows.clear();

    // Сообщить об окончании потока
	printf("ended image display thread!\n");
}

// Создать поток изображений
void makeDisplayThread()
{
    // Разрешить поток изображений
    imageThreadKeepRunning  = true;
    // Запустить функцию в потоке
    imageDisplayThread      = new boost::thread(&displayThreadLoop);
}

void displayImage(const char* windowName, const cv::Mat& image, bool autoSize)
{
    if( useImageDisplayThread )
	{
        // Если поток еще небыл запущен
		if(imageDisplayThread == 0)
            // Запустить поток
			makeDisplayThread();

        // Заблокировать доступ к данным
		boost::unique_lock<boost::mutex> lock(openCVdisplayMutex);

        // Добавить новый экземпляра объекта изображения
        displayQueue.push_back( DisplayImageObect() );

        // Установить прпаметры нового объекта
        displayQueue.back().autoSize    = autoSize;
        displayQueue.back().img         = image.clone();
        displayQueue.back().name        = windowName;

        // Если есть потоки, ожидающие расблокировки
        // Расблокируется один из ожидающих
		openCVdisplaySignal.notify_one();
	}
	else
	{
        // Если параметр не установлен
		if(!autoSize)
		{
			if(openWindows.find(windowName) == openWindows.end())
			{
                cv::namedWindow (windowName, cv::WINDOW_NORMAL);
				cv::resizeWindow(windowName, image.cols, image.rows);

				openWindows.insert(windowName);
			}
		}
		cv::imshow(windowName, image);
	}
    // cv::waitKey(1);
}

int waitKey(int milliseconds)
{
    // Ожидает нажатие кнопок
	return cv::waitKey(milliseconds);
}

int waitKeyNoConsume(int milliseconds)
{
	// Cannot implement this with OpenCV functions.
	return cv::waitKey(milliseconds);
}

void closeAllWindows()
{
    // Заблокировать доступ к данным
	boost::unique_lock<boost::mutex> lock(openCVdisplayMutex);

	if(useImageDisplayThread)
	{
        // Если поток запущен
		if(imageDisplayThread != 0)
		{
            // Останавливаем поток
			imageThreadKeepRunning = false;
            // Сообщить всем ожидающим освобождения
			openCVdisplaySignal.notify_all();

            // Сообщаем, что ожиданием окончания остановки
			printf("waiting for image display thread to end!\n");

            // Освододить мютекс
			lock.unlock();

            // ???
			imageDisplayThread->join();

            // Сообщаем об окончание потока
			printf("done waiting for image display thread to end!\n");

            // Обнулить указатель
			imageDisplayThread = 0;
		}
	}
	else
	{
        // Очистить окна
		cv::destroyAllWindows();
        // Очистить очередь
		openWindows.clear();
	}
}
}

}
