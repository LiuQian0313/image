// gen.cpp : 定义控制台应用程序的入口点。
//camshift实现跟踪


#include "stdafx.h"
#include <windows.h>
#include <vector>
using namespace std;

#include "cv.h"  
#include "highgui.h"  
#include <stdio.h>  
#include <ctype.h>  
  


IplImage *image = 0, *hsv = 0, *hue = 0, *mask = 0, *backproject = 0, *histimg = 0;  

//用HSV中的Hue分量进行跟踪  
CvHistogram *hist = 0;  
//直方图类  
int backproject_mode = 0;  
int select_object = 0;  
int track_object = 0;  
int show_hist = 1;  
CvPoint origin;  
CvRect selection;  
CvRect track_window;  
CvBox2D track_box;  


CvConnectedComp track_comp;  

int hdims = 16;  
//划分直方图bins的个数，越多越精确  
float hranges_arr[] = {0,180};  
//像素值的范围  
float* hranges = hranges_arr;  
//用于初始化CvHistogram类  
int vmin = 10, vmax = 256, smin =30;  
//用于设置滑动条  

void on_mouse( int event, int x, int y, int flags, void* param )  
//鼠标回调函数,该函数用鼠标进行跟踪目标的选择  
{  
    if( !image )  
        return;  
  
    if( image->origin )  
        y = image->height - y;  
    //如果图像原点坐标在左下,则将其改为左上  
      
    if( select_object )  
    //select_object为1,表示在用鼠标进行目标选择  
    //此时对矩形类selection用当前的鼠标位置进行设置  
    {  
        selection.x = MIN(x,origin.x);  
        selection.y = MIN(y,origin.y);  
        selection.width = selection.x + CV_IABS(x - origin.x);  
        selection.height = selection.y + CV_IABS(y - origin.y);  
          
        selection.x = MAX( selection.x, 0 );  
        selection.y = MAX( selection.y, 0 );  
        selection.width = MIN( selection.width, image->width );  
        selection.height = MIN( selection.height, image->height );  
        selection.width -= selection.x;  
        selection.height -= selection.y;  
    }  
  
    switch( event )  
    {  
    case CV_EVENT_LBUTTONDOWN:  
        //鼠标按下,开始点击选择跟踪物体  
        origin = cvPoint(x,y);  
        selection = cvRect(x,y,0,0);  
        select_object = 1;  
        break;  
    case CV_EVENT_LBUTTONUP:  
        //鼠标松开,完成选择跟踪物体  
        select_object = 0;  
        if( selection.width > 0 && selection.height > 0 )  
            //如果选择物体有效，则打开跟踪功能  
            track_object = -1;  
		


        break;  
    }  
}  
  
  
CvScalar hsv2rgb( float hue )  
//用于将Hue量转换成RGB量  
{  
    int rgb[3], p, sector;  
    static const int sector_data[][3]=  
        {{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};  
    hue *= 0.033333333333333333333333333333333f;  
    sector = cvFloor(hue);  
    p = cvRound(255*(hue - sector));  
    p ^= sector & 1 ? 255 : 0;  
  
    rgb[sector_data[sector][0]] = 255;  
    rgb[sector_data[sector][1]] = 0;  
    rgb[sector_data[sector][2]] = p;  
  
    return cvScalar(rgb[2], rgb[1], rgb[0],0);  
}  
  
int _tmain(int argc, _TCHAR* argv[])
{  
    CvCapture* capture = 0;  

	  capture = cvCaptureFromAVI( "E:\\vs上级程序（自己的）\\vs练习\\跟踪程序\\vv\\16.avi" );   
       // capture = cvCaptureFromAVI( "E:\\opencv2.4.6\\opencv\\samples\\gpu\\768x576.avi" );   
     
    if( !capture )  
    //打开视频流失败  
    {  
        fprintf(stderr,"Could not initialize capturing...\n");  
        return -1;  
    }  
  
    printf( "Hot keys: \n"  
        "\tESC - quit the program\n"  
        "\tc - stop the tracking\n"  
        "\tb - switch to/from backprojection view\n"  
        "\th - show/hide object histogram\n"  
        "To initialize tracking, select the object with mouse\n" );  
    //打印程序功能列表  
      
    cvNamedWindow( "Histogram", 0 );  
    //用于显示直方图  
		cvResizeWindow("Histogram",500,400);
    cvNamedWindow( "CamShiftDemo", 0 );  
	
    cvSetMouseCallback( "CamShiftDemo", on_mouse, 0 );  
    //用于显示视频  
	  cvNamedWindow( "反向投影",0 );  
	  	cvResizeWindow("反向投影",500,400);
		cvResizeWindow("CamShiftDemo",500,400);
	//设置鼠标回调函数  

   
    //设置滑动条  

    for(;;)  
    //进入视频帧处理主循环  
    {  
        IplImage* frame = 0;  
        int i, bin_w, c;  
  
        frame = cvQueryFrame( capture );  
        if( !frame )  
            break;  
  
        if( !image )  
        //image为0, 表明刚开始还未对image操作过,先建立一些缓冲区  
		//只进行一次
       {  
            image = cvCreateImage( cvGetSize(frame), 8, 3 );  
            image->origin = frame->origin;  
            hsv = cvCreateImage( cvGetSize(frame), 8, 3 );  
            hue = cvCreateImage( cvGetSize(frame), 8, 1 );  
         
			mask = cvCreateImage( cvGetSize(frame), 8, 1 );  
            //分配掩膜图像空间 

            backproject = cvCreateImage( cvGetSize(frame), 8, 1 );  
		
            //分配反向投影图空间,大小一样,单通道  
            hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );  
            
			
          
          
           
        }  
  
        cvCopy( frame, image, 0 );  
        cvCvtColor( image, hsv, CV_BGR2HSV );  
        //把图像从RGB表色系转为HSV表色系  
      //  Sleep(100);

        if( track_object )  
        //track_object非零,表示有需要跟踪的物体  
        {  
            int _vmin = vmin, _vmax = vmax;  
  
          cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),  
                  cvScalar(180,256,MAX(_vmin,_vmax),0), mask );  
            //制作掩膜板，只处理像素值为H：0~180，S：smin~256，V：vmin~vmax之间的部分  
			/*
			可用于检查图像中的像素灰度是否属于某一指定范围。
			
               0黑1白。
            */

			cvSplit( hsv, hue, 0, 0, 0 );  
            //分离H分量  
              
            if( track_object < 0 )  
            //如果需要跟踪的物体还没有进行属性提取，则进行选取框类的图像属性提取  
            {  
                float max_val = 0.f;  
                cvSetImageROI( hue, selection );  
                //设置原选择框为ROI  
            
				cvSetImageROI( mask, selection );  
               // 设置掩膜板选择框为ROI  
                cvCalcHist( &hue, hist, 0, mask );  
			
				//	cvNormalizeHist(hist,1);
				 //得到选择框内且满足掩膜板内的直方图  
                
				cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );  
                cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );  
                // 对直方图的数值转为0~255  
                cvResetImageROI( hue );  
                //去除ROI  
                
				cvResetImageROI( mask );  
                //去除ROI  
                
				track_window = selection;  
                track_object = 1;  
                //置track_object为1,表明属性提取完成  
             
                
				
            }  
  
       
			cvCalcBackProject( &hue, backproject, hist );  
            //计算hue的反向投影图  
          
		  cvErode(backproject,backproject);  
	    
		  cvDilate(backproject,backproject);  

		   cvAnd( backproject, mask, backproject, 0 );  
            //得到掩膜内的反向投影  

		   /*
			cvAnd:功能是计算两个数组的按位与的结果
			*/

            cvCamShift( backproject, track_window,  
                        cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 1000, 0.1 ),  
                        &track_comp, &track_box );  
            //使用MeanShift算法对backproject中的内容进行搜索,返回跟踪结果  
            track_window = track_comp.rect;  
            //得到跟踪结果的矩形框  
              
            if( backproject_mode )  
                cvCvtColor( backproject, image, CV_GRAY2BGR );  
                  
            if( image->origin )  
                track_box.angle = -track_box.angle;  
           cvEllipseBox( image, track_box, CV_RGB(255,0,0), 3, CV_AA, 0 );  
    	
		   cvRectangle( image , cvPoint(track_window.x,track_window.y),
			cvPoint(track_window.x+track_window.width,track_window.y+track_window.height ),
         CV_RGB(255,0,0),2, 8, 0 );

        }  
          
        if( select_object && selection.width > 0 && selection.height > 0 )  
        //如果正处于物体选择，画出选择框  
        {  
            cvSetImageROI( image, selection );  
            cvXorS( image, cvScalarAll(255), image, 0 );  
            cvResetImageROI( image );  
        }  
  
        cvShowImage( "CamShiftDemo", image );  
        cvShowImage( "Histogram", histimg );  
       cvShowImage( "反向投影", backproject );  
	 
        c = cvWaitKey(10);  
       
    }  
  
    cvReleaseCapture( &capture );  
    cvDestroyWindow("CamShiftDemo");  
  
    return 0;  
}  
  
#ifdef _EiC  
main(1,"gen.cpp");  
#endif


  