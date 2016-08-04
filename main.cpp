#include "mouseCtrl.h"
#include "Guesture.h"
#include "main.hpp"
#include "handGesture.hpp"
#include "myImage.hpp"
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

void showWindows(Mat &src, Mat &bw){
	pyrDown(bw,bw);
	pyrDown(bw,bw);
	pyrDown(bw,bw);
	Rect roi( Point( 3*src.cols/4,0 ), bw.size());
	vector<Mat> channels;
	Mat result;
	for(int i=0;i<3;i++)
		channels.push_back(bw);
	merge(channels,result);
	result.copyTo(src(roi));
	//imshow("img1",m.src);	
}

int findBiggestContour(vector<vector<Point> > contours)
{
    int indexOfBiggestContour = -1;
    int sizeOfBiggestContour = 0;
    for (int i = 0; i < contours.size(); i++){
        if(contours[i].size() > sizeOfBiggestContour){
            sizeOfBiggestContour = contours[i].size();
            indexOfBiggestContour = i;
        }
    }
    return indexOfBiggestContour;
}

void myDrawContours(Mat &src, Mat &bw, HandGesture *hg)
{
	drawContours(src,hg->hullP,hg->cIdx,cv::Scalar(200,0,0),2, 8, vector<Vec4i>(), 0, Point());

	rectangle(src,hg->bRect.tl(),hg->bRect.br(),Scalar(0,0,200));
	vector<Vec4i>::iterator d=hg->defects[hg->cIdx].begin();
	int fontFace = FONT_HERSHEY_PLAIN;
		
	
	vector<Mat> channels;
		Mat result;
		for(int i=0;i<3;i++)
			channels.push_back(bw);
		merge(channels,result);
	//	drawContours(result,hg->contours,hg->cIdx,cv::Scalar(0,200,0),6, 8, vector<Vec4i>(), 0, Point());
		drawContours(result,hg->hullP,hg->cIdx,cv::Scalar(0,0,250),10, 8, vector<Vec4i>(), 0, Point());

		
	while( d!=hg->defects[hg->cIdx].end() ) {
   	    Vec4i& v=(*d);
	    int startidx=v[0]; Point ptStart(hg->contours[hg->cIdx][startidx] );
   		int endidx=v[1]; Point ptEnd(hg->contours[hg->cIdx][endidx] );
  	    int faridx=v[2]; Point ptFar(hg->contours[hg->cIdx][faridx] );
	    float depth = v[3] / 256;
   /*	
		line( m->src, ptStart, ptFar, Scalar(0,255,0), 1 );
	    line( m->src, ptEnd, ptFar, Scalar(0,255,0), 1 );
   		circle( m->src, ptFar,   4, Scalar(0,255,0), 2 );
   		circle( m->src, ptEnd,   4, Scalar(0,0,255), 2 );
   		circle( m->src, ptStart,   4, Scalar(255,0,0), 2 );
*/
   		circle( result, ptFar,   9, Scalar(0,205,0), 5 );
		
		
	    d++;

   	 }
//	imwrite("./images/contour_defects_before_eliminate.jpg",result);

}

void makeContours(Mat &Mat_src, Mat &Mat_bw, HandGesture* hg){
	Mat aBw;
	pyrUp(Mat_bw,Mat_bw);
	Mat_bw.copyTo(aBw);
	findContours(aBw,hg->contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
	hg->initVectors(); 
	hg->cIdx=findBiggestContour(hg->contours);
	if(hg->cIdx!=-1){
//		approxPolyDP( Mat(hg->contours[hg->cIdx]), hg->contours[hg->cIdx], 11, true );
		hg->bRect=boundingRect(Mat(hg->contours[hg->cIdx]));		
		convexHull(Mat(hg->contours[hg->cIdx]),hg->hullP[hg->cIdx],false,true);
		convexHull(Mat(hg->contours[hg->cIdx]),hg->hullI[hg->cIdx],false,false);
		approxPolyDP( Mat(hg->hullP[hg->cIdx]), hg->hullP[hg->cIdx], 18, true );
		if(hg->contours[hg->cIdx].size()>3 ){
			convexityDefects(hg->contours[hg->cIdx],hg->hullI[hg->cIdx],hg->defects[hg->cIdx]);
			hg->eleminateDefects();
		}
		bool isHand=hg->detectIfHand();
		hg->printGestureInfo(Mat_src);
		if(isHand){	
			hg->getFingerTips(Mat_src);
			hg->drawFingerTips(Mat_src);
			myDrawContours(Mat_src, Mat_bw, hg);
		}
	}
}

void fillContours(Mat &bw)
{
    // Another option is to use dilate/erode/dilate:
	int morph_operator = 1; // 0: opening, 1: closing, 2: gradient, 3: top hat, 4: black hat
	int morph_elem = 2; // 0: rect, 1: cross, 2: ellipse
	int morph_size = 3; // 2*n + 1
    int operation = morph_operator + 2;

    // Apply the specified morphology operation
    Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );
    morphologyEx( bw, bw, operation, element );

}    

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
	HandGesture hg;

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

            //imshow("foreground mask", fgmask);
            if(FrameCount < 20)
            {    
                FrameCount++;
                continue;
            }
            else
                update_bg_model = false;
        }
        
        if( !paused )
        {
            cvtColor(image, hsv, COLOR_BGR2HSV);
            circle(circle_mask, circle_point, circle_raidus, Scalar(255), -1);
            mask = Scalar::all(0);
            mask = fgmask & circle_mask;
            //imshow("mask", mask);
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
                    //imshow( "Histogram", histimg );
                }
            }
            else
            {

                split(hsv, vectorOfHSVImages);
                circle_mask = Scalar::all(0);
                circle(circle_mask, circle_point, circle_raidus*2.5, Scalar(255), -1);
                if(!hist.empty())
                {    
                    backproj = Scalar::all(0);
                    hue = vectorOfHSVImages[0];
                    calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
                    threshold(backproj, backproj, 0, 255, THRESH_BINARY + THRESH_OTSU);
                    bitwise_and(backproj, fgmask, backproj);
                    bitwise_and(backproj, circle_mask, backproj);
                    RotatedRect trackBox = CamShift(backproj, trackWindow,
                                    TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
                    //GuestureRecognition(image, backproj);
                    fillContours(backproj);
		            makeContours(image, backproj, &hg);
		            hg.getFingerNumber(image);
		            showWindows(image, backproj);
                    if( trackWindow.area() <= 1 )
                    {
                        int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                        trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                                       trackWindow.x + r, trackWindow.y + r) &
                                  Rect(0, 0, cols, rows);
                    }

                    //if( backprojMode )
                    //    cvtColor( backproj, image, COLOR_GRAY2BGR );
                    circle_point = trackBox.center;
                    //mouseTo(circle_point.x, circle_point.y);
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
                FrameCount = 0;
                histimg = Scalar::all(0);
                init(frame);
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
