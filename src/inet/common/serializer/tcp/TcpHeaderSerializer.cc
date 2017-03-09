//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "inet/common/serializer/headers/bsdint.h"
#include "inet/common/serializer/headers/defs.h"
#include "inet/common/serializer/headers/in_systm.h"
#include "inet/common/serializer/headers/in.h"
#include "inet/common/serializer/tcp/headers/tcphdr.h"
#include "inet/common/serializer/tcp/TcpHeaderSerializer.h"

#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN64)
#include <netinet/in.h>    // htonl, ntohl, ...
#endif // if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN64)

namespace inet {

namespace serializer {

Register_Serializer(TcpHeader, TcpHeaderSerializer);

void TcpHeaderSerializer::serialize(ByteOutputStream& stream, const std::shared_ptr<Chunk>& chunk) const
{
    const auto& tcpHeader = std::static_pointer_cast<const TcpHeader>(chunk);
    struct tcphdr tcp;

    // fill TCP header structure
//    if (tcpHeader->getCrcMode() != CRC_COMPUTED)      //FIXME KLUDGE
//        throw cRuntimeError("Cannot serialize TCP header without a properly computed CRC");
    tcp.th_sum = htons(tcpHeader->getCrc());
    tcp.th_sport = htons(tcpHeader->getSrcPort());
    tcp.th_dport = htons(tcpHeader->getDestPort());
    tcp.th_seq = htonl(tcpHeader->getSequenceNo());
    tcp.th_ack = htonl(tcpHeader->getAckNo());
    tcp.th_x2 = 0;     // unused

    // set flags
    unsigned char flags = 0;
    if (tcpHeader->getFinBit())
        flags |= TH_FIN;
    if (tcpHeader->getSynBit())
        flags |= TH_SYN;
    if (tcpHeader->getRstBit())
        flags |= TH_RST;
    if (tcpHeader->getPshBit())
        flags |= TH_PUSH;
    if (tcpHeader->getAckBit())
        flags |= TH_ACK;
    if (tcpHeader->getUrgBit())
        flags |= TH_URG;
    tcp.th_flags = (TH_FLAGS & flags);
    tcp.th_win = htons(tcpHeader->getWindow());
    tcp.th_urp = htons(tcpHeader->getUrgentPointer());
    if (tcpHeader->getHeaderLength() % 4 != 0)
        throw cRuntimeError("invalid TCP header length=%u: must be dividable by 4", tcpHeader->getHeaderLength());
    tcp.th_offs = tcpHeader->getHeaderLength() / 4;

    for (int i = 0; i < TCP_HEADER_OCTETS; i++)
        stream.writeByte(((uint8_t *)&tcp)[i]);

    unsigned short numOptions = tcpHeader->getHeaderOptionArraySize();
    unsigned int optionsLength = 0;
    if (numOptions > 0) {
        for (unsigned short i = 0; i < numOptions; i++) {
            const TCPOption *option = tcpHeader->getHeaderOption(i);
            serializeOption(stream, option);
            optionsLength += option->getLength();
        }
        if (optionsLength % 4 != 0)
            stream.writeByteRepeatedly(0, 4 - optionsLength % 4);
    }
    ASSERT(tcpHeader->getHeaderLength() == TCP_HEADER_OCTETS + optionsLength);
}

void TcpHeaderSerializer::serializeOption(ByteOutputStream& stream, const TCPOption *option) const
{
    unsigned short kind = option->getKind();
    unsigned short length = option->getLength();    // length >= 1

    stream.writeByte(kind);
    if (length > 1)
        stream.writeByte(length);

    auto *opt = dynamic_cast<const TCPOptionUnknown *>(option);
    if (opt) {
        unsigned int datalen = opt->getBytesArraySize();
        ASSERT(length == 2 + datalen);
        for (unsigned int i = 0; i < datalen; i++)
            stream.writeByte(opt->getBytes(i));
        return;
    }

    switch (kind) {
        case TCPOPTION_END_OF_OPTION_LIST:    // EOL
            check_and_cast<const TCPOptionEnd *>(option);
            ASSERT(length == 1);
            break;

        case TCPOPTION_NO_OPERATION:    // NOP
            check_and_cast<const TCPOptionNop *>(option);
            ASSERT(length == 1);
            break;

        case TCPOPTION_MAXIMUM_SEGMENT_SIZE: {
            auto *opt = check_and_cast<const TCPOptionMaxSegmentSize *>(option);
            ASSERT(length == 4);
            stream.writeUint16(opt->getMaxSegmentSize());
            break;
        }

        case TCPOPTION_WINDOW_SCALE: {
            auto *opt = check_and_cast<const TCPOptionWindowScale *>(option);
            ASSERT(length == 3);
            stream.writeByte(opt->getWindowScale());
            break;
        }

        case TCPOPTION_SACK_PERMITTED: {
            auto *opt = check_and_cast<const TCPOptionSackPermitted *>(option); (void)opt; // UNUSED
            ASSERT(length == 2);
            break;
        }

        case TCPOPTION_SACK: {
            auto *opt = check_and_cast<const TCPOptionSack *>(option);
            ASSERT(length == 2 + opt->getSackItemArraySize() * 8);
            for (unsigned int i = 0; i < opt->getSackItemArraySize(); i++) {
                SackItem si = opt->getSackItem(i);
                stream.writeUint32(si.getStart());
                stream.writeUint32(si.getEnd());
            }
            break;
        }

        case TCPOPTION_TIMESTAMP: {
            auto *opt = check_and_cast<const TCPOptionTimestamp *>(option);
            ASSERT(length == 10);
            stream.writeUint32(opt->getSenderTimestamp());
            stream.writeUint32(opt->getEchoedTimestamp());
            break;
        }

        default: {
            throw cRuntimeError("Unknown TCPOption kind=%d (not in a TCPOptionUnknown option)", kind);
            break;
        }
    }    // switch
}

std::shared_ptr<Chunk> TcpHeaderSerializer::deserialize(ByteInputStream& stream) const
{
    auto position = stream.getPosition();
    uint8_t buffer[TCP_HEADER_OCTETS];
    for (int i = 0; i < TCP_HEADER_OCTETS; i++)
        buffer[i] = stream.readByte();
    auto tcpHeader = std::make_shared<TcpHeader>();
    const struct tcphdr& tcp = *static_cast<const struct tcphdr *>((void *)&buffer);
    ASSERT(sizeof(tcp) == TCP_HEADER_OCTETS);

    // fill TCP header structure
    tcpHeader->setSrcPort(ntohs(tcp.th_sport));
    tcpHeader->setDestPort(ntohs(tcp.th_dport));
    tcpHeader->setSequenceNo(ntohl(tcp.th_seq));
    tcpHeader->setAckNo(ntohl(tcp.th_ack));
    unsigned short headerLength = tcp.th_offs * 4;

    // set flags
    unsigned char flags = tcp.th_flags;
    tcpHeader->setFinBit((flags & TH_FIN) == TH_FIN);
    tcpHeader->setSynBit((flags & TH_SYN) == TH_SYN);
    tcpHeader->setRstBit((flags & TH_RST) == TH_RST);
    tcpHeader->setPshBit((flags & TH_PUSH) == TH_PUSH);
    tcpHeader->setAckBit((flags & TH_ACK) == TH_ACK);
    tcpHeader->setUrgBit((flags & TH_URG) == TH_URG);

    tcpHeader->setWindow(ntohs(tcp.th_win));

    tcpHeader->setUrgentPointer(ntohs(tcp.th_urp));

    if (headerLength > TCP_HEADER_OCTETS) {
        while (stream.getPosition() - position < headerLength) {
            TCPOption *option = deserializeOption(stream);
            tcpHeader->addHeaderOption(option);
        }
    }
    tcpHeader->setHeaderLength(headerLength);
    tcpHeader->setCrc(ntohs(tcp.th_sum));
    tcpHeader->setCrcMode(CRC_COMPUTED);
    return tcpHeader;
}

TCPOption *TcpHeaderSerializer::deserializeOption(ByteInputStream& stream) const
{
    unsigned char kind = stream.readByte();
    unsigned char length = 0;

    switch (kind) {
        case TCPOPTION_END_OF_OPTION_LIST:    // EOL
            return new TCPOptionEnd();

        case TCPOPTION_NO_OPERATION:    // NOP
            return new TCPOptionNop();

        case TCPOPTION_MAXIMUM_SEGMENT_SIZE:
            length = stream.readByte();
            if (length == 4) {
                auto *option = new TCPOptionMaxSegmentSize();
                option->setLength(length);
                option->setMaxSegmentSize(stream.readUint16());
                return option;
            }
            break;

        case TCPOPTION_WINDOW_SCALE:
            length = stream.readByte();
            if (length == 3) {
                auto *option = new TCPOptionWindowScale();
                option->setLength(length);
                option->setWindowScale(stream.readByte());
                return option;
            }
            break;

        case TCPOPTION_SACK_PERMITTED:
            length = stream.readByte();
            if (length == 2) {
                auto *option = new TCPOptionSackPermitted();
                option->setLength(length);
                return option;
            }
            break;

        case TCPOPTION_SACK:
            length = stream.readByte();
            if (length > 2 && (length % 8) == 2) {
                auto *option = new TCPOptionSack();
                option->setLength(length);
                option->setSackItemArraySize(length / 8);
                unsigned int count = 0;
                for (unsigned int i = 2; i < length; i += 8) {
                    SackItem si;
                    si.setStart(stream.readUint32());
                    si.setEnd(stream.readUint32());
                    option->setSackItem(count++, si);
                }
                return option;
            }
            break;

        case TCPOPTION_TIMESTAMP:
            length = stream.readByte();
            if (length == 10) {
                auto *option = new TCPOptionTimestamp();
                option->setLength(length);
                option->setSenderTimestamp(stream.readUint32());
                option->setEchoedTimestamp(stream.readUint32());
                return option;
            }
            break;

        default:
            length = stream.readByte();
            break;
    }    // switch

    auto *option = new TCPOptionUnknown();
    option->setKind(kind);
    option->setLength(length);
    if (length > 2)
        option->setBytesArraySize(length - 2);
    for (unsigned int i = 2; i < length; i++)
        option->setBytes(i-2, stream.readByte());
    return option;
}

} // namespace serializer

} // namespace inet

