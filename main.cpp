#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "dxl.hpp"
#include "opencv2/opencv.hpp"
using namespace cv;
using namespace std;
bool ctrl_c_pressed;
void ctrlc(int)
{
ctrl_c_pressed = true;
}
int main(void)
{
    // image transport
    string src = "nvarguscamerasrc sensor-id=0 ! video/x-raw(memory:NVMM), width=(int)640, height=(int)360, format=(string)NV12, framerate=(fraction)30/1 ! nvvidconv flip-method=0 ! video/x-raw, width=(int)640, height=(int)360, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
   
    VideoCapture source(src, CAP_GSTREAMER);
    if (!source.isOpened()){ cout << "Camera error"<< endl; return -1; }
    
    string dst1 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! h264parse ! rtph264pay pt=96 ! udpsink host=203.234.58.154 port=8001 sync=false";
    VideoWriter writer1(dst1, 0, (double)30,Size(640,360),true);
    VideoWriter savefile("manualdrive2.avi", VideoWriter::fourcc('X','2','6','4'), (double)30, Size(640, 360), true);
    if(!writer1.isOpened()) {cerr<<"Writer open failed!"<<endl; return -1;}
    Mat frame, gray;
    // dynamixel control
    Dxl mx;
    struct timeval start,end1;
    double diff1;
    int vel1=0,vel2=0;
    int goal1=0,goal2=0;
    signal(SIGINT, ctrlc); //시그널 핸들러 지정
    if(!mx.open()) { cout << "dxl open error"<<endl; return -1; }
//장치열기
    while(true)
    {
        gettimeofday(&start,NULL);
        source >> frame;
        if (frame.empty()){ cerr << "frame empty!" << endl; break; }
        writer1 << frame; //image send
        savefile.write(frame);
        if (mx.kbhit())
        {
        char c = mx.getch();
        switch(c)
        {
            case 'q': goal1 = 0; goal2 = 0; break;
            case ' ': goal1 = 0; goal2 = 0; break;
            case 'w': goal1 = 50; goal2 = -50; break;
            case 's': goal1 = -50; goal2 = 50; break;
            case 'a': goal1 = -50; goal2 = -50; break;
            case 'd': goal1 = 50; goal2 = 50; break;
            default : goal1 = 0; goal2 = 0; break;
        }
    }
        // generate accel and decel motion
        if(goal1>vel1) vel1 += 5;
        else if(goal1<vel1) vel1 -= 5;
        else vel1 = goal1;
        if(goal2>vel2) vel2 += 5;
        else if(goal2<vel2) vel2 -= 5;
        else vel2 = goal2;

        if(!mx.setVelocity(vel1,vel2)){ cout << "setVelocity error" <<endl; return -1;}
        
        if (ctrl_c_pressed) break;
        usleep(20*1000);
        gettimeofday(&end1,NULL);
        diff1 = end1.tv_sec + end1.tv_usec / 1000000.0 - start.tv_sec - start.tv_usec / 1000000.0;
        cout <<"vel1:" << vel1 <<','<<"vel2:"<< vel2 << ",time:" << diff1 << endl;}
    mx.close();
    return 0;
}
