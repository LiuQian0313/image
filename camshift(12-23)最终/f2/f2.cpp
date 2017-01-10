// gen.cpp : �������̨Ӧ�ó������ڵ㡣
//camshiftʵ�ָ���


#include "stdafx.h"
#include <windows.h>
#include <vector>
using namespace std;

#include "cv.h"  
#include "highgui.h"  
#include <stdio.h>  
#include <ctype.h>  
  


IplImage *image = 0, *hsv = 0, *hue = 0, *mask = 0, *backproject = 0, *histimg = 0;  

//��HSV�е�Hue�������и���  
CvHistogram *hist = 0;  
//ֱ��ͼ��  
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
//����ֱ��ͼbins�ĸ�����Խ��Խ��ȷ  
float hranges_arr[] = {0,180};  
//����ֵ�ķ�Χ  
float* hranges = hranges_arr;  
//���ڳ�ʼ��CvHistogram��  
int vmin = 10, vmax = 256, smin =30;  
//�������û�����  

void on_mouse( int event, int x, int y, int flags, void* param )  
//���ص�����,�ú����������и���Ŀ���ѡ��  
{  
    if( !image )  
        return;  
  
    if( image->origin )  
        y = image->height - y;  
    //���ͼ��ԭ������������,�����Ϊ����  
      
    if( select_object )  
    //select_objectΪ1,��ʾ����������Ŀ��ѡ��  
    //��ʱ�Ծ�����selection�õ�ǰ�����λ�ý�������  
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
        //��갴��,��ʼ���ѡ���������  
        origin = cvPoint(x,y);  
        selection = cvRect(x,y,0,0);  
        select_object = 1;  
        break;  
    case CV_EVENT_LBUTTONUP:  
        //����ɿ�,���ѡ���������  
        select_object = 0;  
        if( selection.width > 0 && selection.height > 0 )  
            //���ѡ��������Ч����򿪸��ٹ���  
            track_object = -1;  
		


        break;  
    }  
}  
  
  
CvScalar hsv2rgb( float hue )  
//���ڽ�Hue��ת����RGB��  
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

	  capture = cvCaptureFromAVI( "E:\\vs�ϼ������Լ��ģ�\\vs��ϰ\\���ٳ���\\vv\\16.avi" );   
       // capture = cvCaptureFromAVI( "E:\\opencv2.4.6\\opencv\\samples\\gpu\\768x576.avi" );   
     
    if( !capture )  
    //����Ƶ��ʧ��  
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
    //��ӡ�������б�  
      
    cvNamedWindow( "Histogram", 0 );  
    //������ʾֱ��ͼ  
		cvResizeWindow("Histogram",500,400);
    cvNamedWindow( "CamShiftDemo", 0 );  
	
    cvSetMouseCallback( "CamShiftDemo", on_mouse, 0 );  
    //������ʾ��Ƶ  
	  cvNamedWindow( "����ͶӰ",0 );  
	  	cvResizeWindow("����ͶӰ",500,400);
		cvResizeWindow("CamShiftDemo",500,400);
	//�������ص�����  

   
    //���û�����  

    for(;;)  
    //������Ƶ֡������ѭ��  
    {  
        IplImage* frame = 0;  
        int i, bin_w, c;  
  
        frame = cvQueryFrame( capture );  
        if( !frame )  
            break;  
  
        if( !image )  
        //imageΪ0, �����տ�ʼ��δ��image������,�Ƚ���һЩ������  
		//ֻ����һ��
       {  
            image = cvCreateImage( cvGetSize(frame), 8, 3 );  
            image->origin = frame->origin;  
            hsv = cvCreateImage( cvGetSize(frame), 8, 3 );  
            hue = cvCreateImage( cvGetSize(frame), 8, 1 );  
         
			mask = cvCreateImage( cvGetSize(frame), 8, 1 );  
            //������Ĥͼ��ռ� 

            backproject = cvCreateImage( cvGetSize(frame), 8, 1 );  
		
            //���䷴��ͶӰͼ�ռ�,��Сһ��,��ͨ��  
            hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );  
            
			
          
          
           
        }  
  
        cvCopy( frame, image, 0 );  
        cvCvtColor( image, hsv, CV_BGR2HSV );  
        //��ͼ���RGB��ɫϵתΪHSV��ɫϵ  
      //  Sleep(100);

        if( track_object )  
        //track_object����,��ʾ����Ҫ���ٵ�����  
        {  
            int _vmin = vmin, _vmax = vmax;  
  
          cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),  
                  cvScalar(180,256,MAX(_vmin,_vmax),0), mask );  
            //������Ĥ�壬ֻ��������ֵΪH��0~180��S��smin~256��V��vmin~vmax֮��Ĳ���  
			/*
			�����ڼ��ͼ���е����ػҶ��Ƿ�����ĳһָ����Χ��
			
               0��1�ס�
            */

			cvSplit( hsv, hue, 0, 0, 0 );  
            //����H����  
              
            if( track_object < 0 )  
            //�����Ҫ���ٵ����廹û�н���������ȡ�������ѡȡ�����ͼ��������ȡ  
            {  
                float max_val = 0.f;  
                cvSetImageROI( hue, selection );  
                //����ԭѡ���ΪROI  
            
				cvSetImageROI( mask, selection );  
               // ������Ĥ��ѡ���ΪROI  
                cvCalcHist( &hue, hist, 0, mask );  
			
				//	cvNormalizeHist(hist,1);
				 //�õ�ѡ�������������Ĥ���ڵ�ֱ��ͼ  
                
				cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );  
                cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );  
                // ��ֱ��ͼ����ֵתΪ0~255  
                cvResetImageROI( hue );  
                //ȥ��ROI  
                
				cvResetImageROI( mask );  
                //ȥ��ROI  
                
				track_window = selection;  
                track_object = 1;  
                //��track_objectΪ1,����������ȡ���  
             
                
				
            }  
  
       
			cvCalcBackProject( &hue, backproject, hist );  
            //����hue�ķ���ͶӰͼ  
          
		  cvErode(backproject,backproject);  
	    
		  cvDilate(backproject,backproject);  

		   cvAnd( backproject, mask, backproject, 0 );  
            //�õ���Ĥ�ڵķ���ͶӰ  

		   /*
			cvAnd:�����Ǽ�����������İ�λ��Ľ��
			*/

            cvCamShift( backproject, track_window,  
                        cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 1000, 0.1 ),  
                        &track_comp, &track_box );  
            //ʹ��MeanShift�㷨��backproject�е����ݽ�������,���ظ��ٽ��  
            track_window = track_comp.rect;  
            //�õ����ٽ���ľ��ο�  
              
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
        //�������������ѡ�񣬻���ѡ���  
        {  
            cvSetImageROI( image, selection );  
            cvXorS( image, cvScalarAll(255), image, 0 );  
            cvResetImageROI( image );  
        }  
  
        cvShowImage( "CamShiftDemo", image );  
        cvShowImage( "Histogram", histimg );  
       cvShowImage( "����ͶӰ", backproject );  
	 
        c = cvWaitKey(10);  
       
    }  
  
    cvReleaseCapture( &capture );  
    cvDestroyWindow("CamShiftDemo");  
  
    return 0;  
}  
  
#ifdef _EiC  
main(1,"gen.cpp");  
#endif


  