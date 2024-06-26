#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/wrench.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

# define PI           3.14159265358979323846

class WrenchPublisher : public rclcpp::Node
{
public:
    WrenchPublisher()
    : Node("publishforce")
    {    
        publisher_ = this->create_publisher<geometry_msgs::msg::Wrench>("gazebo_ros_force", 10);
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(25),
            std::bind(&WrenchPublisher::publish_wrench, this)
        );
                // Create a subscription to the "joint_states" topic
        subscription_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "joint_states", 10,
            std::bind(&WrenchPublisher::topic_callback, this, std::placeholders::_1)
        );
        cmd_keyboard = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10,
            std::bind(&WrenchPublisher::cmd_telop, this, std::placeholders::_1)
        );
    }
private:
    const int SLIDER_ENC_COUNT=24000;
    const int ANGLE_ENC_COUNT=2400;
    int pendulum_pos;
    int slider_pos;
    int push;
    float position_pid(float curr_err,uint16_t sampling_rate){
        static float sum,old_err;
        float Kp,Ki,Kd;
        Kp=0;    Ki=0;    Kd=0;
        sum+=curr_err;
        if (sum>0.35){
            sum=0.35;
        }
        if (sum<-0.35){
            sum=-0.35;
        }
        float out_pid;
        out_pid=Kp*curr_err+Kd*sampling_rate*(curr_err-old_err)+Ki*sum/sampling_rate;
        old_err=curr_err;
        return out_pid;
    }

    void publish_wrench()
    {
        auto message = geometry_msgs::msg::Wrench();
        
        message.force.x = push;
        // RCLCPP_INFO(this->get_logger(), "Publishing: force(%.2f) slider position: %d, pendulum position: %d",
        //             message.force.x,slider_pos, pendulum_pos);
        publisher_->publish(message);
        if (slider_pos<8000)
        {
            push=105;
            // ecof=-ecof;
        }
        if (slider_pos>12000)
        {
            push=-105;
            // ecof=-ecof;
        }
    }

    void topic_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
    {
        pendulum_pos=msg->position[1]/PI*ANGLE_ENC_COUNT;
        slider_pos=msg->position[0]/0.35*SLIDER_ENC_COUNT;
    }

    void cmd_telop(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "Publishing: force(%.2f) slider position: %f, pendulum position: %f",
                    msg->linear.x,msg->linear.y, msg->linear.z);
    }

        rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr subscription_;
        rclcpp::Publisher<geometry_msgs::msg::Wrench>::SharedPtr publisher_;
        rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_keyboard;
        rclcpp::TimerBase::SharedPtr timer_;
    };




int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<WrenchPublisher>());
    // rclcpp::spin(std::make_shared<JointStateListener>());
    rclcpp::shutdown();
    return 0;
}