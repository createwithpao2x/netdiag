#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <regex>
#include <algorithm>
#include <stdexcept>
#include <climits>
#include <fstream>

using namespace std;

string executeCommand(const string& cmd) {
    char buffer[128];
    string result = "";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) throw runtime_error("popen failed!");
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

struct NetworkInterface {
    std::string interface_name, ip_address, subnet_mask, gateway, dns_servers;
    bool dhcp_enabled;
};

void parseipconfig(const std::string &output, std::vector<NetworkInterface> &interfaces) {
    std::istringstream stream(output);
    std::string line;
    NetworkInterface current_interface;
    bool in_interface = false, is_media_connected = true;

    auto extract_value = [](const std::string& line) -> std::string {
        size_t pos = line.find(":");
        return (pos != std::string::npos) ? line.substr(pos + 2) : "";
    };

    while (std::getline(stream, line)) {
        if (line.find("Ethernet adapter") != std::string::npos || line.find("Wireless LAN adapter") != std::string::npos) {
            if (in_interface && is_media_connected) {
                interfaces.push_back(current_interface);
            }
            in_interface = true;
            current_interface = NetworkInterface();
            is_media_connected = true;
        }
        else if (in_interface) {
            if (line.find("Media disconnected") != std::string::npos) {
                is_media_connected = false;
            }
            else if (line.find("Description") != std::string::npos) {
                current_interface.interface_name = extract_value(line);
            }
            else if (line.find("IPv4 Address") != std::string::npos) {
                current_interface.ip_address = extract_value(line);
            }
            else if (line.find("Subnet Mask") != std::string::npos) {
                current_interface.subnet_mask = extract_value(line);
            }
            else if (line.find("Default Gateway") != std::string::npos) {
                current_interface.gateway = extract_value(line);
            }
            else if (line.find("DNS Servers") != std::string::npos) {
                current_interface.dns_servers = extract_value(line);
            }
            else if (line.find("DHCP Enabled") != std::string::npos) {
                current_interface.dhcp_enabled = (extract_value(line).find("Yes") != std::string::npos);
            }
        }

        if (in_interface && !current_interface.ip_address.empty() && is_media_connected && line.find("DHCP Enabled") != std::string::npos) {
            interfaces.push_back(current_interface);
            in_interface = false;
        }
    }
    if (in_interface && is_media_connected && !current_interface.ip_address.empty()) {
        interfaces.push_back(current_interface);
    }
}

void writeToFile(ofstream &outFile, const string &message) {
    outFile << message << endl;
}

void interfaceBrief(ofstream &outFile) {
    string hostname = executeCommand("hostname");
    hostname.erase(remove(hostname.begin(), hostname.end(), '\n'), hostname.end());
    string result = executeCommand("ipconfig /all");
    std::vector<NetworkInterface> interfaces;
    parseipconfig(result, interfaces);
    cout << "NETWORK INTERFACE BRIEF\n" << "[Result] Host Name: " << hostname << endl;
    writeToFile(outFile, "NETWORK INTERFACE BRIEF\n[Result] Host Name: " + hostname);
    for (size_t i = 0; i < interfaces.size(); ++i) {
        const auto& iface = interfaces[i];
        std::cout << "[Result] INTERFACE " << i + 1 << std::endl;
        std::cout << "[Result] Interface: " << iface.interface_name << std::endl;
        std::cout << "[Result] IP Address: " << iface.ip_address << " " << iface.subnet_mask << std::endl;
        std::cout << "[Result] Gateway: " << iface.gateway << std::endl;
        std::cout << "[Result] DNS Servers: " << iface.dns_servers << std::endl;
        std::cout << "[Result] DHCP Enabled: " << (iface.dhcp_enabled ? "Yes" : "No") << std::endl;
        writeToFile(outFile, "[Result] INTERFACE " + to_string(i + 1));
        writeToFile(outFile, "[Result] Interface: " + iface.interface_name);
        writeToFile(outFile, "[Result] IP Address: " + iface.ip_address + " " + iface.subnet_mask);
        writeToFile(outFile, "[Result] Gateway: " + iface.gateway);
        writeToFile(outFile, "[Result] DNS Servers: " + iface.dns_servers);
        writeToFile(outFile, "[Result] DHCP Enabled: " + (iface.dhcp_enabled ? string("Yes") : string("No")));
    }    
    cout << endl;
}

int countAsterisks(const string& line) {
    return count(line.begin(), line.end(), '*');
}

int extractLatency(const string& line) {
    smatch match;
    regex latencyRegex(R"((\d+)\s+ms)");
    if (regex_search(line, match, latencyRegex)) {
        return stoi(match.str(1));
    }
    return -1;
}

void tracerouteTest(const string& targetIP, ofstream &outFile) {
    string cmd = "tracert " + targetIP;  
    string result = executeCommand(cmd);
    vector<string> lines;
    stringstream ss(result);
    string line;
    while (getline(ss, line)) {
        lines.push_back(line);
    }
    vector<string> asteriskLines;
    vector<string> latencyLines;
    vector<string> hopLines;
    for (const string& line : lines) {
        int asteriskCount = countAsterisks(line);
        int latency = extractLatency(line);
        if (asteriskCount > 0) {
            asteriskLines.push_back(line);
        } else if (latency != -1) {
            latencyLines.push_back(line);
        }

        if (line.find("ms") != string::npos) {
            hopLines.push_back(line);
        }
    }
    sort(asteriskLines.begin(), asteriskLines.end(), [](const string& a, const string& b) {
        return countAsterisks(a) > countAsterisks(b);
    });
    sort(latencyLines.begin(), latencyLines.end(), [](const string& a, const string& b) {
        return extractLatency(a) > extractLatency(b);
    });
    cout << "TRACEROUTE TEST RESULTS (" << targetIP << ")\n";
    cout << "[Result] Lines with the Most Latency:\n";
    writeToFile(outFile, "\nTRACEROUTE TEST RESULTS (" + targetIP + ")");
    writeToFile(outFile, "[Result] Lines with the Most Latency:");
    int count = 0;
    for (const string& line : asteriskLines) {
        cout << line << endl;
            writeToFile(outFile, line);
            if (++count >= 3) break;
    }
    if (count < 3) {
        for (const string& line : latencyLines) {
            cout << line << endl;
            writeToFile(outFile, line);
            if (++count >= 3) break;
        }
    }
    cout << "[Result] Total Hop Count: " << hopLines.size() << "\n" << endl;
    writeToFile(outFile, "[Result] Total Hop Count: " + to_string(hopLines.size()));
}

void pingTest(const string& targetIP, ofstream &outFile) {
    int iterations = 30;
    int minPing = INT_MAX, maxPing = 0, avgPing = 0;
    int packetsSent = 0, packetsReceived = 0, packetsLost = 0;
    vector<int> pingTimes;
    for (int i = 0; i < iterations; i++) {
        string cmd = "ping -n 1 " + targetIP + " | findstr /i \"time=\"";
        string result = executeCommand(cmd);
        if (!result.empty()) {
            size_t pos = result.find("time=");
            if (pos != string::npos) {
                size_t time_pos = pos + 5;
                size_t end_pos = result.find("ms", time_pos);
                string time_str = result.substr(time_pos, end_pos - time_pos);
                int pingTime = stoi(time_str);
                pingTimes.push_back(pingTime);
                minPing = min(minPing, pingTime);
                maxPing = max(maxPing, pingTime);
                avgPing += pingTime;
            }
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
    if (iterations > 0) {
        avgPing /= iterations;
    }
    packetsSent = iterations;
    packetsReceived = pingTimes.size();
    packetsLost = packetsSent - packetsReceived;
    float packetLossPercentage = ((float)packetsLost / packetsSent) * 100;
    cout << "PING TEST RESULTS (" << targetIP << ")\n";
    cout << "[Result] Minimum Ping: " << minPing << " ms | Maximum Ping: " << maxPing << " ms | Average Ping: " << avgPing << " ms\n";
    cout << "[Result] Packets Sent: " << packetsSent << " | Packets Received: " << packetsReceived << " | Packet Loss: " << packetsLost << " (" << packetLossPercentage << "%)\n\n";
    writeToFile(outFile, "\nPING TEST RESULTS (" + targetIP + ")");
    writeToFile(outFile, "[Result] Minimum Ping: " + to_string(minPing) + " ms | Maximum Ping: " + to_string(maxPing) + " ms | Average Ping: " + to_string(avgPing) + " ms");
    writeToFile(outFile, "[Result] Packets Sent: " + to_string(packetsSent) + " | Packets Received: " + to_string(packetsReceived) + " | Packet Loss: " + to_string(packetsLost) + " (" + to_string(packetLossPercentage) + "%)");
    if (pingTimes.size() > 1) {
        int totalVariation = 0;
        for (size_t i = 1; i < pingTimes.size(); ++i) {
            totalVariation += abs(pingTimes[i] - pingTimes[i - 1]);
        }
        double jitter = totalVariation / static_cast<double>(pingTimes.size() - 1);
        cout << "JITTER TEST RESULTS (" << targetIP << ")\n";
        cout << "[Result] Jitter: " << jitter << " ms\n" << endl;
        writeToFile(outFile, "\nJITTER TEST RESULTS (" + targetIP + ")");
        writeToFile(outFile, "[Result] Jitter: " + to_string(jitter) + " ms");
    } else {
        cout << "[Error] The network is unstable!\n" << endl;
        writeToFile(outFile, "[Error] The network is unstable!");
    }
}

int main() {
    string outputFile = "ping-traceroute-result.txt";
    ofstream outFile(outputFile, ios::app);
    if (!outFile) {
        cerr << "[Error] The output file result.txt can't be opened!" << endl;
        return 1;
    }
    string targetIP;
    cout << "Please enter target IP address or domain: ";
    cin >> targetIP;
    interfaceBrief(outFile);
    thread tracerouteTestThread([&targetIP, &outFile]() mutable {
        tracerouteTest(targetIP, outFile);
    });
    thread pingTestThread([&targetIP, &outFile]() mutable {
        pingTest(targetIP, outFile);
    });
    tracerouteTestThread.join();
    pingTestThread.join();
    writeToFile(outFile, "");
    outFile.close();
    return 0;
}