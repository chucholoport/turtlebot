#!/usr/bin/env python3

"""
ROS 2 node that bridges ROS topics with an Arduino UNO Q Router
using MessagePack RPC over a UNIX domain socket.

The node:
- Subscribes to velocity commands (/cmd_vel)
- Converts them to differential wheel speeds
- Sends commands to the MCU
- Receives encoder feedback
- Publishes encoder ticks to ROS
"""

import socket
import threading
import msgpack

import rclpy
from rclpy.node import Node

from geometry_msgs.msg import Twist
from std_msgs.msg import Int32MultiArray


class UnoQBridge(Node):
    """Bridge node for motor control and encoder feedback."""

    SOCKET_PATH = "/var/run/arduino-router.sock"

    def __init__(self) -> None:
        """Initialize ROS interfaces, socket, and threads."""
        super().__init__('uno_q_bridge')

        self._request_id = 0
        self._socket = None
        self._connected = False

        # Robot parameters (tune these)
        self._wheel_base = 0.20      # meters (distance between wheels)
        self._max_linear = 0.5       # m/s
        self._max_pwm = 255

        # ROS interfaces
        self._cmd_sub = self.create_subscription(
            Twist,
            'cmd_vel',
            self._cmd_vel_callback,
            10
        )

        self._encoder_pub = self.create_publisher(
            Int32MultiArray,
            'wheel_ticks',
            10
        )

        self._connect_socket()

        self._rx_thread = threading.Thread(
            target=self._rx_loop,
            daemon=True
        )
        self._rx_thread.start()

        self.get_logger().info("UNO Q Bridge node started")

    # ------------------------------------------------------------------
    # Socket handling
    # ------------------------------------------------------------------

    def _connect_socket(self) -> None:
        """Establish connection to the UNIX socket."""
        try:
            self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self._socket.settimeout(1.0)
            self._socket.connect(self.SOCKET_PATH)
            self._connected = True

            self.get_logger().info("Connected to Arduino router")

        except OSError as exc:
            self._connected = False
            self.get_logger().error(f"Socket connection failed: {exc}")

    def _reconnect(self) -> None:
        """Attempt to reconnect to the socket."""
        self.get_logger().warning("Reconnecting...")

        try:
            if self._socket:
                self._socket.close()
        except Exception:
            pass

        self._connect_socket()

    # ------------------------------------------------------------------
    # RPC
    # ------------------------------------------------------------------

    def _next_request_id(self) -> int:
        """Generate a new RPC request ID."""
        self._request_id += 1
        return self._request_id

    def _send_rpc_request(self, method: str, params: list) -> None:
        """Send a MessagePack RPC request."""
        if not self._connected:
            self._reconnect()
            return

        request = [0, self._next_request_id(), method, params]

        try:
            packed = msgpack.packb(request)
            self._socket.sendall(packed)

        except OSError as exc:
            self.get_logger().error(f"Send failed: {exc}")
            self._connected = False

    # ------------------------------------------------------------------
    # Core logic
    # ------------------------------------------------------------------

    def _cmd_vel_callback(self, msg: Twist) -> None:
        """Convert velocity command to PWM wheel speeds."""
        v = msg.linear.x
        w = msg.angular.z

        # Differential drive kinematics
        v_left = v - (w * self._wheel_base / 2.0)
        v_right = v + (w * self._wheel_base / 2.0)

        # Normalize to [-1, 1]
        v_left_norm = max(min(v_left / self._max_linear, 1.0), -1.0)
        v_right_norm = max(min(v_right / self._max_linear, 1.0), -1.0)

        # Convert to PWM
        pwm_left = int(v_left_norm * self._max_pwm)
        pwm_right = int(v_right_norm * self._max_pwm)

        self.get_logger().debug(
            f"PWM L:{pwm_left} R:{pwm_right}"
        )

        self._send_rpc_request(
            "set_wheel_speeds",
            [pwm_left, pwm_right]
        )

    # ------------------------------------------------------------------
    # RX loop
    # ------------------------------------------------------------------

    def _rx_loop(self) -> None:
        """Continuously receive incoming messages."""
        unpacker = msgpack.Unpacker(raw=False)

        while rclpy.ok():
            if not self._connected:
                self._reconnect()
                continue

            try:
                data = self._socket.recv(1024)

                if not data:
                    self._connected = False
                    continue

                unpacker.feed(data)

                for msg in unpacker:
                    self._handle_incoming(msg)

            except socket.timeout:
                continue

            except OSError as exc:
                self.get_logger().error(f"Receive failed: {exc}")
                self._connected = False

    def _handle_incoming(self, msg) -> None:
        """Handle incoming messages from router."""
        try:
            if isinstance(msg, list) and len(msg) >= 2:
                method = msg[0]
                params = msg[1]

                if method == "encoder_update":
                    self._publish_encoders(params)

        except Exception as exc:
            self.get_logger().error(f"Invalid message: {exc}")

    def _publish_encoders(self, data: list) -> None:
        """Publish encoder ticks."""
        msg = Int32MultiArray()
        msg.data = data
        self._encoder_pub.publish(msg)

    # ------------------------------------------------------------------
    # Shutdown
    # ------------------------------------------------------------------

    def destroy_node(self):
        """Clean shutdown."""
        try:
            if self._socket:
                self._socket.close()
        except Exception:
            pass

        super().destroy_node()


def main(args=None) -> None:
    """Initialize ROS 2 and run the node."""
    rclpy.init(args=args)

    node = UnoQBridge()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()