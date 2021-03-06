#include "ros/ros.h"
#include "std_msgs/Empty.h"
#include "geometry_msgs/Twist.h"
#include "ardrone_autonomy/Navdata.h"
#include "sensor_msgs/Imu.h"
#include <sstream>
#include <iostream>
#include <math.h>
#include <string>
#include <eigen3/Eigen/Geometry>

ardrone_autonomy::Navdata msg_in;
geometry_msgs::Twist msgpos_in;
sensor_msgs::Imu msgyaw_in;
Eigen::Quaterniond quat_orientation;

#define PI 3.14149265

double yaw,pitch,roll;


void read_navdata(const ardrone_autonomy::Navdata::ConstPtr& msg)
{ 
  msg_in.altd = msg->altd; 
}

struct Quaternionm
{
    double w, x, y, z;
};
void GetEulerAngles(Quaternionm q, double& yaw, double& pitch, double& roll)
{
    const double w2 = q.w*q.w;
    const double x2 = q.x*q.x;
    const double y2 = q.y*q.y;
    const double z2 = q.z*q.z;
    const double unitLength = w2 + x2 + y2 + z2;    // Normalised == 1, otherwise correction divisor.
    const double abcd = q.w*q.x + q.y*q.z;
    const double eps = 1e-7;    // TODO: pick from your math lib instead of hardcoding.
    const double pi = 3.14159265358979323846;   // TODO: pick from your math lib instead of hardcoding.
    if (abcd > (0.5-eps)*unitLength)
    {
        yaw = 2 * atan2(q.y, q.w);
        pitch = pi;
        roll = 0;
    }
    else if (abcd < (-0.5+eps)*unitLength)
    {
        yaw = -2 * ::atan2(q.y, q.w);
        pitch = -pi;
        roll = 0;
    }
    else
    {
        const double adbc = q.w*q.z - q.x*q.y;
        const double acbd = q.w*q.y - q.x*q.z;
        yaw = ::atan2(2*adbc, 1 - 2*(z2+x2));
        pitch = ::asin(2*abcd/unitLength);
        roll = ::atan2(2*acbd, 1 - 2*(y2+x2));
    }
}


void read_yaw(const sensor_msgs::Imu::ConstPtr& msgyaw)
{
  msgyaw_in.orientation.x =  msgyaw->orientation.x;
  msgyaw_in.orientation.y =  msgyaw->orientation.y;
  msgyaw_in.orientation.z =  msgyaw->orientation.z;
  msgyaw_in.orientation.w =  msgyaw->orientation.w;
  Quaternionm myq;
  myq.x = msgyaw->orientation.x;
  myq.y = msgyaw->orientation.y;
  myq.z = msgyaw->orientation.z;
  myq.w = msgyaw->orientation.w;  
  GetEulerAngles(myq, yaw, pitch, roll);
 
  //ROS_INFO("Angles %lf %lf %lf",yaw,pitch,roll);
}
int main(int argc, char **argv)
{
  ros::init(argc, argv, "altipid");
  ros::NodeHandle n;
  
  std_msgs::Empty my_msg;  
  geometry_msgs::Twist m;
  ros::Publisher takeoff = n.advertise<std_msgs::Empty>("/ardrone/takeoff", 10,true);
  takeoff.publish(my_msg);
  
  ros::Publisher twist;
  twist = n.advertise<geometry_msgs::Twist>("/cmd_vel", 1);

  ros::Subscriber read = n.subscribe("/ardrone/navdata", 1000, read_navdata);
  ros::Subscriber readyaw = n.subscribe("/ardrone/imu", 1000, read_yaw);
   int c;

      for(c=0;c<10;c++)
      c++;

       ros::Rate loop_rate(10);
     
       m.linear.z = 0.0;
       m.linear.y = 0.0;
       m.angular.x = 0.0;
       m.angular.y = 0.0;
       m.angular.z = 0.0;
       m.linear.x = 0.0;  
        
       double z1,y1;
       double z = 1000.0;
       double y =45.0;
       z1 = msg_in.altd;
       //don't use this conversion   use library for conversion
       y1 = yaw*180/PI;
       double error,pre_error,integral,derivative,errory,pre_errory,integraly,derivativey;
       integral =0;
       integraly = 0;
       pre_error = 0;
       pre_errory = 0;
       double kp,ki,kd,kpy,kiy,kdy;
       if(argc > 1)
       {
          kp = std::atof(argv[1]);
          kd = std::atof(argv[2]);
          ki = std::atof(argv[3]);
          kpy = std::atof(argv[4]);
          kdy = std::atof(argv[5]);
          kiy = std::atof(argv[6]);
        }
       else
       {
          kp = 0.11;
          ki = 0.0000081;
          kd = 2.0; 
	  kpy = 0.003;
	  kdy = 0.1;
	  kiy = 0.0; 
        }

       std::cout<<"Kp: "<<kp<<" Kd: "<<kd<<" Ki: "<<ki<<std::endl;
       std::cout<<"Kpy: "<<kpy<<" Kdy: "<<kdy<<" Kiy: "<<kiy<<std::endl;
    
        int region = 1;
	int regiony=1;
      while (ros::ok())
	 {    //altitude + yaw
       
	  if(abs(error) >500)
          region = 1;
        else if ((abs(error) < 500) && (abs(error) > 100))
          region = 5;
        else
	region = 10;
	 	    if(abs(errory) >500)
          regiony = 1;
        else if ((abs(errory) < 500) && (abs(errory) > 100))
          regiony = 5;
        else
	regiony = 10;
		     errory = (y - y1);
       integraly = integraly + errory;
       derivativey = errory-pre_errory;

        if(errory>=0.0)
		      {
			if(errory>=180.0)
			  errory = 360 - errory;
		      }
		    else if(errory<0.0)
		      {
		      	if(errory<=-180)
			errory = 360 + errory;
		      }
       


          error = (z - z1)/70.0;
       integral = integral + error;
       derivative = error-pre_error;
     if(abs(error)>0.01)
	 {
       //altitude + yaw
       
         m.linear.z = kp*error +  kd*derivative/region + ki*integral*region;
	 }
       else 
	 m.linear.z = 0;
       if(abs(errory)>0.00001)
	 {
	  m.angular.z = kpy*errory +  kdy*derivativey/regiony + kiy*integraly*regiony;
	 }
       else 
	 m.angular.z = 0;
   
       twist.publish(m);
       ROS_INFO("%lf %lf %lf %lf %lf %lf",z1,error,m.linear.z,y1,errory,m.angular.z); 
	
   pre_error = error;
	  z1 = msg_in.altd;
          pre_errory = errory;
  	  //don't use this conversion   use library
	  y1 = yaw*180/PI; 
 ros::spinOnce(); 
       loop_rate.sleep();
     
     }



}  

