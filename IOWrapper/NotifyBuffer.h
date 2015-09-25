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

#ifndef _OBJECT_BUFFER_HPP_
#define _OBJECT_BUFFER_HPP_

#include <deque>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>

namespace lsd_slam
{

/**
 * Contains an internal boost::condition on which notify_all() can be called
 * from the outside.
 */
class Notifiable
{
public:
	/**
	 * Calls notify_all() on notifyCondition.
	 */
    inline void notify()
    {
        // Сообщить всем заинтересованным
		notifyCondition.notify_all();
	}
	
protected:
    // Условия управления потоком
	boost::condition notifyCondition;
};

//********************    NotifyBuffer   ***************************************

/**
 * Thread-safe limited-size buffer which may notify a specific object when
 * a new item becomes available.
 */
// Потокобезопасный буфер с ограниченным размером, сообщающий
// определенным объектам о том, что доступен новый экземпляр (чего либо ??)
template< typename T >
class NotifyBuffer
{
public:
	/**
	 * Creates a queue with the given maximum size.
	 */
    NotifyBuffer        (   int bufferSize  )
        // Установить размер буфера
        : bufferSize    (       bufferSize  )
        // Обнулить указатель
        , receiver      (   nullptr         )
	{
	}
	
	/**
	 * Creates a queue with the given maximum size and Notifiable instance
	 * which will be notified when a new object becomes available.
	 */
    NotifyBuffer        (int    bufferSize, Notifiable* receiver    )
        // Установить размер буфера
        : bufferSize    (       bufferSize                          )
        // Установить указатель на получателя
        , receiver      (       receiver                            )
	{
	}
	
	/**
	 * Sets the notification receiver.
	 * 
	 * The receiver can be nullptr to disable notifications.
	 */
	void setReceiver(Notifiable* receiver)
	{
        // Установить казатель на получателя
		this->receiver = receiver;
	}
	
	/**
	 * Adds an object to the back of the queue.
	 * 
	 * If the queue is full already, discards the object. Returns if there
	 * was enough space to add the object.
	 */
    bool pushBack(const T& object)
	{
        // Ограничиваем доступ к данным
		boost::unique_lock<boost::recursive_mutex> lock(bufferMutex);
		
        // если буффер заполнен
        if (static_cast<int>(queue.size()) >= bufferSize)
        {
            // Вернуть результат
			return false;
		}

        // Добавить в очередь
		queue.push_back(object);
		
        // Освободить мютекс
		lock.unlock();
		
        // Если есть заинтересованный объект
        if (receiver)
        {
            // Сообщить ему
			receiver->notify();
		}

        // Сообщить, что добавлен еще один элемент (что??)
		bufferNonEmptyCondition.notify_one();
        // Вернуть подтверждение
		return true;
	}
	
	/**
	 * Returns the number of objects in the buffer.
	 */
	int size()
	{
		boost::unique_lock<boost::recursive_mutex> lock(bufferMutex);
		return queue.size();
	}
	
	/**
	 * Returns a copy of the first object.
	 */
	T first() {
		boost::unique_lock<boost::recursive_mutex> lock(bufferMutex);
		return queue.front();
	}
	
	/**
	 * Removes the first object and returns a copy of it.
	 * 
	 * If there is no object in the queue, blocks until one is available.
	 */
    T popFront()
    {
        // Захватываем мютекс
		boost::unique_lock<boost::recursive_mutex> lock(bufferMutex);
		
		// Block in case there is no object
        while (!(queue.size() > 0))
        {
            // Ожидаем сообщение ( может разблокируем на время ??? )
			bufferNonEmptyCondition.wait(lock);
		}

        // Забираем объект из начала буффера
		T object = queue.front();
        // Удаляем объект ?????
		queue.pop_front();
        // Возвращаем экземпляр
		return object;
	}
	
	/**
	 * Returns the buffer access mutex. Must be locked when checking size()
	 * and waiting if size() == 0.
	 */
	boost::recursive_mutex& getMutex() {
		return bufferMutex;
	}
	
private:
    boost::recursive_mutex  bufferMutex;
    boost::condition        bufferNonEmptyCondition;
	
    // Размер буфера
	int bufferSize;
    // Очередь объектов
    std::deque< T > queue;
	
    // Указатель на объект-получатель
	Notifiable* receiver;
};
}
#endif
