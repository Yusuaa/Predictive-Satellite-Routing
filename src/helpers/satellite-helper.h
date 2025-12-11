#ifndef SATELLITE_HELPER_H
#define SATELLITE_HELPER_H

#include <iostream>
#include <vector>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "../core/constellation-params.h"

using namespace ns3;

class SatelliteHelper {
public:
    SatelliteHelper() {}

    double DegToRad(double deg) { return deg * PI / 180.0; }

    struct SatellitePosition {
        double angle;          
        double normalizedPos;  
        Vector realPos;        
        Vector displayPos;     
    };
    
    std::vector<SatellitePosition> m_currentPositions;
    
    void UpdatePositions(NodeContainer satellites, double time) {
        try {
            if (satellites.GetN() == 0) return;
            
            uint32_t actualSatellites = satellites.GetN();
            if (m_currentPositions.empty() || m_currentPositions.size() < actualSatellites) {
                m_currentPositions.resize(actualSatellites);
            }
            
            double animTime = time * ANIMATION_SPEED_FACTOR;
            uint32_t satIndex = 0;
            
            double earthCenterX = 600.0;      
            double earthCenterY = 400.0;      
            double orbitScaleFactor = 2.0;    
            
            uint32_t effectivePlanes = std::min((uint32_t)NUM_PLANES, actualSatellites);
            
            for (uint32_t i = 0; i < actualSatellites; i++) {
                try {
                    Ptr<Node> satelliteNode = satellites.Get(i);
                    if (!satelliteNode) continue;
                    
                    Ptr<MobilityModel> mobility = satelliteNode->GetObject<MobilityModel>();
                    if (!mobility) continue;

                    // Round-Robin distribution in planes
                    uint32_t plane = i % effectivePlanes;
                    uint32_t satOrder = i / effectivePlanes;
                    
                    // Calculate number of satellites in this plan for spacing
                    uint32_t satsInThisPlane = actualSatellites / effectivePlanes;
                    if (plane < actualSatellites % effectivePlanes) {
                        satsInThisPlane++;
                    }
                    
                    // Espacement dynamique (360 / N)
                    double dynamicPhaseDiff = 360.0 / satsInThisPlane;
                    
                    // RAAN (Right Ascension of Ascending Node)
                    double raan = plane * (PI / effectivePlanes); 
                    
                    double inclination = DegToRad(INCLINATION_DEG[plane % 6]);
                    
                    double orbitRadius = 150.0; 
                    
                    double satPhase = DegToRad(satOrder * dynamicPhaseDiff);
                    double theta = 2 * PI * (animTime / ORBIT_PERIOD) + satPhase;
                    
                    double x_orb = orbitRadius * cos(theta);
                    double y_orb = orbitRadius * sin(theta);
                    
                    double x3d = x_orb * cos(raan) - y_orb * cos(inclination) * sin(raan);
                    double y3d = x_orb * sin(raan) + y_orb * cos(inclination) * cos(raan);
                    double z3d = y_orb * sin(inclination);
                    
                    double displayX = earthCenterX + x3d * orbitScaleFactor;
                    double displayY = earthCenterY + (y3d * 0.3 - z3d) * orbitScaleFactor;
                    
                    if (i < m_currentPositions.size()) {
                        m_currentPositions[i].angle = theta;
                        m_currentPositions[i].normalizedPos = fmod(theta, 2 * PI) / (2 * PI);
                        m_currentPositions[i].displayPos = Vector(displayX, displayY, 0);
                    }
                    
                    mobility->SetPosition(Vector(displayX, displayY, 0));
                    
                    if (i == 0 && fmod(time, 1.0) < 0.1) {
                        std::cout << "SAT-0 Pos: (" << displayX << ", " << displayY << ") at t=" << time << std::endl;
                    }
                    
                } catch (...) {}
            }
        } catch (...) {}
    }
    
    bool IsSatelliteVisible(uint32_t satA, uint32_t satB, bool isInterPlane) {
        try {
            if (m_currentPositions.empty() || satA >= m_currentPositions.size() || satB >= m_currentPositions.size()) {
                return false;
            }
            
            uint32_t planeA = satA / SATS_PER_PLANE;
            uint32_t planeB = satB / SATS_PER_PLANE;
            
            if (planeA == planeB) {
                return true;
            }
            
            const SatellitePosition& posA = m_currentPositions[satA];
            const SatellitePosition& posB = m_currentPositions[satB];
            
            double posDiff = std::abs(posA.normalizedPos - posB.normalizedPos);
            
            if (posDiff > 0.5) {
                posDiff = 1.0 - posDiff;
            }
            
            return posDiff < LINK_VISIBILITY_THRESHOLD;
            
        } catch (const std::exception& e) {
            std::cerr << "Error is satellite visible: " << e.what() << std::endl;
            return false;
        }
    }
};

#endif // SATELLITE_HELPER_H
