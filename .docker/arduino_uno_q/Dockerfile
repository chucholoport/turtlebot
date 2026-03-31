FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

# Core dev tools
RUN apt-get update && apt-get install -y \
    tmux \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-msgpack \
    git \
    ros-jazzy-rplidar-ros \
    && rm -rf /var/lib/apt/lists/*

# Initialize rosdep
RUN rosdep update

# Workspace
WORKDIR /turtle_ws

# Ensure ROS environment always available
SHELL ["/bin/bash", "-c"]
RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc

CMD ["bash"]