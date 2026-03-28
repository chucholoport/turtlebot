import socket
import msgpack
import sys

# 1. Define the connection to the Router's Unix Socket
SOCKET_PATH = "/var/run/arduino-router.sock"

# 2. Parse command line arguments
# Default to turning LED ON (True) if no argument is provided
led_state = True

if len(sys.argv) > 1:
    arg = sys.argv[1]
    if arg == "1":
        led_state = True
    elif arg == "0":
        led_state = False
    else:
        print("Usage: python3 msgpack_test.py [1|0]")
        sys.exit(1)

print(f"Sending request to set LED: {led_state}")

# 3. Create the MessagePack RPC Request
# Format: [type=0 (Request), msgid=1, method="set_led_state", params=[led_state]]
request = [0, 1, "set_led_state", [led_state]]
packed_req = msgpack.packb(request)

# 4. Send the request
try:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(SOCKET_PATH)
        client.sendall(packed_req)

        # 5. Receive the response
        response_data = client.recv(1024)
        response = msgpack.unpackb(response_data)

        # Response Format: [type=1 (Response), msgid=1, error=None, result=None]
        print(f"Router Response: {response}")

except Exception as e:
    print(f"Connection failed: {e}")
