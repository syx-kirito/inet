//
// Copyright (C) 2005,2006 INRIA, 2014 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
// Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
//

#ifndef __INET_IEEE80211MODULATION_H
#define __INET_IEEE80211MODULATION_H

#include <ostream>
#include <stdint.h>
#include "inet/common/INETDefs.h"

namespace inet {

namespace physicallayer {

enum ModulationClass {
    /** Modulation class unknown or unspecified. A WifiMode with this
       WifiModulationClass has not been properly initialised. */
    MOD_CLASS_UNKNOWN = 0,
    /** Infrared (IR) (Clause 16) */
    MOD_CLASS_IR,
    /** Frequency-hopping spread spectrum (FHSS) PHY (Clause 14) */
    MOD_CLASS_FHSS,
    /** DSSS PHY (Clause 15) and HR/DSSS PHY (Clause 18) */
    MOD_CLASS_DSSS,
    /** ERP-PBCC PHY (19.6) */
    MOD_CLASS_ERP_PBCC,
    /** DSSS-OFDM PHY (19.7) */
    MOD_CLASS_DSSS_OFDM,
    /** ERP-OFDM PHY (19.5) */
    MOD_CLASS_ERP_OFDM,
    /** OFDM PHY (Clause 17) */
    MOD_CLASS_OFDM,
    /** HT PHY (Clause 20) */
    MOD_CLASS_HT
};

/**
 * This enumeration defines the various convolutional coding rates
 * used for the OFDM transmission modes in the IEEE 802.11
 * standard. DSSS (for example) rates which do not have an explicit
 * coding stage in their generation should have this parameter set to
 * WIFI_CODE_RATE_UNDEFINED.
 */
enum CodeRate {
    /** No explicit coding (e.g., DSSS rates) */
    CODE_RATE_UNDEFINED,
    /** Rate 3/4 */
    CODE_RATE_3_4,
    /** Rate 2/3 */
    CODE_RATE_2_3,
    /** Rate 1/2 */
    CODE_RATE_1_2,
    /** Rate 5/6 */
    CODE_RATE_5_6
};

/**
 * \brief represent a single transmission mode
 *
 * A WifiMode is implemented by a single integer which is used
 * to lookup in a global array the characteristics of the
 * associated transmission mode. It is thus extremely cheap to
 * keep a WifiMode variable around.
 */
class ModulationType
{
  public:
    /**
     * \returns the number of Hz used by this signal
     */
    uint32_t getChannelSpacing(void) const { return channelSpacing; }
    void setChannelSpacing(uint32_t p) { channelSpacing = p; }
    uint32_t getBandwidth() const { return bandwidth; }
    void setBandwidth(uint32_t p) { bandwidth = p; }
    /**
     * \returns the physical bit rate of this signal.
     *
     * If a transmission mode uses 1/2 FEC, and if its
     * data rate is 3Mbs, the phy rate is 6Mbs
     */
    /// MANDATORY it is necessary set the dataRate before the codeRate
    void setCodeRate(enum CodeRate cRate)
    {
        codeRate = cRate;
        switch (cRate) {
            case CODE_RATE_5_6:
                phyRate = dataRate * 6 / 5;
                break;

            case CODE_RATE_3_4:
                phyRate = dataRate * 4 / 3;
                break;

            case CODE_RATE_2_3:
                phyRate = dataRate * 3 / 2;
                break;

            case CODE_RATE_1_2:
                phyRate = dataRate * 2 / 1;
                break;

            case CODE_RATE_UNDEFINED:
            default:
                phyRate = dataRate;
                break;
        }
    };
    uint32_t getPhyRate(void) const { return phyRate; }
    /**
     * \returns the data bit rate of this signal.
     */
    uint32_t getDataRate(void) const { return dataRate; }
    void setDataRate(uint32_t p) { dataRate = p; }
    /**
     * \returns the coding rate of this transmission mode
     */
    enum CodeRate getCodeRate(void) const { return codeRate; }

    /**
     * \returns the size of the modulation constellation.
     */
    uint8_t getConstellationSize(void) const { return constellationSize; }
    void setConstellationSize(uint8_t p) { constellationSize = p; }

    /**
     * \returns true if this mode is a mandatory mode, false
     *          otherwise.
     */
    enum ModulationClass getModulationClass() const { return modulationClass; }
    void setModulationClass(enum ModulationClass p) { modulationClass = p; }

    void setIsMandatory(bool val) { isMandatory = val; }
    bool getIsMandatory() { return isMandatory; }
    ModulationType()
    {
        isMandatory = false;
        channelSpacing = 0;
        bandwidth = 0;
        codeRate = CODE_RATE_UNDEFINED;
        dataRate = 0;
        phyRate = 0;
        constellationSize = 0;
        modulationClass = MOD_CLASS_UNKNOWN;
    }

    bool operator==(const ModulationType& b)
    {
        return *this == b;
    }

  private:
    bool isMandatory;
    uint32_t channelSpacing;
    enum CodeRate codeRate;
    uint32_t dataRate;
    uint32_t phyRate;
    uint8_t constellationSize;
    enum ModulationClass modulationClass;
    uint32_t bandwidth;
};

bool operator==(const ModulationType& a, const ModulationType& b);

inline std::ostream& operator<<(std::ostream& out, const ModulationType& m)
{
    //FIXME TODO implements operator<<
    return out;
}

/**
 * See IEEE Std 802.11-2007 section 18.2.2.
 */
enum Ieee80211PreambleMode {
    IEEE80211_PREAMBLE_LONG,
    IEEE80211_PREAMBLE_SHORT,
    IEEE80211_PREAMBLE_HT_MF,
    IEEE80211_PREAMBLE_HT_GF
};

class Ieee80211Modulation
{
  public:
    static ModulationType GetDsssRate1Mbps();
    static ModulationType GetDsssRate2Mbps();
    static ModulationType GetDsssRate5_5Mbps();
    static ModulationType GetDsssRate11Mbps();
    static ModulationType GetErpOfdmRate6Mbps();
    static ModulationType GetErpOfdmRate9Mbps();
    static ModulationType GetErpOfdmRate12Mbps();
    static ModulationType GetErpOfdmRate18Mbps();
    static ModulationType GetErpOfdmRate24Mbps();
    static ModulationType GetErpOfdmRate36Mbps();
    static ModulationType GetErpOfdmRate48Mbps();
    static ModulationType GetErpOfdmRate54Mbps();
    static ModulationType GetOfdmRate6Mbps();
    static ModulationType GetOfdmRate9Mbps();
    static ModulationType GetOfdmRate12Mbps();
    static ModulationType GetOfdmRate18Mbps();
    static ModulationType GetOfdmRate24Mbps();
    static ModulationType GetOfdmRate36Mbps();
    static ModulationType GetOfdmRate48Mbps();
    static ModulationType GetOfdmRate54Mbps();
    static ModulationType GetOfdmRate3MbpsCS10MHz();
    static ModulationType GetOfdmRate4_5MbpsCS10MHz();
    static ModulationType GetOfdmRate6MbpsCS10MHz();
    static ModulationType GetOfdmRate9MbpsCS10MHz();
    static ModulationType GetOfdmRate12MbpsCS10MHz();
    static ModulationType GetOfdmRate18MbpsCS10MHz();
    static ModulationType GetOfdmRate24MbpsCS10MHz();
    static ModulationType GetOfdmRate27MbpsCS10MHz();
    static ModulationType GetOfdmRate1_5MbpsCS5MHz();
    static ModulationType GetOfdmRate2_25MbpsCS5MHz();
    static ModulationType GetOfdmRate3MbpsCS5MHz();
    static ModulationType GetOfdmRate4_5MbpsCS5MHz();
    static ModulationType GetOfdmRate6MbpsCS5MHz();
    static ModulationType GetOfdmRate9MbpsCS5MHz();
    static ModulationType GetOfdmRate12MbpsCS5MHz();
    static ModulationType GetOfdmRate13_5MbpsCS5MHz();

    static simtime_t getPlcpHeaderDuration(ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static simtime_t getPlcpPreambleDuration(ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static simtime_t getPreambleAndHeader(ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static simtime_t getPayloadDuration(uint64_t size, ModulationType payloadMode);
    static simtime_t calculateTxDuration(uint64_t size, ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static simtime_t getSlotDuration(ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static simtime_t getSifsTime(ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static simtime_t get_aPHY_RX_START_Delay(ModulationType payloadMode, Ieee80211PreambleMode preamble);
    static ModulationType getPlcpHeaderMode(ModulationType payloadMode, Ieee80211PreambleMode preamble);
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_IEEE80211MODULATION_H

