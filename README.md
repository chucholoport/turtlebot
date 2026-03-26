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
- [*ROS 2 Jazzy Jalisco*](https://docs.ros.org/en/jazzy/index.html)
- [*Android Debug Bridge*](https://developer.android.com/tools/adb)


## Project Structure

```sh
turtlebot/
│
├── .docker/                   # Docker environment and helper scripts
│   ├── arduino_uno_q/         # Docker setup for Arduino UNO environment
│   │   ├── docker-compose.yml
│   │   └── Dockerfile
│   │
│   └── scripts/               # Utility scripts for container lifecycle
│       ├── dev                # Development container launcher
│       ├── rm                 # Container cleanup script
│       └── robot              # Robot execution script
│
├── src/                       # ROS2 workspace (colcon-based)
│   ├── build/                 # Build artifacts (auto-generated)
│   ├── install/               # Installation space (auto-generated)
│   └── log/                   # Build logs (auto-generated)
│
├── .gitignore                 # Git ignore rules
├── README.md                  # Project documentation
│
└── .git/                      # Git metadata (not relevant for users)
```

> **NOTE:** The `src/` directory follows a standard ROS2 colcon workspace layout.


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
ros2 pkg create turtlebot \
  --build-type ament_python \
  --dependencies rclpy \
  --license "Apache-2.0" \
  --description "TurtleBot core package" \
  --maintainer-name "jesus.loport" \
  --maintainer-email "jesus.loport@outlook.com"
```