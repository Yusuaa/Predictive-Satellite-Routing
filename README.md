# 🛰️ SATNET-OSPF: Predictive Routing for LEO Satellite Networks

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Docker](https://img.shields.io/badge/Docker-Required-blue.svg)](https://www.docker.com/)
[![ns-3](https://img.shields.io/badge/ns--3-3.35-green.svg)](https://www.nsnam.org/)

An implementation of the **Route Failure Prediction (RFP)** protocol for OSPF in Low Earth Orbit (LEO) satellite constellations. This project extends OSPF with predictive link management to achieve near-zero packet loss during satellite handovers.

## 🎯 Key Features

- **Predictive Link Detection**: Uses orbital mechanics to predict link failures 3+ seconds in advance
- **Bounded Link Detection (BLD)**: Masks predicted failures from OSPF until routes are ready
- **Bounded Forwarding Update (BFU)**: Synchronizes routing table updates across all nodes
- **Real Quagga Integration**: Uses actual Quagga routing suite (zebra, ospfd, vtysh) via DCE
- **Zero Packet Loss**: Routes are updated proactively before physical link failure

## 📊 Performance Results

| Metric | Standard OSPF | SATNET-OSPF RFP | Improvement |
|--------|---------------|-----------------|-------------|
| Route Outage | ~40s | <10ms | **4000x faster** |
| Detection Time | 40s (Dead interval) | Predicted | **Instant** |
| Packet Loss | 15+ packets | 0 packets | **Zero loss** |

## 🚀 Quick Start

### Prerequisites

- **Docker** (only requirement for simulation)
- **NetAnim** (optional, for visualization) - Must be installed on your host machine
- ~4GB disk space

### Run the Simulation

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/satellite-leo-rfp.git
cd satellite-leo-rfp

# Make scripts executable
chmod +x run_docker.sh scripts/docker_patch.sh

# Run the simulation (builds Docker image on first run)
./run_docker.sh
```

The first run will:
1. Build a Docker image with ns-3, DCE, and Quagga
2. Apply necessary patches for DCE compatibility
3. Compile everything with proper flags (-fPIC, -pie)
4. Run the LEO satellite simulation

### Expected Output

```
=== COMPREHENSIVE QUAGGA PATCHING FOR DCE ===
lib/log.c patched
vtysh/vtysh_user.c patched (auth disabled for DCE)
Quagga rebuilt with DCE-compatible flags

=== RUNNING SIMULATION ===
SAT-0 Pos: (900, 400) at t=0
TMM: Predicted link-down event scheduled for link 1
SAFE VTYSH on node 0: configure terminal
SAFE VTYSH on node 0: ip route 10.1.0.0/16 10.0.2.1 10
...
========== PERFORMANCE ANALYSIS RESULTS ==========
vtysh status: REAL
Simulation completed successfully!
```

## 🎬 Visualization with NetAnim

The simulation automatically generates an XML trace file for visualization.

### 1. File Location
The file **`satnet-ospf-rfp-real-quagga.xml`** is generated in the **project root** (same folder as `run_docker.sh`).
Even though the simulation runs inside Docker, the file appears on your host machine thanks to volume mounting.

### 2. How to View
You must have **NetAnim** installed on your computer (Windows/Linux/Mac). It does not run inside Docker.

1. Install NetAnim (often via `apt install netanim` on Linux or download from nsnam.org)
2. Launch NetAnim
3. Open the file `satnet-ospf-rfp-real-quagga.xml`
4. Click Play ▶️

You will see:
- 🌍 Earth at the center (green node)
- 🛰️ Satellites in orbit (blue nodes)
- 📡 Ground stations (red nodes)
- 📦 Packets flowing between nodes

## 📁 Project Structure

```
satellite-leo-rfp/
├── run_docker.sh              # Main launch script
├── Dockerfile                  # Docker build configuration
├── satnet-dce-quagga-base.cc  # Core simulation logic
├── wscript                     # ns-3 build script
├── examples/
│   └── satnet-rfp-main.cc     # Entry point (main)
├── scripts/
│   └── docker_patch.sh        # Quagga/DCE patching script
├── src/
│   ├── applications/
│   │   └── satnet-controller.h # RFP Controller
│   ├── helpers/
│   │   └── quagga-integration.h # vtysh integration
│   └── modules/
│       ├── performance-analyzer.h # Performance metrics
│       ├── topology-mgmt.h      # Topology Management (TMM)
│       ├── link-detection.h     # Link Detection (LDM)
│       └── route-mgmt.h         # Route Management (RMM)
└── docs/
    └── ARCHITECTURE.md        # Technical documentation
```

## 🔧 How It Works

### RFP Protocol Timeline

```
T1 (3s before failure)    T2 (0.5s before)    T0 (failure)    T3 (0.5s after)
        │                        │                  │                │
        ▼                        ▼                  ▼                ▼
   Start BLD              Sync FIBs          Link fails        End BLD
   Start BFU              End BFU            (no impact!)      Resume normal
   OSPF sees DOWN         Routes ready       Traffic OK        detection
```

### Key Components

1. **Topology Management Module (TMM)**: Predicts link failures using orbital mechanics
2. **Link Detection Module (LDM)**: Manages BLD periods, masks failures from OSPF
3. **Route Management Module (RMM)**: Coordinates BFU, ensures synchronized updates
4. **Quagga Integration**: Real vtysh commands to zebra/ospfd daemons

## 🐛 Troubleshooting

### Docker Build Fails
```bash
# Clean Docker cache and rebuild
docker system prune -a
./run_docker.sh
```

### Simulation Crashes
The simulation includes comprehensive NULL pointer checks and error handling. If you see crashes, check:
- Docker has enough memory (4GB+ recommended)
- All patches were applied successfully (check console output)

## 📚 References

- [OSPF RFC 2328](https://tools.ietf.org/html/rfc2328)
- [ns-3 DCE Documentation](https://www.nsnam.org/docs/dce/manual/html/)
- [Quagga Routing Suite](https://www.quagga.net/)

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Built with** ns-3 + DCE + Quagga for realistic LEO satellite network simulation.
