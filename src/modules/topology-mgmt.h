#ifndef TOPOLOGY_MGMT_H
#define TOPOLOGY_MGMT_H

#include <iostream>
#include <vector>
#include "ns3/core-module.h"
#include "../core/constellation-params.h"

using namespace ns3;

/**
 * Predictable Link Down Event (PLD_i)
 * PLD_i(X, A_i, B_i, T_0^i, T_1^i, T_2^i, T_3^i)
 */
struct PredictableLinkDownEvent {
    int linkId;           // X - link identifier
    int nodeA;            // A_i - node A of the link
    int nodeB;            // B_i - node B of the link
    double T0;            // T_0^i - time of actual physical failure
    double T1;            // T_1^i = T_0^i - T_c - 2*dT (start BLD/BFU)
    double T2;            // T_2^i = T_0^i - dT (end BFU, sync tables)
    double T3;            // T_3^i = T_0^i + dT (end BLD)
    bool active;          // Event scheduled and active?
    
    PredictableLinkDownEvent() : linkId(-1), nodeA(-1), nodeB(-1), T0(0), T1(0), T2(0), T3(0), active(false) {}
    
    PredictableLinkDownEvent(int lid, int a, int b, double t0) 
        : linkId(lid), nodeA(a), nodeB(b), T0(t0), active(true) {
        T1 = T0 - RFP_CONVERGENCE_TIME_TC - 2 * RFP_SAFETY_MARGIN_DT;
        T2 = T0 - RFP_SAFETY_MARGIN_DT;
        T3 = T0 + RFP_SAFETY_MARGIN_DT;
        
        // Ensure T1 > 0
        if (T1 < 0) {
            T1 = 0.1;
            T2 = T1 + RFP_CONVERGENCE_TIME_TC + RFP_SAFETY_MARGIN_DT;
            T3 = T2 + 2 * RFP_SAFETY_MARGIN_DT;
            T0 = T3 - RFP_SAFETY_MARGIN_DT;
        }
    }
};

/**
 * Topology Management Module (TMM)
 * Manages topological model and extracts predictable events
 */
class TopologyManagementModule {
private:
    std::vector<PredictableLinkDownEvent> m_predictedEvents;
    
public:
    void AddPredictableLinkDown(int linkId, int nodeA, int nodeB, double eventTime) {
        PredictableLinkDownEvent event(linkId, nodeA, nodeB, eventTime);
        m_predictedEvents.push_back(event);
        
        std::cout << "TMM: Predicted link-down event scheduled for link " << linkId << std::endl;
        std::cout << "   Link: " << nodeA << "<->" << nodeB << std::endl;
        std::cout << "   T0 (actual failure): " << event.T0 << "s" << std::endl;
        std::cout << "   T1 (start BLD/BFU): " << event.T1 << "s" << std::endl;
        std::cout << "   T2 (sync forwarding): " << event.T2 << "s" << std::endl;
        std::cout << "   T3 (end BLD): " << event.T3 << "s" << std::endl;
    }
    
    const std::vector<PredictableLinkDownEvent>& GetPredictedEvents() const {
        return m_predictedEvents;
    }
    
    std::vector<PredictableLinkDownEvent> GetActiveEvents(double currentTime) const {
        std::vector<PredictableLinkDownEvent> activeEvents;
        for (const auto& event : m_predictedEvents) {
            if (event.active && currentTime >= event.T1 && currentTime <= event.T3) {
                activeEvents.push_back(event);
            }
        }
        return activeEvents;
    }
    
    bool IsInBldPeriod(int nodeA, int nodeB, double currentTime) const {
        for (const auto& event : m_predictedEvents) {
            if (event.active && 
                ((event.nodeA == nodeA && event.nodeB == nodeB) ||
                 (event.nodeA == nodeB && event.nodeA == nodeA)) &&
                currentTime >= event.T1 && currentTime <= event.T3) {
                return true;
            }
        }
        return false;
    }
    
    bool IsInBfuPeriod(double currentTime) const {
        for (const auto& event : m_predictedEvents) {
            if (event.active && 
                currentTime >= event.T1 && currentTime <= event.T2) {
                return true;
            }
        }
        return false;
    }
};

#endif // TOPOLOGY_MGMT_H