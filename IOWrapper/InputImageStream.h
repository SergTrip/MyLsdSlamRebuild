#pragma once

#include "IOWrapper/NotifyBuffer.h"
#include "IOWrapper/TimestampedObject.h"

namespace lsd_slam
{

/**
 * Virtual ImageStream. Can be from OpenCV's ImageCapture, ROS or Android.
 * Also has to provide the camera calibration for that stream, as well as the
 * respective undistorter object (if required).
 * Runs in it's own thread, and has a NotifyBuffer,
 * in which received images are stored.
 */
class InputImageStream
{
public:
	virtual ~InputImageStream() {};
	
	/**
	 * Starts the thread.
	 */
	virtual void run() {};


    /// Установить параметры калибровки камеры
    virtual void setCalibration( std::string file ) {};

	/**
	 * Gets the NotifyBuffer to which incoming images are stored.
	 */
	inline NotifyBuffer<TimestampedMat>* getBuffer() {return imageBuffer;};


	/**
	 * Gets the Camera Calibration. To avoid any dependencies, just as simple float / int's.
	 */
    inline float    fx()        {   return fx_;     }
    inline float    fy()        {   return fy_;     }
    inline float    cx()        {   return cx_;     }
    inline float    cy()        {   return cy_;     }

    inline int      width()     {   return width_;  }
    inline int      height()    {   return height_; }

protected:
	NotifyBuffer<TimestampedMat>* imageBuffer;

	float fx_, fy_, cx_, cy_;

    int width_  ;
    int height_ ;
};
}
