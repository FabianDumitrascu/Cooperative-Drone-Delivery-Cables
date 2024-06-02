#include "ros/ros.h"
#include "agiros_msgs/Reference.h"
#include "agiros_msgs/Setpoint.h"
#include "agiros_msgs/QuadState.h"
#include "agiros_msgs/Command.h"
#include "nav_msgs/Odometry.h"
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Quaternion.h"
#include "geometry_msgs/Twist.h"
#include "std_msgs/Header.h"
#include <tf/tf.h>
#include <cmath>

class TrajectoryCreator { // Class to create a trajectory
public:
    TrajectoryCreator() : nh("~"), loopRate(1), initialPose1Set(false), initialPose2Set(false), initialPoseBarSet(false){
        // Constructor

        // Read parameters
        ReadParameters();

        // Initialize the Publisher
        trajectoryPub1 = nh.advertise<agiros_msgs::Reference>("/" + droneID1 + "/agiros_pilot/trajectory", 10);
        trajectoryPub2 = nh.advertise<agiros_msgs::Reference>("/" + droneID2 + "/agiros_pilot/trajectory", 10);
        

        std::string subscribeTopic1 = "/" + droneID1 + "/agiros_pilot/odometry";
        std::string subscribeTopic2 = "/" + droneID2 + "/agiros_pilot/odometry";
        std::string subscribeTopicBar = "/" + barID + "/odometry_sensor1/odometry";

        // Initialize the Subscriber
        odometrySub1 = nh.subscribe(subscribeTopic1, 10, &TrajectoryCreator::OdometryCallback1, this);
        odometrySub2 = nh.subscribe(subscribeTopic2, 10, &TrajectoryCreator::OdometryCallback2, this);
        odometrySubBar = nh.subscribe(subscribeTopicBar, 10, &TrajectoryCreator::OdometryCallbackBar, this);
        
        ROS_INFO("Subscribed to odometry topics: %s, %s", subscribeTopic1.c_str(), subscribeTopic2.c_str());

        // Wait for initial pose to be set
        while (ros::ok() && !initialPose1Set && !initialPose2Set && !initialPoseBarSet) {
            ros::spinOnce();
            loopRate.sleep();
            ROS_INFO("Waiting for initial pose to be set, falcon1: %d, falcon2: %d", initialPose1Set, initialPose2Set);
        }

        double currentYaw = GetYawFromQuaternion(currentPose1.orientation);

        ROS_INFO("Current yaw: %f", currentYaw);
        geometry_msgs::Pose pose;
        pose.position.x = targetX;
        pose.position.y = targetY;
        pose.position.z = targetZ;
        pose.orientation = GetQuaternionFromYaw(targetYaw);
        pose = GeneratePoseFromYawAndPosition(pose, targetYaw);
        SendToPose(pose);

        // CalculateBarEndpointPositions(currentPoseBar, GetYawFromQuaternion(currentPoseBar.orientation));


        // Send the pose to the drone
        // SendToPose(pose);

        // Send a triangle trajectory
        // SendTriangleTrajectory();
    }

private:
    double targetX, targetY, targetZ;
    double targetTime;
    double targetYaw;
    double radiusBar, cableLength, deltaZ, zBias;
    bool initialPose1Set, initialPose2Set, initialPoseBarSet;
    std::string droneID1, droneID2, barID;
    ros::NodeHandle nh;
    ros::Publisher trajectoryPub1, trajectoryPub2;
    ros::Subscriber odometrySub1, odometrySub2, odometrySubBar;
    ros::Rate loopRate; // Publish rate (1 Hz)
    geometry_msgs::Pose currentPose1, currentPose2, currentPoseBar;
    geometry_msgs::Pose initialPose1, initialPose2, initialPoseBar;

    void ReadParameters() {
        nh.getParam("targetX", targetX);
        nh.getParam("targetY", targetY);
        nh.getParam("targetZ", targetZ);
        nh.getParam("targetTime", targetTime);
        nh.getParam("droneID1", droneID1);
        nh.getParam("droneID2", droneID2);
        nh.getParam("targetYaw", targetYaw);
        nh.getParam("barID", barID);
        nh.getParam("radiusBar", radiusBar);
        nh.getParam("cableLength", cableLength);
        nh.getParam("deltaZ", deltaZ);
        nh.getParam("zBias", zBias);
        targetYaw = DegreesToRadians(targetYaw);


        ROS_INFO("C++ file launched with target position: (%f, %f, %f)", targetX, targetY, targetZ);
    }

    void OdometryCallback1(const nav_msgs::Odometry::ConstPtr& msg) {
        if (!initialPose1Set) {
            initialPose1 = msg->pose.pose;
            initialPose1.position.z += deltaZ;
            initialPose1Set = true;
        } else {
            currentPose1 = msg->pose.pose;
        }
    }

    void OdometryCallback2(const nav_msgs::Odometry::ConstPtr& msg) {
        if (!initialPose2Set) {
            initialPose2 = msg->pose.pose;
            initialPose2.position.z += deltaZ;
            initialPose2Set = true;
        } else {
            currentPose2 = msg->pose.pose;
        }
    }

    void OdometryCallbackBar(const nav_msgs::Odometry::ConstPtr& msg) {
        if (!initialPoseBarSet) {
            initialPoseBar = msg->pose.pose;
            initialPoseBarSet = true;
            ROS_INFO("Initial pose of the bar: (%f, %f, %f)", initialPoseBar.position.x, initialPoseBar.position.y, initialPoseBar.position.z);
        } else {
            currentPoseBar = msg->pose.pose;
        }
    }


void SendToPose(geometry_msgs::Pose pose) {
    bool sentCommand = false; // Flag to track if the command has been sent

    // Adjust for CableLength and zBias
    pose.position.z += cableLength + zBias;

    while (ros::ok() && !sentCommand) {
        // Create a Reference message
        agiros_msgs::Reference referenceMsg1, referenceMsg2;
        
        // Fill in the header
        std_msgs::Header header;
        header.stamp = ros::Time::now();
        header.frame_id = "world";
        referenceMsg1.header = header;
        referenceMsg2.header = header;

        // Define waypoints
        std::vector<agiros_msgs::Setpoint> waypoints1, waypoints2;

        // First waypoint left drone
        agiros_msgs::Setpoint wp1;
        wp1.state.header = header;
        wp1.state.t = 0.0;
        wp1.state.pose = initialPose1;
        waypoints1.push_back(wp1);

        // First waypoint right drone
        agiros_msgs::Setpoint wp3;
        wp3.state.header = header;
        wp3.state.t = 0.0;
        wp3.state.pose = initialPose2;
        waypoints2.push_back(wp3);

        // Calculate the target positions for the left and right drones based on the midpoint of the bar
        double targetYaw = GetYawFromQuaternion(pose.orientation);
        double targetXLeft = radiusBar * cos(targetYaw + M_PI / 2);
        double targetYLeft = radiusBar * sin(targetYaw + M_PI / 2);
        double targetXRight = radiusBar * cos(targetYaw - M_PI / 2);
        double targetYRight = radiusBar * sin(targetYaw - M_PI / 2);

        geometry_msgs::Pose poseLeft, poseRight;
        poseLeft.orientation = pose.orientation;
        poseRight.orientation = pose.orientation;

        // Adjust positions relative to the midpoint (target pose)
        poseLeft.position.x = pose.position.x + targetXLeft;
        poseLeft.position.y = pose.position.y + targetYLeft;
        poseLeft.position.z = pose.position.z;
        
        poseRight.position.x = pose.position.x + targetXRight;
        poseRight.position.y = pose.position.y + targetYRight;
        poseRight.position.z = pose.position.z;

        // Second waypoint left drone
        agiros_msgs::Setpoint wp2;
        wp2.state.header = header;
        wp2.state.t = targetTime;
        wp2.state.pose = poseLeft;
        waypoints1.push_back(wp2);

        // Second waypoint right drone
        agiros_msgs::Setpoint wp4;
        wp4.state.header = header;
        wp4.state.t = targetTime;
        wp4.state.pose = poseRight;
        waypoints2.push_back(wp4);

        // Add waypoints to the Reference messages
        referenceMsg1.points = waypoints1;
        referenceMsg2.points = waypoints2;

        ROS_INFO("Sending left drone to position: (%f, %f, %f)", poseLeft.position.x, poseLeft.position.y, poseLeft.position.z);
        ROS_INFO("Sending right drone to position: (%f, %f, %f)", poseRight.position.x, poseRight.position.y, poseRight.position.z);

        // Publish the trajectories
        trajectoryPub1.publish(referenceMsg1);
        trajectoryPub2.publish(referenceMsg2);
        ros::spinOnce();
        loopRate.sleep();
    }
}


    geometry_msgs::Pose GeneratePoseFromYawAndPosition(geometry_msgs::Pose position, double yaw) {
        geometry_msgs::Pose pose;
        pose.position = position.position;
        pose.orientation = GetQuaternionFromYaw(yaw);
        return pose;
    }

    void GeneratePointFromMidpointBar(geometry_msgs::Pose poseMidpoint, double yawMidpoint) {

    }

    void GenerateTrajectory(geometry_msgs::Pose pose) {
        while (ros::ok()) {
            // Create a Reference message
            agiros_msgs::Reference referenceMsg1, referenceMsg2;


            // Fill in the header
            std_msgs::Header header;
            header.stamp = ros::Time::now();
            header.frame_id = "world";
            referenceMsg1.header = header;
            referenceMsg2.header = header;

            // Define waypoints
            std::vector<agiros_msgs::Setpoint> waypoints1, waypoints2;

            // Get the bar endpoints

            // First waypoint left drone
            agiros_msgs::Setpoint wp1;
            wp1.state.header = header;
            wp1.state.t = 0.0;
            wp1.state.pose = initialPose1;
            waypoints1.push_back(wp1);

            // Second waypoint
            agiros_msgs::Setpoint wp2;
            wp2.state.header = header;
            wp2.state.t = targetTime;
            wp2.state.pose = pose;
            waypoints1.push_back(wp2);

            // Add waypoints to the Reference message
            referenceMsg1.points = waypoints1;

            // Publish the trajectory
            trajectoryPub1.publish(referenceMsg1);
            ros::spinOnce();
            loopRate.sleep();
        }
    }

    std::vector<geometry_msgs::Pose> CalculateBarEndpointPositions(geometry_msgs::Pose midpointBar, double yawMidpointBar) {  
        std::vector<geometry_msgs::Pose> barEndpoints;

        // Calculate the target position for the drone to the left of the bar
        double targetYawLeft = yawMidpointBar + M_PI / 2; // 90 degrees to the left
        double targetXLeft = radiusBar * cos(targetYawLeft);
        double targetYLeft = radiusBar * sin(targetYawLeft);

        double targetYawRight = yawMidpointBar - M_PI / 2; // 90 degrees to the right
        double targetXRight = radiusBar * cos(targetYawRight);
        double targetYRight = radiusBar * sin(targetYawRight);

        ROS_INFO("Target position to the left of the bar: (%f, %f)", targetXLeft, targetYLeft);
        ROS_INFO("Target position to the right of the bar: (%f, %f)", targetXRight, targetYRight);

        geometry_msgs::Pose poseLeft, poseRight;

        poseLeft.orientation = GetQuaternionFromYaw(yawMidpointBar);
        poseLeft.position.x = targetXLeft + midpointBar.position.x;
        poseLeft.position.y = targetYLeft + midpointBar.position.y;
        poseLeft.position.z = midpointBar.position.z + cableLength;

        poseRight.orientation = GetQuaternionFromYaw(yawMidpointBar);
        poseRight.position.x = targetXRight + midpointBar.position.x;
        poseRight.position.y = targetYRight + midpointBar.position.y;
        poseRight.position.z = midpointBar.position.z + cableLength;

        barEndpoints.push_back(poseLeft);
        barEndpoints.push_back(poseRight);

        return barEndpoints;


    }

    void SendTriangleTrajectory() {
        while (ros::ok()) {
            // Create a Reference message
            agiros_msgs::Reference referenceMsg;

            // Fill in the header
            std_msgs::Header header;
            header.stamp = ros::Time::now();
            header.frame_id = "world";
            referenceMsg.header = header;

            // Define waypoints
            std::vector<agiros_msgs::Setpoint> waypoints;

            // Waypoint 1
            agiros_msgs::Setpoint wp1;
            wp1.state.header = header;
            wp1.state.t = 0.0;
            wp1.state.pose.position.x = 0.0;
            wp1.state.pose.position.y = 0.0;
            wp1.state.pose.position.z = 1.0;
            wp1.state.pose.orientation.w = 1.0; // Neutral orientation
            waypoints.push_back(wp1);

            // Waypoint 2
            agiros_msgs::Setpoint wp2;
            wp2.state.header = header;
            wp2.state.t = 5.0; // Time to reach this waypoint
            wp2.state.pose.position.x = 1.0;
            wp2.state.pose.position.y = 1.0;
            wp2.state.pose.position.z = 1.0;
            wp2.state.pose.orientation.w = 1.0;
            waypoints.push_back(wp2);

            // Waypoint 3
            agiros_msgs::Setpoint wp3;
            wp3.state.header = header;
            wp3.state.t = targetTime; // Time to reach this waypoint
            wp3.state.pose.position.x = 2.0;
            wp3.state.pose.position.y = 0.0;
            wp3.state.pose.position.z = 1.0;
            wp3.state.pose.orientation.w = 1.0;
            waypoints.push_back(wp3);

            // Add waypoints to the Reference message
            referenceMsg.points = waypoints;

            // Publish the trajectory
            trajectoryPub1.publish(referenceMsg);

            ros::spinOnce();
            loopRate.sleep();
        }
    }

    void CalculateMidpoint() {
        geometry_msgs::Pose midpoint;
        midpoint.position.x = (initialPose1.position.x + initialPose2.position.x) / 2;
        midpoint.position.y = (initialPose1.position.y + initialPose2.position.y) / 2;
        midpoint.position.z = (initialPose1.position.z + initialPose2.position.z) / 2;
        midpoint.orientation.w = 1.0;

        GenerateTrajectory(midpoint);
    
    }

    // Function to extract yaw from a geometry_msgs::Quaternion
    double GetYawFromQuaternion(const geometry_msgs::Quaternion& quat) {
        tf::Quaternion q(quat.x, quat.y, quat.z, quat.w);
        double roll, pitch, yaw;
        tf::Matrix3x3(q).getRPY(roll, pitch, yaw);
        return yaw;
    }

    // Function to create a geometry_msgs::Quaternion from a yaw angle
    geometry_msgs::Quaternion GetQuaternionFromYaw(double yaw) {
        tf::Quaternion q;
        q.setRPY(0, 0, yaw); // Roll and pitch are set to zero
        geometry_msgs::Quaternion quat_msg;
        tf::quaternionTFToMsg(q, quat_msg);
        return quat_msg;
    }

    double DegreesToRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "trajectoryCreator");

    TrajectoryCreator trajectoryCreator;
    ros::spin();

    return 0;
}
