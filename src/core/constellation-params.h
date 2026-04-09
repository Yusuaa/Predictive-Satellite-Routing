/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Global constellation and simulation parameters
 * SATNET-OSPF RFP Simulation
 */

#ifndef CONSTELLATION_PARAMS_H
#define CONSTELLATION_PARAMS_H

#include <vector>
#include <string>
#include <utility>

// ================================
// PHYSICAL CONSTANTS
// ================================
const double EARTH_RADIUS = 6371.0;
const double PI = 3.14159265358979323846;

// ================================
// SATELLITE ORBIT PARAMETERS
// ================================
const int NUM_PLANES = 6;
const int SATS_PER_PLANE = 18;
const double ALTITUDE = 1200.0;
const double INCLINATION_DEG[6] = {45.0, 60.0, 75.0, 30.0, 55.0, 80.0};
const double PLANE_PHASE_DIFF = 60.0;
const double SAT_PHASE_DIFF = 360.0 / SATS_PER_PLANE;

// ================================
// RFP TIMING PARAMETERS
// ================================
const double RFP_CONVERGENCE_TIME_TC = 2.0;    // OSPF convergence time (Tc)
const double RFP_SAFETY_MARGIN_DT = 0.5;       // Safety margin (dT)

// ================================
// INTER-SATELLITE LINK PARAMETERS
// ================================
const double INTER_PLANE_VISIBILITY_DISTANCE = 500.0;
const double INTRA_PLANE_VISIBILITY_DISTANCE = 1000.0;

// Link visualization parameters
const uint8_t LINK_ACTIVE_COLOR_R = 0;
const uint8_t LINK_ACTIVE_COLOR_G = 255;
const uint8_t LINK_ACTIVE_COLOR_B = 0;
const uint8_t LINK_INACTIVE_COLOR_R = 255;
const uint8_t LINK_INACTIVE_COLOR_G = 0;
const uint8_t LINK_INACTIVE_COLOR_B = 0;
const double LINK_UPDATE_INTERVAL = 0.5;

// ================================
// GROUND STATION LOCATIONS (lat, lon)
// ================================
const std::vector<std::pair<double, double>> GROUND_STATIONS = {
    {40.7128, -74.0060},   // New York
    {51.5074, -0.1278},    // London
    {35.6762, 139.6503},   // Tokyo
    {-33.8688, 151.2093}   // Sydney
};

// ================================
// NETWORK PARAMETERS
// ================================
const std::string P2P_RATE = "10Mbps";
const std::string SATELLITE_DELAY = "20ms";
const std::string GROUND_TO_SAT_DELAY = "20ms";
const uint16_t UDP_PORT = 9;

// ================================
// SIMULATION / ANIMATION PARAMETERS
// ================================
const double SIM_START = 1.0;
const double SIM_STOP = 100.0;
const double END_TIME = 105.0;
const double ORBIT_PERIOD = 60.0;
const double ANIMATION_SPEED_FACTOR = 10.0;
const double LINK_VISIBILITY_THRESHOLD = 0.2;

// Colors for different orbital planes
const uint8_t ORBITAL_PLANE_COLORS[6][3] = {
    {255, 50, 50},     // Red
    {50, 255, 50},     // Green
    {50, 50, 255},     // Blue
    {255, 255, 50},    // Yellow
    {255, 50, 255},    // Magenta
    {50, 255, 255}     // Cyan
};
const double LINK_WIDTH_FACTOR = 2.0;

// Logging parameters
const bool ENABLE_DETAILED_LINK_LOGS = true;
const bool ENABLE_DETAILED_POSITION_LOGS = false;

#endif // CONSTELLATION_PARAMS_H
