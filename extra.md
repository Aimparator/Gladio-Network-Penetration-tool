🔍 Technical Architecture & Logic

The GLADIO C2 system is built on three core pillars to ensure high performance and accurate diagnostic data:
1. High-Velocity UDP Flooding Engine

The stress testing component uses a Multithreaded UDP Flood mechanism. Unlike TCP, UDP does not require a handshake, allowing the tool to saturate the network interface with raw packets at maximum speed.

    Randomized Port Rotation: Each packet is sent to a random port (1-65535) to bypass simple port-based filters and test the target's firewall state-tracking capabilities.

    Thread Isolation: Each worker thread operates independently to prevent CPU bottlenecks.

2. Real-Time ICMP Diagnostic Engine (The Monitor)

The monitoring system is decoupled from the attack engine. It uses the operating system's native ICMP (Internet Control Message Protocol) stack to send echo requests to the designated Monitor IP.

    Non-Simulated Data: The latency values (ms) are captured directly from the system's ping utility output, providing authentic proof of the network's condition under stress.

    Independent Targeting: Allows the operator to monitor a gateway or a secondary server while attacking the primary target.

3. Asynchronous Web Control Interface

The dashboard is served via a custom-built C++ HTTP Server. It handles incoming API requests from the browser using an asynchronous-like polling method.

    API Endpoints: * /api/start: Triggers the thread pool and resets counters.

        /api/status: Returns a JSON object containing total packets, active target, and real-time latency.

        /api/stop: Safely joins threads and puts the system into standby.
