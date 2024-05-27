#include "ros/ros.h"
#include "geometry_msgs/PoseStamped.h"
#include "nav_msgs/Odometry.h"
#include <cmath>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

// Global variables
double end_angle_degrees, radius;
double target_x, target_y, target_z;

class CirclePathController {
public:
    CirclePathController(ros::NodeHandle& nh, double radius, double end_angle_degrees, int steps = -1) 
        : nh(nh), centerX(0.0), centerY(0.0), centerZ(1.0), startX(0.0), startY(0.0),
          degree_increment(30.0), radius(radius), end_angle_degrees(end_angle_degrees),
          init_position_flycrane_set(false), init_position_flycrane1_set(false),
          angle_degrees(0.0), start_angle_degrees(0.0), steps(steps) {

        // Assign values from global variables to member variables
        target_x = ::target_x;
        target_y = ::target_y;
        target_z = ::target_z;

        // Calculate the number of steps, if none inputted. 
        if (steps == -1) {
            steps = calculateSteps();
        }

        odom_sub_flycrane = nh.subscribe("/flycrane/agiros_pilot/odometry", 1, &CirclePathController::OdometryCallbackFlycrane, this);
        odom_sub_flycrane1 = nh.subscribe("/flycrane1/agiros_pilot/odometry", 1, &CirclePathController::OdometryCallbackFlycrane1, this);

        pose_pub_flycrane = nh.advertise<geometry_msgs::PoseStamped>("/flycrane/agiros_pilot/go_to_pose", 2);
        pose_pub_flycrane1 = nh.advertise<geometry_msgs::PoseStamped>("/flycrane1/agiros_pilot/go_to_pose", 2);

        // Start the process after ensuring initial positions are set
        std::thread(&CirclePathController::waitForInitialPositions, this).detach();
    }

private:
    ros::NodeHandle& nh;
    ros::Publisher pose_pub_flycrane, pose_pub_flycrane1;
    ros::Subscriber odom_sub_flycrane, odom_sub_flycrane1;
    ros::Timer timer, shutdown_timer;
    bool init_position_flycrane_set, init_position_flycrane1_set;
    int steps;
    double centerX, centerY, centerZ;
    double startX, startY;
    double angle_degrees, start_angle_degrees;
    
    double degree_increment;
    geometry_msgs::Point initial_position_flycrane, initial_position_flycrane1;
    geometry_msgs::PoseStamped pose_flycrane, pose_flycrane1;
    geometry_msgs::PoseStamped initialCenter;
    std::mutex mutex_;
    std::condition_variable cv_;

    double radius, end_angle_degrees, target_x, target_y, target_z;

    void OdometryCallbackFlycrane(const nav_msgs::Odometry::ConstPtr& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!init_position_flycrane_set) {
            init_position_flycrane_set = true;
            initial_position_flycrane = msg->pose.pose.position;
            ROS_INFO("The initial position of flycrane is set at x=%f, y=%f, z=%f", initial_position_flycrane.x, initial_position_flycrane.y, initial_position_flycrane.z);
            cv_.notify_all();
        }
    }

    void OdometryCallbackFlycrane1(const nav_msgs::Odometry::ConstPtr& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!init_position_flycrane1_set) {
            init_position_flycrane1_set = true;
            initial_position_flycrane1 = msg->pose.pose.position;
            ROS_INFO("The initial position of flycrane1 is set at x=%f, y=%f, z=%f", initial_position_flycrane1.x, initial_position_flycrane1.y, initial_position_flycrane1.z);
            cv_.notify_all();
        }
    }

    double calculateDistance(const geometry_msgs::Point& p1, const geometry_msgs::Point& p2) {
        return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
    }

    geometry_msgs::PoseStamped CalculateStepsVector() {
        geometry_msgs::PoseStamped stepsVector;
        double xValue = (target_x - centerX);
        double yValue = (target_y - centerY);
        double zValue = (target_z - centerZ);

        double length = sqrt(xValue * xValue + yValue * yValue + zValue * zValue);

        stepsVector.pose.position.x = xValue / length;
        stepsVector.pose.position.y = yValue / length;
        stepsVector.pose.position.z = zValue / length;

        return stepsVector;
    }

    std::pair<geometry_msgs::PoseStamped, geometry_msgs::PoseStamped> CalculateNormalVector() {
        geometry_msgs::PoseStamped leftDrone;
        geometry_msgs::PoseStamped rightDrone;
        double target_z = initialCenter.pose.position.z;  // Assuming constant z-plane

        double dx = target_x - initialCenter.pose.position.x;
        double dy = target_y - initialCenter.pose.position.y;
        double length = sqrt(dx * dx + dy * dy);

        if (length != 0) {
            // Normalized normal vectors in the xy-plane
            double normal_x = -dy / length;
            double normal_y = dx / length;

            // Calculate potential positions
            geometry_msgs::Point leftPoint, rightPoint;
            leftPoint.x = initialCenter.pose.position.x + radius * normal_x;
            leftPoint.y = initialCenter.pose.position.y + radius * normal_y;
            leftPoint.z = initialCenter.pose.position.z;

            rightPoint.x = initialCenter.pose.position.x - radius * normal_x;
            rightPoint.y = initialCenter.pose.position.y - radius * normal_y;
            rightPoint.z = initialCenter.pose.position.z;

            // Determine which drone is closer to the left point
            double distanceFlycraneToLeft = calculateDistance(initial_position_flycrane, leftPoint);
            double distanceFlycrane1ToLeft = calculateDistance(initial_position_flycrane1, leftPoint);

            if (distanceFlycraneToLeft < distanceFlycrane1ToLeft) {
                leftDrone.pose.position = leftPoint;
                rightDrone.pose.position = rightPoint;
            } else {
                leftDrone.pose.position = rightPoint;
                rightDrone.pose.position = leftPoint;
            }

            ROS_INFO("Calculated normal vectors: left drone at x=%f, y=%f, z=%f; "
                     "right drone at x=%f, y=%f, z=%f", 
                     leftDrone.pose.position.x, leftDrone.pose.position.y, leftDrone.pose.position.z, 
                     rightDrone.pose.position.x, rightDrone.pose.position.y, rightDrone.pose.position.z);
        } else {
            // Handle the case where length is zero to avoid division by zero
            leftDrone.pose.position = initialCenter.pose.position;
            rightDrone.pose.position = initialCenter.pose.position;

            // Move the left drone to the right by the radius
            leftDrone.pose.position.x += radius;
            rightDrone.pose.position.x -= radius;

            ROS_WARN("The target position is the same as the initial center position. Moving the drones to the left and right by the radius.");
        }
        return std::make_pair(leftDrone, rightDrone);
    }

    void waitForInitialPositions() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{ return init_position_flycrane_set && init_position_flycrane1_set; });

        // Add a small delay
        ros::Duration(0.5).sleep();

        // Send drones to center positions
        SendToCenter();

        // Start the timer for regular updates
        timer = nh.createTimer(ros::Duration(0.8), &CirclePathController::updateCallback, this, false);
    }

    void CalculateCenter() {
        geometry_msgs::PoseStamped pose;
        pose.pose.position.x = (initial_position_flycrane.x + initial_position_flycrane1.x) / 2;
        pose.pose.position.y = (initial_position_flycrane.y + initial_position_flycrane1.y) / 2;
        pose.pose.position.z = (initial_position_flycrane.z + initial_position_flycrane1.z) / 2;
        ROS_INFO("Calculated center position at x=%f, y=%f, z=%f", pose.pose.position.x, pose.pose.position.y, pose.pose.position.z);
        initialCenter = pose;
    }

    void updateCallback(const ros::TimerEvent& event) {
        // TODO: Add your update logic here
    }

    void SendToCenter() {
        CalculateCenter();

        ROS_INFO("Sending drones to center positions: x=%f, y=%f, z=%f", initialCenter.pose.position.x, initialCenter.pose.position.y, initialCenter.pose.position.z);
        std::pair<geometry_msgs::PoseStamped, geometry_msgs::PoseStamped> dronePositions = CalculateNormalVector();
        geometry_msgs::PoseStamped leftDrone = dronePositions.first;
        geometry_msgs::PoseStamped rightDrone = dronePositions.second;
        pose_pub_flycrane.publish(leftDrone);
        pose_pub_flycrane1.publish(rightDrone);
        
    }

    void UpdatePositions() {
        // Update the center position
        double increment_x = (target_x - startX) / static_cast<double>(steps);
        centerX += increment_x;

        pose_flycrane.pose.position.x = centerX; 
        pose_flycrane1.pose.position.x = centerX;
        pose_flycrane.pose.position.y = centerY + radius;
        pose_flycrane1.pose.position.y = centerY - radius;
        pose_flycrane.pose.position.z = centerZ;
        pose_flycrane1.pose.position.z = centerZ;
        pose_flycrane.pose.orientation.w = 1.0;
        pose_flycrane1.pose.orientation.w = 1.0;

        pose_pub_flycrane.publish(pose_flycrane);
        pose_pub_flycrane1.publish(pose_flycrane1);

        ROS_INFO("Updated positions: x=%f, y=%f, z=%f", centerX, centerY, centerZ);
    }

    double degreesToRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    int calculateSteps() {        
        return 4; // Missing semicolon fixed
    }
};

void RetrieveParams(ros::NodeHandle& nh){
    if (!nh.getParam("radius", radius)) {
        ROS_WARN("Could not retrieve 'radius' from the parameter server; defaulting to 1.0");
        radius = 1.0;
    }

    if (!nh.getParam("end_angle_degrees", end_angle_degrees)) {
        ROS_WARN("Could not retrieve 'end_angle_degrees' from the parameter server; defaulting to 90 degrees");
        end_angle_degrees = 90.0;
    }

    if (!nh.getParam("target_x", target_x)) {
        ROS_WARN("Could not retrieve 'target_x' from the parameter server; defaulting to 2.0");
        target_x = 2.0;
    }

    if (!nh.getParam("target_y", target_y)) {
        ROS_WARN("Could not retrieve 'target_y' from the parameter server; defaulting to 2.0");
        target_y = 2.0;
    }

    if (!nh.getParam("target_z", target_z)) {
        ROS_WARN("Could not retrieve 'target_z' from the parameter server; defaulting to 1.0");
        target_z = 1.0;
    }
};

int main(int argc, char **argv) {
    ros::init(argc, argv, "circle_path_controller"); // Ensure ros::init is called first
    ros::NodeHandle nh("~"); // Create NodeHandle after ros::init
    std::this_thread::sleep_for(std::chrono::seconds(4)); // Wait 4 seconds before takeoff
    RetrieveParams(nh);
    CirclePathController controller(nh, radius, end_angle_degrees);
    ros::spin();
    return 0;
}
