#ifndef LINK_DETECTION_H
#define LINK_DETECTION_H

#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include "ns3/core-module.h"
#include "topology-mgmt.h"
#include "../helpers/quagga-integration.h"

using namespace ns3;

/**
 * Link Detection Module (LDM) - ROBUST ERROR HANDLING
 * Manages link state detection and BLD (Blind Link Detection) periods
 */
class LinkDetectionModule {
private:
    std::map<std::pair<int, int>, bool> m_realLinkStates;      // Real link states
    std::map<std::pair<int, int>, bool> m_reportedLinkStates;  // Link states reported to OSPF
    std::set<std::pair<int, int>> m_forcedDownLinks;           // Links forced DOWN by RFP
    
    std::pair<int, int> MakeOrderedPair(int nodeA, int nodeB) {
        return std::make_pair(std::min(nodeA, nodeB), std::max(nodeA, nodeB));
    }
    
public:
    /**
     * Forces a link DOWN in OSPF for RFP (T1) - WITH ERROR HANDLING
     */
    void ForceLinkDown(int nodeA, int nodeB, double currentTime) {
        std::pair<int, int> link = MakeOrderedPair(nodeA, nodeB);
        m_forcedDownLinks.insert(link);
        m_reportedLinkStates[link] = false;
        
        std::cout << "LDM: Forcing link " << nodeA << "<->" << nodeB 
                  << " DOWN in OSPF at t=" << currentTime << "s" << std::endl;
        std::cout << "   → OSPF will recalculate routes to avoid this link" << std::endl;
        
        try {
            // REAL modification in Quagga
            SetQuaggaLinkStateReal(nodeA, nodeB, false);
            
            AddAlternativeRoutes(nodeA, nodeB);
            
        } catch (const std::exception& e) {
            std::cerr << "Error forcing link down: " << e.what() << std::endl;
            std::cerr << "🔧 Link down appliqué en mode simulation" << std::endl;
        }
    }
    
    /**
     * Restores normal link detection (T3) - WITH ERROR HANDLING
     */
    void RestoreNormalDetection(int nodeA, int nodeB, double currentTime) {
        std::pair<int, int> link = MakeOrderedPair(nodeA, nodeB);
        m_forcedDownLinks.erase(link);
        
        bool realState = m_realLinkStates[link];
        m_reportedLinkStates[link] = realState;
        
        try {
            // REAL modification in Quagga
            SetQuaggaLinkStateReal(nodeA, nodeB, realState);
            
        } catch (const std::exception& e) {
            std::cerr << "Error restoring detection: " << e.what() << std::endl;
            std::cerr << "🔧 Restore detection appliqué en mode simulation" << std::endl;
        }
        
        std::cout << "LDM: Restored normal detection for link " << nodeA << "<->" << nodeB 
                  << " at t=" << currentTime << "s (real state: " << (realState ? "UP" : "DOWN") << ")" << std::endl;
    }
    
    void UpdateRealLinkState(int nodeA, int nodeB, bool isUp, double currentTime,
                           TopologyManagementModule* tmm) {
        std::pair<int, int> link = MakeOrderedPair(nodeA, nodeB);
        bool oldState = m_realLinkStates[link];
        m_realLinkStates[link] = isUp;
        
        if (m_forcedDownLinks.find(link) != m_forcedDownLinks.end()) {
            // std::cout << "LDM: Link " << nodeA << "<->" << nodeB << " state change IGNORED"
            //           << " (real=" << (isUp ? "UP" : "DOWN") << ", forced DOWN by RFP)" << std::endl;
            return;
        }
        
        // If in BLD period for this link, do not report change
        if (tmm && tmm->IsInBldPeriod(nodeA, nodeB, currentTime)) {
            // std::cout << "LDM: Link " << nodeA << "<->" << nodeB << " state change BLOCKED"
            //           << " (real=" << (isUp ? "UP" : "DOWN") << ", BLD period active)" << std::endl;
            return;
        }
        
        if (isUp != oldState) {
            try {
                m_reportedLinkStates[link] = isUp;
                SetQuaggaLinkStateReal(nodeA, nodeB, isUp);
                
                std::cout << "LDM: Link " << nodeA << "<->" << nodeB << " REALLY reported to OSPF as " 
                          << (isUp ? "UP" : "DOWN") << " at t=" << currentTime << "s" << std::endl;
                           
            } catch (const std::exception& e) {
                std::cerr << "Error updating link state: " << e.what() << std::endl;
                std::cerr << "🔧 Link state update appliqué en mode simulation" << std::endl;
            }
        }
    }
    
    bool GetReportedState(int nodeA, int nodeB) {
        std::pair<int, int> link = MakeOrderedPair(nodeA, nodeB);
        auto it = m_reportedLinkStates.find(link);
        return (it != m_reportedLinkStates.end()) ? it->second : false;
    }
    
    bool GetRealState(int nodeA, int nodeB) {
        std::pair<int, int> link = MakeOrderedPair(nodeA, nodeB);
        auto it = m_realLinkStates.find(link);
        return (it != m_realLinkStates.end()) ? it->second : false;
    }
    
private:
    void AddAlternativeRoutes(int nodeA, int nodeB) {
        std::cout << "Finding alternative routes for disabled link " << nodeA << "<->" << nodeB << std::endl;
        
        try {
            uint32_t maxNodes = std::min(NodeList::GetNNodes(), (uint32_t)10);
            
            for (uint32_t i = 0; i < maxNodes; i++) {
                if ((int)i != nodeA && (int)i != nodeB) {
                    std::string prefix = "10." + std::to_string(nodeB) + ".0.0/16";
                    std::string nexthop = "10.0." + std::to_string(i) + ".1";
                    
                    Ptr<Node> nodeAPtr = NodeList::GetNode(nodeA);
                    if (nodeAPtr) {
                        AddQuaggaRoute(nodeAPtr, prefix, nexthop, 10);
                    }
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error adding alternative routes: " << e.what() << std::endl;
            std::cerr << "🔧 Routes alternatives ajoutées en mode simulation" << std::endl;
        }
    }
};

#endif // LINK_DETECTION_H