#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
ROS 2 Jazzy node that bridges ROS interfaces with Arduino UNO Q Router
via UNIX domain socket using MessagePack RPC.

Compliant with Python 3 and rclpy conventions.
"""

import socket
import msgpack

import rclpy
from rclpy.node import Node

from std_msgs.msg import Bool


class UnoQBridge(Node):
    """
    ROS 2 node that sends commands to Arduino Router using msgpack RPC.
    """

    SOCKET_PATH = "/var/run/arduino-router.sock"

    def __init__(self) -> None:
        super().__init__('uno_q_bridge')

        # Subscriber: receives LED commands
        self._sub = self.create_subscription(
            Bool,
            'set_led',
            self._led_callback,
            10
        )

        self.get_logger().info("UNO Q Bridge node started")

    def _send_rpc_request(self, method: str, params: list) -> None:
        """
        Sends a MessagePack RPC request to the Arduino router.

        :param method: RPC method name
        :param params: List of parameters
        """

        request = [0, 1, method, params]

        try:
            packed_req = msgpack.packb(request)

            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                client.connect(self.SOCKET_PATH)
                client.sendall(packed_req)

                response_data = client.recv(1024)
                response = msgpack.unpackb(response_data)

                self.get_logger().info(f"Router response: {response}")

        except Exception as exc:
            self.get_logger().error(f"RPC communication failed: {exc}")

    def _led_callback(self, msg: Bool) -> None:
        """
        Callback for LED control topic.

        :param msg: Bool message (True = ON, False = OFF)
        """

        led_state = bool(msg.data)

        self.get_logger().info(f"Received LED command: {led_state}")

        # TODO: Extend mapping logic if additional RPC methods are needed
        self._send_rpc_request("set_led_state", [led_state])


def main(args=None) -> None:
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