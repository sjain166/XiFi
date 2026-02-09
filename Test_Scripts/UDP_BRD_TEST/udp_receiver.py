#!/usr/bin/env python3
import socket
import struct
import time
from collections import defaultdict

BROADCAST_PORT = 5000
BUFFER_SIZE = 1024

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', BROADCAST_PORT))

    print(f"UDP Receiver listening on port {BROADCAST_PORT}")
    print("Waiting for packets...\n")

    last_seq = None
    total_received = 0
    total_expected = 0
    total_lost = 0
    start_time = time.time()
    last_report_time = start_time

    sequence_gaps = []

    try:
        while True:
            data, addr = sock.recvfrom(BUFFER_SIZE)

            if len(data) < 4:
                print(f"Warning: Received packet too small ({len(data)} bytes)")
                continue

            seq_num = struct.unpack('<I', data[:4])[0]
            total_received += 1

            if last_seq is not None:
                expected_seq = (last_seq + 1) % (2**32)

                if seq_num != expected_seq:
                    if seq_num > last_seq:
                        lost = seq_num - expected_seq
                    else:
                        lost = (2**32 - last_seq - 1) + seq_num

                    total_lost += lost
                    sequence_gaps.append({
                        'last_seq': last_seq,
                        'received_seq': seq_num,
                        'lost_count': lost,
                        'time': time.time() - start_time
                    })
                    print(f"[LOSS] Expected seq {expected_seq}, got {seq_num}. Lost {lost} packet(s)")

            last_seq = seq_num

            if last_seq is not None:
                total_expected = total_received + total_lost
            else:
                total_expected = total_received

            current_time = time.time()
            if current_time - last_report_time >= 5.0:
                elapsed = current_time - start_time
                loss_rate = (total_lost / total_expected * 100) if total_expected > 0 else 0
                packet_rate = total_received / elapsed if elapsed > 0 else 0

                print(f"\n--- Statistics (t={elapsed:.1f}s) ---")
                print(f"Received: {total_received} packets")
                print(f"Expected: {total_expected} packets")
                print(f"Lost: {total_lost} packets")
                print(f"Loss Rate: {loss_rate:.2f}%")
                print(f"Packet Rate: {packet_rate:.1f} pkt/s")
                print(f"Current Seq: {seq_num}")
                print(f"Loss Events: {len(sequence_gaps)}")
                print("-" * 35 + "\n")

                last_report_time = current_time

    except KeyboardInterrupt:
        print("\n\n=== Final Statistics ===")
        elapsed = time.time() - start_time
        loss_rate = (total_lost / total_expected * 100) if total_expected > 0 else 0
        packet_rate = total_received / elapsed if elapsed > 0 else 0

        print(f"Total Duration: {elapsed:.1f}s")
        print(f"Packets Received: {total_received}")
        print(f"Packets Expected: {total_expected}")
        print(f"Packets Lost: {total_lost}")
        print(f"Loss Rate: {loss_rate:.2f}%")
        print(f"Average Packet Rate: {packet_rate:.1f} pkt/s")
        print(f"Total Loss Events: {len(sequence_gaps)}")

        if sequence_gaps:
            print("\n=== Loss Event Details ===")
            for i, gap in enumerate(sequence_gaps[:10]):  # Show first 10 events
                print(f"Event {i+1}: Lost {gap['lost_count']} packet(s) at t={gap['time']:.2f}s "
                      f"(seq {gap['last_seq']} -> {gap['received_seq']})")
            if len(sequence_gaps) > 10:
                print(f"... and {len(sequence_gaps) - 10} more loss events")

        print("\nReceiver stopped.")
        sock.close()

if __name__ == "__main__":
    main()
