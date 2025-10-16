# # concurrent_io_test.py
# import asyncio, time, random, json
# import serial
# import serial_asyncio

# # ==== CONFIG ====
# PORT_XBEE  = "/dev/cu.usbserial-A50285BI"   # <-- change if needed
# BAUD_XBEE  = 9600

# PORT_ESP32 = "/dev/cu.SLAB_USBtoUART"       # <-- change if needed
# BAUD_ESP32 = 115200

# TEST_SECONDS = 30
# SMALL_THRESHOLD = 64  # bytes: <= goes to XBee, > goes to ESP32

# # ===== helpers =====
# class SerialEndpoint(asyncio.Protocol):
#     def __init__(self, name, on_line_cb, stats):
#         self.name = name
#         self.on_line_cb = on_line_cb
#         self.transport = None
#         self.buf = bytearray()
#         self.stats = stats

#     def connection_made(self, transport):
#         self.transport = transport
#         print(f"[{self.name}] opened")

#     def data_received(self, data):
#         self.stats[self.name]["rx_bytes"] += len(data)
#         self.buf.extend(data)
#         # line-based parsing for test (both endpoints send '\n')
#         while b'\n' in self.buf:
#             line, _, rest = self.buf.partition(b'\n')
#             self.buf = bytearray(rest)
#             try:
#                 text = line.decode(errors="replace")
#             except:
#                 text = repr(line)
#             self.on_line_cb(self.name, text.strip())

#     def connection_lost(self, exc):
#         print(f"[{self.name}] closed: {exc}")

#     def write_line(self, text: str):
#         payload = (text + "\n").encode()
#         self.stats[self.name]["tx_bytes"] += len(payload)
#         if self.transport:
#             self.transport.write(payload)

# async def open_serial(loop, port, baud, name, on_line_cb, stats):
#     # pyserial-asyncio open
#     def factory():
#         return SerialEndpoint(name, on_line_cb, stats)
#     transport, protocol = await serial_asyncio.create_serial_connection(
#         loop, factory, port, baudrate=baud
#     )
#     return protocol  # we return the Protocol so we can write()

# # ===== scheduler (very simple demo) =====
# def choose_interface(payload: bytes) -> str:
#     # demo: small -> XBee, large -> ESP32
#     return "xbee" if len(payload) <= SMALL_THRESHOLD else "esp32"

# # ===== main test =====
# async def main():
#     loop = asyncio.get_running_loop()

#     stats = {
#         "xbee":  {"tx_bytes": 0, "rx_bytes": 0},
#         "esp32": {"tx_bytes": 0, "rx_bytes": 0}
#     }

#     def on_line(name, line):
#         now = time.strftime("%H:%M:%S")
#         print(f"[{now}] <- {name}: {line}")

#     # Open both ports
#     xbee = await open_serial(loop, PORT_XBEE, BAUD_XBEE, "xbee", on_line, stats)
#     esp  = await open_serial(loop, PORT_ESP32, BAUD_ESP32, "esp32", on_line, stats)

#     start = time.time()
#     last_report = start

#     async def traffic_generator():
#         """Generate mixed payloads and send to whichever interface the scheduler chooses."""
#         i = 0
#         while time.time() - start < TEST_SECONDS:
#             i += 1
#             size = random.choice([16, 32, 48, 64, 128, 256, 512])
#             msg = {
#                 "id": i,
#                 "t": time.time(),
#                 "size": size,
#                 "note": "test"
#             }
#             payload = json.dumps(msg).encode()
#             iface = choose_interface(payload)

#             if iface == "xbee":
#                 xbee.write_line(json.dumps({"tx": "XB", **msg}))
#             else:
#                 esp.write_line(json.dumps({"tx": "ESP", **msg}))

#             await asyncio.sleep(0.25)

#     async def periodic_report():
#         while time.time() - start < TEST_SECONDS:
#             await asyncio.sleep(2.0)
#             nonlocal last_report
#             now = time.time()
#             dt = now - last_report
#             last_report = now
#             for name in ["xbee", "esp32"]:
#                 tx = stats[name]["tx_bytes"]; rx = stats[name]["rx_bytes"]
#                 # compute Mbps over the interval since last report
#                 # convert bytes to bits: *8, then divide by seconds, then /1e6
#                 mbps_tx = (tx * 8 / dt) / 1e6 if dt > 0 else 0
#                 mbps_rx = (rx * 8 / dt) / 1e6 if dt > 0 else 0
#                 print(f"[stats] {name}: TX {tx}B ({mbps_tx:.3f} Mb/s), RX {rx}B ({mbps_rx:.3f} Mb/s) in {dt:.1f}s")
#                 # reset window counters
#                 stats[name]["tx_bytes"] = 0
#                 stats[name]["rx_bytes"] = 0

#     # Kick off tasks
#     tg = asyncio.create_task(traffic_generator())
#     rpt = asyncio.create_task(periodic_report())

#     await asyncio.gather(tg, rpt)
#     print("Test complete.")

# if __name__ == "__main__":
#     try:
#         asyncio.run(main())
#     except serial.SerialException as e:
#         print("Serial error:", e)

import serial, time

ser = serial.Serial('COM4', 9600, timeout=1)  # Adjust COM port
while True:
    ser.write(b'+++')
    time.sleep(1)
    ser.write(b'ATND\r')
    time.sleep(2)
    data = ser.read_all().decode(errors='ignore')
    print(data)
    time.sleep(5)

