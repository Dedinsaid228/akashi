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

// This file is for commands under the area category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdCM(int argc, QStringList argv)
{
    AreaData* l_area = server->m_areas[m_current_area];

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
        emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"NEW AREA OWNER","Owner UID: " + QString::number(m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
        arup(ARUPType::CM, true);
    }
    else if (!l_area->owners().contains(m_id)) { // there is already a CM, and it isn't us
        sendServerMessage("You cannot become a CM in this area.");
    }
    else if (argc == 1) { // we are CM, and we want to make ID argv[0] also CM
        bool l_ok;
        AOClient* l_owner_candidate = server->getClientByID(argv[0].toInt(&l_ok));
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
        emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"NEW AREA OWNER","Owner UID: " + QString::number(l_owner_candidate->m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
        arup(ARUPType::CM, true);
    }
    else {
        sendServerMessage("You are already a CM in this area.");
    }
}

void AOClient::cmdUnCM(int argc, QStringList argv)
{
    AreaData* l_area = server->m_areas[m_current_area];
    int l_uid;

    if (l_area->owners().isEmpty()) {
        sendServerMessage("There are no CMs in this area.");
        return;
    }
    else if (argc == 0) {
        l_uid = m_id;
        QString l_sender_name = getSenderName(m_id);

        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " no longer CM in this area.");
        emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"REMOVE AREA OWNER","Owner UID: " + QString::number(m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
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
        AOClient* target = server->getClientByID(l_uid);
        if (target == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }
        QString l_sender_name = getSenderName(target->m_id);

        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " no longer CM in this area.");
        emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"REMOVE AREA OWNER","Owner UID: " + QString::number(target->m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
        target->sendServerMessage("You have been unCMed.");
    }

    if (l_area->removeOwner(l_uid)) {
        arup(ARUPType::LOCKED, true);
    }

    arup(ARUPType::CM, true);
}

void AOClient::cmdInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData* area = server->m_areas[m_current_area];
    bool l_ok;
    int l_invited_id = argv[0].toInt(&l_ok);
    if (!l_ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* target_client = server->getClientByID(l_invited_id);
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
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"IVNITE","Invited UID: " + QString::number(target_client->m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdUnInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData* l_area = server->m_areas[m_current_area];
    bool l_ok;
    int l_uninvited_id = argv[0].toInt(&l_ok);
    if (!l_ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* target_client = server->getClientByID(l_uninvited_id);
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
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"UNIVNITE","Uninvited UID: " + QString::number(target_client->m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];
    if (l_area->lockStatus() == AreaData::LockStatus::LOCKED) {
        sendServerMessage("This area is already locked.");
        return;
    }
    sendServerMessageArea("This area is now locked.");
    l_area->lock();
    for (AOClient* client : qAsConst(server->m_clients)) { // qAsConst here avoids detaching the container
        if (client->m_current_area == m_current_area && client->m_joined) {
            l_area->invite(client->m_id);
        }
    }
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"AREALOCK","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdSpectatable(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];
    if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }
    sendServerMessageArea("This area is now spectatable.");
    l_area->spectatable();
    for (AOClient* client : qAsConst(server->m_clients)) {
        if (client->m_current_area == m_current_area && client->m_joined) {
            l_area->invite(client->m_id);
        }
    }
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"AREASPECTATABLE","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdAreaMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];

    if (l_area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }

    sendServerMessageArea("This area is now muted.");
    l_area->spectatable();
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"AREAMUTE","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}
void AOClient::cmdUnLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];

    if (l_area->lockStatus() == AreaData::LockStatus::FREE) {
        sendServerMessage("This area is not locked.");
        return;
    }

    sendServerMessageArea("This area is now unlocked.");
    l_area->unlock();
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"AREAUNLOCK","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdGetAreas(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries;

    l_entries.append("== Area List ==");

    for (int i = 0; i < server->m_area_names.length(); i++) {
        QStringList l_cur_area_lines = buildAreaList(i);
        l_entries.append(l_cur_area_lines);
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

    if (!ok || l_new_area >= server->m_areas.size() || l_new_area < 0) {
        sendServerMessage("That does not look like a valid area ID.");
        return;
    }

    changeArea(l_new_area);
}

void AOClient::cmdAreaKick(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData* l_area = server->m_areas[m_current_area];
    bool ok;
    int l_idx = argv[0].toInt(&ok);

    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* l_client_to_kick = server->getClientByID(l_idx);

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
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"AREAKICK","Kicked UID: " + QString::number(l_client_to_kick->m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::CM, true);
    sendServerMessage("Client " + argv[0] + " kicked from area.");
    l_client_to_kick->sendServerMessage("You kicked from area.");
}

void AOClient::cmdModAreaKick(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData* l_area = server->m_areas[m_current_area];
    bool ok;
    int l_idx = argv[0].toInt(&ok);
    int l_idz = argv[1].toInt(&ok);

    if (!ok || l_idz >= server->m_areas.size() || l_idz < 0) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* l_client_to_kick = server->getClientByID(l_idx);

    if (l_client_to_kick == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    l_client_to_kick->changeArea(l_idz);
    l_area->uninvite(l_client_to_kick->m_id);
    l_area->removeOwner(l_client_to_kick->m_id);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"MODAREAKICK","Kicked UID: " + QString::number(l_client_to_kick->m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    arup(ARUPType::CM, true);
    sendServerMessage("Client " + argv[0] + " kicked from area.");
    l_client_to_kick->sendServerMessage("You kicked from area by moderator.");
}

void AOClient::cmdSetBackground(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = getSenderName(m_id);
    AreaData* area = server->m_areas[m_current_area];
    QString f_background = argv.join(" ");

    if (m_authenticated || !area->bgLocked()) {
        if (server->m_backgrounds.contains(f_background), Qt::CaseInsensitive || area->ignoreBgList() == true) {
            area->setBackground(f_background);
            server->broadcast(AOPacket("BN", {f_background}), m_current_area);
            server->broadcast(AOPacket("MS", {"chat", "-", " ", " ", "", "jud", "0", "0", "-1", "0", "0", "0", "0", "0", "0", " ", "-1", "0", "0", "100<and>100", "0", "0", "0", "0", "0", "", "", "", "0", "||"}), m_current_area);
            sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " changed the background to " + f_background);
            emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"SETBACKGROUND","Client UID: " + QString::number(m_id),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
        }
        else {
            sendServerMessage("Invalid background name.");
        }
    }
    else {
        sendServerMessage("This area's background is locked.");
    }

}

void AOClient::cmdBgLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QString l_sender_name = getSenderName(m_id);
    AreaData* l_area = server->m_areas[m_current_area];

    if (l_area->bgLocked() == false) {
        l_area->toggleBgLock();
    };

    server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(m_id) + "] " + l_sender_name + " locked the background.", "1"}), m_current_area);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"BGLOCK","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdBgUnlock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QString l_sender_name = getSenderName(m_id);
    AreaData* l_area = server->m_areas[m_current_area];

    if (l_area->bgLocked() == true) {
        l_area->toggleBgLock();
    };

    server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(m_id) + "] " + l_sender_name + " unlocked the background.", "1"}), m_current_area);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"BGUNLOCK","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdStatus(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = getSenderName(m_id);
    AreaData* l_area = server->m_areas[m_current_area];
    QString l_arg = argv[0].toLower();

    if (l_area->changeStatus(l_arg)) {
        arup(ARUPType::STATUS, true);
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(m_id) + "] " + l_sender_name + " changed status to " + l_arg.toUpper(), "1"}), m_current_area);
        emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"SETSTATUS",l_arg.toUpper(),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
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

    AreaData* l_area = server->m_areas[m_current_area];

    if (l_area->judgelog().isEmpty()) {
        sendServerMessage("There have been no judge actions in this area.");
        return;
    }

    QString l_message = l_area->judgelog().join("\n");
    //Judgelog contains an IPID, so we shouldn't send that unless the caller has appropriate permissions

    if (checkAuth(ACLFlags.value("KICK")) == 1 || checkAuth(ACLFlags.value("BAN")) == 1) {
            sendServerMessage(l_message);
    }
    else {
        QString filteredmessage = l_message.remove(QRegularExpression("[(].*[)]")); //Filter out anything between two parentheses. This should only ever be the IPID
        sendServerMessage(filteredmessage);
    }
}

void AOClient::cmdIgnoreBgList(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* area = server->m_areas[m_current_area];

    area->toggleIgnoreBgList();
    QString state = area->ignoreBgList() ? "ignored." : "enforced.";
    sendServerMessage("BG list in this area is now " + state);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"IGNOREBGLIST",state,server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdAreaMessage(int argc, QStringList argv)
{
    AreaData* l_area = server->m_areas[m_current_area];
    if (argc == 0) {
        sendServerMessage(l_area->areaMessage());
        return;
    }

    if (argc >= 1) {
        l_area->changeAreaMessage(argv.join(" "));
        sendServerMessage("Updated this area's message.");
        emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"UPDATEAREAMESSAGE",argv.join(" "),server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
    }
}

void AOClient::cmdToggleAreaMessageOnJoin(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];
    l_area->toggleAreaMessageJoin();
    QString l_state = l_area->sendAreaMessageOnJoin() ? "enabled." : "disabled.";
    sendServerMessage("Sending message on area join is now " +l_state);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"TOGGLEAREAMESSAGE",l_state,server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdClearAreaMessage(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];
    l_area->changeAreaMessage(QString{});
    if (l_area->sendAreaMessageOnJoin()) //Turn off the automatic sending.
        cmdToggleAreaMessageOnJoin(0,QStringList{}); //Dummy values.
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"CLEARAREAMESSAGE","",server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
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

    AreaData* l_area = server->m_areas[m_current_area];
    AreaData::EvidenceMod l_curevimod = l_area->eviMod();

    QString message = "Current evidence mod: ";

    switch (l_curevimod) {
    case AreaData::EvidenceMod::FFA:
        message += "FFA";
        break;
    case AreaData::EvidenceMod::CM:
        message += "CM";
        break;
    case AreaData::EvidenceMod::HIDDEN_CM:
        message += "HIDDEN_CM";
        break;
    case AreaData::EvidenceMod::MOD:
        message += "MOD";
        break;
    }

    sendServerMessage(message);
}

void AOClient::cmdBgs(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_bgs  = server->m_backgrounds;

    sendServerMessage("Backgrounds:\n" + l_bgs.join("\n"));
}

void AOClient::cmdCurBg(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];
    QString l_background = l_area->background();

    sendServerMessage("Current background: " + l_background);
}

void AOClient::cmdToggleFloodguardActuve(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];

    l_area->toggleFloodguardActive();
    QString l_state = l_area->floodguardActive() ? "actived." : "disabled.";
    sendServerMessage("Floodguard in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"TOGGLEFLOODGUARD",l_state,server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleChillMod(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];

    l_area->toggleChillMod();
    QString l_state = l_area->chillMod() ? "actived." : "disabled.";
    sendServerMessage("Chill Mod in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"TOGGLECHILLMOD",l_state,server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}

void AOClient::cmdToggleAutoMod(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData* l_area = server->m_areas[m_current_area];

    l_area->toggleAutoMod();
    QString l_state = l_area->autoMod() ? "actived." : "disabled.";
    sendServerMessage("Automoderator in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname),m_ipid, m_ooc_name,"TOGGLEAUTOMOD",l_state,server->m_areas[m_current_area]->name(), QString::number(m_id), m_hwid);
}
