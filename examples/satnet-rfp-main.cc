/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SATNET-OSPF avec mécanisme RFP complet intégré + VRAIE INTERFACE QUAGGA
 * Simulation constellation satellite avec DCE + Quagga OSPF + RFP
 */

#include <iostream>
#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/dce-module.h"
#include "ns3/quagga-helper.h"

#include "core/constellation-params.h"
#include "core/constellation.h"
#include "helpers/quagga-integration.h"
#include "helpers/satellite-helper.h"
#include "helpers/animation-helper.h"
#include "applications/satnet-controller.h"
#include "applications/traffic-generator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SatnetDceQuaggaRfpConstellation");

// Variables globales
SatnetOspfController* g_rfpController = nullptr;
SatelliteHelper* g_satHelper = nullptr;
AnimationHelper* g_animHelper = nullptr;

// Callbacks
void CreatePredictableLinkEvents() {
    try {
        if (!g_rfpController) return;
        
        NS_LOG_INFO("========== CREATING PREDICTABLE LINK EVENTS ==========");
        
        uint32_t eventCount = 0;
        double currentTime = 10.0;
        
        uint32_t totalNodes = NodeList::GetNNodes();
        uint32_t maxSatellites = std::min(totalNodes, (uint32_t)30);
        
        if (maxSatellites < 2) {
            NS_LOG_ERROR("Not enough nodes to create links");
            return;
        }
        
        for (uint32_t i = 0; i < 6 && eventCount < 6; i++) {
            uint32_t nodeA = i % maxSatellites;
            uint32_t nodeB = (i + 1) % maxSatellites;
            
            if (nodeA == nodeB) {
                nodeB = (nodeB + 1) % maxSatellites;
            }
            
            if (!ValidateNodeIndices(nodeA, nodeB)) {
                continue;
            }
            
            double linkDownTime = currentTime + (eventCount + 1) * 8.0;
            
            if (linkDownTime < SIM_STOP - 15.0) {
                g_rfpController->SchedulePredictableLinkDown(eventCount + 1, nodeA, nodeB, linkDownTime);
                eventCount++;
            }
        }
        
        NS_LOG_INFO("📅 Successfully scheduled " << eventCount << " predictable link-down events");
        
    } catch (const std::exception& e) {
        NS_LOG_ERROR("Error creating predictable link events: " << e.what());
    }
}

static void GlobalSatPosUpdate(double time) {
    try {
        if (!g_satHelper) return;
        
        uint32_t totalNodes = NodeList::GetNNodes();
        uint32_t theoreticalSatellites = NUM_PLANES * SATS_PER_PLANE;
        uint32_t maxSats = std::min(theoreticalSatellites, (uint32_t)25);
        
        if (maxSats == 0) return;
        
        NodeContainer satellites;
        for (uint32_t i = 0; i < maxSats; i++) {
            if (i < totalNodes) {
                Ptr<Node> node = NodeList::GetNode(i);
                if (node) satellites.Add(node);
            }
        }
        
        if (satellites.GetN() > 0) {
            g_satHelper->UpdatePositions(satellites, time);
        }
        
    } catch (const std::exception& e) {
        NS_LOG_ERROR("Error updating satellite positions: " << e.what());
    }
}

int main(int argc, char *argv[]) {
    try {
        LogComponentEnable("SatnetDceQuaggaRfpConstellation", LOG_LEVEL_INFO);
        
        SetupDceEnvironmentSafe(); // DCE enabled
        
        double simTime = SIM_STOP;
        std::string animFile = "satnet-ospf-rfp-real-quagga.xml";
        
        CommandLine cmd(__FILE__);
        cmd.AddValue("simTime", "Simulation time", simTime);
        cmd.AddValue("animFile", "File name for animation output", animFile);
        cmd.Parse(argc, argv);
        
        g_rfpController = new SatnetOspfController();
        g_satHelper = new SatelliteHelper();
        
        uint32_t theoreticalSatellites = NUM_PLANES * SATS_PER_PLANE;
        uint32_t numSatellites = std::min(theoreticalSatellites, (uint32_t)25);
        
        NodeContainer satellites;
        satellites.Create(numSatellites);
        
        NodeContainer groundStations;
        groundStations.Create(GROUND_STATIONS.size());
        
        // Create a node to visualize Earth
        NodeContainer earthNodeContainer;
        earthNodeContainer.Create(1);
        Ptr<Node> earthNode = earthNodeContainer.Get(0);
        
        // DCE Manager enabled
        
        DceManagerHelper dceManager;
        dceManager.SetTaskManagerAttribute("FiberManagerType", StringValue("UcontextFiberManager"));
        dceManager.SetNetworkStack("ns3::Ns3SocketFdFactory");
        dceManager.Install(satellites);
        dceManager.Install(groundStations);
        
        
        InternetStackHelper internet;
        Ipv4DceRoutingHelper ipv4DceRouting;
        internet.SetRoutingHelper(ipv4DceRouting);
        internet.Install(satellites);
        internet.Install(groundStations);
        
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(satellites);
        mobility.Install(groundStations);
        mobility.Install(earthNodeContainer);
        
        g_satHelper->UpdatePositions(satellites, 0.0);
        std::cout << "DEBUG: Positions updated" << std::endl;
        
        g_animHelper = new AnimationHelper(animFile);
        g_animHelper->ConfigureEarth(earthNode);
        g_animHelper->ConfigureSatellites(satellites);
        g_animHelper->ConfigureGroundStations(groundStations);
        
        // Enable packet tracing to visualize flows
        g_animHelper->GetAnim()->EnablePacketMetadata(true);
        
        Ipv4AddressHelper ipv4;
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue(P2P_RATE));
        p2p.SetChannelAttribute("Delay", StringValue(SATELLITE_DELAY));
        
        uint32_t maxLinks = std::min(8U, numSatellites - 1);
        
        std::cout << "DEBUG: Creating links..." << std::endl;
        if (numSatellites >= 2) {
            for (uint32_t i = 0; i < maxLinks && (i + 1) < numSatellites; i++) {
                if (i < satellites.GetN() && (i + 1) < satellites.GetN()) {
                    NetDeviceContainer link = p2p.Install(satellites.Get(i), satellites.Get(i + 1));
                    std::string subnet = "10.0." + std::to_string(i + 1) + ".0";
                    ipv4.SetBase(subnet.c_str(), "255.255.255.0");
                    ipv4.Assign(link);
                }
            }
        }
        std::cout << "DEBUG: Links created" << std::endl;
        
        
        QuaggaHelper quagga;
        std::cout << "DEBUG: QuaggaHelper created" << std::endl;
        
        uint32_t maxQuaggaNodes = std::min(5U, numSatellites);
        for (uint32_t i = 0; i < maxQuaggaNodes; i++) {
            std::cout << "DEBUG: Installing Quagga on satellite " << i << std::endl;
            quagga.EnableOspf(satellites.Get(i), "10.0.0.0/8");
            quagga.Install(satellites.Get(i));
        }
        
        for (uint32_t i = 0; i < groundStations.GetN(); i++) {
            std::cout << "DEBUG: Installing Quagga on ground station " << i << std::endl;
            quagga.EnableOspf(groundStations.Get(i), "192.168.0.0/16");
            quagga.Install(groundStations.Get(i));
        }
        std::cout << "DEBUG: Quagga installed" << std::endl;
        
        
        // Use standard static routing or OLSR as fallback if needed, but for now just basic stack
        // Ipv4GlobalRoutingHelper::PopulateRoutingTables();
        
        TrafficGenerator::Install(groundStations, UDP_PORT, SIM_START, SIM_STOP);
        
        Simulator::Schedule(Seconds(2.0), &CreatePredictableLinkEvents);
        
        // Frequent update for smooth animation (0.1s)
        for (double t = 0.0; t <= simTime; t += 0.1) {
            Simulator::Schedule(Seconds(t), &GlobalSatPosUpdate, t);
        }
        
        Simulator::Stop(Seconds(simTime));
        Simulator::Run();
        
        if (g_rfpController) {
            g_rfpController->PrintFinalStatistics();
        }
        
        Simulator::Destroy();
        
        delete g_rfpController;
        delete g_satHelper;
        delete g_animHelper;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}