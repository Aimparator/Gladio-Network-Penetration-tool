🛡️ GLADIO C2 - Network Resilience Analyzer

Developed by @aimparator

GLADIO is a high-performance C++ tool designed for network stress testing and real-time latency analysis. It features a dual-channel system that allows independent UDP flooding while simultaneously monitoring a target's network stability via ICMP (Real Ping).
🚀 Features

    Dual-Target Control: Set a different IP for flooding and another for monitoring.

    Real-Time Latency Proof: Integrated ICMP engine that captures actual ms response times from the OS.

    Web-Based C2 Panel: Modern, dark-themed dashboard (Dark/Blue) for remote management.

    High Performance: Multithreaded UDP engine optimized for maximum throughput.

    Live Analytics: Auto-updating table showing "Proof of Work" (Stability vs. Stress).

🛠️ Technical Setup
1. Prerequisites (Kali Linux / Ubuntu)

Ensure you have g++ and standard network tools installed:
Bash

sudo apt update
sudo apt install g++ iputils-ping -y

2. Compilation

Compile with O3 optimization and pthread support for maximum speed:
Bash

g++ -O3 gladio.cpp -o gladio -pthread

3. Execution

Run the binary with root privileges (required for raw network operations):
Bash

sudo ./gladio

🖥️ How to Use

    Start the Server: Once executed, the dashboard will be live at http://localhost:8080.

    Configure Targets:

        Flood Target: The IP address you wish to stress test.

        Monitor Target: The IP you want to watch (e.g., 8.8.8.8 or the flood target itself).

    Analyze: Watch the Real-Time Proof of Work table. If the target is successfully stressed, the latency will spike or show TIMEOUT.

⚠️ Disclaimer

    IMPORTANT: This tool is created for educational purposes and authorized security auditing only. Unauthorized use of this tool against targets without prior written consent is illegal. The developer (aimparator) is not responsible for any misuse or damage caused by this program.

👤 Author

    Developer: aimparator

    Language: C++ / HTML / JavaScript

    Environment: Linux (Optimized for Kali)
