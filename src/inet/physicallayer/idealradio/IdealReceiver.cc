//
// Copyright (C) 2013 OpenSim Ltd.
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

#include "inet/physicallayer/common/packetlevel/ListeningDecision.h"
#include "inet/physicallayer/common/packetlevel/ReceptionDecision.h"
#include "inet/physicallayer/common/packetlevel/SignalTag_m.h"
#include "inet/physicallayer/idealradio/IdealListening.h"
#include "inet/physicallayer/idealradio/IdealNoise.h"
#include "inet/physicallayer/idealradio/IdealReceiver.h"
#include "inet/physicallayer/idealradio/IdealReception.h"

namespace inet {

namespace physicallayer {

Define_Module(IdealReceiver);

IdealReceiver::IdealReceiver() :
    ReceiverBase(),
    ignoreInterference(false)
{
}

void IdealReceiver::initialize(int stage)
{
    ReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        ignoreInterference = par("ignoreInterference");
}

std::ostream& IdealReceiver::printToStream(std::ostream& stream, int level) const
{
    stream << "IdealReceiver";
    if (level <= PRINT_LEVEL_INFO)
        stream << (ignoreInterference ? ", ignoring interference" : ", considering interference");
    return stream;
}

bool IdealReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    auto power = check_and_cast<const IdealReception *>(reception)->getPower();
    return power == IdealReception::POWER_RECEIVABLE;
}

bool IdealReceiver::computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISNIR *snir) const
{
    auto power = check_and_cast<const IdealReception *>(reception)->getPower();
    if (power == IdealReception::POWER_RECEIVABLE) {
        if (ignoreInterference)
            return true;
        else {
            auto startTime = reception->getStartTime(part);
            auto endTime = reception->getEndTime(part);
            auto interferingReceptions = interference->getInterferingReceptions();
            for (auto interferingReception : *interferingReceptions) {
                auto interferingPower = check_and_cast<const IdealReception *>(interferingReception)->getPower();
                if (interferingPower >= IdealReception::POWER_INTERFERING && startTime <= interferingReception->getEndTime() && endTime >= interferingReception->getStartTime())
                    return false;
            }
            return true;
        }
    }
    else
        return false;
}

const IListening *IdealReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition) const
{
    return new IdealListening(radio, startTime, endTime, startPosition, endPosition);
}

const IListeningDecision *IdealReceiver::computeListeningDecision(const IListening *listening, const IInterference *interference) const
{
    auto interferingReceptions = interference->getInterferingReceptions();
    for (auto interferingReception : *interferingReceptions) {
        auto interferingPower = check_and_cast<const IdealReception *>(interferingReception)->getPower();
        if (interferingPower != IdealReception::POWER_UNDETECTABLE)
            return new ListeningDecision(listening, true);
    }
    return new ListeningDecision(listening, false);
}

const IReceptionResult *IdealReceiver::computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISNIR *snir, const std::vector<const IReceptionDecision *> *decisions) const
{
    auto noise = check_and_cast_nullable<const IdealNoise *>(snir->getNoise());
    double errorRate = check_and_cast<const IdealReception *>(reception)->getPower() == IdealReception::POWER_RECEIVABLE && (noise == nullptr || !noise->isInterfering()) ? 0 : 1;
    auto receptionResult = ReceiverBase::computeReceptionResult(listening, reception, interference, snir, decisions);
    auto errorRateInd = const_cast<Packet *>(receptionResult->getPacket())->ensureTag<ErrorRateInd>();
    errorRateInd->setSymbolErrorRate(errorRate);
    errorRateInd->setBitErrorRate(errorRate);
    errorRateInd->setPacketErrorRate(errorRate);
    return receptionResult;
}

} // namespace physicallayer

} // namespace inet

