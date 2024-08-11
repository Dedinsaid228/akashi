#include "packet/packet_tt.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QQueue>

PacketTT::PacketTT(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketTT::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 3,
        .header = "TT"};
    return info;
}

void PacketTT::handlePacket(AreaData *area, AOClient &client) const { client.getServer()->broadcast(PacketFactory::createPacket("TT", {m_content}), area->index()); }
