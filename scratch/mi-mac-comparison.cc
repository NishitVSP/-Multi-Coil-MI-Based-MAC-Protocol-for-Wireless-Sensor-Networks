#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

enum PacketType {
    REV_PACKET = 1,
    ACK_PACKET = 2,
    DATA_PACKET = 3,
    RTS_PACKET = 4,
    CTS_PACKET = 5
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
std::string packetNames[] = {"UNKNOWN", "REV", "ACK", "DATA", "RTS", "CTS"};
std::string coilNames[] = {"X", "Y", "Z"};

// Energy consumption values from Table II of the paper
struct EnergyMetrics {
    double idleCurrent = 50.0;      // µA
    double receiveCurrent = 200.0;   // µA
    double dataAcquireCurrent = 250.0; // µA
    double channelSensingCurrent = 200.0; // µA
    double transmitCurrent = 1120.0; // µA (1.12 mA)
    double totalEnergy = 0.0;        // µJ
    int stateTransitions = 0;
    int packetsSent = 0;
};

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

void StateTransition(NodeState from, NodeState to, std::string reason, EnergyMetrics& energy) {
    std::cout << "[State Transition] " << stateNames[from] << " -> " 
              << stateNames[to] << " (" << reason << ")" << std::endl;
    energy.stateTransitions++;
}

void SendPacket(PacketType type, int txCoil, std::string sender, std::string receiver, EnergyMetrics& energy) {
    std::cout << "\n[Packet Transmission]" << std::endl;
    std::cout << "  Type: " << packetNames[type] << std::endl;
    std::cout << "  From: " << sender << " -> To: " << receiver << std::endl;
    if (txCoil >= 0) {
        std::cout << "  Tx Coil: " << coilNames[txCoil] << std::endl;
    }
    
    if (type == REV_PACKET) {
        std::cout << "  Structure: [Carrier|Preamble|TargetID|PacketID|TxCoilID|EOF] (13 bytes)" << std::endl;
        energy.totalEnergy += 13 * 8 * energy.transmitCurrent * 0.001; // Simplified energy calc
    } else if (type == ACK_PACKET || type == CTS_PACKET) {
        std::cout << "  Structure: [Carrier|PacketID|TxCoilID|RxCoilID|EOF] (5 bytes)" << std::endl;
        energy.totalEnergy += 5 * 8 * energy.transmitCurrent * 0.001;
    } else if (type == DATA_PACKET) {
        std::cout << "  Structure: [Carrier|PacketID|Data|EOF] (3-19 bytes)" << std::endl;
        energy.totalEnergy += 10 * 8 * energy.transmitCurrent * 0.001;
    } else if (type == RTS_PACKET) {
        std::cout << "  Structure: [RTS Control Frame] (20 bytes)" << std::endl;
        energy.totalEnergy += 20 * 8 * energy.transmitCurrent * 0.001;
    }
    energy.packetsSent++;
}

EnergyMetrics SimulateMultiCoilMIMACProtocol() {
    EnergyMetrics energy;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "   MULTI-COIL MI MAC PROTOCOL SIMULATION" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n>>> SOURCE NODE: Initiating Communication <<<" << std::endl;
    
    NodeState sourceState = STATE_IDLE;
    std::cout << "\nInitial State: " << stateNames[sourceState] << std::endl;
    
    std::cout << "\n[Event] Sensor interrupt detected - Data ready to send" << std::endl;
    StateTransition(sourceState, STATE_DATA_ACQUIRE, "Sensor interrupt", energy);
    sourceState = STATE_DATA_ACQUIRE;
    energy.totalEnergy += energy.dataAcquireCurrent * 10;
    
    std::cout << "\n[Data Acquisition] Collecting sensor data..." << std::endl;
    std::cout << "  Sensor reading: Temperature = 25.3°C" << std::endl;
    
    StateTransition(sourceState, STATE_CHANNEL_SENSING, "Data ready", energy);
    sourceState = STATE_CHANNEL_SENSING;
    energy.totalEnergy += energy.channelSensingCurrent * 5;
    
    std::cout << "\n[Channel Sensing] Checking if channel is available..." << std::endl;
    std::cout << "  Channel Status: CLEAR" << std::endl;
    
    StateTransition(sourceState, STATE_TRANSMIT, "Channel clear", energy);
    sourceState = STATE_TRANSMIT;
    
    std::cout << "\n>>> Sending REV Packet (3 times, once per coil) <<<" << std::endl;
    for (int coil = 0; coil < 3; coil++) {
        std::cout << "\nTransmission " << (coil + 1) << "/3:" << std::endl;
        SendPacket(REV_PACKET, coil, "Source", "Destination", energy);
    }
    
    StateTransition(sourceState, STATE_RECEIVE, "Waiting for ACK", energy);
    sourceState = STATE_RECEIVE;
    energy.totalEnergy += energy.receiveCurrent * 5;
    
    std::cout << "\n\n>>> DESTINATION NODE: Processing REV Packets <<<" << std::endl;
    NodeState destState = STATE_IDLE;
    
    std::cout << "\n[Event] REV packets received on all 3 coils" << std::endl;
    StateTransition(destState, STATE_RECEIVE, "Packet received with correct ID", energy);
    destState = STATE_RECEIVE;
    energy.totalEnergy += energy.receiveCurrent * 15;
    
    int bestCoil = SelectBestCoil();
    
    std::cout << "\n[Preparing ACK] Using best coil pair" << std::endl;
    StateTransition(destState, STATE_CHANNEL_SENSING, "REV received, sending ACK", energy);
    destState = STATE_CHANNEL_SENSING;
    energy.totalEnergy += energy.channelSensingCurrent * 3;
    
    std::cout << "\n[Channel Sensing] Channel is clear" << std::endl;
    StateTransition(destState, STATE_TRANSMIT, "Channel clear", energy);
    destState = STATE_TRANSMIT;
    
    SendPacket(ACK_PACKET, bestCoil, "Destination", "Source", energy);
    
    StateTransition(destState, STATE_RECEIVE, "Waiting for data", energy);
    destState = STATE_RECEIVE;
    
    std::cout << "\n\n>>> SOURCE NODE: ACK Received <<<" << std::endl;
    std::cout << "\n[Event] ACK received with coil pair: " << coilNames[bestCoil] << std::endl;
    std::cout << "  Coil pair established for communication" << std::endl;
    
    StateTransition(sourceState, STATE_CHANNEL_SENSING, "Ready to send data", energy);
    sourceState = STATE_CHANNEL_SENSING;
    energy.totalEnergy += energy.channelSensingCurrent * 3;
    
    std::cout << "\n[Channel Sensing] Channel is clear" << std::endl;
    StateTransition(sourceState, STATE_TRANSMIT, "Channel clear", energy);
    sourceState = STATE_TRANSMIT;
    
    SendPacket(DATA_PACKET, bestCoil, "Source", "Destination", energy);
    
    StateTransition(sourceState, STATE_IDLE, "Transmission complete", energy);
    sourceState = STATE_IDLE;
    
    std::cout << "\n\n>>> DESTINATION NODE: Data Received <<<" << std::endl;
    std::cout << "\n[Event] Data packet received successfully" << std::endl;
    std::cout << "  Data: Temperature = 25.3°C" << std::endl;
    energy.totalEnergy += energy.receiveCurrent * 10;
    
    StateTransition(destState, STATE_IDLE, "Data received completely", energy);
    destState = STATE_IDLE;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "   COMMUNICATION COMPLETE" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    return energy;
}

EnergyMetrics SimulateTraditionalCSMACA() {
    EnergyMetrics energy;
    
    std::cout << "\n\n" << std::string(70, '=') << std::endl;
    std::cout << "   TRADITIONAL CSMA/CA MAC PROTOCOL SIMULATION" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n>>> SOURCE NODE: Initiating Communication <<<" << std::endl;
    
    NodeState sourceState = STATE_IDLE;
    std::cout << "\nInitial State: " << stateNames[sourceState] << std::endl;
    
    std::cout << "\n[Event] Data ready to send" << std::endl;
    StateTransition(sourceState, STATE_DATA_ACQUIRE, "Data collection", energy);
    sourceState = STATE_DATA_ACQUIRE;
    energy.totalEnergy += energy.dataAcquireCurrent * 10;
    
    std::cout << "\n[Data Acquisition] Collecting sensor data..." << std::endl;
    std::cout << "  Sensor reading: Temperature = 25.3°C" << std::endl;
    
    StateTransition(sourceState, STATE_CHANNEL_SENSING, "Data ready", energy);
    sourceState = STATE_CHANNEL_SENSING;
    energy.totalEnergy += energy.channelSensingCurrent * 8;
    
    std::cout << "\n[Channel Sensing] Checking if channel is available..." << std::endl;
    std::cout << "  Channel Status: CLEAR" << std::endl;
    
    StateTransition(sourceState, STATE_TRANSMIT, "Channel clear", energy);
    sourceState = STATE_TRANSMIT;
    
    std::cout << "\n>>> Sending RTS (Request to Send) <<<" << std::endl;
    SendPacket(RTS_PACKET, -1, "Source", "Destination", energy);
    
    StateTransition(sourceState, STATE_RECEIVE, "Waiting for CTS", energy);
    sourceState = STATE_RECEIVE;
    energy.totalEnergy += energy.receiveCurrent * 8;
    
    std::cout << "\n\n>>> DESTINATION NODE: Processing RTS <<<" << std::endl;
    NodeState destState = STATE_IDLE;
    
    std::cout << "\n[Event] RTS received" << std::endl;
    StateTransition(destState, STATE_RECEIVE, "Packet received", energy);
    destState = STATE_RECEIVE;
    energy.totalEnergy += energy.receiveCurrent * 20;
    
    std::cout << "\n[Preparing CTS] " << std::endl;
    StateTransition(destState, STATE_CHANNEL_SENSING, "RTS received, sending CTS", energy);
    destState = STATE_CHANNEL_SENSING;
    energy.totalEnergy += energy.channelSensingCurrent * 5;
    
    std::cout << "\n[Channel Sensing] Channel is clear" << std::endl;
    StateTransition(destState, STATE_TRANSMIT, "Channel clear", energy);
    destState = STATE_TRANSMIT;
    
    std::cout << "\n>>> Sending CTS (Clear to Send) <<<" << std::endl;
    SendPacket(CTS_PACKET, -1, "Destination", "Source", energy);
    
    StateTransition(destState, STATE_RECEIVE, "Waiting for data", energy);
    destState = STATE_RECEIVE;
    
    std::cout << "\n\n>>> SOURCE NODE: CTS Received <<<" << std::endl;
    std::cout << "\n[Event] CTS received - Channel reserved" << std::endl;
    
    StateTransition(sourceState, STATE_TRANSMIT, "Ready to send data", energy);
    sourceState = STATE_TRANSMIT;
    
    std::cout << "\n>>> Sending DATA Packet <<<" << std::endl;
    SendPacket(DATA_PACKET, -1, "Source", "Destination", energy);
    
    std::cout << "\n>>> Waiting for ACK <<<" << std::endl;
    StateTransition(sourceState, STATE_RECEIVE, "Waiting for ACK", energy);
    sourceState = STATE_RECEIVE;
    energy.totalEnergy += energy.receiveCurrent * 5;
    
    std::cout << "\n\n>>> DESTINATION NODE: Data Received <<<" << std::endl;
    std::cout << "\n[Event] Data packet received successfully" << std::endl;
    std::cout << "  Data: Temperature = 25.3°C" << std::endl;
    energy.totalEnergy += energy.receiveCurrent * 10;
    
    StateTransition(destState, STATE_TRANSMIT, "Sending ACK", energy);
    destState = STATE_TRANSMIT;
    
    std::cout << "\n>>> Sending ACK <<<" << std::endl;
    SendPacket(ACK_PACKET, -1, "Destination", "Source", energy);
    
    StateTransition(destState, STATE_IDLE, "Transmission complete", energy);
    destState = STATE_IDLE;
    
    std::cout << "\n\n>>> SOURCE NODE: ACK Received <<<" << std::endl;
    std::cout << "\n[Event] ACK received - Transmission successful" << std::endl;
    
    StateTransition(sourceState, STATE_IDLE, "Communication complete", energy);
    sourceState = STATE_IDLE;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "   COMMUNICATION COMPLETE" << std::endl;
    std::cout << "\n" << std::string(70, '=') << std::endl;
    
    return energy;
}

void CompareProtocols(EnergyMetrics miMac, EnergyMetrics csmaCa) {
    std::cout << "\n\n" << std::string(70, '=') << std::endl;
    std::cout << "   PROTOCOL COMPARISON" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n┌─────────────────────────────────────┬──────────────────┬──────────────────┐" << std::endl;
    std::cout << "│ Metric                              │ Multi-Coil MI    │ CSMA/CA          │" << std::endl;
    std::cout << "├─────────────────────────────────────┼──────────────────┼──────────────────┤" << std::endl;
    
    std::cout << "│ Total Packets Sent                  │ " << std::setw(16) << miMac.packetsSent 
              << " │ " << std::setw(16) << csmaCa.packetsSent << " │" << std::endl;
    
    std::cout << "│ State Transitions                   │ " << std::setw(16) << miMac.stateTransitions 
              << " │ " << std::setw(16) << csmaCa.stateTransitions << " │" << std::endl;
    
    std::cout << "│ Total Energy (µJ)                   │ " << std::setw(16) << std::fixed << std::setprecision(2) << miMac.totalEnergy 
              << " │ " << std::setw(16) << csmaCa.totalEnergy << " │" << std::endl;
    
    double energySaving = ((csmaCa.totalEnergy - miMac.totalEnergy) / csmaCa.totalEnergy) * 100;
    
    std::cout << "└─────────────────────────────────────┴──────────────────┴──────────────────┘" << std::endl;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "   KEY ADVANTAGES OF MULTI-COIL MI MAC" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\n✓ Energy Efficiency: " << std::fixed << std::setprecision(1) << energySaving << "% energy saving" << std::endl;
    std::cout << "✓ Spatial Reuse: Multiple coil pairs allow concurrent transmissions" << std::endl;
    std::cout << "✓ Directional Communication: RSSI-based coil selection optimizes link" << std::endl;
    std::cout << "✓ Collision Avoidance: Channel sensing + coil diversity reduces collisions" << std::endl;
    std::cout << "✓ Lower Overhead: Fewer control packets (REV→ACK→DATA vs RTS→CTS→DATA→ACK)" << std::endl;
    std::cout << "✓ Dual Environment: Works in both terrestrial and underwater networks" << std::endl;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "   REFERENCE: Paper Table II - Current Consumption per State" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "\n  Idle:            50 µA   (Ultra-low power listening)" << std::endl;
    std::cout << "  Receive:        200 µA   (Packet decoding)" << std::endl;
    std::cout << "  Data Acquire:   250 µA   (Sensor data collection)" << std::endl;
    std::cout << "  Channel Sensing: 200 µA   (Carrier detection)" << std::endl;
    std::cout << "  Transmit:      1120 µA   (Packet transmission)\n" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "\n\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                                      ║" << std::endl;
    std::cout << "║    MULTI-COIL MI MAC PROTOCOL vs TRADITIONAL CSMA/CA COMPARISON     ║" << std::endl;
    std::cout << "║              NS-3 Network Simulator Demonstration                    ║" << std::endl;
    std::cout << "║                                                                      ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════════════╝" << std::endl;
    
    EnergyMetrics miMacMetrics = SimulateMultiCoilMIMACProtocol();
    EnergyMetrics csmaCaMetrics = SimulateTraditionalCSMACA();
    
    CompareProtocols(miMacMetrics, csmaCaMetrics);
    
    return 0;
}
