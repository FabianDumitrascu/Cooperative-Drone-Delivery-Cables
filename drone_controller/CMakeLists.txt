cmake_minimum_required(VERSION 3.0.2)
project(drone_controller)

add_compile_options(-std=c++11)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  nav_msgs
  roscpp
  std_msgs
)

catkin_package(
  CATKIN_DEPENDS geometry_msgs nav_msgs roscpp std_msgs
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)


# Apollo1 Node
add_executable(apollo1_node src/apollo1.cpp)
target_link_libraries(apollo1_node ${catkin_LIBRARIES})

# Takeoff Node
add_executable(takeoff_node src/takeoff_2_drones.cpp)
target_link_libraries(takeoff_node ${catkin_LIBRARIES})

# Land Node
add_executable(land_node src/land_2_drones.cpp)
target_link_libraries(land_node ${catkin_LIBRARIES})

# Command 2 drones Node
add_executable(command_node src/command_2_drones.cpp)
target_link_libraries(command_node ${catkin_LIBRARIES})

# Command 2 drones position controller Node
add_executable(Position_node src/PositionController_2_drones.cpp)
target_link_libraries(Position_node ${catkin_LIBRARIES})

# Command 2 drones diff position Node
add_executable(command_node_2_pos src/VelocityController_2_drones.cpp)
target_link_libraries(command_node_2_pos ${catkin_LIBRARIES})