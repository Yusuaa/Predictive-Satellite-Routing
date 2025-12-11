#include <iostream>
#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

class TrafficGenerator {
public:
    static void Install(NodeContainer nodes, uint16_t port, double startTime, double stopTime) {
        if (nodes.GetN() < 2) return;
        
        UdpEchoServerHelper echoServer(port);
        ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
        serverApps.Start(Seconds(startTime));
        serverApps.Stop(Seconds(stopTime));
        
        UdpEchoClientHelper echoClient(Ipv4Address("192.168.1.1"), port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(10));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));
        
        ApplicationContainer clientApps = echoClient.Install(nodes.Get(1));
        clientApps.Start(Seconds(startTime + 5.0));
        clientApps.Stop(Seconds(stopTime));
    }
};

#endif 
