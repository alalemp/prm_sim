#!/bin/bash

echo "Starting roscore..."
gnome-terminal -e "roscore"

# We sleep for 2 seconds to ensure roscore has properly started
sleep 2

echo "Starting world stage..."
gnome-terminal -e "rosrun stage_ros stageros $(rospack find a3_help)/worlds/uoa_robotics_lab.world"

echo "Starting local_map..."
gnome-terminal -e "rosrun local_map local_map /local_map/scan:=/base_scan_1 _map_width:=200 _map_height:=200 _map_resolution:=0.1"

echo "Starting map to image node..."
gnome-terminal -e "rosrun a3_help map_to_image_node map:=/local_map/local_map /pose:=/odom"

echo "Starting rviz..."
gnome-terminal -e "rosrun rviz rviz -d $(rospack find a3_help)/rviz/pfms.rviz"
