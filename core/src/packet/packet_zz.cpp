#include "include/packet/packet_zz.h"
#include "include/config_manager.h"
#include "include/packet/packet_factory.h"
#include "include/server.h"

#include <QQueue>

PacketZZ::PacketZZ(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketZZ::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 0,
        .header = "ZZ"};
    return info;
}

void PacketZZ::handlePacket(AreaData *area, AOClient &client) const
{
    QString l_modcallNotice = "!!!MODCALL!!!\nArea: [" +
                              QString::number(client.m_area_list.indexOf(client.m_current_area)) + "] " + area->name() +
                              "\nHub: [" + QString::number(client.m_hub) + "] " + client.getServer()->getHubName(client.m_hub) +
                              "\nCaller: [" + QString::number(client.m_id) + "] " + client.getSenderName(client.m_id) + "(" + client.m_ipid + ")\n";

    if (!client.m_usemodcall) {
        client.sendServerMessage("You cannot use Mod Call anymore. Be patient.");
        return;
    }

    if (m_content.size() > 0 && !m_content[0].isEmpty())
        l_modcallNotice.append("Reason: " + m_content[0]);
    else
        l_modcallNotice.append("No reason given.");

    const QVector<AOClient *> l_clients = client.getServer()->getClients();
    for (AOClient *l_client : l_clients)
        if (l_client->m_authenticated)
            l_client->sendPacket(PacketFactory::createPacket("ZZ", {l_modcallNotice}));

    emit client.logModcall((client.m_current_char + " " + client.m_showname), client.m_ipid, client.m_ooc_name, area->name(), QString::number(client.m_id), client.m_hwid, client.getServer()->getHubName(client.m_hub));

    if (ConfigManager::discordModcallWebhookEnabled())
        emit client.getServer()->modcallWebhookRequest(client.getSenderName(client.m_id), client.getServer()->getHubName(client.m_hub), area->name(), m_content.value(0), client.getServer()->getAreaBuffer(area->name()));

    if (client.m_wuso)
        client.m_usemodcall = false;
}

bool PacketZZ::validatePacket() const { return true; }
