#include "include/packet/packet_rd.h"
#include "include/config_manager.h"
#include "include/server.h"

#include <QDebug>

PacketRD::PacketRD(QStringList &contents) :
    AOPacket(contents)
{
}

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

    if (client.m_joined) {
        return;
    }

    client.m_joined = true;
    client.getServer()->updateCharsTaken(area);
    client.sendEvidenceList(area);
    client.sendPacket("HP", {"1", QString::number(area->defHP())});
    client.sendPacket("HP", {"2", QString::number(area->proHP())});
    client.sendPacket("FA", client.getServer()->getAreaNames());
    // Here lies OPPASS, the genius of FanatSors who send the modpass to everyone in plain text.
    client.sendPacket("DONE");
    client.sendPacket("BN", {area->background(), client.m_pos});
    client.sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");
    client.fullArup(); // Give client all the area data
    client.getServer()->check_version();

    QString version_message = "This server uses kakashi " + QCoreApplication::applicationVersion() + ".";


    if (QCoreApplication::applicationVersion() == "unstable")
        version_message += "See: https://github.com/Ddedinya/kakashi";
    else if (client.getServer()->m_latest_version.isEmpty())
        version_message += "Unable to get the latest version.";
    else if (QCoreApplication::applicationVersion() == client.getServer()->m_latest_version)
        version_message += "It's latest version. \n";
    else
        version_message += "New version available! \n";

    if (!client.getServer()->m_latest_version.isEmpty() && QCoreApplication::applicationVersion() != "unstable")
        version_message += "See: https://github.com/Ddedinya/kakashi/releases/tag/v" + client.getServer()->m_latest_version;

    client.sendServerMessage(version_message);

    if (client.getServer()->timer->isActive()) {
        client.sendPacket("TI", {"0", "2"});
        client.sendPacket("TI", {"0", "0", QString::number(QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(client.getServer()->timer->remainingTime())))});
    }
    else {
        client.sendPacket("TI", {"0", "3"});
    }

    const QList<QTimer *> l_timers = area->timers();

    for (QTimer *l_timer : l_timers) {
        int l_timer_id = area->timers().indexOf(l_timer) + 1;
        if (l_timer->isActive()) {
            client.sendPacket("TI", {QString::number(l_timer_id), "2"});
            client.sendPacket("TI", {QString::number(l_timer_id), "0", QString::number(QTime(0, 0).msecsTo(QTime(0, 0).addMSecs(l_timer->remainingTime())))});
        }
        else {
            client.sendPacket("TI", {QString::number(l_timer_id), "3"});
        }
    }

    emit client.joined();
    client.getServer()->increasePlayerCount();
    area->clientJoinedArea(-1, client.m_id);
    client.arup(client.ARUPType::PLAYER_COUNT, true); // Tell everyone there is a new player

    if (client.m_web_client && ConfigManager::webUsersSpectableOnly())
        client.m_wuso = true;
}

bool PacketRD::validatePacket() const
{
    return true;
}
