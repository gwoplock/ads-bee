#ifndef _AIRCRAFT_DICTIONARY_HH_
#define _AIRCRAFT_DICTIONARY_HH_

#include <cstring>
#include <unordered_map>

#include "transponder_packet.hh"

class Aircraft {
   public:
    static const uint16_t kCallSignMaxNumChars = 7;

    enum AirframeType : uint16_t {
        kAirframeTypeInvalid = 0,
        kAirframeTypeReserved,
        kAirframeTypeNoCategoryInfo,
        kAirframeTypeSurfaceEmergencyVehicle,
        kAirframeTypeSurfaceServiceVehicle,
        kAirframeTypeGroundObstruction,
        kAirframeTypeGliderSailplane,
        kAirframeTypeLighterThanAir,
        kAirframeTypeParachutistSkydiver,
        kAirframeTypeUltralightHangGliderParaglider,
        kAirframeTypeUnmannedAerialVehicle,
        kAirframeTypeSpaceTransatmosphericVehicle,
        kAirframeTypeLight,    // < 7000kg
        kAirframeTypeMedium1,  // 7000kg - 34000kg
        kAirframeTypeMedium2,  // 34000kg - 136000kg
        kAirframeTypeHighVortexAircraft,
        kAirframeTypeHeavy,            // > 136000kg
        kAirframeTypeHighPerformance,  // >5g acceleration and >400kt speed
        kAirframeTypeRotorcraft
    };

    enum AltitudeSource : int16_t {
        kAltitudeNotAvailable = -2,
        kAltitudeSourceNotSet = -1,
        kAltitudeSourceBaro = 0,
        kAltitudeSourceGNSS = 1
    };

    enum VerticalRateSource : int16_t {
        kVerticalRateNotAvailable = -2,
        kVerticalRateSourceNotSet = -1,
        kVerticalRateSourceGNSS = 0,
        kVerticalRateSourceBaro = 1
    };

    enum VelocitySource : int16_t {
        kVelocitySourceNotAvailable = -2,
        kVelocitySourceNotSet = -1,
        kVelocitySourceGroundSpeed = 0,
        kVelocitySourceAirspeedTrue = 1,
        kVelocitySourceAirspeedIndicated = 2
    };

    enum BitFlag : uint32_t {
        kBitFlagIsAirborne = 0,          // Received messages or flags indicating the aircraft is airborne.
        kBitFlagIsMilitary,              // Received at least one military ES message from the aircraft.
        kBitFlagIsClassB2GroundVehicle,  // Is a class B2 ground vehicle transmitting at <70W.
        kBitFlagHas1090ESIn,             // Aircraft is equipped with 1090MHz Extended Squitter receive capability.
        kBitFlagHasUATIn,                // Aircraft can receive UAT.
        kBitFlagTCASOperational,         // TCAS system on aircraft is functional.
        kBitFlagSingleAntenna,           // Indicates that the aircraft is using a single antenna. Transmissions may be
                                         // intermittent.
        kBitFlagSurfacePositionUsesHeading,  // Surface position heading is aircraft heading and not track angle.
        kBitFlagHeadingUsesMagneticNorth,    // Heading in surface and airborne position messages is magnetic north, not
                                             // true north.
        kBitFlagIdent,                       // IDENT switch is currently active.
        kBitFlagAlert,                       // Aircraft is indicating an alert.
        kBitFlagTCASRAActive,                // Indicates a TCAS resolution advisory is active.
        // Flags after kBitFlagUpdatedBaroAltitude are cleared at the end of every reporting interval.
        kBitFlagUpdatedBaroAltitude,
        kBitFlagUpdatedGNSSAltitude,
        kBitFlagUpdatedPosition,
        kBitFlagUpdatedTrack,
        kBitFlagUpdatedHorizontalVelocity,
        kBitFlagUpdatedVerticalVelocity,
        kBitFlagNumFlagBits
    };

    enum NICBit : uint16_t { kNICBitA = 0, kNICBitB = 1, kNICBitC = 2 };

    enum SystemDesignAssurance : uint8_t {
        kSDASupportedFailureUnknownOrNoSafetyEffect = 0b00,
        kSDASupportedFailureMinor = 0b01,
        kSDASupportedFailureMajor = 0b10,
        kSDASupportedFailureHazardous = 0b11
    };

    enum NICRadiusOfContainment : uint8_t {
        kROCUnknown = 0,
        kROCLessThan20NauticalMiles = 1,
        kROCLessThan8NauticalMiles = 2,
        kROCLessThan4NauticalMiles = 3,
        kROCLessThan2NauticalMiles = 4,
        kROCLessThan1NauticalMile = 5,
        kROCLessThan0p6NauticalMiles = 6,  // Lump together with <0.5NM and <0.3NM since they share a NIC value.
        kROCLessThan0p2NauticalMiles = 7,
        kROCLessThan0p1NauticalMiles = 8,
        kROCLessThan75Meters = 9,
        kROCLessThan25Meters = 10,
        kROCLessThan7p5Meters = 11
    };

    enum NICBarometricAltitudeIntegrity : uint8_t {
        kBAIGillhamInputNotCrossChecked = 0,
        kBAIGillHamInputCrossCheckedOrNonGillhamSource = 1
    };

    enum NACHorizontalVelocityError : uint8_t {
        kHVEUnknownOrGreaterThanOrEqualTo10MetersPerSecond = 0b000,
        kHVELessThan10MetersPerSecond = 0b110,
        kHVELessThan3MetersPerSecond = 0b010,
        kHVELessThan1MeterPerSecond = 0b011,
        kHVELessThan0p3MetersPerSecond = 0b100
    };

    enum NACEstimatedPositionUncertainty : uint8_t {
        kEPUUnknownOrGreaterThanOrEqualTo10NauticalMiles = 0,
        kEPULessThan10NauticalMiles = 1,
        kEPULessThan4NauticalMiles = 2,
        kEPULessThan2NauticalMiles = 3,
        kEPULessThan1NauticalMile = 4,
        kEPULessThan0p5NauticalMiles = 5,
        kEPULessThan0p3NauticalMiles = 6,
        kEPULessThan0p1NauticalMiles = 7,
        kEPULessThan0p05NauticalMiles = 8,
        kEPULessThan30Meters = 9,
        kEPULessThan10Meters = 10,
        kEPULessThan3Meters = 11
    };

    // This composite SIL value is SIL | (SIL_supplement << 2).
    enum SILProbabilityOfExceedingNICRadiusOfContainmnent : uint8_t {
        kPOERCUnknownOrGreaterThan1em3PerFlightHour = 0b000,
        kPOERCLessThanOrEqualTo1em3PerFligthHour = 0b001,
        kPOERCLessThanOrEqualTo1em5PerFlightHour = 0b010,
        kPOERCLessThanOrEqualTo1em7PerFlightHour = 0b011,
        kPOERCUnknownOrGreaterThan1em3PerSample = 0b100,
        kPOERCLessThanOrEqualTo1em3PerSample = 0b101,
        kPOERCLessThanOrEqualTo1em5PerSample = 0b110,
        kPOERCLessThanOrEqualTo1em7PerSample = 0b111,
    };

    enum GeometricVerticalAccurary : uint8_t {
        kGVAUnknownOrGreaterThan150Meters = 0,
        GVALessThanOrEqualTo150Meters = 1,
        GVALessThanOrEqualTo45Meters = 2,
        GVAReserved = 3
    };

    Aircraft(uint32_t icao_address_in);
    Aircraft();

    /**
     * Set an aircraft's position in Compact Position Reporting (CPR) format. Takes either an even or odd set of lat/lon
     * coordinates and uses them to set the aircraft's position.
     * @param[in] n_lat_cpr 17-bit latitude count.
     * @param[in] n_lon_cpr 17-bit longitude count.
     * @param[in] odd Boolean indicating that the position update is relative to an odd grid reference (if true) or an
     * even grid reference.
     * @param[in] redigesting Boolean flag used if SetCPRLatLon is being used to re-digest a packet. Assures that it
     * won't call itself again if set.
     * @retval True if coordinates were parsed successfully, false if not. NOTE: invalid positions can still be
     * considered a successful parse.
     */
    bool SetCPRLatLon(uint32_t n_lat_cpr, uint32_t n_lon_cpr, bool odd, bool redigesting = false);

    /**
     * Simple helper that checks to see whether a packet decode can be attempted (does not guarantee it will succeed,
     * for instance an odd and even packet may have been received, but from different CPR zones).
     * @retval True if decode can be attempted, false otherwise.
     */
    bool CanDecodePosition() {
        return last_odd_packet_.received_timestamp_ms > 0 && last_even_packet_.received_timestamp_ms > 0;
    }

    /**
     * Decodes the aircraft position using last_odd_packet_ and last_even_packet_.
     * @retval True if position was decoded successfully, false otherwise.
     */
    bool DecodePosition();

    /**
     * Set or clear a bit on the Aircraft.
     */
    inline void WriteBitFlag(BitFlag bit, bool value) { value ? flags |= (0b1 << bit) : flags &= ~(0b1 << bit); }

    /**
     * Write a value for a NIC supplement bit. Used to piece together a NIC from separate messages, so that the NIC can
     * be determined based on a received TypeCode.
     * @param[in] bit NIC supplement bit to write.
     * @param[in] value Value to write to the bit.
     */
    inline void WriteNICBit(NICBit bit, bool value) {
        value ? nic_bits |= (0b1 << bit) : nic_bits &= ~(0b1 << bit);
        // FIXME: Permanently setting NIC bits valid like this can cause invalid navigation integrity values to be read
        // if a stale NIC supplement bit is being used. Hopefully this isn't a big problem if NIC values don't change
        // frequently.
        nic_bits_valid |= (0b1 << bit);
    }

    /**
     * Returns whether a NIC supplement bit has been written to. Used to decide when to use NIC supplement bit values to
     * determine NIC value based on received TypeCodes.
     * @param[in] bit NIC supplement bit to check.
     * @retval True if bit has been written to, false otherwise.
     */
    inline bool NICBitIsValid(NICBit bit) { return nic_bits & (0b1 << bit); }

    /**
     * Checks whether a flag bit is set.
     * @param[in] bit Position of bit to check.
     * @retval True if bit has been set, false if bit has been cleared.
     */
    inline bool HasBitFlag(BitFlag bit) { return flags & (0b1 << bit) ? true : false; }

    /**
     * Resets just the flag bits that show that something updated within the last reporting interval.
     */
    inline void ResetUpdatedBitFlags() { flags &= ~(~0b0 << kBitFlagUpdatedBaroAltitude); }

    uint32_t flags = 0b0;

    uint32_t last_seen_timestamp_ms = 0;

    uint16_t transponder_capability = 0;
    uint32_t icao_address = 0;
    char callsign[kCallSignMaxNumChars + 1] = "?";  // put extra EOS character at end
    uint16_t squawk = 0;
    AirframeType airframe_type = kAirframeTypeInvalid;

    int32_t baro_altitude_ft = 0;
    int32_t gnss_altitude_ft = 0;
    AltitudeSource altitude_source = kAltitudeSourceNotSet;

    // Airborne Position Message
    float latitude_deg = 0.0f;
    float longitude_deg = 0.0f;
    bool position_valid = false;

    // Airborne Velocities Message
    float heading_deg = 0.0f;
    float velocity_kts = 0;
    VelocitySource velocity_source = kVelocitySourceNotSet;
    int vertical_rate_fpm = 0.0f;
    VerticalRateSource vertical_rate_source = kVerticalRateSourceNotSet;
    int altitude_difference_gnss_above_baro_ft = 0;

    // Aircraft Operation Status Message
    // Navigation Integrity Category (NIC)
    uint8_t nic_bits_valid = 0b000;  // MSb to LSb: nic_c_valid nic_b_valid nic_a_valid.
    uint8_t nic_bits = 0b000;        // MSb to LSb: nic_c nic_b nic_a.
    NICRadiusOfContainment nic = kROCUnknown;
    NICBarometricAltitudeIntegrity nic_baro = kBAIGillhamInputNotCrossChecked;  // Default to worst case.
    // Navigation Accuracy Category (NAC)
    NACHorizontalVelocityError nac_velocity = kHVEUnknownOrGreaterThanOrEqualTo10MetersPerSecond;     // 3 bits.
    NACEstimatedPositionUncertainty nac_position = kEPUUnknownOrGreaterThanOrEqualTo10NauticalMiles;  // 4 bits.
    // Geometric Vertical Accuracy (GVA)
    GeometricVerticalAccurary gva = kGVAUnknownOrGreaterThan150Meters;  // 2 bits.
    SILProbabilityOfExceedingNICRadiusOfContainmnent sil = kPOERCUnknownOrGreaterThan1em3PerFlightHour;
    // System Design Assurance
    SystemDesignAssurance system_design_assurance = kSDASupportedFailureUnknownOrNoSafetyEffect;
    // GPS Antenna Offset
    int8_t gnss_antenna_offset_right_of_roll_axis_m =
        INT8_MAX;  // Defaults to INT8_MAX to indicate it hasn't been read yet.
    // Aircraft dimensions (on the ground).
    uint16_t length_m = 0;
    uint16_t width_m = 0;

    int8_t adsb_version = -1;

   private:
    struct CPRPacket {
        // SetCPRLatLon values.
        uint32_t received_timestamp_ms = 0;  // [ms] time since boot when packet was recorded
        uint32_t n_lat = 0;                  // 17-bit latitude count
        uint32_t n_lon = 0;                  // 17-bit longitude count

        // DecodePosition values.
        uint32_t calculated_timestamp_ms = 0;  // [ms] time since boot when packet was calculated
        float lat_cpr = 0.0f;
        float lon_cpr = 0.0f;
        uint16_t nl_cpr = 0;  // number of longitude cells in latitude band
        float lat =
            0.0f;  // only keep latitude since it's reused in cooperative calculations between odd and even packets
        // float lon = 0.0f; // longitude
    };

    CPRPacket last_odd_packet_;
    CPRPacket last_even_packet_;
};

class AircraftDictionary {
   public:
    struct AircraftDictionaryConfig_t {
        uint32_t aircraft_prune_interval_ms = 60e3;
    };
    static const uint16_t kMaxNumAircraft = 100;

    /**
     * Default constructor. Uses default config values.
     */
    AircraftDictionary() {};

    /**
     * Constructor with config values specified.
     */
    AircraftDictionary(AircraftDictionaryConfig_t config_in) : config_(config_in) {};

    /**
     * Removes all aircraft from the aircraft dictionary.
     */
    void Init();

    /**
     * Prunes stale aircraft from the dictionary.
     * @param[in] timestamp_us Current timestamp, in microseconds, to use for pruning. Aircraft older than timestamp_us
     * minus the pruning interval will be removed.
     */
    void Update(uint32_t timestamp_us);

    /**
     * Ingests a DecodedTransponderPacket and uses it to insert and update the relevant aircraft.
     * @param[in] packet DecodedTransponderPacket to ingest. Can be 56-bit (Squitter) or 112-bit (Extended Squitter).
     * Passed as a reference, since packets can be marked as valid by this function.
     * @retval True if successful, false if something broke.
     */
    bool IngestDecodedTransponderPacket(DecodedTransponderPacket &packet);

    /**
     * Ingests a Mode A (Identity Surveillance Reply) packet and uses it to update the relevant aircraft. Exposed for
     * testing, but usually called by IngestDecodedTransponderPacket.
     * Note: this function requires that the packet be marked as valid using the ForceValid() function. If the packet is
     * valid and does not match an ICAO in the aircraft dictionary, a new aircraft will be inserted.
     * @param[in] packet ModeAPacket to ingest.
     * @retval True if successful, false if something broke.
     */
    bool IngestModeAPacket(ModeAPacket packet);

    /**
     * Ingests a Mode C (Altitude Surveillance Reply) packet and uses it to update the relevant aircraft. Exposed for
     * testing, but usually called by IngestDecodedTransponderPacket.
     * Note: this function requires that the packet be marked as valid using the ForceValid() function. If the packet is
     * valid and does not match an ICAO in the aircraft dictionary, a new aircraft will be inserted.
     * @param[in] packet ModeCPacket to ingest.
     * @retval True if successful, false if something broke.
     */
    bool IngestModeCPacket(ModeCPacket packet);

    /**
     * Ingests an ADSBPacket directly. Exposed for testing, but usually this gets called by
     * IngestDecodedTransponderPacket and should not get touched directly.
     * @param[in] packet ADSBPacket to ingest. Derived from a DecodedTransponderPacket with DF=17-19.
     * @retval True if successful, false if something broke.
     */
    bool IngestADSBPacket(ADSBPacket packet);

    /**
     * Returns the number of aircraft currently in the dictionary.
     * @retval Number of aircraaft that are currently in the dictionary.
     */
    uint16_t GetNumAircraft();

    /**
     * Adds an Aircraft object to the aircraft dictionary, hashed by ICAO address.
     * @param[in] aircraft Aircraft to insert.
     * @retval True if insertaion succeeded, false if failed.
     */
    bool InsertAircraft(const Aircraft &aircraft);

    /**
     * Remove an aircraft from the dictionary, by ICAO address.
     * @param[in] icao_address ICAO address of the aircraft to remove from the dictionary.
     * @retval True if removal succeeded, false if aircraft was not found.
     */
    bool RemoveAircraft(uint32_t icao_address);

    /**
     * Retrieve an aircraft from the dictionary.
     * @param[in] icao_address Address to use for looking up the aircraft.
     * @param[out] aircraft_out Aircraft reference to put the retrieved aircraft into if successful.
     * @retval True if aircraft was found and retrieved, false if aircraft was not in the dictionary.
     */
    bool GetAircraft(uint32_t icao_address, Aircraft &aircraft_out) const;

    /**
     * Check if an aircraft is contained in the dictionary.
     * @param[in] icao_address Address to use for looking up the aircraft.
     * @retval True if aircraft is in the dictionary, false if not.
     */
    bool ContainsAircraft(uint32_t icao_address) const;

    /**
     * Return a pointer to an aircraft if it's in the aircraft dictionary.
     * @param[in] icao_address ICAO address of the aircraft to find.
     * @retval Pointer to the aircraft if it exists, or NULL if it wasn't in the dictionary.
     */
    Aircraft *GetAircraftPtr(uint32_t icao_address);

    std::unordered_map<uint32_t, Aircraft> dict;  // index Aircraft objects by their ICAO identifier

   private:
    // Helper functions for ingesting specific ADS-B packet types, called by IngestADSBPacket.

    /**
     * GENERIC COMMENT FOR ALL MESSAGE INGESTION HELPERS
     * Ingests a <Message Type> ADS-B message. Called by IngestADSBPacket, which makes sure that the packet
     * is valid and has the correct Downlink Format.
     * @param[out] aircraft Reference to the Aircraft to populate with info pulled from packet.
     * @param[in] packet ADSBPacket to ingest.
     * @retval True if message was ingested successfully, false otherwise.
     */

    bool ApplyAircraftIDMessage(Aircraft &aircraft, ADSBPacket packet);
    bool ApplySurfacePositionMessage(Aircraft &aircraft, ADSBPacket packet);
    bool ApplyAirbornePositionMessage(Aircraft &aircraft, ADSBPacket packet);
    bool ApplyAirborneVelocitiesMessage(Aircraft &aircraft, ADSBPacket packet);
    bool ApplyAircraftStatusMessage(Aircraft &aircraft, ADSBPacket packet);
    bool ApplyTargetStateAndStatusInfoMessage(Aircraft &aircraft, ADSBPacket packet);
    bool ApplyAircraftOperationStatusMessage(Aircraft &aircraft, ADSBPacket packet);

    AircraftDictionaryConfig_t config_;
};

#endif /* _AIRCRAFT_DICTIONARY_HH_ */