#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

enum PacketType {
    REV_PACKET = 1,
    ACK_PACKET = 2,
    DATA_PACKET = 3
};

enum CoilID {
    COIL_X = 0,
    COIL_Y = 1,
    COIL_Z = 2
};

enum NodeState {
    STATE_IDLE = 0,
    STATE_RECEIVE = 1,
    STATE_CHANNEL_SENSING = 2,
    STATE_DATA_ACQUIRE = 3,
    STATE_TRANSMIT = 4
};

std::string stateNames[] = {"IDLE", "RECEIVE", "CHANNEL_SENSING", "DATA_ACQUIRE", "TRANSMIT"};
std::string packetNames[] = {"UNKNOWN", "REV", "ACK", "DATA"};
std::string coilNames[] = {"X", "Y", "Z"};

double CalculateRSSI(int coilId) {
    double rssiValues[] = {-45.5, -52.3, -48.7};
    return rssiValues[coilId];
}

int SelectBestCoil() {
    double maxRSSI = -1000;
    int bestCoil = COIL_X;
    
    std::cout << "\n=== Coil Selection Process ===" << std::endl;
    for (int i = 0; i < 3; i++) {
        double rssi = CalculateRSSI(i);
        std::cout << "  Coil " << coilNames[i] << ": RSSI = " << rssi << " dBm" << std::endl;
        if (rssi > maxRSSI) {
            maxRSSI = rssi;
            bestCoil = i;
        }
    }
    std::cout << "  Selected Coil: " << coilNames[bestCoil] << " (RSSI = " << maxRSSI << " dBm)" << std::endl;
    return bestCoil;
}

void StateTransition(NodeState from, NodeState to, std::string reason) {
    std::cout << "[State Transition] " << stateNames[from] << " -> " 
              << stateNames[to] << " (" << reason << ")" << std::endl;
}

void SendPacket(PacketType type, int txCoil, std::string sender, std::string receiver) {
    std::cout << "\n[Packet Transmission]" << std::endl;
    std::cout << "  Type: " << packetNames[type] << std::endl;
    std::cout << "  From: " << sender << " -> To: " << receiver << std::endl;
    std::cout << "  Tx Coil: " << coilNames[txCoil] << std::endl;
    
    if (type == REV_PACKET) {
        std::cout << "  Structure: [Carrier|Preamble|TargetID|PacketID|TxCoilID|EOF] (13 bytes)" << std::endl;
    } else if (type == ACK_PACKET) {
        std::cout << "  Structure: [Carrier|PacketID|TxCoilID|RxCoilID|EOF] (5 bytes)" << std::endl;
    } else if (type == DATA_PACKET) {
        std::cout << "  Structure: [Carrier|PacketID|Data|EOF] (3-19 bytes)" << std::endl;
    }
}

void SimulateMACProtocol() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "   MULTI-COIL MI MAC PROTOCOL SIMULATION" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\n>>> SOURCE NODE: Initiating Communication <<<" << std::endl;
    
    NodeState sourceState = STATE_IDLE;
    std::cout << "\nInitial State: " << stateNames[sourceState] << std::endl;
    
    std::cout << "\n[Event] Sensor interrupt detected - Data ready to send" << std::endl;
    StateTransition(sourceState, STATE_DATA_ACQUIRE, "Sensor interrupt");
    sourceState = STATE_DATA_ACQUIRE;
    
    std::cout << "\n[Data Acquisition] Collecting sensor data..." << std::endl;
    std::cout << "  Sensor reading: Temperature = 25.3°C" << std::endl;
    
    StateTransition(sourceState, STATE_CHANNEL_SENSING, "Data ready");
    sourceState = STATE_CHANNEL_SENSING;
    
    std::cout << "\n[Channel Sensing] Checking if channel is available..." << std::endl;
    std::cout << "  Channel Status: CLEAR" << std::endl;
    
    StateTransition(sourceState, STATE_TRANSMIT, "Channel clear");
    sourceState = STATE_TRANSMIT;
    
    std::cout << "\n>>> Sending REV Packet (3 times, once per coil) <<<" << std::endl;
    for (int coil = 0; coil < 3; coil++) {
        std::cout << "\nTransmission " << (coil + 1) << "/3:" << std::endl;
        SendPacket(REV_PACKET, coil, "Source", "Destination");
    }
    
    StateTransition(sourceState, STATE_RECEIVE, "Waiting for ACK");
    sourceState = STATE_RECEIVE;
    
    std::cout << "\n\n>>> DESTINATION NODE: Processing REV Packets <<<" << std::endl;
    NodeState destState = STATE_IDLE;
    
    std::cout << "\n[Event] REV packets received on all 3 coils" << std::endl;
    StateTransition(destState, STATE_RECEIVE, "Packet received with correct ID");
    destState = STATE_RECEIVE;
    
    int bestCoil = SelectBestCoil();
    
    std::cout << "\n[Preparing ACK] Using best coil pair" << std::endl;
    StateTransition(destState, STATE_CHANNEL_SENSING, "REV received, sending ACK");
    destState = STATE_CHANNEL_SENSING;
    
    std::cout << "\n[Channel Sensing] Channel is clear" << std::endl;
    StateTransition(destState, STATE_TRANSMIT, "Channel clear");
    destState = STATE_TRANSMIT;
    
    SendPacket(ACK_PACKET, bestCoil, "Destination", "Source");
    
    StateTransition(destState, STATE_RECEIVE, "Waiting for data");
    destState = STATE_RECEIVE;
    
    std::cout << "\n\n>>> SOURCE NODE: ACK Received <<<" << std::endl;
    std::cout << "\n[Event] ACK received with coil pair: " << coilNames[bestCoil] << std::endl;
    std::cout << "  Coil pair established for communication" << std::endl;
    
    StateTransition(sourceState, STATE_CHANNEL_SENSING, "Ready to send data");
    sourceState = STATE_CHANNEL_SENSING;
    
    std::cout << "\n[Channel Sensing] Channel is clear" << std::endl;
    StateTransition(sourceState, STATE_TRANSMIT, "Channel clear");
    sourceState = STATE_TRANSMIT;
    
    SendPacket(DATA_PACKET, bestCoil, "Source", "Destination");
    
    StateTransition(sourceState, STATE_IDLE, "Transmission complete");
    sourceState = STATE_IDLE;
    
    std::cout << "\n\n>>> DESTINATION NODE: Data Received <<<" << std::endl;
    std::cout << "\n[Event] Data packet received successfully" << std::endl;
    std::cout << "  Data: Temperature = 25.3°C" << std::endl;
    
    StateTransition(destState, STATE_IDLE, "Data received completely");
    destState = STATE_IDLE;
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "   COMMUNICATION COMPLETE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "\nSummary:" << std::endl;
    std::cout << "  - REV packets sent: 3 (one per coil)" << std::endl;
    std::cout << "  - Best coil selected: " << coilNames[bestCoil] << std::endl;
    std::cout << "  - ACK packet sent: 1" << std::endl;
    std::cout << "  - Data packet sent: 1" << std::endl;
    std::cout << "  - Total state transitions: Multiple" << std::endl;
    std::cout << "  - Protocol: REV -> ACK -> DATA successful\n" << std::endl;
}

int main(int argc, char *argv[]) {
    SimulateMACProtocol();
    return 0;
}
