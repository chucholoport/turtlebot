# Arduino Uno Q TurtleBot Project

This repository contains the required infrastructure base for a turtlebot.


## Platform

### **Hardware**
Development Board:
- [*Arduino Uno Q*](https://www.arduino.cc/product-uno-q)

### **Software**

Operating Systems for MCU & SoC: 
- [*Zephyr RTOS*](https://www.zephyrproject.org/)
- [*Debian Trixie 13.1*](https://www.debian.org/releases/trixie/index.es.html)

Development & Debugging Tools:
- [*Docker*](https://www.docker.com/)
- [*Arduino IDE*](https://www.arduino.cc/en/software/)
- [*ROS 2 Jazzy Jalisco*](https://docs.ros.org/en/jazzy/index.html)
- [*Android Debug Bridge*](https://developer.android.com/tools/adb)


## Project Structure

```sh
turtlebot/
│
├── .docker/                   # Docker environment and helper scripts
│   ├── arduino_uno_q/         # Docker setup for Arduino UNO Q
│   │   ├── docker-compose.yml
│   │   └── Dockerfile
│   │
│   └── scripts/               # Utility scripts
│       ├── .config
│       ├── dev
│       ├── rm
│       └── robot
│
├── turtle_ws/                 # ROS 2 workspace (colcon)
│   ├── src/
│   │   ├── uno_q_bridge/      # ROS 2 package (ament_python)
│   │   └── turtlebot_core/    # ROS 2 package (ament_python)
│   │
│   ├── build/                 # Auto-generated (colcon)
│   ├── install/               # Auto-generated (colcon)
│   └── log/                   # Auto-generated (colcon)
│
├── test/                      # Standalone tests (outside ROS)
│   ├── msgpack_test.py        # Direct socket test
│   └── test_led.ino           # MCU test sketch
│
├── LICENSE
├── README.md
└── .gitignore
```

> **NOTE:** The `turtle_ws/` directory follows a standard ROS2 colcon workspace layout.


## Installation

To enable the `robot` command globally, you need to add it to your shell configuration file (`~/.bashrc`).

Append the following function to your `~/.bashrc`:

```sh
# Automatically enable the robot command when entering a robotics project
robot() {
    if [ -f ".docker/scripts/robot" ]; then
        bash .docker/scripts/robot "$@"
    else
        echo "robot: not inside a robotics project"
    fi
}
```

Then reload your shell configuration:

```sh
source ~/.bashrc
```

After this, you can invoke `robot` from any directory.
If executed inside a valid project (containing `.docker/scripts/robot`), it will run the corresponding script.


## Development Script (`dev`)

The `dev` script is designed to provide a seamless way to enter a ROS2 development environment without manually managing Docker.

### What it does

1. **Resolves project paths**
   The script determines its own location and computes the project root directory. From there, it locates the Docker Compose setup inside:

   ```
   .docker/arduino_uno_q
   ```

2. **Checks if the Docker image exists**
   It looks for a Docker image named `turtlebot`.

   * If the image does not exist, it triggers:

     ```
     docker compose build
     ```

   This ensures the environment is built only once.

3. **Ensures the container is running**
   It verifies whether a container named `turtlebot` is currently active.

   * If not running, it starts the container in detached mode:

     ```sh
     docker compose up -d
     ```

4. **Opens an interactive ROS2 shell**
   Once the container is running, the script attaches to it using `docker exec` and initializes the ROS2 environment:

   * Loads the base ROS2 installation:

     ```sh
     /opt/ros/jazzy/setup.bash
     ```
   * Optionally loads the workspace overlay (if already built):

     ```sh
     /turtle_ws/install/setup.bash
     ```
   * Spawns an interactive Bash session

### Result

After execution, you are dropped into a fully configured ROS2 environment inside the container, with:

* ROS2 Jazzy sourced
* Workspace overlays applied (if available)
* No manual Docker interaction required

This effectively abstracts Docker and standardizes the development workflow to a single command:

```sh
robot dev
```

## Cleanup Script (`rm`)

> **WARNING:** This script performs a full cleanup of the Docker environment, including containers, volumes, images, and unused resources.

The `rm` script is intended to completely reset the development environment by removing all Docker artifacts associated with the project.

### What it does

1. **Resolves project paths**
   Similar to the `dev` script, it determines the project root and navigates to the Docker Compose directory:

   ```
   .docker/arduino_uno_q
   ```

2. **Stops and removes the container**
   It shuts down the running container and removes associated resources:

   ```
   docker compose down -v
   ```

   This includes:

   * Container
   * Network
   * Volumes

3. **Removes the Docker image**
   The script checks if an image named `turtlebot` exists.

   * If found, it forcefully removes it:

     ```
     docker rmi -f <image_id>
     ```

4. **Performs global Docker cleanup**
   It runs:

   ```
   docker system prune -f
   ```

   This removes:

   * Unused containers
   * Dangling images
   * Unused networks
   * Build cache

### Result

After execution, the system is left in a clean state:

* No running containers
* No project-specific images
* Freed disk space

This is useful when:

* You need a fresh rebuild of the environment
* Disk space must be reclaimed
* The container state becomes inconsistent

### Usage

```sh
robot rm
```

> **CAUTION:** This operation is destructive. Any non-persisted data inside the container will be permanently lost.


## Package Creation

Use the following template to ease the package creation on your workspace:

```sh
ros2 pkg create turtlebot_core \
  --build-type ament_python \
  --dependencies rclpy \
  --license "Apache-2.0" \
  --description "TurtleBot core package" \
  --maintainer-name "jesus.loport" \
  --maintainer-email "jesus.loport@outlook.com"
```

## **The Key:** Arduino Router Bridge

> **NOTE:** Check the official documentation for further details about the [Arduino Router Bridge](https://docs.arduino.cc/tutorials/uno-q/user-manual/#interacting-via-unix-socket-advanced)

The ROS 2 package `uno_q_bridge` implements the connection between the `Arduino Router Brigde` and the `ROS 2` environment, by the integration of the following requirements:

1. **arduino-router.sock** 

   ```yml
      volumes:
         - /var/run/arduino-router.sock:/var/run/arduino-router.sock
         ...
   ```

2. **python3-msgpack**

   - `.docker/arduino_uno_q/Dockerfile`
   
      ```Dockerfile
      RUN apt-get update && apt-get install -y \
         python3-msgpack \
         ...
      ```

   - `uno_q_bridge/package.xml`
   
      ```xml
      <exec_depend>python3-msgpack</exec_depend>
      ```

   - `uno_q_bridge/setup.py`

      ```python
      install_requires=[
         'msgpack',
         ...
      ],
      ```

## How to run

### MCU (STM32)

Flash with Arduino IDE the MCU in board the sketch in `test/test_led.ino` 

> **NOTE:** For further details refer to [Arduino Documentation](https://docs.arduino.cc/tutorials/uno-q/user-manual/#usage-example-custom-python-client)

### Linux (QRB)

1. Initialize the container with the command `robot run`
2. Inside the container, you can talk directly with the MCU running the following parametrized script:
   
   ```sh
   cd /test
   python3 msgpack_test.py 0 # turn OFF the led
   python3 msgpack_test.py 1 # turn ON the led 
   ```

3. Inside the container, you can control the ROS 2 node `uno_q_bridge` with the topic `/set_led` as follows:
   ```sh
   cd /turtle_ws
   tmux

   # to raise the node & topic
   ros2 run uno_q_bridge uno_q_bridge # then Ctrl+B+C for a new multiplexed terminal
   
   # to monitor the topic
   ros2 topic echo /set_led           # then Ctrl+B+C for a new multiplexed terminal

   # to control the node with the topic
   ros2 topic pub /set_led std_msgs/msg/Bool "{data: true}" --once
   ros2 topic pub /set_led std_msgs/msg/Bool "{data: false}" --once
   ```

### Expected Outputs

On the control multiplexed terminal:
```sh
root@chucholoport:/turtle_ws# ros2 topic pub /set_led std_msgs/msg/Bool "{data: true}" --once
publisher: beginning loop
publishing #1: std_msgs.msg.Bool(data=True)

root@chucholoport:/turtle_ws# ros2 topic pub /set_led std_msgs/msg/Bool "{data: false}" --once
publisher: beginning loop
publishing #1: std_msgs.msg.Bool(data=False)
```

On the node/topic raiser multiplexed terminal:
```sh
root@chucholoport:/turtle_ws# ros2 run uno_q_bridge uno_q_bridge
[INFO] [1774654192.304000567] [uno_q_bridge]: UNO Q Bridge node started
[INFO] [1774654320.625608364] [uno_q_bridge]: Received LED command: True
[INFO] [1774654320.635854698] [uno_q_bridge]: Router response: [1, 1, None, None]
[INFO] [1774654321.628431679] [uno_q_bridge]: Received LED command: False
[INFO] [1774654321.640391873] [uno_q_bridge]: Router response: [1, 1, None, None]
```

On the monitoring multiplexed terminal:
```sh
root@chucholoport:/turtle_ws# ros2 topic list
/parameter_events
/rosout
/set_led
root@chucholoport:/turtle_ws# ros2 node list
/uno_q_bridge
root@chucholoport:/turtle_ws# ros2 topic echo /set_led
data: true
---
data: false
---
```

---

## Author

**Jesus Salvador Lopez Ortega**

Digital Systems & Robotics Engineer, graduated from [Tecnologico de Monterrey Campus Queretaro](https://tec.mx/es/queretaro/)

Software & Robotics professor at [Universidad Politecnica de Santa Rosa](https://upsrj.edu.mx/)

**Contact:**
- [LinkedIn](https://www.linkedin.com/in/jesus-salvador-lopez-ortega/)
- [GitHub](https://github.com/chucholoport)