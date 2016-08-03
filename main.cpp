#include <opencv2/core/utility.hpp>
#include "opencv2/video/tracking.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "mouseCtrl.h"

#include <iostream>
#include <ctype.h>

using namespace cv;
using namespace std;


Mat image;
const int pi = 3.1415;
bool backprojMode = false;
bool selectObject = false;
bool trackObject = false;
bool showHist = true;
Point origin;
Rect selection;
int vmin = 10, vmax = 256, smin = 30;

//circle
Point circle_point;
int   circle_raidus;
Mat   circle_mask;
bool  circle_detect = false;

static void help()
{
    cout << "\nThis is a demo that shows mean-shift based tracking\n"
            "You select a color objects such as your face and it tracks it.\n"
            "This reads from video camera (0 by default, or the camera number the user enters\n"
            "Usage: \n"
            "   ./camshiftdemo [camera number]\n";

    cout << "\n\nHot keys: \n"
            "\tESC - quit the program\n"
            "\tc - stop the tracking\n"
            "\tb - switch to/from backprojection view\n"
            "\th - show/hide object histogram\n"
            "\tp - pause video\n"
            "To initialize tracking, select the object with mouse\n";
}

void init(Mat frame)
{
    circle_point = Point(3*frame.cols/4, 1*frame.rows/4);
    circle_raidus = frame.rows/6;
    circle_mask = Mat::zeros(frame.size(), CV_8UC1);

    selection.x = circle_point.x - circle_raidus;
    selection.y = circle_point.y - circle_raidus;
    selection.width = circle_raidus*2;
    selection.height = circle_raidus*2;

    selection &= Rect(0, 0, frame.cols, frame.rows);
}    

bool JudgeSkinRang(Mat Mask)
{
    int count_white = countNonZero(Mask);
    if(count_white > 5*pi*circle_raidus*circle_raidus/6)
        return true;
    else
        return false;
}    

const char* keys =
{
    "{@camera_number| 0 | camera number}"
    "{m  method   |mog2     | method (knn or mog2) }"
};

int main( int argc, const char** argv )
{
    help();
    
    VideoCapture cap;
    Rect trackWindow;
    int hsize = 16;
    float hranges[] = {0,180};
    const float* phranges = hranges;
    CommandLineParser parser(argc, argv, keys);
    int camNum = parser.get<int>(0);
    string method = parser.get<string>("method");
    bool update_bg_model = true;

    cap.open(camNum);

    if( !cap.isOpened() )
    {
        help();
        cout << "***Could not initialize capturing...***\n";
        cout << "Current parameter's value: \n";
        parser.printMessage();
        return -1;
    }

    Mat frame, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;
    cap >> frame;
    if( frame.empty() )
        return 0;
    init(frame);

    namedWindow( "Histogram", 0 );
    namedWindow( "CamShift Demo", 0 );

    bool paused = false;

    //fg bg segment
    Ptr<BackgroundSubtractor> bg_model = method == "knn" ?
            createBackgroundSubtractorKNN().dynamicCast<BackgroundSubtractor>() :
            createBackgroundSubtractorMOG2().dynamicCast<BackgroundSubtractor>();

    Mat img0, img, fgmask, fgimg;
    std::vector<Mat> vectorOfHSVImages;

    int FrameCount = 0;
    for(;;)
    {
        if( !paused )
        {
            cap >> frame;
            cv::flip(frame, frame, 1);
            frame.copyTo(image);

            if( frame.empty() )
                break;
            if( fgimg.empty() )
                fgimg.create(frame.size(), frame.type());
            //update the model
            bg_model->apply(frame, fgmask, update_bg_model ? -1 : 0);
            threshold(fgmask, fgmask, 0, 255, THRESH_BINARY + THRESH_OTSU);
            erode(fgmask, fgmask, Mat());
            dilate(fgmask, fgmask, Mat());

            imshow("foreground mask", fgmask);
            FrameCount++;
            if(FrameCount < 100)
                continue;
        }
        
        if( !paused )
        {
            cvtColor(image, hsv, COLOR_BGR2HSV);
            circle(circle_mask, circle_point, circle_raidus, Scalar(255), -1);
            mask = Scalar::all(0);
            mask = fgmask & circle_mask;
            imshow("mask", mask);
            circle_detect = JudgeSkinRang(mask);
            if(!trackObject)
            {
                int ch[] = {0, 0};
                hue.create(hsv.size(), hsv.depth());
                mixChannels(&hsv, 1, &hue, 1, ch, 1);
                if(circle_detect)
                {
                    calcHist(&hue, 1, 0, mask, hist, 1, &hsize, &phranges);
                    normalize(hist, hist, 0, 255, NORM_MINMAX);

                    trackWindow = selection;
                    trackObject = true;

                    histimg = Scalar::all(0);
                    int binW = histimg.cols / hsize;
                    Mat buf(1, hsize, CV_8UC3);
                    for( int i = 0; i < hsize; i++ )
                        buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180./hsize), 255, 255);
                    cvtColor(buf, buf, COLOR_HSV2BGR);

                    for( int i = 0; i < hsize; i++ )
                    {
                        int val = saturate_cast<int>(hist.at<float>(i)*histimg.rows/255);
                        rectangle( histimg, Point(i*binW,histimg.rows),
                                   Point((i+1)*binW,histimg.rows - val),
                                   Scalar(buf.at<Vec3b>(i)), -1, 8 );
                    }
                    imshow( "Histogram", histimg );
                }
            }
            else
            {

                split(hsv, vectorOfHSVImages);
                if(!hist.empty())
                {    
                    hue = vectorOfHSVImages[0];
                    calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
                    backproj &= circle_mask;
                    imshow("backproj", backproj);
                    RotatedRect trackBox = CamShift(backproj, trackWindow,
                                    TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
                    if( trackWindow.area() <= 1 )
                    {
                        int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                        trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                                       trackWindow.x + r, trackWindow.y + r) &
                                  Rect(0, 0, cols, rows);
                    }

                    if( backprojMode )
                        cvtColor( backproj, image, COLOR_GRAY2BGR );
                    circle_point = trackBox.center;
                }    
            }
        }
        //else if( trackObject < 0 )
        //    paused = false;

        /*if( selectObject && selection.width > 0 && selection.height > 0 )
        {
            Mat roi(image, selection);
            bitwise_not(roi, roi);
        }*/

        if(!circle_detect)
            circle(image, circle_point, circle_raidus, Scalar(0, 255, 0), 2);
        else
            circle(image, circle_point, circle_raidus, Scalar(255, 0, 0), 2);

        imshow( "CamShift Demo", image );

        char c = (char)waitKey(10);
        if( c == 27 )
            break;
        switch(c)
        {
            case 'u':
                if(update_bg_model == true)
                {
                    printf("stop update background");
                    update_bg_model = false;
                }
                else
                {
                    printf("start update background");
                    update_bg_model = true;
                }
            break;    
            case 'b':
                backprojMode = !backprojMode;
            break;
            case 'c':
                trackObject = false;
                histimg = Scalar::all(0);
            break;
            case 'h':
                showHist = !showHist;
                if( !showHist )
                    destroyWindow( "Histogram" );
                else
                    namedWindow( "Histogram", 1 );
                break;
            case 'p':
                paused = !paused;
                break;
            default:
            ;
        }
    }

    return 0;
}
