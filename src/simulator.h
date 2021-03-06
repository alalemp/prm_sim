/*! @file
 *
 *  @brief Simulation for robot path finding.
 *
 *  Using an internal LD-PRM path planner, this class listens
 *  for goal requests on /request_goal then builds a PRM network
 *  within a supplied configuration space.
 *  The PRM network is sent as an image to /prm, and the path waypoints
 *  between robot and goal are sent as a PoseArray to /path.
 *
 *  @author arosspope
 *  @date 12-10-2017
*/
#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <opencv2/opencv.hpp>
#include <atomic>
#include <image_transport/image_transport.h>

#include "ros/ros.h"
#include "prm_sim/RequestGoal.h"
#include "prmplanner.h"
#include "types.h"

template <typename T>
struct TDataContainer { /*!< A container to control access to data that is accessed from multiple threads */
  std::mutex access;  /*!< An access mutex to lock this container when the data is changing */
  T data;             /*!< The data ordiante */
  std::atomic<bool> dirty{false}; /*!< Atomic boolean to indicate if the data has been modified */
};

class Simulator
{
public:
  /*! @brief Constructor for Simulator.
   *
   *  @param nh The handle of the ros node using this class
   *  @param buffer A reference to a shared world data buffer. This buffer should
   *                be populated by another thread.
   */
  Simulator(ros::NodeHandle nh, TWorldDataBuffer &buffer);

  /*! @brief Creates a path between robot and goal using PRM planner.
   *
   *  This thread waits on a goal then attempts to plan a path between
   *  the robot's last known position and the goal. Will send waypoints
   *  of the path between start and goal to topic /path.
   *
   *  @note Whilst the planner is building the network, multiple goal requests
   *        are ignored.
   */
  void plannerThread(void);

  /*! @brief Sends an overlay of the prm network and path to topic /prm.
   *
   */
  void overlayThread();

private:
  ros::NodeHandle nh_;                      /*!< The handle of the ros node using this class */
  image_transport::ImageTransport it_;      /*!< Transport mechanism for images */
  ros::ServiceServer reqGoal_;              /*!< Advertises a service '/request_goal' to set the goal */
  image_transport::Publisher overlayPub_;   /*!< Publishes an overlay of the prm on top of the OgMap to /prm */
  ros::Publisher pathPub_;                  /*!< Publishes the path between robot and goal on /path */

  TWorldDataBuffer &buffer_;                /*!< A shared global structure that gets updated with world information */
  PrmPlanner planner_;                      /*!< The LD-PRM planner for path finding */

  double robotDiameter_;                    /*!< Diameter of the robot in meters */
  cv::Mat cspace_;                          /*!< The current configuration space (greyscale) */
  geometry_msgs::Pose robotPos_;            /*!< The current robot position */

  TDataContainer<TGlobalOrd> goalContainer_;  /*!< The current goal for the robot to reach (shared between threads/callbacks) */
  TDataContainer<cv::Mat> overlayContainer_;  /*!< An image of the last known prm/path overlayed onto the cspace (shared between threads) */

  /*! @brief Callback function for service /request_goal.
   *
   *  This callback will notify the plannerThread that a new goal has been
   *  recieved. Even if the goal is invalid, this request service will return
   *  true. The error will occur (and be displayed) in the planning thread.
   *
   *  @param req The request containing a goal of type (float, float).
   *  @param res The response sent back. TRUE if goal is accessible.
   *  @return TRUE - Always true as there is no failure case for recieving a goal.
   */
  bool requestGoal(prm_sim::RequestGoal::Request &req, prm_sim::RequestGoal::Response &res);

  /*! @brief Consumes data from the shared WorldInfoBuffer.
   *
   *  @param ogMap A reference to a variable to hold the new ogMap.
   *  @param robotPos A reference to a variable to hold the new robot position.
   */
  void consumeWorldData(cv::Mat &ogMap, geometry_msgs::Pose &robotPos);

  /*! @brief Send an overlay of the prm and path to the /prm topic.
   *
   *  @param overlay The overlay to send.
   */
  void sendOverlay(cv::Mat &overlay);

  /*! @brief Sends a series of waypoint poses to the /path topic.
   *
   *  @param path The path to send.
   */
  void sendPath(std::vector<TGlobalOrd> path);

  /*! @brief A blocking funtion that waits until there is data in the shared worldInfo buffer.
   *
   *  @note This waits for data in both the ogMapDeq and poseDeq.
   */
  void waitForWorldData();
};

#endif // SIMULATOR_H
