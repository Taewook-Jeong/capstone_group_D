#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <string>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <boost/thread.hpp>


#include <ros/ros.h>
#include <ros/package.h>
#include "core_msgs/ball_position.h"
#include "core_msgs/ball_position_modify.h"
#include "ros/ros.h"
#include "sensor_msgs/LaserScan.h"
#include "std_msgs/Int8.h"
#include "std_msgs/String.h"


#include "opencv2/opencv.hpp"


#define RAD2DEG(x) ((x)*180./M_PI)

#define PORT 4000
#define IPADDR "172.16.0.1" // myRIO ipadress

using namespace std;

boost::mutex map_mutex;

// webcam1 global variables
int web1_blue_number=0;//number of blue balls detected
int web1_red_number=0;//number of red balls detected
float web1_blue_X_array[20];//x poisition array of blue balls
float web1_blue_X = -100;//x position of blue ball
float web1_blue_Z_array[20];//z position array of blue balls
float web1_blue_Z = -100;//z position of blue ball
float web1_red_X_array[20];//x position of blue ball
float web1_red_X = -100;//x poisition of red ball
float web1_red_Y_array[20];//y position array of red balls
float web1_red_Y = -100;//y position of red ball
float web1_red_Z_array[20];//z position array of red balls
float web1_red_Z = 100;//z position of red ball
int web1_green_number=0;//number of green balls
float web1_green_X_array[20];//x position array of green balls
float web1_green_X=-100;//x position of green ball
float web1_green_Y_array[20];//y position array of green balls
float web1_green_Y=100;//y position of green ball
float web1_green_Z_array[20];//z position array of green balls
float web1_green_Z=-100;//z position of green ball
float web1_green_X_min;//x value of green ball at the very left
float web1_green_X_max;//x value of green ball at the very right
float web1_green_X_closest;//x value of closest green ball
float web1_green_Y_closest;//y value of closest green ball
float web1_green_Z_closest;//z value of closest green ball
float web1_green_X_target;//x value of targeted green ball - one that robot should follow
float web1_green_X_average;//average x value of green balls
float web1_green_Y_left;//y value of left green ball
float web1_green_Y_right;//y value of right green ball
float web1_center=0; // x position of center of webcam1 - for calibration


// webcam2 global variables
int web2_red_number=0; //number of red balls
float web2_red_X_array[20];//x position array of red balls
float web2_red_X = -100;//x position of red ball
int web2_green_number = 0;//number of green balls
int web2_blue_number=0;//number of blue balls
float web2_blue_X_array[20];//x position array of blue balls
float web2_green_Y_array[20];//y position array of green balls
float web2_blue_X = -100;//x position of blue ball
float web2_green_X = -100;//x position of green ball
float web2_green_Y = -100;//y position of green ball
float web2_green_Z = 100;//z position of green ball
float web2_green_X_max;//x position of green ball at the very right
float web2_green_X_min;//x position of green ball at the very left
float web2_green_X_array[20];//x position array of green balls
float web2_green_Z_array[20];//z positon array of green balls
float web2_center=0;//x position of center of webcam2 - for calibration


// etc 	variables
float t = 0.025;//duration time

float insurance =0;//variable needed to move to release mode when no blue ball found

int leftright = -1; //when left: 0 when right: 1 - value to decide whether the closest green ball is left or right at releasing part

int collection=0;// number of collected blue balls

float acc=0.1;//acceleration when moving

float suc=10;//variable to count suction time and 'collection'

int release_direction = 0;//variable to decide whether we would turn CW or CCW when looking for basket

int suction_switch=0;//variable to help avoid redundant counting of 'collection'

int c_socket, s_socket;
int len;
int n;
float data[24];
struct sockaddr_in c_addr;

void dataInit()
{
	data[0] = 0; //lx*data[3];
	data[1] = 0; //ly*data[3];
	data[2] = 0; //GamepadStickAngle(_dev, STICK_LEFT);
	data[3] = 0; //GamepadStickLength(_dev, STICK_LEFT);
	data[4] = 0; //rx*data[7];
	data[5] = 0; //ry*data[7];
	data[6] = 0; //GamepadStickAngle(_dev, STICK_RIGHT);
	data[7] = 0; //GamepadStickLength(_dev, STICK_RIGHT);
	data[8] = 0; //GamepadTriggerLength(_dev, TRIGGER_LEFT);
	data[9] = 0; //GamepadTriggerLength(_dev, TRIGGER_RIGHT);
	data[10] = 0; //GamepadButtonDown(_dev, BUTTON_DPAD_UP);
	data[11] = 0; //GamepadButtonDown(_dev, BUTTON_DPAD_DOWN);
	data[12] = 0; //GamepadButtonDown(_dev, BUTTON_DPAD_LEFT);
	data[13] = 0; //GamepadButtonDown(_dev, BUTTON_DPAD_RIGHT);
	data[14] = 0; //GamepadButtonDown(_dev, BUTTON_A);
	data[15] = 0; //GamepadButtonDown(_dev, BUTTON_B);
	data[16] = 0; //GamepadButtonDown(_dev, BUTTON_X);
	data[17] = 0; //GamepadButtonDown(_dev, BUTTON_Y);
	data[18] = 0; //GamepadButtonDown(_dev, BUTTON_BACK);
	data[19] = 0; //GamepadButtonDown(_dev, BUTTON_START);
	data[20] = 0; //GamepadButtonDown(_dev, BUTTON_LEFT_SHOULDER);//servo motor
	data[21] = 0; //GamepadButtonDown(_dev, BUTTON_RIGHT_SHOULDER);//suction
	data[22] = 0; //GamepadButtonDown(_dev, BUTTON_LEFT_THUMB);
	data[23] = 0; //GamepadButtonDown(_dev, BUTTON_RIGHT_THUMB);
}


void camera1_Callback(const core_msgs::ball_position::ConstPtr& position_modify1)
//dealing with ball informations from webcam1
{
	  map_mutex.lock();//do nothing else until camera1_Callback finishes

		//initializing position value of balls
	  web1_blue_X = -100;
    web1_blue_Z = -100;
	  web1_red_X = -100;
		web1_red_Y = -100;
	  web1_red_Z = 100;
	  web1_green_X = -100;
	  web1_green_Y = -100;
		web1_green_Z = 100;

	  // assigning number of balls
    web1_blue_number = position_modify1->b_size;
		web1_green_number= position_modify1->g_size;
		web1_red_number = position_modify1->r_size;

    //assigning x,z position of blue ball at the very right
    for(int i = 0; i < web1_blue_number; i++)
    {
        web1_blue_X_array[i] = position_modify1->b_img_x[i];
        web1_blue_Z_array[i] = position_modify1->b_img_z[i];
				if(web1_blue_X < position_modify1->b_img_x[i]){
					web1_blue_X = position_modify1->b_img_x[i];
					web1_blue_Z = position_modify1->b_img_z[i];
				}
    }

		//assigning x, y, z position of closest red ball
		for(int i = 0; i < web1_red_number; i++)
		{
				web1_red_X_array[i] = position_modify1->r_img_x[i];
				web1_red_Z_array[i] = position_modify1->r_img_z[i];
				web1_red_Y_array[i] = position_modify1->r_img_y[i];
				if(web1_red_Y < position_modify1->r_img_y[i]){
					web1_red_X = position_modify1->r_img_x[i];
					web1_red_Z = position_modify1->r_img_z[i];
					web1_red_Y = position_modify1->r_img_y[i];
				}
		}

		// assigning x,y,z positions of green balls
		for(int i = 0; i < web1_green_number; i++)
		{
				web1_green_X_array[i] = position_modify1->g_img_x[i];
				web1_green_Y_array[i] = position_modify1->g_img_y[i];
				web1_green_Z_array[i] = position_modify1->g_img_z[i];
		}

		//assigning x, z position of green ball when only 1 green ball detected
		web1_green_Y_closest = web1_green_Y_array[0];
		web1_green_X_closest = web1_green_X_array[0];
		web1_green_Z_closest = web1_green_Z_array[0];

    //assigning y position of closest green ball(decided in y coordinate) when more than 1 green ball detected
		for(int i=1; i<web1_green_number; i++){
				if(web1_green_Y_closest < web1_green_Y_array[i]){
					web1_green_Y_closest = web1_green_Y_array[i];
				}
		}

		//assigning x, z position of closest green ball(decided in z coordinate) when more than 1 green ball detected
		for(int i=1; i<web1_green_number; i++){
				if(web1_green_Z_closest > web1_green_Z_array[i]){
					web1_green_Z_closest = web1_green_Z_array[i];
					web1_green_X_closest = web1_green_X_array[i];
				}
		}

   //assigning same x position as left and right green ball  when only 1 green ball detected
		web1_green_X_min = web1_green_X_array[0];
		web1_green_X_max = web1_green_X_array[0];

  //assigning x value of green ball at the very left and very right
		for(int i=1; i<web1_green_number; i++){
				if(web1_green_X_min > web1_green_X_array[i]){
					web1_green_X_min = web1_green_X_array[i];
					}
				if(web1_green_X_max < web1_green_X_array[i]){
					web1_green_X_max = web1_green_X_array[i];
					}
		}

    //initializing average x value of green balls as 0
		web1_green_X_average = 0;

    //calculating average value of all the green balls detected
		for(int i=0; i<web1_green_number; i++){
				web1_green_X_average = web1_green_X_average + web1_green_X_array[i];
		}
		web1_green_X_average = web1_green_X_average/web1_green_number;

    //if the closest green ball is left one, our target ball is left one
		if(leftright == 0){
				web1_green_X_target = web1_green_X_min;
		}
		//if the closest green ball is right one, our target ball is right one
		else if(leftright == 1){
				web1_green_X_target = web1_green_X_max;
		}
		//if not yet decided about leftright, do nothing
		else{
		}

    map_mutex.unlock();
}


//dealing with ball informations from webcam2
void camera2_Callback(const core_msgs::ball_position::ConstPtr& position_modify2)
{
	  map_mutex.lock();//do nothing until camear2_Callback finishes

		//initializing position values of balls
		web2_red_X = -100;
		web2_blue_X = -100;
		web2_green_Z = 100;
		web2_green_Y = -100;

	  // assigning number of balls detected
    web2_red_number = position_modify2->r_size;
		web2_blue_number = position_modify2->b_size;
		web2_green_number = position_modify2->g_size;

		// assigning x position of closest red ball
    for(int i = 0; i < count1; i++)
    {
        web2_red_X_array[i] = position_modify2->r_img_x[i];
				if(web2_red_X < position_modify2->r_img_x[i]){
					web2_red_X = position_modify2->r_img_x[i];
				}
    }

		// assigning x position of blue ball at the very right
		for(int i = 0; i < web2_blue_number; i++)
		{
				web2_blue_X_array[i] = position_modify2->b_img_x[i];
				if(web2_blue_X < position_modify2->b_img_x[i]){
					web2_blue_X = position_modify2->b_img_x[i];
				}
		}

		//assigning x, y, z position of closest green ball
		for(int i = 0; i < web2_green_number; i++)
		{
				web2_green_X_array[i] = position_modify2->g_img_x[i];
				web2_green_Z_array[i] = position_modify2->g_img_z[i];
				//do not assign noise(noise sometimes occur as x=0 && z=0)
				if(web2_green_Z > position_modify2->g_img_z[i] && position_modify2->g_img_z[i] != 0){
					web2_green_X = position_modify2->g_img_x[i];
					web2_green_Z = position_modify2->g_img_z[i];
					web2_green_Y = position_modify2->g_img_y[i];
				}
		}

		//find max and min of web2_green_x when web2 green number > 2.
		web2_green_X_max = web2_green_X_array[0];
		web2_green_X_min = web2_green_X_array[0];
		for(int i = 0; i < web2_green_number; i++)
		{
				if(web2_green_X_max < position_modify2->g_img_x[i]){
					web2_green_X_max = position_modify2->g_img_x[i];
				}
				if(web2_green_X_min > position_modify2->g_img_x[i]){
					web2_green_X_min = position_modify2->g_img_x[i];
				}
		}


		map_mutex.unlock();
}

//function to turn off the suction
void suction_check(){
	//activate suction motor for 1.5 secs
	if(suc<=1.5){
		data[21]=1;
	}
	else{
		data[21]=0;
	}
}

//function to count 'suc' and number of balls detected
void sleep_count(float sleeprate){

	ros::Duration(sleeprate).sleep();

	suc = suc + sleeprate;

  //recognize a ball collected, 0.5sec after blue ball disappears from webcam2 with suction_switch on
	if(collection<2){
		if(0.6<=suc && suction_switch ==1){
				collection = collection + 1;
				suction_switch =0;
				}
			 }
	//do the ball counting faster when last ball is collected - in order to go over to release mode quickly
  else{
		if(0.1<=suc && suction_switch ==1){
			collection = collection + 1;
			suction_switch = 0;
		}
	}
}

void move_forward(float v){
	//move slowly when red ball is close in the forward direction, except for the releasing step
	//accelerate til speed 0.5
	if(web1_red_Y > 2.52 && collection < 3){
		if(data[1] < 0.5){
		 data[0] = 0;
		 data[1] = data[1]+acc;
		 data[4] = 0;
		 data[5] = 0;
	  }
		else if(data[1] > 0.5){
		 data[0] = 0;
		 data[1] = data[1]-acc;
		 data[4] = 0;
		 data[5] = 0;
	  }
	}
	 //move as speed v, when red ball is not close to the ball
	 //accelerate til speed v
	 else{
		 if(data[1] < v){
	 	 data[0] = 0;
	 	 data[1] = data[1]+acc;
	 	 data[4] = 0;
	 	 data[5] = 0;
	   }
	 	else if(data[1] > v){
	 	 data[	0] = 0;
	 	 data[1] = data[1]-acc;
	 	 data[4] = 0;
	 	 data[5] = 0;
	   }
	 }
	suction_check();
	write(c_socket, data, sizeof(data));
	ROS_INFO("%f, %f, %f, %f, %f", data[0], data[1], data[4], data[5],	data[21]);
 }

void turn_CW(float w){
 //turn CW with speed of w
 data[0] = 0;
 data[1] = 0;
 data[4] = w;
 data[5] = 0;
 suction_check();
 write(c_socket, data, sizeof(data));
 ROS_INFO("%f, %f, %f, %f, %f", data[0], data[1], data[4], data[5],	data[21]);
}

void turn_CCW(float w){
 //turn CCW with speed of w
 data[0] = 0;
 data[1] = 0;
 data[4] = -w;
 data[5] = 0;
 suction_check();
 write(c_socket, data, sizeof(data));
ROS_INFO("%f, %f, %f, %f, %f", data[0], data[1], data[4], data[5],	data[21]);
}


void move_left(float v){
	//move left, accelerate til speed v
	if(data[0]>-v){
	data[0] = data[0]-acc;
	data[1] = 0;
	data[4] = 0;
	data[5] = 0;
   }
	suction_check();
	write(c_socket, data, sizeof(data));
	ROS_INFO("%f, %f, %f, %f, %f", data[0], data[1], data[4], data[5],	data[21]);
}

void move_right(float v){
	//move right, accelerate til speed 0.8
	if(data[0]<v){
	data[0] = data[0]+acc;
	data[1] = 0;
	data[4] = 0;
	data[5] = 0;
}
  suction_check();
	write(c_socket, data, sizeof(data));
	ROS_INFO("%f, %f, %f, %f, %f", data[0], data[1], data[4], data[5],	data[21]);
}

void pick_up(){
	if(web2_blue_X > -2.0 && web2_blue_X < 1.7){
		suction_switch = 1; //turn on suction_switch when blue ball comes into certain scope during pick_up mode - in order to count ball collected
	}
  suc = 0;//initializing suc in order to turn on suction motor

	//adjusting direction towards blue ball
	if(web2_blue_X>0.8+web2_center){//if x position of blue ball is over 1
	 while(web2_blue_X>0.5+web2_center && web2_blue_number != 0){//turn CW until x positoin of blue ball is less than 0.6
		 turn_CW(0.28);
		 ros::spinOnce();
		 ros::Duration(t).sleep();
	 }
 }
	else if(web2_blue_X<-0.8+web2_center){//if x position of blue ball is less than -1
	 while(web2_blue_X<-0.5+web2_center && web2_blue_number != 0){//turn CCW until x position of blue ball is over -0.6
		turn_CCW(0.28);
		ros::spinOnce();
		ros::Duration(t).sleep();
		}
	 }
	else{//go forward if blue ball is in the middle
	 move_forward(0.3);
  }
}

void release(){
          // webcam1 green ball center targeting
          bool a = true;//initializing 'a' when starting release step
          while(web2_green_number==0 && a){
            ros::spinOnce();
            sleep_count(t);
            if(web1_green_number <2){//when less then 2 green balls are detected in webcam1
							if(release_direction == 0){//turn CCW when green ball is detected while looking for last blue ball
								turn_CCW(0.4);
							}
              else{//turn CW when green ball is not detected
								turn_CW(0.4);
							}
            }
            else{ //when 2 green balls are detected in webcam1
              if(web1_green_X_average > 1.3+web1_center){//when average x position of 2 green balls is over 1.3
								//turn CW until the average x position of 2 green balls is lower than 0.6
                while(web1_green_X_average>0.6+web1_center && web1_green_number>=2 && web2_green_number ==0){
                  turn_CW(0.35);
                  ros::spinOnce();
                  sleep_count(t);
                }
              }
             else if(web1_green_X_average < -1.3+web1_center){//when average x position of 2 green balls is lower than -1.3
							 //turn CCW until the average x position of 2 green balls is over -0.6
               while(web1_green_X_average <-0.6+web1_center && web1_green_number>=2 && web2_green_number ==0){
                 turn_CCW(0.35);
                 ros::spinOnce();
                 sleep_count(t);
               }
             }
             else{
               a=false;//change 'a' to false - do the center targeting only once during the drive
             }
           }
         }


         // decide closest ball (left? right?)
         if(abs(web1_green_X_min-web1_green_X_closest)<0.05){
             leftright = 0;   // left = closest.
         }
         else{
             leftright = 1;   // right = closest.
         }

				 bool b = true;//initializing 'b' outside the while loop
         // go to closest
         while(ros::ok()){
           ros::spinOnce();
           sleep_count(t);

					 //adjusting direction towards green ball
					 if (web1_green_Y_closest>1.8 && b){//when green ball is close enough in webcam1
						 if(abs(web1_green_X_min-web1_green_X_closest)<0.05){//decide whether the ball following right now is left or right ball
		             leftright = 0;   // left = closest.
		         }
		         else{
		             leftright = 1;   // right = closest.
		         }
					 b = false;//do the left-right judging only once in a drive

           if (web2_green_number!=0){//when green ball is detected in webcam2
               if(web2_green_Y > 2.7){//when green ball is close enough
                   if (leftright == 0){//when the green ball detedcted is left ball
										 //move right until green ball disappears from webcam2
                     while(web2_green_number != 0 && web2_green_X_max-web2_green_X_min<400){
                       move_right(0.4);
                       ros::spinOnce();
                       sleep_count(t);
                     }
										 //turn on the servomotor - to release balls collected
                     for(float k=0; k<2; k=k+t){
                      data[20]=1;
                      data[0]=0;
                      data[1]=0;
                      data[4]=0;
                      data[5]=0;
                      write(c_socket, data, sizeof(data));
                      sleep_count(t);
                     }
                     dataInit();//do nothing
                     break; //end while looop
                   }
                   else if(leftright ==1){//when the green ball detected is right ball
										 //move left until green ball disappears from webcam2
                     while(web2_green_number != 0 && web2_green_X_max-web2_green_X_min<400){
                       move_left(0.35);
                       ros::spinOnce();
                       sleep_count(t);
                     }
										 //turn on the servo motor - to release balls collected
                     for(float k=0; k<2; k=k+t){
                        data[20]=1;
                        data[0]=0;
                        data[1]=0;
                        data[4]=0;
                        data[5]=0;
                      write(c_socket, data, sizeof(data));
                      sleep_count(t);
                     }
                     dataInit();//do nothing
                     break; //end while loop
                   }
                   else{//in case leftright was not decided, do nothing and stop release mode
                     break;
                   }
               }
               else{//move towards greenball until the distacne is close enough
                 move_forward(0.25);
               }
           }

           else{//if no green ball is detected in webcam2
             if(web1_green_X_target+web1_center > 1){ //when x position of targeted green ball is over 1
							   //turn CW until x position of trageted green ball is lower than 0.7
                 while(web1_green_X_target+web1_center > 0.7 && web1_green_number != 0 && web2_green_number ==0){
                   turn_CW(0.35);
                   ros::spinOnce();
                   sleep_count(t);
                 }
             }
             else if(web1_green_X_target < -1+web1_center){//when x position of targeted green ball is lower than -1
							   //turn CW until x position of trageted green ball is over -0.7
                 while(web1_green_X_target <-0.7+web1_center && web1_green_number != 0 && web2_green_number ==0){
                   turn_CCW(0.35);
                   ros::spinOnce();
                   sleep_count(t);
                 }
             }
             else{//when web1_green_closest in middle
                 move_forward(1);
             }
         }
       }
  }

int main(int argc, char **argv)
{
    ros::init(argc, argv, "data_integration");
    ros::NodeHandle n;

		//subscribe modified ball position information from webcam1
    ros::Subscriber sub1 = n.subscribe<core_msgs::ball_position>("/position_modify1", 1000, camera1_Callback);

		//subscribe modified ball position information from webcam2
    ros::Subscriber sub2 = n.subscribe<core_msgs::ball_position>("/position_modify2", 1000, camera2_Callback);

		c_socket = socket(PF_INET, SOCK_STREAM, 0);
		dataInit();

    c_addr.sin_addr.s_addr = inet_addr(IPADDR);
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(PORT);

     if(connect(c_socket, (struct sockaddr*) &c_addr, sizeof(c_addr)) == -1){
        printf("Failed to connect\n");
        close(c_socket);
         return -1;
    }


		while(ros::ok){
      ros::spinOnce();
			if(collection>2){//if collected ball is over 2, go to release mode
				release();
				data[0] = 0;
			  data[1] = 0;
			  data[4] = 0;
			  data[5] = 0;
				break;
			 }
			else{//when number of collected balls is less than 3
			 if(web2_blue_number!=0){//start pick_up when blue ball is detected in webcam2
				pick_up();
			 }
			 else{//when blue ball is not detected in webcam2
				if(web2_red_number != 0){//when red ball is not detected in webcam2
					if(web2_red_X > 0){//when x position of red ball in webcam2 is in the right side
						//move left until red ball disappears in webcam2
						while(web2_red_number != 0){
							move_left(1);
							ros::spinOnce();
							sleep_count(t);
						}
					}
					else{//when x posotion of red ball in webcam2 is in the left side
						//move right until red ball disappears in webcam2
						while(web2_red_number != 0){
							move_right(1);
							ros::spinOnce();
							sleep_count(t);
						}
					}
				}
				else{//when red ball is not deteced in webcam2
					if(web1_blue_X>1.3+web1_center){//when x positon of blue ball in webcam1 is over 1.3
					 //turn CW until x position of blue ball in webcam is lower than 1.1
					 while(web1_blue_X>1.1+web1_center && web2_blue_number==0 && collection < 3){
						 turn_CW(0.7);
						 ros::spinOnce();
		 				 sleep_count(t);
					 }
				}
					else if(web1_blue_X<-1.3+web1_center){//when x positon of blue ball in webcam1 is less than -1.3
					 insurance = 0;//initialize variable 'insurance' before blue ball searching
					 while(web1_blue_X<-1.1+web1_center && web2_blue_number==0 && collection < 3){//do the followings until x position of blue ball in webcam 1 is less than -1.1
						if(collection == 2 && web1_green_number != 0 && web1_blue_X == -100){//when green ball is detected while searching for 3rd blue ball
							release_direction = 1;//decide to turn CW when searching for basket after collecting last ball
						}
						if(web1_blue_number == 0){//turn CCW searching for blue ball in webcam1
							turn_CCW(1);
							insurance = insurance+t;// accumulate variable 'insurance' while blue ball is not found
							if(insurance > 4){
								suction_switch=1;//if blue ball is not foud for 4 seconds(1 full turn), turn on suction_switch to decide as 1 blue ball collected
							}
						}
						else{//turn a bit slower when blue ball is in sight, than when no blue ball is found in webcam1
							turn_CCW(0.7);
						}
						ros::spinOnce();
						sleep_count(t);
					  }
					}
					else{//web1_blue_X at middle
						 if(web1_red_Y > 3.5){//start avoiding when red ball is close
								if(web1_red_X>0+web1_center){//when red ball is on right side
									//move left until red ball disappears from webcam1
									//do not avoid red ball far away from kirby
								  while(web1_red_number!=0 && web1_red_Y > 3 && web2_blue_number==0 && collection < 3){
									move_left(1);
									ros::spinOnce();
			 	 				  sleep_count(t);
						 			}
						      float k = 0;//initializing variable 'k'
									bool c = true;//initializing 'c' as true
									//go forward for 0.8 seconds
									while(k<0.8 && c){
										ros::spinOnce();
										if(web2_blue_number != 0){//when blue ball is detected in webcam2 while moving forward
											pick_up();
											c = false;//stop and get out of the while loop after picking up the blue ball
										}
										data[0] = 0;
							 	 	  data[1] = 1;
							 	 	  data[4] = 0;
							 	 	  data[5] = 0;
										sleep_count(t);
										suction_check();
										write(c_socket, data, sizeof(data));
										k=k+t;
									}
									k=0;//initializing variable 'k'
									//turn CW for 0.5 seconds
									//do not turn CW when pick_up was done while moving forward in avoiding step
									while(k<0.5 && c){
										data[0] = 0;
										data[1] = 0;
										data[4] = 0.8;
										data[5] = 0;
										suction_check();
										write(c_socket, data, sizeof(data));
										sleep_count(t);
										k=k+t;
									}
								}

							 else{//when red ball is on left side of webcam1
								 //move right until red ball disappears from webcam1
								 //do not avoid red ball far away from kirby
								 while(web1_red_number != 0 && web1_red_Y > 3 && web2_blue_number==0 && collection < 3){
								 move_right(1);
								 ros::spinOnce();
				 				 sleep_count(t);
						}
								 bool c = true;//intializing 'c' to true
								 float k = 0;//initializing variable 'k'
								 //go forward for 0.8 seconds
								 while(k<0.8 && c){
									 ros::spinOnce();
									 if(web2_blue_number != 0){
										 pick_up();
										 c = false;//stop and get out of the while loop after picking up the blue ball
									 }
									 data[0] = 0;
									 data[1] = 1;
									 data[4] = 0;
									 data[5] = 0;
									 sleep_count(t);
									 suction_check();
									 write(c_socket, data, sizeof(data));
									 k=k+t;
								 }
						 }
					  }
					 else{//when x position of blue ball in webcam1 is in the middle
						 move_forward(1);

				  	 }
				 	 }
				 }

	  		}
			}
		ROS_INFO("%f, %f, %f, %f, %f", data[0], data[1], data[4], data[5],	data[21]);
    sleep_count(t);
		}
    return 0;
}
