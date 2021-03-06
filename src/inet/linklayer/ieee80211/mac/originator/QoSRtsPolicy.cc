//
// Copyright (C) 2016 OpenSim Ltd.
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
// along with this program; if not, see http://www.gnu.org/licenses/.
// 

#include "QoSRtsPolicy.h"

namespace inet {
namespace ieee80211 {

Define_Module(QoSRtsPolicy);

void QoSRtsPolicy::initialize(int stage)
{
    ModeSetListener::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        rtsThreshold = par("rtsThreshold");
        ctsTimeout = par("ctsTimeout");
    }
}

//
// The RTS/CTS mechanism cannot be used for MPDUs with a group addressed immediate destination because
// there are multiple recipients for the RTS, and thus potentially multiple concurrent senders of the CTS in
// response. The RTS/CTS mechanism is not used for every data frame transmission. Because the additional RTS
// and CTS frames add overhead inefficiency, the mechanism is not always justified, especially for short data
// frames.
//
// The use of the RTS/CTS mechanism is under control of the dot11RTSThreshold attribute. This attribute may
// be set on a per-STA basis. This mechanism allows STAs to be configured to initiate RTS/CTS either always,
// never, or only on frames longer than a specified length.
//
bool QoSRtsPolicy::isRtsNeeded(Ieee80211Frame* protectedFrame) const
{
    if (dynamic_cast<Ieee80211BlockAckReq*>(protectedFrame))
        return false;
    if (dynamic_cast<Ieee80211DataOrMgmtFrame*>(protectedFrame))
        return protectedFrame->getByteLength() >= rtsThreshold && !protectedFrame->getReceiverAddress().isMulticast();
    else
        return false;
}

//
// After transmitting an RTS frame, the STA shall wait for a CTSTimeout interval, with a value of aSIFSTime +
// aSlotTime + aPHY-RX-START-Delay, starting at the PHY-TXEND.confirm primitive. If a PHY-
// RXSTART.indication primitive does not occur during the CTSTimeout interval, the STA shall conclude that
// the transmission of the RTS has failed, and this STA shall invoke its backoff procedure upon expiration of the
// CTSTimeout interval.
//
simtime_t QoSRtsPolicy::getCtsTimeout(Ieee80211RTSFrame *rtsFrame) const
{
    return ctsTimeout == -1 ? modeSet->getSifsTime() + modeSet->getSlotTime() + modeSet->getPhyRxStartDelay() : ctsTimeout;
}

} /* namespace ieee80211 */
} /* namespace inet */
