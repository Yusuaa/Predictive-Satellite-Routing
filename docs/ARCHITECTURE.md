# SATNET RFP Architecture Documentation

## Overview

The SATNET RFP (Routing and Forwarding for Predictable link-down events) project implements a sophisticated satellite network simulation with proactive failure management. This document describes the system architecture, component interactions, and design decisions.

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    SATNET RFP System                        │
├─────────────────┬─────────────────┬─────────────────────────┤
│   Application   │    RFP Core     │      Infrastructure     │
│     Layer       │     Modules     │         Layer          │
├─────────────────┼─────────────────┼─────────────────────────┤
│ • Main Sim      │ • TMM          │ • NS-3 Core            │
│ • Traffic Gen   │ • LDM          │ • DCE Integration       │
│ • Controller    │ • RMM          │ • Quagga OSPF           │
│ • Analyzer      │ • Perf Analysis │ • NetAnim Visualization │
└─────────────────┴─────────────────┴─────────────────────────┘
```

### Component Hierarchy

```
SatnetRfpSimulation (Main Application)
├── SatnetOspfController (RFP Coordinator)
│   ├── TopologyManagementModule (TMM)
│   ├── LinkDetectionModule (LDM)
│   ├── RouteManagementModule (RMM)
│   └── PerformanceAnalyzer
├── SatelliteHelper (Orbital Mechanics)
├── QuaggaIntegrationHelper (Routing Interface)
└── AnimationHelper (Visualization)
```

## Core Components

### 1. Topology Management Module (TMM)

**Purpose**: Manages the topological model and extracts predictable events from satellite orbital mechanics.

**Key Responsibilities**:
- Extract predictable link events from orbital model
- Maintain timeline of all predicted failures
- Provide event lookup and status queries
- Coordinate with satellite constellation model

**Data Structures**:
```cpp
struct PredictableLinkDownEvent {
    int linkId;           // Link identifier
    int nodeA, nodeB;     // Link endpoints
    double T0;            // Physical failure time
    double T1, T2, T3;    // RFP timeline markers
    bool active;          // Event status
};
```

**Timeline Calculation**:
- T1 = T0 - Tc - 2×dT (Start BLD/BFU)
- T2 = T0 - dT (End BFU, sync tables)
- T3 = T0 + dT (End BLD)

### 2. Link Detection Module (LDM)

**Purpose**: Controls when to report link state changes to OSPF, implementing the core RFP blocking mechanism.

**Key Responsibilities**:
- Monitor real link state changes
- Control what is reported to OSPF
- Force links DOWN during BLD periods
- Restore normal detection after RFP events

**State Management**:
```cpp
struct LinkStateInfo {
    bool realState;         // Actual physical state
    bool reportedState;     // State reported to OSPF
    bool forcedDown;        // RFP forced down flag
    Time lastChange;        // Last state change time
    uint32_t changeCount;   // Change counter
};
```

**Control Logic**:
1. **Normal Operation**: Real state = Reported state
2. **BLD Period**: Real state ≠ Reported state (forced DOWN)
3. **BFU Period**: State changes blocked/buffered

### 3. Route Management Module (RMM)

**Purpose**: Controls when to apply new routing table updates to achieve synchronized forwarding.

**Key Responsibilities**:
- Buffer routing updates during BFU periods
- Apply updates synchronously at designated times
- Interface with real Quagga OSPF through DCE
- Track routing convergence performance

**Buffering Mechanism**:
```cpp
struct RouteUpdate {
    Ptr<Node> targetNode;       // Target node
    RouteUpdateType operation;  // ADD/DELETE/MODIFY
    std::string prefix;         // Destination network
    std::string nexthop;        // Next hop address
    uint32_t metric;            // Route metric
    Time timestamp;             // Creation time
};
```

**Operation Modes**:
- **Normal Mode**: Updates applied immediately
- **BFU Mode**: Updates buffered and applied synchronously at T2

### 4. Performance Analyzer

**Purpose**: Collects and analyzes performance metrics comparing RFP vs standard OSPF.

**Metrics Tracked**:
- Route outage duration
- Packet loss during failures
- Detection time improvement
- Protocol overhead reduction
- Quagga command execution statistics

## RFP Protocol Implementation

### Timeline Sequence

```
Time →  T1          T2       T0        T3
        │           │        │         │
        ▼           ▼        ▼         ▼
     ┌─────────────────────────────────────┐
     │         RFP Active Period           │
     └─────────────────────────────────────┘
        │           │        │         │
        │           │        │         └─ Restore normal detection
        │           │        └─ Physical failure occurs (0 packet loss)
        │           └─ End BFU, sync all forwarding tables
        └─ Start BLD + BFU, force link DOWN in OSPF
```

### State Machine

```
┌─────────────┐    T1 Event    ┌─────────────┐
│   Normal    │───────────────▶│  BLD + BFU  │
│ Operation   │                │   Active    │
└─────────────┘                └─────────────┘
       ▲                              │
       │                              │ T2 Event
       │                              ▼
┌─────────────┐    T3 Event    ┌─────────────┐
│  Normal     │◀───────────────│ BLD Active  │
│ Detection   │                │ (BFU Ended) │
│  Restored   │                └─────────────┘
└─────────────┘                       │
       ▲                              │ T0 Event
       │                              │ (Physical
       └──────────────────────────────┘  Failure)
```

## Integration Architecture

### NS-3 Integration

```
┌─────────────────────────────────────────────────────────┐
│                    NS-3 Simulator                       │
├─────────────────┬─────────────────┬───────────────────┤
│  Network Stack  │   Mobility      │   Applications    │
├─────────────────┼─────────────────┼───────────────────┤
│ • IPv4/IPv6     │ • Constellation │ • UDP Echo        │
│ • Point-to-Point│ • Orbital Mech  │ • Traffic Gen     │
│ • WiFi/LTE      │ • Ground Stas   │ • Flow Monitor    │
└─────────────────┴─────────────────┴───────────────────┘
```

### DCE Integration

```
┌─────────────────────────────────────────────────────────┐
│              DCE (Direct Code Execution)                │
├─────────────────┬─────────────────┬───────────────────┤
│   Real Apps     │   File System   │   Network Stack   │
├─────────────────┼─────────────────┼───────────────────┤
│ • Quagga zebra  │ • /etc/quagga   │ • Real sockets    │
│ • Quagga ospfd  │ • Config files  │ • Real routing    │
│ • vtysh CLI     │ • Log files     │ • Real protocols  │
└─────────────────┴─────────────────┴───────────────────┘
```

### Quagga Integration

```
┌─────────────────────────────────────────────────────────┐
│                   Quagga Suite                          │
├─────────────────┬─────────────────┬───────────────────┤
│     zebra       │     ospfd       │      vtysh        │
├─────────────────┼─────────────────┼───────────────────┤
│ • Route mgmt    │ • OSPF protocol │ • CLI interface   │
│ • Interface ctl │ • LSA handling  │ • Configuration   │
│ • Kernel sync   │ • SPF calc      │ • Status queries  │
└─────────────────┴─────────────────┴───────────────────┘
```

## Data Flow

### Normal OSPF Operation

```
Link State Change
       │
       ▼
   ┌─────────┐      ┌─────────┐      ┌─────────┐
   │   LDM   │─────▶│  OSPF   │─────▶│   RMM   │
   │ (Pass)  │      │ Process │      │ (Apply) │
   └─────────┘      └─────────┘      └─────────┘
                           │
                           ▼
                    Route Table Update
```

### RFP Operation (BLD Period)

```
Link State Change
       │
       ▼
   ┌─────────┐      ┌─────────┐      ┌─────────┐
   │   LDM   │─────▶│  OSPF   │─────▶│   RMM   │
   │ (Block) │      │ Process │      │(Buffer) │
   └─────────┘      └─────────┘      └─────────┘
       │                                   │
       │ T2 Event                          │
       ▼                                   ▼
   Force Sync                      Synchronous Apply
```

## Error Handling Strategy

### Graceful Degradation

1. **vtysh Unavailable**: Fall back to simulation mode
2. **DCE Failure**: Continue with standard NS-3 routing
3. **Configuration Error**: Apply default settings
4. **Resource Exhaustion**: Limit simulation scope

### Recovery Mechanisms

```cpp
try {
    // Attempt real Quagga operation
    ExecuteVtyshCommand(node, command);
} catch (const std::exception& e) {
    // Log error and fall back to simulation
    NS_LOG_WARN("Quagga error: " << e.what());
    SimulateVtyshCommand(node, command);
}
```

## Performance Considerations

### Memory Management

- **Smart Pointers**: Use NS-3 Ptr<> for automatic memory management
- **Container Optimization**: Reserve capacity for known-size containers
- **Event Cleanup**: Properly clean up scheduled events

### Computational Efficiency

- **Orbital Calculations**: Cache satellite positions, update periodically
- **Route Updates**: Batch updates during BFU periods
- **Link State Tracking**: Use efficient data structures (maps, sets)

### Scalability Limits

- **Maximum Satellites**: 30 (configurable, stability-tested)
- **Maximum Events**: 100 concurrent RFP events
- **Update Frequency**: 5-second intervals for position updates

## Configuration Management

### Hierarchical Configuration

```
constellation-params.h (Global Constants)
├── Orbital Parameters
├── RFP Timing Parameters
├── Network Parameters
└── Visualization Parameters
```

### Runtime Configuration

- Command-line parameters via NS-3 CommandLine
- Environment variables for DCE/Quagga paths
- Configuration file support (future enhancement)

## Testing Strategy

### Unit Testing

- Individual module testing with mock dependencies
- State machine validation
- Timeline calculation verification

### Integration Testing

- End-to-end simulation scenarios
- Quagga integration validation
- Error condition testing

### Performance Testing

- Scalability benchmarks
- Memory usage profiling
- Execution time analysis

## Future Enhancements

### Short Term

- Configuration file support
- Enhanced visualization
- Additional routing protocols (BGP, IS-IS)

### Long Term

- Multi-layer satellite networks
- Machine learning for prediction improvement
- Real satellite hardware integration

---

*This architecture document is maintained as part of the SATNET RFP project. For implementation details, see the API documentation and source code comments.*