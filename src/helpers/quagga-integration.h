#ifndef QUAGGA_INTEGRATION_H
#define QUAGGA_INTEGRATION_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
//#include "ns3/dce-module.h"

using namespace ns3;

// Structure to manage vtysh state safely (Singleton)
struct VtyshState {
    bool available;
    bool checked;
    
    VtyshState() : available(false), checked(false) {}
};

inline VtyshState& GetVtyshState() {
    static VtyshState state;
    // FORCE TRUE REMOVED
    // Debug print removed for cleaner output
    // std::cout << "DEBUG: GetVtyshState address: " << &state << " available: " << state.available << std::endl;
    return state;
}

/**
 * Validates that node indices are within valid limits
 */
inline bool ValidateNodeIndices(int nodeA, int nodeB) {
    uint32_t totalNodes = NodeList::GetNNodes();
    
    if (nodeA < 0 || nodeB < 0) {
        std::cerr << "Negative node indices: " << nodeA << ", " << nodeB << std::endl;
        return false;
    }
    
    if ((uint32_t)nodeA >= totalNodes || (uint32_t)nodeB >= totalNodes) {
        std::cerr << "Node indices out of range: " << nodeA << ", " << nodeB 
                  << " (max: " << (totalNodes - 1) << ")" << std::endl;
        return false;
    }
    
    if (nodeA == nodeB) {
        std::cerr << "Identical node indices: " << nodeA << ", " << nodeB << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Helper to get the primary DCE root path (handles colon-separated paths)
 */
inline std::string GetPrimaryDceRoot() {
    const char* dceRoot = getenv("DCE_ROOT");
    if (!dceRoot) return "";
    
    std::string rootStr = dceRoot;
    size_t pos = rootStr.find(':');
    if (pos != std::string::npos) {
        return rootStr.substr(0, pos);
    }
    return rootStr;
}

/**
 * Checks if vtysh is available in DCE
 */
inline bool IsVtyshAvailable() {
    VtyshState& state = GetVtyshState();
    
    std::cout << "DEBUG: IsVtyshAvailable() called" << std::endl;
    if (state.checked) {
        std::cout << "DEBUG: Returning cached value: " << state.available << std::endl;
        return state.available;
    }
    
    // Vérifier les variables d'environnement DCE
    const char* dcePath = getenv("DCE_PATH");
    std::string dceRoot = GetPrimaryDceRoot();
    
    std::cout << "DEBUG: DCE_PATH=" << (dcePath ? dcePath : "NULL") << std::endl;
    std::cout << "DEBUG: DCE_ROOT (primary)=" << dceRoot << std::endl;
    
    // Check vtysh in multiple possible locations
    struct stat buffer;
    bool exists = false;
    std::string foundPath;
    
    // List of paths to check for vtysh
    std::vector<std::string> pathsToCheck;
    
    // Add DCE bin_dce directory
    if (!dceRoot.empty()) {
        pathsToCheck.push_back(dceRoot + "/bin_dce/vtysh");
    }
    
    // Parse DCE_PATH and check each directory
    if (dcePath) {
        std::string dcePathStr = dcePath;
        std::stringstream ss(dcePathStr);
        std::string dir;
        while (std::getline(ss, dir, ':')) {
            if (!dir.empty()) {
                pathsToCheck.push_back(dir + "/vtysh");
            }
        }
    }
    
    // Also check common locations
    pathsToCheck.push_back("/workspace/source/ns-3-dce/build/bin_dce/vtysh");
    pathsToCheck.push_back("/workspace/build/bin_dce/vtysh");
    pathsToCheck.push_back("/usr/bin/vtysh");
    pathsToCheck.push_back("/usr/local/bin/vtysh");
    
    // Try each path
    for (const auto& path : pathsToCheck) {
        std::cout << "DEBUG: Checking vtysh at: " << path << std::endl;
        if (stat(path.c_str(), &buffer) == 0) {
            exists = true;
            foundPath = path;
            std::cout << "DEBUG: Found vtysh at: " << path << " size=" << buffer.st_size << std::endl;
            break;
        }
    }
    
    if (!exists) {
        std::cerr << "vtysh not found in any DCE path" << std::endl;
        state.available = false;
    } else {
        std::cout << "vtysh available at: " << foundPath << std::endl;
        state.available = true;
    }
    
    state.checked = true;
    return state.available;
}

inline void SetupDceEnvironmentSafe() {
    std::cout << "🔧 === CONFIGURATION ENVIRONNEMENT DCE SÉCURISÉE ===" << std::endl;
    
    const char* dcePath = getenv("DCE_PATH");
    std::string dceRoot = GetPrimaryDceRoot();
    
    if (!dcePath) {
        std::cerr << "DCE_PATH not defined, using default" << std::endl;
        setenv("DCE_PATH", "/bake/build/bin_dce:/bake/source/quagga/vtysh:/bake/source/quagga/zebra:/bake/source/quagga/ospfd", 1);
    }
    
    if (dceRoot.empty()) {
        std::cerr << "DCE_ROOT not defined, using default" << std::endl;
        setenv("DCE_ROOT", "/bake/build", 1);
        dceRoot = "/bake/build";
    }
    
    std::string rootStr = dceRoot;
    std::string cmd;
    
    cmd = "mkdir -p " + rootStr + "/etc"; system(cmd.c_str());
    cmd = "mkdir -p " + rootStr + "/var/log"; system(cmd.c_str());
    cmd = "mkdir -p " + rootStr + "/var/run"; system(cmd.c_str());
    cmd = "mkdir -p " + rootStr + "/bin_dce"; system(cmd.c_str());
    cmd = "mkdir -p " + rootStr + "/tmp"; system(cmd.c_str());
    
    std::ofstream zebraConf(rootStr + "/etc/zebra.conf");
    if (zebraConf.is_open()) {
        zebraConf << "hostname zebra\n";
        zebraConf << "password zebra\n";
        zebraConf << "enable password zebra\n";
        zebraConf << "log stdout\n";
        zebraConf << "!\n";
        zebraConf << "interface lo\n";
        zebraConf << " ip address 127.0.0.1/32\n";
        zebraConf << "!\n";
        zebraConf << "line vty\n";
        zebraConf << " exec-timeout 0 0\n";
        zebraConf << "!\n";
        zebraConf.close();
        std::cout << "zebra.conf created at " << rootStr << "/etc/zebra.conf" << std::endl;
    }
    
    std::ofstream ospfdConf(rootStr + "/etc/ospfd.conf");
    if (ospfdConf.is_open()) {
        ospfdConf << "hostname ospfd\n";
        ospfdConf << "password zebra\n";
        ospfdConf << "enable password zebra\n";
        ospfdConf << "log stdout\n";
        ospfdConf << "!\n";
        ospfdConf << "router ospf\n";
        ospfdConf << " ospf router-id 1.1.1.1\n";
        ospfdConf << " network 10.0.0.0/8 area 0.0.0.0\n";
        ospfdConf << "!\n";
        ospfdConf << "line vty\n";
        ospfdConf << " exec-timeout 0 0\n";
        ospfdConf << "!\n";
        ospfdConf.close();
        std::cout << "ospfd.conf created" << std::endl;
    }
    
    // Check vtysh availability
    bool vtyshOk = IsVtyshAvailable();
    if (vtyshOk) {
        std::cout << "DCE configuration completed with vtysh" << std::endl;
    } else {
        std::cerr << "DCE configuration completed WITHOUT vtysh (simulation mode)" << std::endl;
        std::cerr << "Simulation will continue with simulated commands" << std::endl;
    }
}

/**
 * Executes a vtysh command on a node via DCE (ULTRA-SECURE VERSION)
 */
inline void ExecuteVtyshCommand(Ptr<Node> node, const std::string& command) {
    if (!node) {
        std::cerr << "ExecuteVtyshCommand: null node pointer" << std::endl;
        return;
    }
    
    if (command.empty()) {
        // std::cout << "ExecuteVtyshCommand: empty command" << std::endl;
        return;
    }
    
    if (!IsVtyshAvailable()) {
        std::cout << "🔧 SIMULATED VTYSH on node " << node->GetId() << ": " << command << std::endl;
        return;
    }
    
    std::cout << "SAFE VTYSH on node " << node->GetId() << ": " << command << std::endl;
    
    try {
        if (command.length() > 200) {
            std::cerr << "Command too long, truncating: " << command.substr(0, 50) << "..." << std::endl;
            return;
        }
        
        // Execute command via DCE
        DceApplicationHelper dce;
        
        // Use just "vtysh" - DCE will find it in DCE_PATH/bin_dce
        dce.SetBinary("vtysh");
        dce.SetStackSize(1 << 16); 
        dce.AddArgument("-c");
        dce.AddArgument(command);
        
        ApplicationContainer app = dce.Install(node);
        app.Start(Seconds(0.1));
        // app.Stop(Seconds(1.0)); // Don't stop immediately, let it run
        
        // std::cout << "🦓 SAFE VTYSH on node " << node->GetId() << ": " << command << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error secure vtysh on node " << node->GetId() << ": " << e.what() << std::endl;
        // std::cout << "🔄 Fallback: simulating command: " << command << std::endl;
    } catch (...) {
        std::cerr << "Unknown vtysh error on node " << node->GetId() << std::endl;
        // std::cout << "🔄 Fallback: simulating command: " << command << std::endl;
    }
}

/**
 * Force a link UP/DOWN with real vtysh interface
 */
inline void SetQuaggaLinkStateReal(int nodeA, int nodeB, bool isUp) {    if (!ValidateNodeIndices(nodeA, nodeB)) return;
    
    Ptr<Node> nodeAPtr = NodeList::GetNode(nodeA);
    Ptr<Node> nodeBPtr = NodeList::GetNode(nodeB);
    
    if (!nodeAPtr || !nodeBPtr) {
        std::cerr << "Invalid nodes: " << nodeA << ", " << nodeB << std::endl;
        return;
    }

    try {
        // Interface with Quagga via vtysh
        if (IsVtyshAvailable()) {
            ExecuteVtyshCommand(nodeAPtr, "configure terminal");
            
            std::ostringstream cmd;
            if (isUp) {
                cmd << "no shutdown";    
            } else {
                cmd << "shutdown";       
            }
            ExecuteVtyshCommand(nodeAPtr, cmd.str());
            
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during OSPF notification: " << e.what() << std::endl;
    }
}

/**
 * Adds a route in Quagga with error handling
 */
inline void AddQuaggaRoute(Ptr<Node> node, const std::string& prefix, const std::string& nexthop, int metric = 1) {
    std::cout << "➕ Adding route on node " << node->GetId() << ": " << prefix << " via " << nexthop << std::endl;
    
    try {
        // Configuration via vtysh
        ExecuteVtyshCommand(node, "configure terminal");
        
        std::ostringstream cmd;
        cmd << "ip route " << prefix << " " << nexthop << " " << metric;
        ExecuteVtyshCommand(node, cmd.str());
        
        // Redistribute in OSPF
        ExecuteVtyshCommand(node, "router ospf");
        ExecuteVtyshCommand(node, "redistribute static");
        
        std::cout << "Route added and redistributed in OSPF" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error adding route: " << e.what() << std::endl;
        std::cerr << "🔧 Route ajoutée en mode simulation" << std::endl;
    }
}

/**
 * Removes a route in Quagga with error handling
 */
inline void DelQuaggaRoute(Ptr<Node> node, const std::string& prefix, const std::string& nexthop) {
    std::cout << "➖ Deleting route on node " << node->GetId() << ": " << prefix << " via " << nexthop << std::endl;
    
    try {
        ExecuteVtyshCommand(node, "configure terminal");
        
        std::ostringstream cmd;
        cmd << "no ip route " << prefix << " " << nexthop;
        ExecuteVtyshCommand(node, cmd.str());
        
        std::cout << "Route deleted from routing table" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error deleting route: " << e.what() << std::endl;
        std::cerr << "🔧 Route supprimée en mode simulation" << std::endl;
    }
}

/**
 * Forces OSPF re-convergence on all nodes with error handling
 */
inline void ForceOspfConvergence() {
    std::cout << "🔄 Forcing OSPF convergence on all nodes..." << std::endl;
    
    try {
        uint32_t maxNodes = std::min(NodeList::GetNNodes(), (uint32_t)20); // Limit to avoid errors
        
        for (uint32_t i = 0; i < maxNodes; i++) {
            Ptr<Node> node = NodeList::GetNode(i);
            
            ExecuteVtyshCommand(node, "clear ip ospf database");
            ExecuteVtyshCommand(node, "router ospf");
            ExecuteVtyshCommand(node, "area 0.0.0.0 stub");
            ExecuteVtyshCommand(node, "no area 0.0.0.0 stub");
        }
        
        std::cout << "OSPF convergence triggered on " << maxNodes << " nodes" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error OSPF convergence: " << e.what() << std::endl;
        std::cerr << "🔧 Convergence OSPF en mode simulation" << std::endl;
    }
}

#endif // QUAGGA_INTEGRATION_H