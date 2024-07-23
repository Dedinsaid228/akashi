#include "packet/packet_zz.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QQueue>

PacketZZ::PacketZZ(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketZZ::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "ZZ"};
    return info;
}

void PacketZZ::handlePacket(AreaData *area, AOClient &client) const
{
    QString l_modcallNotice = "!!!MODCALL!!!\nArea: [" +
                              QString::number(client.m_area_list.indexOf(client.areaId())) + "] " + area->name() +
                              "\nHub: [" + QString::number(client.hubId()) + "] " + client.getServer()->getHubName(client.hubId()) +
                              "\nCaller: [" + QString::number(client.clientId()) + "] " + client.getSenderName(client.clientId()) + "(" + client.m_ipid + ")\n";

    if (!client.m_usemodcall) {
        client.sendServerMessage("You cannot use Mod Call anymore. Be patient.");
        return;
    }

    int target_id = m_content.at(1).toInt();
    if (target_id != -1) {
        AOClient *target = client.getServer()->getClientByID(target_id);
        if (target) {
            l_modcallNotice.append("Regarding: " + target->name() + "\n");
        }
    }
    l_modcallNotice.append("Reason: " + m_content[0]);

    const QVector<AOClient *> l_clients = client.getServer()->getClients();
    for (AOClient *l_client : l_clients)
        if (l_client->m_authenticated)
            l_client->sendPacket(PacketFactory::createPacket("ZZ", {l_modcallNotice}));

    QString webhook_reason = m_content.value(0);
    if (target_id != -1) {
        AOClient *target = client.getServer()->getClientByID(target_id);
        if (target) {
            webhook_reason.append(" (Regarding: " + target->name() + ")");
        }
    }

    emit client.logModcall((client.character() + " " + client.characterName()), client.m_ipid, client.name(), area->name(), QString::number(client.clientId()), client.m_hwid, client.getServer()->getHubName(client.hubId()));

    if (ConfigManager::discordModcallWebhookEnabled())
        emit client.getServer()->modcallWebhookRequest(client.getSenderName(client.clientId()), client.getServer()->getHubName(client.hubId()), area->name(), webhook_reason, client.getServer()->getAreaBuffer(area->name()));

    if (client.m_wuso)
        client.m_usemodcall = false;
}
