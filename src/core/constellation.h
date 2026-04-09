/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Constellation model definitions
 * SATNET-OSPF RFP Simulation
 */

#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include "constellation-params.h"
#include <cmath>

/**
 * Utility functions for satellite constellation geometry
 */

/**
 * Calculate the distance between two points in 3D space
 */
inline double CalculateDistance3D(double x1, double y1, double z1,
                                  double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

/**
 * Convert degrees to radians
 */
inline double DegToRad(double deg) {
    return deg * PI / 180.0;
}

/**
 * Convert radians to degrees
 */
inline double RadToDeg(double rad) {
    return rad * 180.0 / PI;
}

/**
 * Calculate orbital radius from altitude
 */
inline double GetOrbitalRadius() {
    return EARTH_RADIUS + ALTITUDE;
}

/**
 * Get the plane index for a given satellite index
 */
inline uint32_t GetPlaneIndex(uint32_t satIndex) {
    return satIndex / SATS_PER_PLANE;
}

/**
 * Get the intra-plane index for a given satellite index
 */
inline uint32_t GetIntraPlaneIndex(uint32_t satIndex) {
    return satIndex % SATS_PER_PLANE;
}

/**
 * Check if two satellites are in the same orbital plane
 */
inline bool SamePlane(uint32_t satA, uint32_t satB) {
    return GetPlaneIndex(satA) == GetPlaneIndex(satB);
}

#endif // CONSTELLATION_H
