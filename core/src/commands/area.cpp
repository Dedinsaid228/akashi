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
    QString sender_name = ooc_name;
    QString sender_showname = showname;
    QString sender_char = current_char;
    AreaData* area = server->areas[current_area];

    if (area->isProtected()) {
        sendServerMessage("This area is protected, you may not become CM.");
        return;
    }

    else if (area->owners.isEmpty()) { // no one owns this area, and it's not protected
        area->owners.append(id);
        area->invited.append(id);

    if (sender_showname.isEmpty() && sender_char.isEmpty())
        sendServerMessageArea("[" + QString::number(id) + "] " + "Spectator is now CM in this area.");
    else if (sender_showname.isEmpty())
        sendServerMessageArea("[" + QString::number(id) + "] " + sender_char +" is now CM in this area.");
    else
        sendServerMessageArea("[" + QString::number(id) + "] " + sender_showname +" is now CM in this area.");

    area->logCmdAdvanced(current_char, ipid, hwid, "NEW AREA OWNER", "Owner UID: " + QString::number(id), showname, ooc_name, QString::number(id));

    arup(ARUPType::CM, true);
  }

    else if (!area->owners.contains(id)) { // there is already a CM, and it isn't us
        sendServerMessage("You cannot become a CM in this area.");
    }

    else if (argc == 1) { // we are CM, and we want to make ID argv[0] also CM
        bool ok;
        AOClient* owner_candidate = server->getClientByID(argv[0].toInt(&ok));

        if (!ok) {
            sendServerMessage("That doesn't look like a valid ID.");
            return;
        }

        if (owner_candidate == nullptr) {
            sendServerMessage("Unable to find client with ID " + argv[0] + ".");
            return;
        }

        if (area->owners.contains(owner_candidate->id)) {
            sendServerMessage("User " + QString::number(owner_candidate->id) + " are already a CM in this area.");
            return;
        }


        area->owners.append(owner_candidate->id);

        if (owner_candidate->showname.isEmpty() && owner_candidate->current_char.isEmpty())
            sendServerMessageArea("[" + QString::number(owner_candidate->id) + "] " + "Spectator is now CM in this area.");
        else if (owner_candidate->showname.isEmpty())
            sendServerMessageArea("[" + QString::number(owner_candidate->id) + "] " + owner_candidate->current_char +" is now CM in this area.");
        else
            sendServerMessageArea("[" + QString::number(owner_candidate->id) + "] " + owner_candidate->showname +" is now CM in this area.");

        area->logCmdAdvanced(current_char, ipid, hwid, "NEW AREA OWNER", "Owner UID: " + QString::number(owner_candidate->id), showname, ooc_name, QString::number(id));
        arup(ARUPType::CM, true);
    }
    else {
        sendServerMessage("You are already a CM in this area.");
    }
}

void AOClient::cmdUnCM(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    int uid;

    if (area->owners.isEmpty()) {
        sendServerMessage("There are no CMs in this area.");
        return;
    }
    else if (argc == 0) {
        uid = id;

        if (showname.isEmpty() && current_char.isEmpty())
            sendServerMessageArea("[" + QString::number(id) + "] " + "Spectator no longer CM in this area.");
        else if (showname.isEmpty())
            sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " no longer CM in this area.");
        else
            sendServerMessageArea("[" + QString::number(id) + "] " + showname + " no longer CM in this area.");

        area->logCmdAdvanced(current_char, ipid, hwid, "REMOVE AREA OWNER", "Owner UID: " + QString::number(id), showname, ooc_name, QString::number(id));

        sendServerMessage("You are no longer CM in this area.");
    }
    else {
        bool conv_ok = false;
        uid = argv[0].toInt(&conv_ok);
        if (!conv_ok) {
            sendServerMessage("Invalid user ID.");
            return;
        }
        if (!area->owners.contains(uid)) {
            sendServerMessage("That user is not CMed.");
            return;
        }
        AOClient* target = server->getClientByID(uid);

        if (target->showname.isEmpty() && target->current_char.isEmpty())
            sendServerMessageArea("[" + QString::number(target->id) + "] " + "Spectator no longer CM in this area.");
        else if (target->showname.isEmpty())
            sendServerMessageArea("[" + QString::number(target->id) + "] " + target->current_char + " no longer CM in this area.");
        else
            sendServerMessageArea("[" + QString::number(target->id) + "] " + target->showname + " no longer CM in this area.");

        area->logCmdAdvanced(current_char, ipid, hwid, "REMOVE AREA OWNER", "Owner UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));

        target->sendServerMessage("You have been unCMed.");
    }

    area->owners.removeAll(uid);
    area->invited.removeAll(uid);
    arup(ARUPType::CM, true);
    if (area->owners.isEmpty()) {
        area->invited.clear();
        if (area->m_locked != AreaData::FREE) {
            area->m_locked = AreaData::FREE;
            arup(ARUPType::LOCKED, true);
        }
    }
}

void AOClient::cmdInvite(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool ok;
    int invited_id = argv[0].toInt(&ok);

    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* target_client = server->getClientByID(invited_id);

    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    else if (area->invited.contains(invited_id)) {
        sendServerMessage("That ID is already on the invite list.");
        return;
    }

    area->invited.append(invited_id);
    sendServerMessage("You invited ID " + argv[0]);
    target_client->sendServerMessage("You were invited and given access to " + area->name());
    area->logCmdAdvanced(current_char, ipid, hwid, "INVITE", "Invited UID: " + QString::number(target_client->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUnInvite(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool ok;
    int uninvited_id = argv[0].toInt(&ok);

    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* target_client = server->getClientByID(uninvited_id);

        if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    else if (area->owners.contains(uninvited_id)) {
        sendServerMessage("You cannot uninvite a CM!");
        return;
    }

    else if (!area->invited.contains(uninvited_id)) {
        sendServerMessage("That ID is not on the invite list.");
        return;
    }

    area->invited.removeAll(uninvited_id);
    sendServerMessage("You uninvited ID " + argv[0]);
    target_client->sendServerMessage("You were uninvited from " + area->name());
    area->logCmdAdvanced(current_char, ipid, hwid, "UNINVITE", "Uninvited UID: " + QString::number(target_client->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdLock(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->m_locked == AreaData::LockStatus::LOCKED) {
        sendServerMessage("This area is already locked.");
        return;
    }

    sendServerMessageArea("This area is now locked.");
    area->m_locked = AreaData::LockStatus::LOCKED;

    for (AOClient* client : server->clients) {
        if (client->current_area == current_area && client->joined) {
            area->invited.append(client->id);
        }
    }

    area->logCmdAdvanced(current_char, ipid, hwid, "LOCK AREA", "", showname, ooc_name, QString::number(id));
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdSpectatable(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }

    sendServerMessageArea("This area is now spectatable.");
    area->spectatable();

    for (AOClient* client : server->clients) {
        if (client->current_area == current_area && client->joined) {
            area->invite(client->id);
        }
    }

    area->logCmdAdvanced(current_char, ipid, hwid, "SPECTATABLE AREA", "", showname, ooc_name, QString::number(id));
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdAreaMute(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->lockStatus() == AreaData::LockStatus::SPECTATABLE) {
        sendServerMessage("This area is already in spectate mode.");
        return;
    }

    sendServerMessageArea("This area is now muted.");
    area->spectatable();
    area->logCmdAdvanced(current_char, ipid, hwid, "MUTE AREA", "", showname, ooc_name, QString::number(id));
    arup(ARUPType::LOCKED, true);
}
void AOClient::cmdUnLock(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->lockStatus() == AreaData::LockStatus::FREE) {
        sendServerMessage("This area is not locked.");
        return;
    }

    sendServerMessageArea("This area is now unlocked.");
    area->unlock();
    area->logCmdAdvanced(current_char, ipid, hwid, "UNLOCK AREA", "", showname, ooc_name, QString::number(id));
    arup(ARUPType::LOCKED, true);
}

void AOClient::cmdGetAreas(int argc, QStringList argv)
{
    QStringList entries;

    entries.append("== Area List ==");

    for (int i = 0; i < server->area_names.length(); i++) {
        QStringList cur_area_lines = buildAreaList(i);
        entries.append(cur_area_lines);
    }

    sendServerMessage(entries.join("\n"));
}

void AOClient::cmdGetArea(int argc, QStringList argv)
{
    QStringList entries = buildAreaList(current_area);

    sendServerMessage(entries.join("\n"));
}

void AOClient::cmdArea(int argc, QStringList argv)
{
    bool ok;
    int new_area = argv[0].toInt(&ok);

    if (!ok || new_area >= server->areas.size() || new_area < 0) {
        sendServerMessage("That does not look like a valid area ID.");
        return;
    }

    changeArea(new_area);
}

void AOClient::cmdAreaKick(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool ok;
    int idx = argv[0].toInt(&ok);

    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* client_to_kick = server->getClientByID(idx);

    if (client_to_kick == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (client_to_kick->current_area != current_area) {
        sendServerMessage("That client is not in this area.");
        return;
        }

    client_to_kick->changeArea(0);
    area->uninvite(client_to_kick->id);
    area->owners.removeAll(client_to_kick->id);
    arup(ARUPType::CM, true);

    sendServerMessage("Client " + argv[0] + " kicked from area.");
    client_to_kick->sendServerMessage("You kicked from area.");
    area->logCmdAdvanced(current_char, ipid, hwid, "AREA KICK", "Kicked UID: " + QString::number(client_to_kick->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdModAreaKick(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool ok;
    int idx = argv[0].toInt(&ok);
    int idz = argv[1].toInt(&ok);

    if (!ok || idz >= server->areas.size() || idz < 0) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* client_to_kick = server->getClientByID(idx);

    if (client_to_kick == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    client_to_kick->changeArea(idz);
    area->uninvite(client_to_kick->id);
    area->owners.removeAll(client_to_kick->id);
    arup(ARUPType::CM, true);

    sendServerMessage("Client " + argv[0] + " kicked from area.");
    client_to_kick->sendServerMessage("You kicked from area by moderator.");
    area->logCmdAdvanced(current_char, ipid, hwid, "MOD AREA KICK", "Kicked UID: " + QString::number(client_to_kick->id) + ". Area to kick: " + server->area_names[idz], showname, ooc_name, QString::number(id));
}

void AOClient::cmdSetBackground(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString f_background = argv.join(" ");

    if (authenticated || !area->bgLocked()) {
        if (server->backgrounds.contains(f_background), Qt::CaseInsensitive || area->ignoreBgList() == true) {
            area->setBackground(f_background);
            server->broadcast(AOPacket("BN", {f_background}), current_area);
            server->broadcast(AOPacket("MS", {"chat", "-", " ", " ", "", "jud", "0", "0", "-1", "0", "0", "0", "0", "0", "0", " ", "-1", "0", "0", "100<and>100", "0", "0", "0", "0", "0", "", "", "", "0", "||"}), current_area);
            if (showname.isEmpty() && current_char.isEmpty())
                sendServerMessageArea("[" + QString::number(id) + "] " + "Spectator changed the background to " + f_background);
            else if (showname.isEmpty())
                sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " changed the background to " + f_background);
            else
                sendServerMessageArea("[" + QString::number(id) + "] " + showname + " changed the background to " + f_background);

            area->logCmdAdvanced(current_char, ipid, hwid, "CHANGE BG", f_background, showname, ooc_name, QString::number(id));
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
    AreaData* area = server->areas[current_area];

    if (area->bgLocked() == false) {
        area->toggleBgLock();
    };

    if (showname.isEmpty() && current_char.isEmpty())
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + "Spectator locked the background.", "1"}), current_area);
    else if (showname.isEmpty())
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + current_char + " locked the background.", "1"}), current_area);
    else
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + showname + " locked the background.", "1"}), current_area);

    area->logCmdAdvanced(current_char, ipid, hwid, "LOCK BG", "", showname, ooc_name, QString::number(id));
}

void AOClient::cmdBgUnlock(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->bgLocked() == true) {
        area->toggleBgLock();
    };

    if (showname.isEmpty() && current_char.isEmpty())
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + "Spectator unlocked the background.", "1"}), current_area);
    else if (showname.isEmpty())
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + current_char + " unlocked the background.", "1"}), current_area);
    else
        server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + showname + " unlocked the background.", "1"}), current_area);

    area->logCmdAdvanced(current_char, ipid, hwid, "UNLOCK BG", "", showname, ooc_name, QString::number(id));
}

void AOClient::cmdStatus(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString arg = argv[0].toLower();

    if (area->changeStatus(arg)) {
        arup(ARUPType::STATUS, true);

        if (showname.isEmpty() && current_char.isEmpty())
            server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + "Spectator" + " changed status to " + arg.toUpper(), "1"}), current_area);
        else if (showname.isEmpty())
            server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + current_char + " changed status to " + arg.toUpper(), "1"}), current_area);
        else
            server->broadcast(AOPacket("CT", {ConfigManager::serverName(), "[" + QString::number(id) + "] " + showname + " changed status to " + arg.toUpper(), "1"}), current_area);

        area->logCmdAdvanced(current_char, ipid, hwid, "CHANGE STATUS", arg.toUpper(), showname, ooc_name, QString::number(id));
        }
        else {
            sendServerMessage("That does not look like a valid status. Valid statuses are " + AreaData::map_statuses.keys().join(", "));
        }
}

void AOClient::cmdJudgeLog(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->judgelog().isEmpty()) {
        sendServerMessage("There have been no judge actions in this area.");
        return;
    }

    QString message = area->judgelog().join("\n");
    //Judgelog contains an IPID, so we shouldn't send that unless the caller has appropriate permissions

    if (checkAuth(ACLFlags.value("KICK")) == 1 || checkAuth(ACLFlags.value("BAN")) == 1) {
            sendServerMessage(message);
    }
    else {
        QString filteredmessage = message.remove(QRegularExpression("[(].*[)]")); //Filter out anything between two parentheses. This should only ever be the IPID
        sendServerMessage(filteredmessage);
    }
}

void AOClient::cmdIgnoreBgList(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    area->toggleIgnoreBgList();
    QString state = area->ignoreBgList() ? "ignored." : "enforced.";
    sendServerMessage("BG list in this area is now " + state);

    area->logCmdAdvanced(current_char, ipid, hwid, "INGORE BGLIST", state, showname, ooc_name, QString::number(id));
}

void AOClient::cmdSneak(int argc, QStringList argv)
{
    sneaked = !sneaked;

    QString str_en = sneaked ? "now sneaking (area transfer announcements will now be hidden)" : "no longer sneaking (area transfer announcements will no longer be hidden)";
    sendServerMessage("You are " + str_en + ".");
}

void AOClient::cmdCurrentEvimod(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    AreaData::EvidenceMod curevimod = area->eviMod();   

    if (curevimod == AreaData::EvidenceMod::FFA)
        sendServerMessage("Current evidence mod: FFA");
    else if (curevimod == AreaData::EvidenceMod::CM)
        sendServerMessage("Current evidence mod: CM");
    else if (curevimod == AreaData::EvidenceMod::HIDDEN_CM)
        sendServerMessage("Current evidence mod: HIDDEN CM");
    else if (curevimod == AreaData::EvidenceMod::MOD)
        sendServerMessage("Current evidence mod: MOD");
    else
        sendServerMessage("Current evidence mod: Unknown");
}

void AOClient::cmdBgs(int argc, QStringList argv)
{
    QStringList bgs  = server->backgrounds;

    sendServerMessage("Backgrounds:\n" + bgs.join("\n"));
}

void AOClient::cmdCurBg(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString background = area->background();

    sendServerMessage("Current background: " + background);
}

void AOClient::cmdToggleFloodguardActuve(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    area->toggleFloodguardActive();
    QString state = area->floodguardActive() ? "actived." : "disabled.";
    sendServerMessage("Floodguard in this area is now " + state);

    area->logCmdAdvanced(current_char, ipid, hwid, "TOGGLE FLOODGUARD", state, showname, ooc_name, QString::number(id));
}

void AOClient::cmdToggleChillMod(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    area->toggleChillMod();
    QString state = area->chillMod() ? "actived." : "disabled.";
    sendServerMessage("Chill Mod in this area is now " + state);

    area->logCmdAdvanced(current_char, ipid, hwid, "TOGGLE CHILLMOD", state, showname, ooc_name, QString::number(id));
}

void AOClient::cmdToggleAutoMod(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    area->toggleAutoMod();
    QString state = area->autoMod() ? "actived." : "disabled.";
    sendServerMessage("Auto Mod in this area is now " + state);

    area->logCmdAdvanced(current_char, ipid, hwid, "TOGGLE AUTOMOD", state, showname, ooc_name, QString::number(id));
}
