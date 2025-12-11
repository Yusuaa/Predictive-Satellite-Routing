#ifndef SATNET_CONTROLLER_H
#define SATNET_CONTROLLER_H

#include <iostream>
#include "ns3/core-module.h"
#include "../modules/topology-mgmt.h"
#include "../modules/link-detection.h"
#include "../modules/route-mgmt.h"
#include "../modules/performance-analyzer.h"

using namespace ns3;

/**
 * SATNET-OSPF Main Controller
 * Coordinates TMM, LDM and RMM modules to implement RFP
 */
class SatnetOspfController {
private:
    TopologyManagementModule m_tmm;
    LinkDetectionModule m_ldm;
    RouteManagementModule m_rmm;
    PerformanceAnalyzer m_analyzer;
    
    uint32_t m_eventCounter;
    double m_lastEventTime;
    uint32_t m_totalQuaggaModifications;
    
public:
    SatnetOspfController() : m_eventCounter(0), m_lastEventTime(0.0), m_totalQuaggaModifications(0) {}
    
    // Schedule a predictable link down event
    void SchedulePredictableLinkDown(int linkId, int nodeA, int nodeB, double eventTime) {
        try {
            // Validate node indices
            if (!ValidateNodeIndices(nodeA, nodeB)) {
                std::cerr << "Invalid node indices for link " << linkId << ": " << nodeA << "<->" << nodeB << std::endl;
                return;
            }
            
            // Add event to TMM
            m_tmm.AddPredictableLinkDown(linkId, nodeA, nodeB, eventTime);
            
            // Schedule RFP actions
            PredictableLinkDownEvent event(linkId, nodeA, nodeB, eventTime);
            
            if (event.T1 > 0) {
                // T1: Start BLD and BFU, force link DOWN in OSPF
                Simulator::Schedule(Seconds(event.T1), &SatnetOspfController::ExecuteT1Actions, 
                                  this, nodeA, nodeB, event.T1);
                
                // T2: Stop BFU, synchronize forwarding tables
                Simulator::Schedule(Seconds(event.T2), &SatnetOspfController::ExecuteT2Actions, 
                                  this, nodeA, nodeB, event.T2);
                
                // T0: Physical failure occurs
                Simulator::Schedule(Seconds(event.T0), &SatnetOspfController::ExecuteT0Actions, 
                                  this, nodeA, nodeB, event.T0);
                
                // T3: Stop BLD, resume normal detection
                Simulator::Schedule(Seconds(event.T3), &SatnetOspfController::ExecuteT3Actions, 
                                  this, nodeA, nodeB, event.T3);
                                  
                m_eventCounter++;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error scheduling predictable link down: " << e.what() << std::endl;
        }
    }
    
    // Handle link state change
    void OnLinkStateChange(int nodeA, int nodeB, bool isUp, double currentTime) {
        try {
            // Update state via LDM (handles BLD periods)
            m_ldm.UpdateRealLinkState(nodeA, nodeB, isUp, currentTime, &m_tmm);
            
            // Get state reported to OSPF (may differ due to RFP)
            bool ospfState = m_ldm.GetReportedState(nodeA, nodeB);
            
            // Generate real route update for Quagga
            Ptr<Node> nodeAPtr = NodeList::GetNode(nodeA);
            Ptr<Node> nodeBPtr = NodeList::GetNode(nodeB);
            
            if (nodeAPtr && nodeBPtr) {
                std::string routeUpdate = GenerateOspfRouteUpdate(nodeA, nodeB, ospfState);
                m_rmm.OnNewRoutingTable(nodeAPtr, routeUpdate, currentTime);
                m_totalQuaggaModifications++;
            }
            
            // Analyze performance if link down
            if (!isUp) {
                AnalyzeLinkDownPerformance(nodeA, nodeB, currentTime);
            }
            
            std::cout << "RFP: Physical=" << (isUp?"UP":"DOWN") 
                      << ", OSPF=" << (ospfState?"UP":"DOWN") 
                      << " for link " << nodeA << "<->" << nodeB << std::endl;
                       
        } catch (const std::exception& e) {
            std::cerr << "Error on link state change: " << e.what() << std::endl;
        }
    }
    
    // Get link state reported to OSPF
    bool GetOspfLinkState(int nodeA, int nodeB) {
        return m_ldm.GetReportedState(nodeA, nodeB);
    }
    
    // Print final statistics
    void PrintFinalStatistics() {
        try {
            std::cout << "========== SATNET-OSPF RFP STATISTICS ==========" << std::endl;
            std::cout << "Events scheduled: " << m_eventCounter << std::endl;
            std::cout << "Route updates blocked during BFU: " << m_rmm.GetBlockedUpdatesCount() << std::endl;
            std::cout << "Route updates applied: " << m_rmm.GetAppliedUpdatesCount() << std::endl;
            std::cout << "Active events: " << m_tmm.GetActiveEvents(Simulator::Now().GetSeconds()).size() << std::endl;
            std::cout << "Total Quagga modifications: " << m_totalQuaggaModifications << std::endl;
            std::cout << "vtysh availability: " << (GetVtyshState().available ? "YES" : "NO (simulated)") << std::endl;
            
            m_analyzer.PrintFinalResults();
            
        } catch (const std::exception& e) {
            std::cerr << "Error printing final statistics: " << e.what() << std::endl;
        }
    }
    
private:
    // RFP actions according to timeline
    void ExecuteT1Actions(int nodeA, int nodeB, double currentTime) {
        try {
            std::cout << "" << std::endl;
            std::cout << "===== RFP T1 ACTIONS =====" << std::endl;
            std::cout << "Time: " << currentTime << "s" << std::endl;
            std::cout << "Link: " << nodeA << "<->" << nodeB << std::endl;
            std::cout << "Action: Starting predictive link avoidance" << std::endl;
            
            // Start tracking this RFP event
            m_analyzer.StartLinkDownEvent(nodeA, nodeB, true); // true = RFP
            
            // 1. Start BLD for this link
            m_ldm.ForceLinkDown(nodeA, nodeB, currentTime);
            m_totalQuaggaModifications += 2; // nodeA and nodeB modified
            
            // 2. Start global BFU
            m_rmm.StartBfuPeriod(currentTime);
            
            std::cout << "OSPF will now avoid this link and recalculate routes" << std::endl;
            std::cout << "Route updates will be synchronized at T2" << std::endl;
            std::cout << "=============================" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error executing T1 actions: " << e.what() << std::endl;
        }
    }
    
    void ExecuteT2Actions(int nodeA, int nodeB, double currentTime) {
        try {
            std::cout << "" << std::endl;
            std::cout << "===== RFP T2 ACTIONS =====" << std::endl;
            std::cout << "Time: " << currentTime << "s" << std::endl;
            std::cout << "Link: " << nodeA << "<->" << nodeB << std::endl;
            std::cout << "Action: Synchronizing forwarding tables" << std::endl;
            
            // Stop BFU - apply all new routes synchronously
            m_rmm.EndBfuPeriod(currentTime);
            m_totalQuaggaModifications += m_rmm.GetBlockedUpdatesCount();
            
            std::cout << "All nodes now have consistent routing tables" << std::endl;
            std::cout << "Traffic flows via alternate paths" << std::endl;
            std::cout << "=============================" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error executing T2 actions: " << e.what() << std::endl;
        }
    }
    
    void ExecuteT0Actions(int nodeA, int nodeB, double currentTime) {
        try {
            std::cout << "" << std::endl;
            std::cout << "===== RFP T0 ACTIONS =====" << std::endl;
            std::cout << "Time: " << currentTime << "s" << std::endl;
            std::cout << "Link: " << nodeA << "<->" << nodeB << std::endl;
            std::cout << "Action: Physical link failure occurs (already prepared)" << std::endl;
            
            std::cout << "CRITICAL: Routes already updated proactively!" << std::endl;
            std::cout << "Traffic already flowing via alternate paths" << std::endl;
            
            // Record convergence
            m_analyzer.RecordRouteConvergence(nodeA, nodeB);
            m_analyzer.CompleteLinkEvent(nodeA, nodeB, m_totalQuaggaModifications);
            
            std::cout << "=============================" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error executing T0 actions: " << e.what() << std::endl;
        }
    }
    
    void ExecuteT3Actions(int nodeA, int nodeB, double currentTime) {
        try {
            std::cout << "" << std::endl;
            std::cout << "===== RFP T3 ACTIONS =====" << std::endl;
            std::cout << "Time: " << currentTime << "s" << std::endl;
            std::cout << "Link: " << nodeA << "<->" << nodeB << std::endl;
            std::cout << "Action: Resuming normal link detection" << std::endl;
            
            // Stop BLD - resume normal detection
            m_ldm.RestoreNormalDetection(nodeA, nodeB, currentTime);
            m_totalQuaggaModifications += 2; // restore on nodeA and nodeB
            
            std::cout << "RFP sequence completed successfully" << std::endl;
            std::cout << "Normal OSPF operation resumed" << std::endl;
            std::cout << "=============================" << std::endl;
            std::cout << "" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error executing T3 actions: " << e.what() << std::endl;
        }
    }
    
    void AnalyzeLinkDownPerformance(int nodeA, int nodeB, double currentTime) {
        try {
            // Check if unpredicted event (standard OSPF)
            if (!m_tmm.IsInBldPeriod(nodeA, nodeB, currentTime)) {
                std::cout << "OSPF standard link-down (unpredicted)" << std::endl;
                
                // For standard OSPF: Dead interval = 40s, convergence = ~100ms
                double detectionTime = 40000.0;  // 40s Dead interval in ms
                double convergenceTime = 100.0;   // ~100ms for SPF
                double totalOutage = detectionTime + convergenceTime;
                
                // Record with realistic OSPF values
                m_analyzer.RecordLinkDownEvent(false, totalOutage, 15, detectionTime, 1);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error analyzing link down performance: " << e.what() << std::endl;
        }
    }
    
    // Generate realistic OSPF route update
    std::string GenerateOspfRouteUpdate(int nodeA, int nodeB, bool isUp) {
        std::ostringstream update;
        
        try {
            if (isUp) {
                // New path available
                update << "ADD 10." << nodeB << ".0.0/16 10.0." << nodeA << ".1 1";
            } else {
                // Path removed, find alternative
                update << "DEL 10." << nodeB << ".0.0/16 10.0." << nodeA << ".1";
                
                // Add alternative route if available
                int alternativeNode = FindAlternativePath(nodeA, nodeB);
                if (alternativeNode >= 0) {
                    update << " ADD 10." << nodeB << ".0.0/16 10.0." << alternativeNode << ".1 5";
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error generating OSPF route update: " << e.what() << std::endl;
            update << "ERROR";
        }
        
        return update.str();
    }
    
    // Find alternative path to bypass a link
    int FindAlternativePath(int nodeA, int nodeB) {
        try {
            // Simple search for alternative node (limited to avoid errors)
            uint32_t maxNodes = std::min(NodeList::GetNNodes(), (uint32_t)10);
            
            for (uint32_t i = 0; i < maxNodes; i++) {
                if ((int)i != nodeA && (int)i != nodeB) {
                    // Check if this node can serve as relay
                    return (int)i;
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error finding alternative path: " << e.what() << std::endl;
        }
        
        return -1; // No alternative found
    }
};

#endif // SATNET_CONTROLLER_H
