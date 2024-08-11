#ifndef PACKET_CU_H
#define PACKET_CU_H

#include "network/aopacket.h"

class PacketCU : public AOPacket
{
  public:
    PacketCU(QStringList &contents);
    virtual PacketInfo getPacketInfo() const;
    virtual void handlePacket(AreaData *area, AOClient &client) const;
};

#endif
