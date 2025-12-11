#include <iostream>
#ifndef ANIMATION_HELPER_H
#define ANIMATION_HELPER_H

#include "ns3/netanim-module.h"

using namespace ns3;

// Simple wrapper around AnimationInterface for consistency
class AnimationHelper {
public:
    AnimationHelper(std::string filename) {
        m_anim = new AnimationInterface(filename);
        m_anim->SetMaxPktsPerTraceFile(500000);
        
        m_anim->SetMobilityPollInterval(Seconds(0.1));
    }
    
    ~AnimationHelper() {
        if (m_anim) {
            delete m_anim;
            m_anim = nullptr;
        }
    }
    
    AnimationInterface* GetAnim() {
        return m_anim;
    }
    
    void ConfigureEarth(Ptr<Node> earthNode) {
        m_anim->UpdateNodeColor(earthNode, 0, 100, 0); 
        m_anim->UpdateNodeSize(earthNode->GetId(), 200.0, 200.0); 
        m_anim->UpdateNodeDescription(earthNode, "EARTH");
        
        m_anim->SetConstantPosition(earthNode, 600.0, 400.0);
    }
    
    void ConfigureSatellites(NodeContainer satellites) {
        for (uint32_t i = 0; i < satellites.GetN(); ++i) {
            Ptr<Node> node = satellites.Get(i);
            m_anim->UpdateNodeColor(node, 0, 0, 255); 
            m_anim->UpdateNodeSize(node->GetId(), 10.0, 10.0);
            m_anim->UpdateNodeDescription(node, "SAT-" + std::to_string(i));
        }
    }
    
    void ConfigureGroundStations(NodeContainer stations) {
        for (uint32_t i = 0; i < stations.GetN(); ++i) {
            Ptr<Node> node = stations.Get(i);
            m_anim->UpdateNodeColor(node, 255, 0, 0);
            m_anim->UpdateNodeSize(node->GetId(), 15.0, 15.0);
            m_anim->UpdateNodeDescription(node, "GS-" + std::to_string(i));
        }
    }
    
private:
    AnimationInterface* m_anim;
};

#endif // ANIMATION_HELPER_H
