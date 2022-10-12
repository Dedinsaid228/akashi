//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "include/aoclient.h"

#include "include/area_data.h"
#include "include/config_manager.h"
#include "include/music_manager.h"
#include "include/packet/packet_factory.h"
#include "include/server.h"

// This file is for commands under the area category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdCM(int argc, QStringList argv)
{
    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->isProtected()) {
        sendServerMessage("This area is protected, you may not become CM.");
        return;
    }
    else if (l_area->owners().contains(m_id) && argc == 0) {
        sendServerMessage("You are already a CM in this area.");
        return;
    }
    else if (l_area->owners().isEmpty()) { // no one owns this area, and it's not protected
        l_area->addOwner(m_id);

        QString l_sender_name = getSenderName(m_id);

        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " is now CM in this area.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "NEW AREA OWNER", "Owner UID: " + QString::number(m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        arup(ARUPType::CM, true);
        sendEvidenceList(server->getAreaById(m_current_area));
    }
    else if (!l_area->owners().contains(m_id)) { // there is already a CM, and it isn't us
        sendServerMessage("You cannot become a CM in this area.");
    }
    else if (argc == 1) { // we are CM, and we want to make ID argv[0] also CM
        bool l_ok;
        AOClient *l_owner_candidate = server->getClientByID(argv[0].toInt(&l_ok));
        if (!l_ok) {
            sendServerMessage("That doesn't look like a valid ID.");
            return;
        }
        if (l_owner_candidate == nullptr) {
            sendServerMessage("Unable to find client with ID " + argv[0] + ".");
            return;
        }
        if (l_area->owners().contains(l_owner_candidate->m_id)) {
            sendServerMessage("This client are already a CM in this area.");
            return;
        }
        l_area->addOwner(l_owner_candidate->m_id);

        QString l_sender_name = getSenderName(l_owner_candidate->m_id);

        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " is now CM in this area.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "NEW AREA OWNER", "Owner UID: " + QString::number(l_owner_candidate->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        arup(ARUPType::CM, true);
        sendEvidenceList(server->getAreaById(m_current_area));
    }
    else
        sendServerMessage("You are already a CM in this area.");
}

void AOClient::cmdUnCM(int argc, QStringList argv)
{
    AreaData *l_area = server->getAreaById(m_current_area);
    int l_uid;

    if (l_area->owners().isEmpty()) {
        sendServerMessage("There are no CMs in this area.");
        return;
    }
    else if (argc == 0) {
        l_uid = m_id;
        QString l_sender_name = getSenderName(m_id);

        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " no longer CM in this area.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "REMOVE AREA OWNER", "Owner UID: " + QString::number(m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        sendServerMessage("You are no longer CM in this area.");
    }
    else {
        bool l_conv_ok = false;
        l_uid = argv[0].toInt(&l_conv_ok);

        if (!l_conv_ok) {
            sendServerMessage("Invalid user ID.");
            return;
        }

        if (!l_area->owners().contains(l_uid)) {
            sendServerMessage("That user is not CMed.");
            return;
        }

        AOClient *target = server->getClientByID(l_uid);

        if (target == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }

        QString l_sender_name = getSenderName(target->m_id);

        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " no longer CM in this area.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "REMOVE AREA OWNER", "Owner UID: " + QString::number(target->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        target->sendServerMessage("You have been unCMed.");
    }

    if (l_area->removeOwner(l_uid)) {
        arup(ARUPType::LOCKED, true);
    }

    arup(ARUPType::CM, true);
    sendEvidenceList(server->getAreaById(m_current_area));
}

void AOClient::cmdInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *area = server->getAreaById(m_current_area);
    bool l_ok;
    int l_invited_id = argv[0].toInt(&l_ok);

    if (!l_ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *target_client = server->getClientByID(l_invited_id);

    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (!area->invite(l_invited_id)) {
        sendServerMessage("That ID is already on the invite list.");
        return;
    }

    sendServerMessage("You invited ID " + argv[0]);
    target_client->sendServerMessage("You were invited and given access to " + area->name());
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "IVNITE", "Invited UID: " + QString::number(target_client->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdUnInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(m_current_area);
    bool l_ok;
    int l_uninvited_id = argv[0].toInt(&l_ok);

    if (!l_ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *target_client = server->getClientByID(l_uninvited_id);

    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (l_area->owners().contains(l_uninvited_id)) {
        sendServerMessage("You cannot uninvite a CM!");
        return;
    }
    else if (!l_area->uninvite(l_uninvited_id)) {
        sendServerMessage("That ID is not on the invite list.");
        return;
    }

    sendServerMessage("You uninvited ID " + argv[0]);
    target_client->sendServerMessage("You were uninvited from " + l_area->name());
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "UNIVNITE", "Uninvited UID: " + QString::number(target_client->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->lockStatus() == AreaData::LockStatus::LOCKED) {
        sendServerMessage("This area is already locked.");
        return;
    }

    sendServerMessageArea("This area is now locked.");
    l_area->lock();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == m_current_area && l_client->hasJoined()) {
            l_area->invite(l_client->m_id);
        }
    }

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREALOCK", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdSpectatable(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }

    sendServerMessageArea("This area is now spectatable.");
    l_area->spectatable();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == m_current_area && l_client->hasJoined()) {
            l_area->invite(l_client->m_id);
        }
    }

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREASPECTATABLE", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdAreaMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }

    sendServerMessageArea("This area is now muted.");
    l_area->spectatable();
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREAMUTE", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}
void AOClient::cmdUnLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->lockStatus() == AreaData::LockStatus::FREE) {
        sendServerMessage("This area is not locked.");
        return;
    }

    sendServerMessageArea("This area is now unlocked.");
    l_area->unlock();
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREAUNLOCK", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdGetAreas(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries;

    l_entries.append("== Area List ==");

    l_entries.append("\n== Currently Online: " + QString::number(server->getPlayerCount()) + " ==");
    for (int i = 0; i < server->getAreaCount(); i++) {
        if (server->getAreaById(i)->playerCount() > 0) {
            QStringList l_cur_area_lines = buildAreaList(i);
            l_entries.append(l_cur_area_lines);
        }
    }

    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdGetArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries = buildAreaList(m_current_area);

    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_new_area = argv[0].toInt(&ok);

    if (!ok || l_new_area >= server->getAreaCount() || l_new_area < 0) {
        sendServerMessage("That does not look like a valid area ID.");
        return;
    }

    changeArea(l_new_area);
}

void AOClient::cmdAreaKick(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(m_current_area);
    bool ok;
    int l_idx = argv[0].toInt(&ok);

    if (!ok && argv[0] != "*") {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }
    else if (ok) {

        AOClient *l_client_to_kick = server->getClientByID(l_idx);

        if (l_client_to_kick == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }
        else if (l_client_to_kick->m_current_area != m_current_area) {
            sendServerMessage("That client is not in this area.");
            return;
        }

        l_client_to_kick->changeArea(0);
        l_area->uninvite(l_client_to_kick->m_id);
        l_area->removeOwner(l_client_to_kick->m_id);
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREAKICK", "Kicked UID: " + QString::number(l_client_to_kick->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        arup(ARUPType::CM, true);
        sendServerMessage("Client " + argv[0] + " kicked from area.");
        l_client_to_kick->sendServerMessage("You kicked from area.");
    }
    else if (argv[0] == "*") { // kick all clients in the area
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREAKICK", "Kicked all players from area", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients) {
            if (l_client->m_current_area == m_current_area && l_client->m_id != m_id) {
                l_client->changeArea(0);
                l_area->uninvite(l_client->m_id);
                l_area->removeOwner(l_client->m_id);
                l_client->sendServerMessage("You kicked from area.");
            }
            arup(ARUPType::CM, true);
            sendServerMessage("Clients kicked from area.");
        }
    }
}

void AOClient::cmdModAreaKick(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(m_current_area);
    bool ok;
    int l_idx = argv[0].toInt(&ok);
    int l_idz = argv[1].toInt(&ok);

    if (!ok || l_idz >= server->getAreaCount() || l_idz < 0) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *l_client_to_kick = server->getClientByID(l_idx);

    if (l_client_to_kick == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    l_client_to_kick->changeArea(l_idz);
    l_area->uninvite(l_client_to_kick->m_id);
    l_area->removeOwner(l_client_to_kick->m_id);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "MODAREAKICK", "Kicked UID: " + QString::number(l_client_to_kick->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::CM, true);
    sendServerMessage("Client " + argv[0] + " kicked from area.");
    l_client_to_kick->sendServerMessage("You kicked from area by moderator.");
}

void AOClient::cmdSetBackground(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = getSenderName(m_id);
    AreaData *area = server->getAreaById(m_current_area);
    QString f_background = argv.join(" ");

    if (checkPermission(ACLRole::CM) || !area->bgLocked()) {
        if (server->getBackgrounds().contains(f_background), Qt::CaseInsensitive || area->ignoreBgList() == true) {
            area->setBackground(f_background);

            const QVector<AOClient *> l_clients = server->getClients();
            for (AOClient *l_client : l_clients) {
                if (l_client->m_current_area == m_current_area && !l_client->m_blinded)
                    l_client->sendPacket(PacketFactory::createPacket("BN", {f_background, l_client->m_pos}));
            }

            sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " changed the background to " + f_background);
            emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "SETBACKGROUND", "Client UID: " + QString::number(m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
        }
        else
            sendServerMessage("Invalid background name.");
    }
    else
        sendServerMessage("This area's background is locked.");
}

void AOClient::cmdBgLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QString l_sender_name = getSenderName(m_id);
    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->bgLocked() == false) {
        l_area->toggleBgLock();
    };

    server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverName(), "[" + QString::number(m_id) + "] " + l_sender_name + " locked the background.", "1"}), m_current_area);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "BGLOCK", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdBgUnlock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QString l_sender_name = getSenderName(m_id);
    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->bgLocked() == true) {
        l_area->toggleBgLock();
    };

    server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverName(), "[" + QString::number(m_id) + "] " + l_sender_name + " unlocked the background.", "1"}), m_current_area);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "BGUNLOCK", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdStatus(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = getSenderName(m_id);
    AreaData *l_area = server->getAreaById(m_current_area);
    QString l_arg = argv[0].toLower();

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_status_change_time < 2) {
        sendServerMessage("You change status very often!");
        return;
    }

    m_last_status_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (!l_area->allowChangeStatus() && !checkPermission(ACLRole::CM)) {
        sendServerMessage("Change of status is prohibited in this area.");
        return;
    }

    if (l_area->changeStatus(l_arg)) {
        arup(ARUPType::STATUS, true);
        server->broadcast(PacketFactory::createPacket("CT", {ConfigManager::serverName(), "[" + QString::number(m_id) + "] " + l_sender_name + " changed status to " + l_arg.toUpper(), "1"}), m_current_area);
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "SETSTATUS", l_arg.toUpper(), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    }
    else {
        const QStringList keys = AreaData::map_statuses.keys();
        sendServerMessage("That does not look like a valid status. Valid statuses are " + keys.join(", "));
    }
}

void AOClient::cmdJudgeLog(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->judgelog().isEmpty()) {
        sendServerMessage("There have been no judge actions in this area.");
        return;
    }

    QString l_message = l_area->judgelog().join("\n");
    // Judgelog contains an IPID, so we shouldn't send that unless the caller has appropriate permissions

    if (checkPermission(ACLRole::KICK) || checkPermission(ACLRole::BAN))
        sendServerMessage(l_message);
    else {
        QString filteredmessage = l_message.remove(QRegularExpression("[(].*[)]")); // Filter out anything between two parentheses. This should only ever be the IPID
        sendServerMessage(filteredmessage);
    }
}

void AOClient::cmdIgnoreBgList(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *area = server->getAreaById(m_current_area);

    area->toggleIgnoreBgList();
    QString state = area->ignoreBgList() ? "ignored." : "enforced.";
    sendServerMessage("BG list in this area is now " + state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "IGNOREBGLIST", state, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdAreaMessage(int argc, QStringList argv)
{
    AreaData *l_area = server->getAreaById(m_current_area);

    if (argc == 0) {
        sendServerMessage(l_area->areaMessage());
        return;
    }

    if (argc >= 1) {
        l_area->changeAreaMessage(argv.join(" "));
        sendServerMessage("Updated this area's message.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "UPDATEAREAMESSAGE", argv.join(" "), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
    }
}

void AOClient::cmdToggleAreaMessageOnJoin(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleAreaMessageJoin();

    QString l_state = l_area->sendAreaMessageOnJoin() ? "enabled." : "disabled.";

    sendServerMessage("Sending message on area join is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEAREAMESSAGE", l_state, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);
    l_area->toggleWtceAllowed();
    QString l_state = l_area->isWtceAllowed() ? "enabled." : "disabled.";
    sendServerMessage("Using testimony animations is now " + l_state);
}

void AOClient::cmdToggleShouts(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);
    l_area->toggleShoutAllowed();
    QString l_state = l_area->isShoutAllowed() ? "enabled." : "disabled.";
    sendServerMessage("Using shouts is now " + l_state);
}

void AOClient::cmdClearAreaMessage(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->changeAreaMessage(QString{});

    if (l_area->sendAreaMessageOnJoin())              // Turn off the automatic sending.
        cmdToggleAreaMessageOnJoin(0, QStringList{}); // Dummy values.

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "CLEARAREAMESSAGE", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdSneak(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_sneaked = !m_sneaked;

    QString l_str_en = m_sneaked ? "now sneaking (area transfer announcements will now be hidden)" : "no longer sneaking (area transfer announcements will no longer be hidden)";

    sendServerMessage("You are " + l_str_en + ".");
}

void AOClient::cmdCurrentEvimod(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("Current evidence mod: " + getEviMod(m_current_area));
}

void AOClient::cmdBgs(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_bgs = server->getBackgrounds();

    sendServerMessage("Backgrounds:\n" + l_bgs.join("\n"));
}

void AOClient::cmdCurBg(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);
    QString l_background = l_area->background();

    sendServerMessage("Current background: " + l_background);
}

void AOClient::cmdToggleFloodguardActuve(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleFloodguardActive();

    QString l_state = l_area->floodguardActive() ? "actived." : "disabled.";

    sendServerMessage("Floodguard in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEFLOODGUARD", l_state, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleChillMod(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleChillMod();

    QString l_state = l_area->chillMod() ? "actived." : "disabled.";

    sendServerMessage("Chill Mod in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLECHILLMOD", l_state, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleAutoMod(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleAutoMod();

    QString l_state = l_area->autoMod() ? "actived." : "disabled.";

    sendServerMessage("Automoderator in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEAUTOMOD", l_state, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdSetAreaPassword(int argc, QStringList argv)
{
    AreaData *l_area = server->getAreaById(m_current_area);

    if (argc == 0)
        sendServerMessage("Area password: " + l_area->areaPassword());
    else if (argv[0] == "\"\"") {
        l_area->setAreaPassword("");
        sendServerMessage("Area password cleaned!");
    }
    else {
        l_area->setAreaPassword(argv[0]);
        sendServerMessage("New area password: " + argv[0]);
    }
}

void AOClient::cmdSetClientPassword(int argc, QStringList argv)
{
    if (argc == 0)
        sendServerMessage("Password: " + m_password);
    else if (argv[0] == "\"\"") {
        m_password = "";
        sendServerMessage("Password cleaned!");
    }
    else {
        m_password = argv[0];
        sendServerMessage("Set password: " + argv[0]);
    }
}

void AOClient::cmdRenameArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    for (int i = 0; i < server->getAreaCount(); i++) {
        QString area = server->getAreaName(i);
        QString l_area_name = dezalgo(argv.join(" "));

        if (server->getAreaNames().contains(l_area_name)) {
            sendServerMessage("An area with that name already exists!");
            return;
        }

        if (area == server->getAreaName(m_current_area)) {
            emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "RENAMEAREA", l_area_name, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
            server->renameArea(l_area_name, i);
            server->broadcast(PacketFactory::createPacket("FA", server->getAreaNames()));

            const QVector<AOClient *> l_clients = server->getClients();
            for (AOClient *l_client : l_clients)
                l_client->fullArup();

            sendServerMessage("Area has been renamed!");
            return;
        }
    }

    sendServerMessage("For unknown reasons area could not be renamed.");
}

void AOClient::cmdCreateArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    if (server->getAreaCount() == 100) {
        sendServerMessage("The limit on the number of areas has been reached... what are you doing?");
        return;
    }

    QString l_area_name = dezalgo(argv.join(" "));

    if (server->getAreaNames().contains(l_area_name)) {
        sendServerMessage("An area with that name already exists!");
        return;
    }

    server->addArea(l_area_name, server->getAreaCount());
    server->broadcast(PacketFactory::createPacket("FA", server->getAreaNames()));

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        l_client->fullArup();

    sendServerMessage("Area " + argv.join(" ") + " has been created!");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "CREATEAREA", l_area_name, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdRemoveArea(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_area = argv[0].toInt(&ok);

    if (!ok || l_area >= server->getAreaCount() || l_area < 0) {
        sendServerMessage("That does not look like a valid area ID.");
        return;
    }

    if (l_area == 0) {
        sendServerMessage("You cannot delete area 0!");
        return;
    }

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == l_area)
            l_client->changeArea(0, true);
    }

    server->removeArea(l_area);
    server->broadcast(PacketFactory::createPacket("FA", server->getAreaNames()));

    for (AOClient *l_client : l_clients)
        l_client->fullArup();

    sendServerMessage("Area " + QString::number(l_area) + " has been removed!");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "DELETEAREA", QString::number(l_area), server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdSaveAreas(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool l_permission_found = false;
    if (checkPermission(ACLRole::SAVEAREA))
        l_permission_found = true;

    if (m_area_saving == true)
        l_permission_found = true;

    if (l_permission_found) {
        QString l_areas_ini_name = dezalgo(argv.join(" "));
        QFile new_areas_ini;

        new_areas_ini.setFileName(QString("config/areas_%1.ini").arg(l_areas_ini_name));

        if (new_areas_ini.exists()) {
            sendServerMessage("A file with the same name already exists!");
            return;
        }

        if (new_areas_ini.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream file_stream(&new_areas_ini);
            file_stream.setCodec("UTF-8");
            for (int i = 0; i < server->getAreaCount(); i++) {
                AreaData *l_area = server->getAreaById(i);
                QStringList l_evidence_list;
                QString l_evidence_format("%1%2name%3desc%4image");
                int l_evidence_count = 0;
                const QList<AreaData::Evidence> l_area_evidence = l_area->evidence();
                for (const AreaData::Evidence &evidence : l_area_evidence) {
                    l_evidence_list.append(l_evidence_format.arg(QString::number(l_evidence_count), evidence.name, evidence.description, evidence.image));
                    l_evidence_count++;
                }
                file_stream << "[" + QString::number(i) + ":" + server->getAreaName(i) + "]" +
                                   "\nbackground=" + QVariant(l_area->background()).toString() +
                                   "\nprotected_area=" + QVariant(l_area->isProtected()).toString() +
                                   "\niniswap_allowed=" + QVariant(l_area->iniswapAllowed()).toString() +
                                   "\nevidence_mod=" + getEviMod(i) +
                                   "\nblankposting_allowed=" + QVariant(l_area->blankpostingAllowed()).toString() +
                                   "\nforce_immediate=" + QVariant(l_area->forceImmediate()).toString() +
                                   "\nchillmod=" + QVariant(l_area->chillMod()).toString() +
                                   "\nautomod=" + QVariant(l_area->autoMod()).toString() +
                                   "\nfloodguard_active=" + QVariant(l_area->floodguardActive()).toString() +
                                   "\nignore_bglist=" + QVariant(l_area->ignoreBgList()).toString() +
                                   "\npassword=" + l_area->areaPassword() +
                                   "\nevidence=" + l_evidence_list.join(",") +
                                   "\nmusiclist=" + m_music_manager->getCustomMusicList(i).join(",") + "\n\n";
            }
        }
        new_areas_ini.close();

        sendServerMessage("New areas.ini file created!");
        m_area_saving = false;
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "SAVEAREAS", l_areas_ini_name, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
    }
    else {
        sendServerMessage("You don't have permission to save a areas. Please contact a moderator for permission.");
        return;
    }
}

void AOClient::cmdPermitAreaSaving(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AOClient *l_client = server->getClientByID(argv[0].toInt());

    if (l_client == nullptr) {
        sendServerMessage("Invalid ID.");
        return;
    }

    l_client->m_area_saving = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "ALLOWAREASAVING", "Target UID: " + QString::number(l_client->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdSwapAreas(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok1, ok2;
    int l_area1 = argv[0].toInt(&ok1), l_area2 = argv[1].toInt(&ok2);

    if (!ok1 || !ok2 || l_area1 >= server->getAreaCount() || l_area2 >= server->getAreaCount() || l_area1 < 0 || l_area2 < 0) {
        sendServerMessage("That does not look like a valid area ID.");
        return;
    }

    if (l_area1 == 0 || l_area2 == 0) {
        sendServerMessage("You cannot swap area 0!");
        return;
    }

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == l_area1 || l_client->m_current_area == l_area2)
            l_client->changeArea(0, true);
    }

    server->swapAreas(l_area1, l_area2);
    server->broadcast(PacketFactory::createPacket("FA", server->getAreaNames()));

    for (AOClient *l_client : l_clients)
        l_client->fullArup();

    sendServerMessage("The areas " + QString::number(l_area1) + " and " + QString::number(l_area2) + " have been swapped.");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "SWAPAREAS", QString::number(l_area1) + " and " + QString::number(l_area2), server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleProtected(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleIsProtected();

    QString l_state = l_area->isProtected() ? "protected." : "not protected.";

    sendServerMessage("This area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEPROTECTED", l_state, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleStatus(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleChangeStatus();

    QString l_state = l_area->allowChangeStatus() ? "allowed." : "prohibited.";

    sendServerMessage("Change area status " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLESTATUS", l_state, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdOocType(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(m_current_area);
    argv[0] = argv[0].toLower();

    if (argv[0] == "all")
        l_area->setOocType(AreaData::OocType::ALL);
    else if (argv[0] == "invited")
        l_area->setOocType(AreaData::OocType::INVITED);
    else if (argv[0] == "cm")
        l_area->setOocType(AreaData::OocType::CM);
    else {
        sendServerMessage("Invalid OOC chat type.");
        return;
    }
    sendServerMessage("Changed OOC chat type.");
}
