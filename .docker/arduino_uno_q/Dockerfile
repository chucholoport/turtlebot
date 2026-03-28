FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    tmux \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-msgpack \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN rosdep update

WORKDIR /turtle_ws

CMD ["bash"]