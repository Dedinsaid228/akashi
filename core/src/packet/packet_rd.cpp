#include "include/packet/packet_rd.h"
#include "include/config_manager.h"
#include "include/hub_data.h"
#include "include/server.h"

#include <QDebug>

PacketRD::PacketRD(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketRD::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 0,
        .header = "RD"};
    return info;
}

void PacketRD::handlePacket(AreaData *area, AOClient &client) const
{
    if (client.m_hwid == "") {
        // No early connecting!
        client.m_socket->close();
        return;
    }

    if (client.m_joined)
        return;

    client.m_joined = true;
    client.getServer()->updateCharsTaken(area);
    client.sendEvidenceList(area);
    client.getAreaList();
    client.sendPacket("HP", {"1", QString::number(area->defHP())});
    client.sendPacket("HP", {"2", QString::number(area->proHP())});
    client.sendPacket("FA", client.getServer()->getClientAreaNames(0));
    // Here lies OPPASS, the genius of FanatSors who send the modpass to everyone in plain text.
    client.sendPacket("DONE");
    client.sendPacket("BN", {area->background(), client.m_pos});
    client.sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");
    client.fullArup(); // Give client all the area data
    client.getServer()->check_version();

    if (client.getServer()->timer->isActive()) {
        client.sendPacket("TI", {"0", "2"});
        client.sendPacket("TI", {"0", "0", QString::number(QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(client.getServer()->timer->remainingTime())))});
    }
    else
        client.sendPacket("TI", {"0", "3"});

    const QList<QTimer *> l_timers = area->timers();
    for (QTimer *l_timer : l_timers) {
        int l_timer_id = area->timers().indexOf(l_timer) + 1;
        if (l_timer->isActive()) {
            client.sendPacket("TI", {QString::number(l_timer_id), "2"});
            client.sendPacket("TI", {QString::number(l_timer_id), "0", QString::number(QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_timer->remainingTime())))});
        }
        else
            client.sendPacket("TI", {QString::number(l_timer_id), "3"});
    }

    emit client.joined();
    client.getServer()->increasePlayerCount();
    client.getServer()->getHubById(client.m_hub)->clientJoinedHub();
    area->clientJoinedArea(-1, client.m_id);
    client.arup(client.ARUPType::PLAYER_COUNT, true, 0); // Tell everyone there is a new player

    if (client.m_web_client && ConfigManager::webUsersSpectableOnly())
        client.m_wuso = true;

    QString info_message = "This server works on kakashi " + QCoreApplication::applicationVersion() + ". ";
    if (QCoreApplication::applicationVersion() == "unstable")
        info_message += "Github: https://github.com/Ddedinya/kakashi \n";
    else if (client.getServer()->m_latest_version.isEmpty())
        info_message += "Unable to get the latest version. \n";
    else if (QCoreApplication::applicationVersion() == client.getServer()->m_latest_version)
        info_message += "It's latest version. \n";
    else
        info_message += "New version available! \n";

    if (!client.getServer()->m_latest_version.isEmpty() && QCoreApplication::applicationVersion() != "unstable")
        info_message += "Github: https://github.com/Ddedinya/kakashi/releases/tag/v" + client.getServer()->m_latest_version + " \n";

    info_message += "Built on Qt " + QLatin1String(QT_VERSION_STR) + ". Build date: " + QLatin1String(__DATE__);

    QStringList l_hub_list;
    for (int i = 0; i < client.getServer()->getHubsCount(); i++) {
        HubData *l_hub = client.getServer()->getHubById(i);
        QString l_playercount;
        if (l_hub->getHidePlayerCount())
            l_playercount = "?";
        else
            l_playercount = QString::number(l_hub->getHubPlayerCount());

        l_hub_list.append("[" + QString::number(i) + "] " + client.getServer()->getHubName(i) + " with " + l_playercount + " players.");
    }

    info_message += "\nYou are in hub [" + QString::number(client.m_hub) + "] " + client.getServer()->getHubName(client.m_hub) + "\nHub list:\n" + l_hub_list.join("\n") + "\nTo view a more detailed list, you can use /hub command.";
    client.sendServerMessage(info_message);
}

bool PacketRD::validatePacket() const { return true; }
