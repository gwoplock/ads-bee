#ifndef _AIRCRAFT_DICTIONARY_HH_
#define _AIRCRAFT_DICTIONARY_HH_

#include "ads_b_packet.hh"
#include <unordered_map>
#include <cstring>

class Aircraft {
public:
    static const uint16_t kCallSignMaxNumChars = 8;

    typedef enum {
        WV_INVALID = 0,
        WV_RESERVED,
        WV_NO_CATEGORY_INFO,
        WV_SURFACE_EMERGENCY_VEHICLE,
        WV_SURVACE_SERVICE_VEHICLE,
        WV_GROUND_OBSTRUCTION,
        WV_GLIDER_SAILPLANE,
        WV_LIGHTER_THAN_AIR,
        WV_PARACHUTIST_SKYDIVER,
        WV_ULTRALIGHT_HANG_GLIDER_PARAGLIDER,
        WV_UNMANNED_AERIAL_VEHICLE,
        WV_SPACE_TRANSATMOSPHERIC_VEHICLE,
        WV_LIGHT, // < 7000kg
        WV_MEDIUM_1, // 7000kg - 34000kg
        WV_MEDIUM_2, // 34000kg - 136000kg
        WV_HIGH_VORTEX_AIRCRAFT,
        WV_HEAVY, // > 136000kg
        WV_HIGH_PERFORMANCE, // >5g acceleration and >400kt speed
        WV_ROTORCRAFT
    } WakeVortex_t;

    typedef enum {
        SS_NO_CONDITION = 0,
        SS_PERMANENT_ALERT = 1,
        SS_TEMPORARY_ALERT = 2,
        SS_SPI_CONDITION = 3
    } SurveillanceStatus_t;

    uint32_t last_seen_timestamp_ms = 0;

    uint16_t transponder_capability = 0;
    uint32_t icao_address = 0;
    char callsign[kCallSignMaxNumChars+1]; // put extra EOS character at end
    WakeVortex_t wake_vortex = WV_INVALID;

    SurveillanceStatus_t surveillance_status = SS_NO_CONDITION;
    bool single_antenna_flag = false;
    uint16_t barometric_altitude = 0;
    uint16_t gnss_altitude = 0;

    float latitude = 0.0f;
    float longitude = 0.0f;
    bool position_valid = false;
    bool is_airborne = true; // assume that most aircraft encountered will be airborne, so put them there until proven otherwise

    Aircraft(uint32_t icao_address_in);
    Aircraft();

    bool SetCPRLatLon(uint32_t n_lat_cpr, uint32_t n_lon_cpr, bool odd, bool redigesting=false);
    bool DecodePosition();

private:

    struct CPRPacket {
        // SetCPRLatLon values.
        uint32_t received_timestamp_ms = 0; // [ms] time since boot when packet was recorded
        uint32_t n_lat = 0; // 17-bit latitude count
        uint32_t n_lon = 0; // 17-bit longitude count

        // DecodePosition values.
        uint32_t calculated_timestamp_ms = 0; // [ms] time since boot when packet was calculated
        float lat_cpr = 0.0f;
        float lon_cpr = 0.0f;
        uint16_t nl_cpr = 0; // number of longitude cells in latitude band
        float lat = 0.0f; // only keep latitude since it's reused in cooperative calculations between odd and even packets
        // float lon = 0.0f; // longitude
    };

    CPRPacket last_odd_packet_;
    CPRPacket last_even_packet_;

};

class AircraftDictionary {
public:
    static const uint16_t kMaxNumAircraft = 100;

    AircraftDictionary();

    bool IngestADSBPacket(ADSBPacket packet);
    uint16_t GetNumAircraft();

    void Update(uint32_t timestamp_us);

    bool InsertAircraft(const Aircraft aircraft);
    bool RemoveAircraft(uint32_t icao_address);
    bool GetAircraft(uint32_t icao_address, Aircraft &aircraft_out) const;
    bool ContainsAircraft(uint32_t icao_address) const;
    Aircraft * GetAircraftPtr(uint32_t icao_address);

private:
    std::unordered_map<uint32_t, Aircraft> aircraft_dictionary_; // index Aircraft objects by their ICAO identifier

    // Helper functions for ingesting specific ADS-B packet types, called by IngestADSBPacket.
    bool IngestAircraftIDMessage(Aircraft * aircraft, ADSBPacket packet);
    bool IngestSurfacePositionMessage(Aircraft * aircraft, ADSBPacket packet);
    // bool IngestAirbornePositionBaroAltMessage(ADSBPacket packet);
    // bool IngestAirbornePositionGNSSAltMessage(ADSBPacket packet);
    bool IngestAirbornePositionMessage(Aircraft * aircraft, ADSBPacket packet);
    bool IngestAirborneVelocitiesMessage(Aircraft * aircraft, ADSBPacket packet);
    bool IngestAircraftStatusMessage(Aircraft * aircraft, ADSBPacket packet);
    bool IngestTargetStateAndStatusInfoMessage(Aircraft * aircraft, ADSBPacket packet);
    bool IngestAircraftOperationStatusMessage(Aircraft * aircraft, ADSBPacket packet);

};

#endif /* _AIRCRAFT_DICTIONARY_HH_ */