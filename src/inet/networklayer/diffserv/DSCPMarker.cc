//
// Copyright (C) 2012 Opensim Ltd.
// Author: Tamas Borbely
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

#include "inet/networklayer/diffserv/DSCPMarker.h"

#include "inet/common/ProtocolTag_m.h"

#ifdef WITH_IPv4
#include "inet/networklayer/ipv4/IPv4Header.h"
#endif // ifdef WITH_IPv4

#ifdef WITH_IPv6
#include "inet/networklayer/ipv6/IPv6Header.h"
#endif // ifdef WITH_IPv6

#include "inet/networklayer/diffserv/DSCP_m.h"

#include "inet/networklayer/diffserv/DiffservUtil.h"

namespace inet {

using namespace DiffservUtil;

Define_Module(DSCPMarker);

simsignal_t DSCPMarker::markPkSignal = registerSignal("markPk");

void DSCPMarker::initialize()
{
    parseDSCPs(par("dscps"), "dscps", dscps);
    if (dscps.empty())
        dscps.push_back(DSCP_BE);
    while ((int)dscps.size() < gateSize("in"))
        dscps.push_back(dscps.back());

    numRcvd = 0;
    numMarked = 0;
    WATCH(numRcvd);
    WATCH(numMarked);
}

void DSCPMarker::handleMessage(cMessage *msg)
{
    Packet *packet = check_and_cast<Packet *>(msg);
    numRcvd++;
    int dscp = dscps.at(msg->getArrivalGate()->getIndex());
    if (markPacket(packet, dscp)) {
        emit(markPkSignal, packet);
        numMarked++;
    }

    send(packet, "out");
}
void DSCPMarker::refreshDisplay() const
{
    char buf[50] = "";
    if (numRcvd > 0)
        sprintf(buf + strlen(buf), "rcvd: %d ", numRcvd);
    if (numMarked > 0)
        sprintf(buf + strlen(buf), "mark:%d ", numMarked);
    getDisplayString().setTagArg("t", 0, buf);
}

bool DSCPMarker::markPacket(Packet *packet, int dscp)
{
    EV_DETAIL << "Marking packet with dscp=" << dscpToString(dscp) << "\n";

    auto protocol = packet->getMandatoryTag<PacketProtocolTag>()->getProtocol();

    //TODO processing link-layer headers when exists

#ifdef WITH_IPv4
    if (protocol == &Protocol::ipv4) {
        auto ipv4Header = std::dynamic_pointer_cast<IPv4Header>(packet->popHeader<IPv4Header>()->dupShared());
        packet->removePoppedHeaders();
        ipv4Header->setDiffServCodePoint(dscp);
        ipv4Header->markImmutable();
        packet->pushHeader(ipv4Header);
        return true;
    }
#endif // ifdef WITH_IPv4
#ifdef WITH_IPv6
    if (protocol == &Protocol::ipv6) {
        auto ipv6Header = std::dynamic_pointer_cast<IPv6Header>(packet->popHeader<IPv6Header>()->dupShared());
        packet->removePoppedHeaders();
        ipv6Header->setDiffServCodePoint(dscp);
        ipv6Header->markImmutable();
        packet->pushHeader(ipv6Header);
        return true;
    }
#endif // ifdef WITH_IPv6

    return false;
}

} // namespace inet
