/*! @file
 *
 *  @brief TODO
 *
 *  @author arosspope
 *  @date 12-10-2017
*/
#include "simulator.h"
#include "globalmap.h"

#include "sensor_msgs/image_encodings.h"
#include "geometry_msgs/PoseArray.h"
#include "nav_msgs/Odometry.h"
#include "prm_sim/RequestGoal.h"

#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <algorithm>
#include <limits>
#include <string>

//TODO: Examine header file...
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <signal.h>

namespace enc = sensor_msgs::image_encodings;
static std::mutex              GoalProcess;      /*!< TODO */
static std::condition_variable WaitOnGoal;      /*!< TODO */
static bool GoalRecieved = false;

Simulator::Simulator(ros::NodeHandle nh, TWorldInfoBuffer &buffer):
  buffer_(buffer), nh_(nh), it_(nh)
{
  path_     = nh_.advertise<geometry_msgs::PoseArray>("path", 1);
  overlay_  = it_.advertise("prm", 1);

  reqGoal_ = nh_.advertiseService("request_goal", &Simulator::requestGoal, this);

  //Get parameters from command line
  ros::NodeHandle pn("~");
  double mapSize;
  double mapResolution;
  double robotDiameter;
  pn.param<double>("map_size", mapSize, 20.0);
  pn.param<double>("resolution", mapResolution, 0.1);
  pn.param<double>("robot_diameter", robotDiameter, 0.2);

  //TODO: Proper flow?
  gmap_ = GlobalMap(mapSize, mapResolution, robotDiameter);
  //  gmap_.setMapSize(mapSize);
  //  gmap_.setResolution(mapResolution);
  //  gmap_.setRobotDiameter(robotDiameter);
}

void Simulator::prmThread() {
  //Initialise images and goals??
  cv::Mat ogMap;
  geometry_msgs::Pose robotPos;

  //We must wait until information about the world has been recieved
  //so that we can begin
  while(ros::ok()){
    int mapSz, poseSz;
    buffer_.access.lock();
    mapSz = buffer_.ogMapDeq.size();
    poseSz = buffer_.poseDeq.size();
    buffer_.access.unlock();

    if(mapSz > 0 && poseSz > 0){
      break;
    }
  }

  ROS_INFO("Ready to recieve requests...");
  while(ros::ok()){
    //Condition var to wait on goal...
    std::unique_lock<std::mutex> ulock(GoalProcess);

    WaitOnGoal.wait(ulock, []{return GoalRecieved || !ros::ok();});

    if(!ros::ok()){
      break;
    }

    GoalRecieved = false;

    //Get information about the world if available
    buffer_.access.lock();
    if(buffer_.ogMapDeq.size() > 0){
      ogMap = buffer_.ogMapDeq.front();
      buffer_.ogMapDeq.pop_front();
    }

    if(buffer_.poseDeq.size() > 0){
      robotPos = buffer_.poseDeq.front();
      buffer_.poseDeq.pop_front();
    }
    buffer_.access.unlock();

    //Update the reference for the localMap
    TGlobalOrd robotOrd = {robotPos.position.x, robotPos.position.y};
    gmap_.setReference(robotOrd);

    if(ogMap.empty()){
      ROS_ERROR("Empty OgMap.");
      continue;
    }

    //Create colour copy of the OgMap
    cv::Mat colourMap;
    cv::cvtColor(ogMap, colourMap, CV_GRAY2BGR);

    ROS_INFO("Starting build: {%f, %f} to {%f, %f}",
             robotOrd.x, robotOrd.y, currentGoal_.x, currentGoal_.y);

    int cnt(1);
    std::vector<TGlobalOrd> path = gmap_.build(ogMap, robotOrd, currentGoal_);
    while(path.size() == 0){
      cnt++;
      ROS_INFO("Path find failed... Trying again. Attempt: %d", cnt);
      path = gmap_.build(ogMap, robotOrd, currentGoal_);

      if(cnt > 2){
        ROS_INFO("Cannot reach goal.");
        break;
      }
    }

    //Show the overlay of the prm
    gmap_.showOverlay(colourMap, path);
    sensor_msgs::ImagePtr msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", colourMap).toImageMsg();
    overlay_.publish(msg);
    ROS_INFO("Sent prm overlay");

    if(path.size() > 0){
      //Send the waypoints
      geometry_msgs::PoseArray posePath;
      for(auto const &waypoint: path){
        geometry_msgs::Pose w;
        w.position.x = waypoint.x;
        w.position.y = waypoint.y;
        w.position.z = robotPos.position.z;

        posePath.poses.push_back(w);
      }

      path_.publish(posePath);
      ROS_INFO("Sent path");
    }
  }

}


bool Simulator::requestGoal(prm_sim::RequestGoal::Request &req, prm_sim::RequestGoal::Response &res)
{
  ROS_INFO("request: x=%ld, y=%ld", (long int)req.x, (long int)req.y);

  //TODO: Check if Goal is within map space??
  res.ack = true;
  currentGoal_.x = req.x;
  currentGoal_.y = req.y;

  GoalRecieved = true;
  WaitOnGoal.notify_one();

  //TODO: Smart check. Is current goal equal to old goal?

  ROS_INFO("sending back response: [%d]", res.ack);
  return true;
}
