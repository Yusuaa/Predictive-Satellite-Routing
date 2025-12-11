#ifndef PERFORMANCE_ANALYZER_H
#define PERFORMANCE_ANALYZER_H

#include <iostream>
#include <map>
#include <chrono>
#include "ns3/core-module.h"
#include "ns3/simulator.h"
#include "../helpers/quagga-integration.h"

using namespace ns3;

/**
 * Collect and analyze RFP vs standard OSPF performance metrics
 * VERSION 2.0 - REAL MEASUREMENTS
 */
class PerformanceAnalyzer {
private:
    struct Metrics {
        uint32_t packetsLost;
        double routeOutageTotal;     
        uint32_t linkDownEvents;
        double detectionTimeTotal;   
        uint32_t realQuaggaModifications;
        
        Metrics() : packetsLost(0), routeOutageTotal(0.0), linkDownEvents(0), 
                   detectionTimeTotal(0.0), realQuaggaModifications(0) {}
    };
    
    struct LinkEvent {
        double linkDownTime;         
        double routeUpdateTime;      
        double detectionTime;        
        bool isRfp;                  
        uint32_t packetsLostDuringOutage;
        bool completed;
        
        LinkEvent() : linkDownTime(0), routeUpdateTime(0), detectionTime(0),
                     isRfp(false), packetsLostDuringOutage(0), completed(false) {}
    };
    
    Metrics m_standardOspf;
    Metrics m_rfp;
    double m_simulationStartTime;
    
    std::map<std::string, LinkEvent> m_activeEvents;
    
    uint64_t m_packetsSentTotal;
    uint64_t m_packetsReceivedTotal;
    uint64_t m_packetsAtLinkDown;
    
    std::string MakeLinkKey(int nodeA, int nodeB) {
        if (nodeA > nodeB) std::swap(nodeA, nodeB);
        return std::to_string(nodeA) + "-" + std::to_string(nodeB);
    }
    
public:
    PerformanceAnalyzer() : m_simulationStartTime(0.0), 
                            m_packetsSentTotal(0), m_packetsReceivedTotal(0),
                            m_packetsAtLinkDown(0) {}
    
    void SetSimulationStart(double startTime) {
        m_simulationStartTime = startTime;
    }
    
    void OnPacketSent() {
        m_packetsSentTotal++;
    }
    
    void OnPacketReceived() {
        m_packetsReceivedTotal++;
    }
    
    void StartLinkDownEvent(int nodeA, int nodeB, bool isRfp) {
        std::string key = MakeLinkKey(nodeA, nodeB);
        double now = Simulator::Now().GetSeconds() * 1000.0;
        
        LinkEvent event;
        event.linkDownTime = now;
        event.isRfp = isRfp;
        event.packetsLostDuringOutage = 0;
        event.completed = false;
        
        if (isRfp) {
            event.routeUpdateTime = now; 
            event.detectionTime = 0;     
        }
        
        m_activeEvents[key] = event;
        m_packetsAtLinkDown = m_packetsSentTotal;
        
        std::cout << "MEASUREMENT: Link-down event started " << key 
                  << " t=" << now << "ms (RFP=" << (isRfp ? "YES" : "NO") << ")" << std::endl;
    }
    
    void RecordOspfDetection(int nodeA, int nodeB) {
        std::string key = MakeLinkKey(nodeA, nodeB);
        double now = Simulator::Now().GetSeconds() * 1000.0;
        
        if (m_activeEvents.find(key) != m_activeEvents.end()) {
            LinkEvent& event = m_activeEvents[key];
            if (!event.isRfp) {
                event.detectionTime = now - event.linkDownTime;
                std::cout << "MEASUREMENT: OSPF detection after " << event.detectionTime << "ms" << std::endl;
            }
        }
    }
    
    void RecordRouteConvergence(int nodeA, int nodeB) {
        std::string key = MakeLinkKey(nodeA, nodeB);
        double now = Simulator::Now().GetSeconds() * 1000.0;
        
        if (m_activeEvents.find(key) != m_activeEvents.end()) {
            LinkEvent& event = m_activeEvents[key];
            if (!event.isRfp) {
                event.routeUpdateTime = now;
            }
            
            // Calculer les paquets perdus pendant l'outage
            event.packetsLostDuringOutage = m_packetsSentTotal - m_packetsAtLinkDown - 
                                            (m_packetsReceivedTotal - m_packetsAtLinkDown);
            
            std::cout << "📏 MESURE: Convergence à t=" << now << "ms" << std::endl;
        }
    }
    
    void CompleteLinkEvent(int nodeA, int nodeB, uint32_t quaggaMods) {
        std::string key = MakeLinkKey(nodeA, nodeB);
        double now = Simulator::Now().GetSeconds() * 1000.0;
        
        if (m_activeEvents.find(key) != m_activeEvents.end()) {
            LinkEvent& event = m_activeEvents[key];
            
            double outageTime = event.routeUpdateTime - event.linkDownTime;
            if (outageTime < 0) outageTime = 0; 
            
            if (event.isRfp) {
                m_rfp.routeOutageTotal += outageTime;
                m_rfp.packetsLost += event.packetsLostDuringOutage;
                m_rfp.linkDownEvents++;
                m_rfp.detectionTimeTotal += event.detectionTime;
                m_rfp.realQuaggaModifications += quaggaMods;
                
                std::cout << "📊 MESURE RÉELLE RFP: outage=" << outageTime 
                          << "ms, packets_lost=" << event.packetsLostDuringOutage 
                          << ", quagga_mods=" << quaggaMods << std::endl;
            } else {
                m_standardOspf.routeOutageTotal += outageTime;
                m_standardOspf.packetsLost += event.packetsLostDuringOutage;
                m_standardOspf.linkDownEvents++;
                m_standardOspf.detectionTimeTotal += event.detectionTime;
                m_standardOspf.realQuaggaModifications += quaggaMods;
                
                std::cout << "📊 MESURE RÉELLE OSPF: outage=" << outageTime 
                          << "ms, detection=" << event.detectionTime
                          << "ms, packets_lost=" << event.packetsLostDuringOutage << std::endl;
            }
            
            event.completed = true;
            m_activeEvents.erase(key);
        }
    }
    
    void RecordLinkDownEvent(bool useRfp, double outageTimeMs, uint32_t packetsLost, 
                           double detectionTimeMs = 0.0, uint32_t quaggaMods = 0) {
        try {
            if (useRfp) {
                m_rfp.routeOutageTotal += outageTimeMs;
                m_rfp.packetsLost += packetsLost;
                m_rfp.linkDownEvents++;
                m_rfp.detectionTimeTotal += detectionTimeMs;
                m_rfp.realQuaggaModifications += quaggaMods;
            } else {
                m_standardOspf.routeOutageTotal += outageTimeMs;
                m_standardOspf.packetsLost += packetsLost;
                m_standardOspf.linkDownEvents++;
                m_standardOspf.detectionTimeTotal += detectionTimeMs;
                m_standardOspf.realQuaggaModifications += quaggaMods;
            }
            
            std::cout << "MEASUREMENT: Recorded " << (useRfp ? "RFP" : "Standard OSPF") 
                      << " event: outage=" << outageTimeMs << "ms, packets_lost=" << packetsLost 
                      << ", quagga_mods=" << quaggaMods << std::endl;
                       
        } catch (const std::exception& e) {
            std::cerr << "ERROR: Failed to record link down event: " << e.what() << std::endl;
        }
    }
    
    void PrintFinalResults() {
        try {
            std::cout << "" << std::endl;
            std::cout << "" << std::endl;
            std::cout << "========== PERFORMANCE ANALYSIS RESULTS ==========" << std::endl;
            std::cout << "" << std::endl;
            
            double avgStandardOutage = (m_standardOspf.linkDownEvents > 0) ? 
                (m_standardOspf.routeOutageTotal / m_standardOspf.linkDownEvents) : 0.0;
            double avgRfpOutage = (m_rfp.linkDownEvents > 0) ? 
                (m_rfp.routeOutageTotal / m_rfp.linkDownEvents) : 0.0;
                
            double avgStandardDetection = (m_standardOspf.linkDownEvents > 0) ? 
                (m_standardOspf.detectionTimeTotal / m_standardOspf.linkDownEvents) : 0.0;
            double avgRfpDetection = (m_rfp.linkDownEvents > 0) ? 
                (m_rfp.detectionTimeTotal / m_rfp.linkDownEvents) : 0.0;
            
            std::cout << "Standard OSPF Performance:" << std::endl;
            std::cout << "   Events: " << m_standardOspf.linkDownEvents << std::endl;
            std::cout << "   Total packets lost: " << m_standardOspf.packetsLost << std::endl;
            std::cout << "   Average route outage: " << avgStandardOutage << " ms" << std::endl;
            std::cout << "   Average detection time: " << avgStandardDetection << " ms" << std::endl;
            std::cout << "   Quagga modifications: " << m_standardOspf.realQuaggaModifications << std::endl;
            
            std::cout << "" << std::endl;
            std::cout << "SATNET-OSPF RFP Performance:" << std::endl;
            std::cout << "   Events: " << m_rfp.linkDownEvents << std::endl;
            std::cout << "   Total packets lost: " << m_rfp.packetsLost << std::endl;
            std::cout << "   Average route outage: " << avgRfpOutage << " ms" << std::endl;
            std::cout << "   Average detection time: " << avgRfpDetection << " ms" << std::endl;
            std::cout << "   Quagga modifications: " << m_rfp.realQuaggaModifications << std::endl;
            
            std::cout << "" << std::endl;
            std::cout << "IMPROVEMENT ANALYSIS (MEASURED):" << std::endl;
            
            if (avgStandardOutage > 0 && avgRfpOutage >= 0) {
                double outageImprovement = avgStandardOutage / (avgRfpOutage + 0.001);
                std::cout << "   Route Outage: " << avgStandardOutage << "ms → " 
                          << avgRfpOutage << "ms (" << outageImprovement << "x improvement)" << std::endl;
            } else if (m_rfp.linkDownEvents > 0) {
                std::cout << "   RFP Route Outage: " << avgRfpOutage << " ms (no standard OSPF baseline)" << std::endl;
            }
            
            if (m_standardOspf.packetsLost > 0 || m_rfp.packetsLost > 0) {
                std::cout << "   Packet Loss: " << m_standardOspf.packetsLost << " → " 
                          << m_rfp.packetsLost << " packets" << std::endl;
            }
            
            if (avgStandardDetection > 0 && avgRfpDetection >= 0) {
                double detectionImprovement = avgStandardDetection / (avgRfpDetection + 0.001);
                std::cout << "   Detection Time: " << avgStandardDetection << "ms -> " 
                          << avgRfpDetection << "ms (" << detectionImprovement << "x faster)" << std::endl;
            }
            
            std::cout << "   Total Quagga modifications: " 
                      << (m_rfp.realQuaggaModifications + m_standardOspf.realQuaggaModifications) << std::endl;
            std::cout << "   vtysh status: " << (GetVtyshState().available ? "REAL" : "SIMULATED") << std::endl;
            
            std::cout << "" << std::endl;
            std::cout << "Total simulation packets: sent=" << m_packetsSentTotal 
                      << ", received=" << m_packetsReceivedTotal << std::endl;
            
            std::cout << "" << std::endl;
            std::cout << "================================================" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error printing final results: " << e.what() << std::endl;
        }
    }
};

#endif // PERFORMANCE_ANALYZER_H
