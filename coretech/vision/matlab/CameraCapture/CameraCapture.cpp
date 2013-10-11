#include "opencv2/opencv.hpp"

cv::VideoCapture *capture = NULL;


// A Simple Camera Capture Framework
int main(void)
{

    
    capture = new cv::VideoCapture(0);
    
    capture->set(CV_CAP_PROP_FRAME_WIDTH, 640.);
    capture->set(CV_CAP_PROP_FRAME_HEIGHT, 480.);
    
    printf("Camera width x height = %.0fx%.0f\n",
            capture->get(CV_CAP_PROP_FRAME_WIDTH),
            capture->get(CV_CAP_PROP_FRAME_HEIGHT));
    
    cv::waitKey(2000);
    
    if(capture == NULL || not capture->isOpened()) {
        printf("Camera not opened.\n");
        return -1;
    }
    
    bool success = false;
    cv::Mat frame; 
    for(int i=0; i<10; ++i)
    {
        //(*capture) >> frame; // get a new frame from camera
        //for(int attempt=0; attempt<100 && not success && frame.empty(); ++attempt)
        //{
        success = capture->read(frame);
        //}
        
        if(not success) {
            printf("Frame grab failed!\n");
        }
        else if(frame.empty()) {
            printf("Empty frame grabbed!\n");
            success = false;
        }
        
        if(success)
        {
            cv::cvtColor(frame, frame, CV_BGR2RGB);
            
            cv::namedWindow("Frame", 1);
            cv::imshow("Frame", frame);
            cv::waitKey(0);
        }
        else
        {
            cv::waitKey(1000);
        }
    } // FOR each frame
    
    capture->release();
    
    delete capture;
    
    return 0;

}
