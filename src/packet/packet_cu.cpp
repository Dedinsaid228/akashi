#include "packet/packet_cu.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QQueue>

PacketCU::PacketCU(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketCU::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 1,
        .header = "CU"};
    return info;
}

void PacketCU::handlePacket(AreaData *area, AOClient &client) const
{
    Q_UNUSED(area);

    client.sendPacket("CU", m_content);
}
