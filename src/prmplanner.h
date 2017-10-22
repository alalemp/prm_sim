/*! @file
 *
 *  @brief A low dispersion PRM planner.
 *
 *  The idea of the LD-PRM is to capture the connectivity of the
 *  configuration space with fewer samples, which reduces the
 *  running time of the algorithm. Different to a simple PRM planner,
 *  the samples generated by the LD-PRM must fufil an important criterion
 *  in order to be included inside the roadmap. Samples are forbidden to
 *  be close to each other more than a predefined radius. This criterion
 *  creates an almost uniform distribution of samples which helps in
 *  narrow passageways.
 *  For more information, please see http://cdn.intechopen.com/pdfs/45913.pdf
 *
 *  @author arosspope
 *  @date 12-10-2017
*/
#ifndef PRMPLANNER_H
#define PRMPLANNER_H

#include <map>
#include <utility>

#include "localmap.h"
#include "graph.h"
#include "types.h"

//PrmPlanner default constants
const double PLANNER_DEF_MAP_SIZE = 20.0;   /*!< The default ogmap size is 20x20m */
const double PLANNER_DEF_MAP_RES = 0.1;     /*!< The default ogmap resolution is 0.1m per pixel */
const unsigned int PLANNER_DEF_DENSITY = 5; /*!< The default max amount of neighbours a node in the network can have */

class PrmPlanner
{
public:
  /*! @brief Default constructor for PrmPlanner.
   */
  PrmPlanner();

  /*! @brief Constructor for a PrmPlanner.
   *
   *  @param mapSize The size of the OgMap in meters (square maps only).
   *  @param res The resolution of the OgMaps provided to this object.
   *  @param density The density of the prm network (max neighbours a node can have).
   *
   *  @note This will set the reference position to 0,0 by default. To change this,
   *        call setReference().
   */
  PrmPlanner(double mapSize, double mapRes, unsigned int density);

  /*! @brief Builds a prm network between a start and end ordinate.
   *
   *  @param cspace The OgMap to build the prm network within. Must be already expanded.
   *  @param start The starting ordinate. This is usually the robot's position.
   *  @param goal  The goal ordiante to reach from start.
   *  @return vector<TGlobalOrd> - An ordered vector of globalOrd's between start
   *                              and goal. This will be empty if no path was
   *                              discovered.
   */
  std::vector<TGlobalOrd> build(cv::Mat &cspace, TGlobalOrd start, TGlobalOrd goal);

  /*! @brief Query the network for a path between start and goal within cspace.
   *
   *  @param cspace The map configuration space. Must be already expanded.
   *  @param start The starting ordinate. This is usually the robot's position.
   *  @param goal  The goal ordiante to reach from start.
   *  @return vector<TGlobalOrd> - An ordered vector of globalOrd's between start
   *                              and goal. This will be empty if no path was
   *                              discovered.
   */
  std::vector<TGlobalOrd> query(cv::Mat &cspace, TGlobalOrd start, TGlobalOrd goal);

  /*! @brief Expands the configuration space of a map.
   *
   *  So that we are able to treat the robot as a point in space,
   *  we expand the boundaries of non-free space by the diameter of
   *  the robot.
   *
   *  @param space The space (map) to expand.
   *  @param robotDiameter The diameter of the robot in meters.
   */
  void expandConfigSpace(cv::Mat &space, double robotDiameter);

  /*! @brief Overlays the current state of the PRM unto a colour OgMap.
   *
   *  Not only will this overlay the prm (in blue), but if supplied with
   *  a valid non-empty path, it will also overaly this trajectory (red).
   *
   *  @param space The OgMap to overlay the PRM and path on top of.
   *  @param path The path to also overlay. Empty if no path to display.
   */
  void showOverlay(cv::Mat &space, std::vector<TGlobalOrd> path);

  /*! @brief Sets the reference position of the provided OgMaps.
   *
   *  @param reference The reference to set, this is usually the robot's
   *                   global position.
   */
  void setReference(const TGlobalOrd reference);

  /*! @brief Updates the size of the OgMaps provided.
   *
   *  @param mapSize The size of the OgMap in meters (square maps only).
   */
  void setMapSize(double mapSize);

  /*! @brief Updates the resolution of the OgMaps provided.
   *
   *  @param resolution The size of the OgMap in meters.
   */
  void setResolution(double resolution);

  /*! @brief Indicates if the given ordinate is acessible in cspace.
   *
   *  @param cspace The space to check for the ordinate.
   *  @param ordinate The ordiante to check.
   *  @return TRUE - If the ordinate is within known, free space.
   */
  bool ordinateAccessible(cv::Mat &cspace, TGlobalOrd ordinate);

private:
  Graph graph_;                             /*!< A graph representation of the roadmap network */
  LocalMap lmap_;                           /*!< An object for interacting with the ogMap provided to this object */
  std::map<vertex, TGlobalOrd> network_;    /*!< A look up table to convert a vertex to coordinate within map */
  vertex nextVertexId_;                     /*!< Used for generating unique vertex ids for coordiantes */
  TGlobalOrd reference_;                    /*!< Reference ordinate for the local map, this is usually the robot position */
  unsigned int density_;                    /*!< The density of the prm network (max neighbours a node can have). */

  /*! @brief Optimises a path between two points in a config space.
   *
   *  In some cases, the shortest path in a PRM network may not be the
   *  most direct route between a start and goal. This function aims
   *  to remove the points in the path that can be directly accessed
   *  by earlier points. For example, if there is a direct path between
   *  start and goal, this function will find it.
   *
   *  @param cspace The configuration space to find direct access within.
   *  @param path An ordered representation of the path, where the first element
   *              is the start, and the end element is the goal.
   */
  std::vector<TGlobalOrd> optimisePath(cv::Mat &cspace, std::vector<TGlobalOrd> path);

  /*! @brief Embeds a node in the prm network.
   *
   *  Given a node, determine the closest k neighbours and
   *  attempt to connect to them.
   *
   *  @param cspace The configuration space to embed the node in.
   *  @param node The node to connect and embed.
   *  @param k The amount of neighbours to attempt to connect to.
   *  @param retry If this is TRUE, then the algorithm will ensure
   *               that k neighbours have been connected to. However,
   *               this assurance will depend on the density of the network
   *               and the amount of actual nodes in the network.
   */
  void embedNode(cv::Mat &cspace, vertex node, unsigned int k, bool retry);

  /*! @brief Joins node configurations to eachother within the network.
   *
   *
   *  @param cspace The configuration space to embed the nodes in.
   *  @param k The amount of neighbours to attempt to connect too (for each node).
   */
  void joinNetwork(cv::Mat &cspace, unsigned int k);


  /*! @brief Returns a representation of the internal PRM.
   *
   *  @return vector<<Point, Point>> - A vector of pairs of points. This represents
   *                                   their connection to eachother. Verticies that have
   *                                   no neighbours are simply paired with themselves.
   */
  std::vector<std::pair<cv::Point, cv::Point>> composePRM();

  /*! @brief Converts a path of globalOrds to OgMap points.
   *
   *  @param path The path of ordiantes to convert.
   *  @return vector<points> - The converted path of OgMap points.
   */
  std::vector<cv::Point> toPointPath(std::vector<TGlobalOrd> path);

  /*! @brief Converts a path of verticies to Global ords.
   *
   *  @param path The path of verticies to convert.
   *  @return vector<TGlobalOrd> - The converted path of Verticies.
   */
  std::vector<TGlobalOrd> toOrdPath(std::vector<vertex> path);

  /*! @brief Returns a list of neighbours for the node.
   *
   *  This function will return a list of neighbours around the
   *  node in the PRM. This list is ordered by its distance to
   *  the node (first element is closest).
   *
   *  @param cspace The configuration space.
   *  @param node The node to get neighbours for.
   *  @param shouldConnect An extra qualifier for a neighbour. TRUE if you
   *                       want to actually connect to it.
   *  @return vector<TGlobalOrd> - A list of neighbours
   */
  std::vector<TGlobalOrd> getNeighbours(cv::Mat &cspace, vertex node, bool shouldConnect);

  /*! @brief Get the vertex corresponding to a global ordiante.
   *
   *  If no vertex exists for this ordinate, then add it to the graph
   *  and return the newly generated vertex.
   *
   *  @param ordinate The ordiante to find or add.
   *  @return vertex - The vertex representation of the ordinate.
   */
  vertex findOrAdd(TGlobalOrd ordinate);

  /*! @brief Determines if an ordiante exists within the graph as a vertex.
   *
   *  @param ordinate The ordiante to find or add.
   *  @return TRUE - If the ordiante exists.
   */
  bool existsAsVertex(TGlobalOrd ord);

  /*! @brief Finds the vertex corresponding to an ordiante.
   *
   *  @param ordinate The ordiante to find.
   *  @param v A reference to put the found vertex into.
   *  @return TRUE - If the ordiante was found within the network_.
   */
  bool lookup(TGlobalOrd ord, vertex &v);

  /*! @brief Returns the next unique vertex id within the graph.
   *
   *  @return vertex The next unique vertex id.
   */
  vertex nextVertexId();

  /*! @brief Determines if an ordinate lies within the radius of any other nodes.
   *
   *  @param ordinate The ordinate to check for.
   *  @param r The radius qualifier.
   *  @return TRUE - If the ordindate is violating the space of any other nodes.
   */
  bool violatingSpace(TGlobalOrd ord, double r);

  /*! @brief Calculates the euclidean distance between two ordiantes.
   *
   *  @param p1 The first ordinate.
   *  @param p2 The second ordinate.
   *  @return double - The distance between the two ordiantes.
   */
  static double distance(TGlobalOrd o1, TGlobalOrd o2);

  /*! @brief Adds ordiante to the internal graph/network.
   *
   *  @return ordinate The ordiante to add.
   *  @return vertex - The ordinate's unique vertex id.
   */
  vertex addOrdinate(TGlobalOrd ordinate);

  /*! @brief Prioritises all nodes in network based on their edge count.
   *
   *  Nodes (or verticies) with the least amount of edge connections appear
   *  early in the list.
   *
   *  TODO: This is currently not used.
   *
   *  @return vector<vertex> - Prioritised list of nodes.
   */
  std::vector<vertex> prioritiseNodes();
};

#endif // PRMPLANNER_H
