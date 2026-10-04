#!/usr/bin/env python3
"""Calibrate TCP loopback inside the same shaped namespace as P0/P1."""

import argparse
import json
import socket
import statistics
import threading
import time


def exact(sock, count):
    data = bytearray()
    while len(data) < count:
        part = sock.recv(count - len(data))
        if not part:
            raise RuntimeError("calibration TCP EOF")
        data.extend(part)
    return data


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", choices=("LAN", "WAN"), required=True)
    parser.add_argument("--target-rtt-ms", type=float, required=True)
    parser.add_argument("--target-mbps", type=float, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]
    count = 21
    payload_bytes = 8 * 1024 * 1024
    failure = []

    def serve():
        try:
            peer, _ = listener.accept()
            with peer:
                peer.settimeout(30)
                peer.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                for _ in range(count):
                    peer.sendall(exact(peer, 16))
                remaining = payload_bytes
                while remaining:
                    part = peer.recv(min(65536, remaining))
                    if not part:
                        raise RuntimeError("calibration bulk EOF")
                    remaining -= len(part)
                peer.sendall(b"A")
        except Exception as error:
            failure.append(repr(error))

    worker = threading.Thread(target=serve)
    worker.start()
    with socket.create_connection(("127.0.0.1", port), timeout=30) as client:
        client.settimeout(30)
        client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        rtts = []
        for sample in range(count):
            started = time.perf_counter_ns()
            client.sendall(sample.to_bytes(16, "little"))
            if exact(client, 16) != sample.to_bytes(16, "little"):
                raise RuntimeError("calibration echo mismatch")
            if sample:
                rtts.append((time.perf_counter_ns() - started) / 1e6)
        started = time.perf_counter_ns()
        client.sendall(b"Z" * payload_bytes)
        if exact(client, 1) != b"A":
            raise RuntimeError("calibration acknowledgement mismatch")
        bulk_seconds = (time.perf_counter_ns() - started) / 1e9
    worker.join(timeout=31)
    listener.close()
    if worker.is_alive() or failure:
        raise RuntimeError(f"calibration server failed: {failure}")
    result = {
        "profile": args.profile,
        "topology": "single WSL2 host, TCP 127.0.0.1 inside dedicated network namespace",
        "connection": "same loopback qdisc as all P0/P1 protocol TCP sockets",
        "target_rtt_ms": args.target_rtt_ms,
        "target_mbps": args.target_mbps,
        "rtt_samples": len(rtts),
        "rtt_median_ms": statistics.median(rtts),
        "rtt_min_ms": min(rtts),
        "rtt_max_ms": max(rtts),
        "bulk_application_bytes": payload_bytes,
        "bulk_seconds": bulk_seconds,
        "throughput_mbps": payload_bytes * 8 / bulk_seconds / 1e6,
        "measurement_layer": "TCP application payload, not packet capture or wire bytes",
    }
    with open(args.output, "x", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
