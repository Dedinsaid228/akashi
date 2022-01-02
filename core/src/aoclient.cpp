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

void AOClient::clientData()
{
    if (last_read + socket->bytesAvailable() > 30720) { // Client can send a max of 30KB to the server over two sequential reads
        socket->close();
    }

    if (last_read == 0) { // i.e. this is the first packet we've been sent
        if (!socket->waitForConnected(1000)) {
            socket->close();
        }
    }

    QString data = QString::fromUtf8(socket->readAll());
    last_read = data.size();

    if (is_partial) {
        data = partial_packet + data;
    }
    if (!data.endsWith("%")) {
        is_partial = true;
    }

    QStringList all_packets = data.split("%");
    all_packets.removeLast(); // Remove the entry after the last delimiter

    for (QString single_packet : all_packets) {
        AOPacket packet(single_packet);
        handlePacket(packet);
    }
}
void AOClient::clientDisconnected()
{
#ifdef NET_DEBUG
    qDebug() << remote_ip.toString() << "disconnected";
#endif

    server->areas.value(current_area)->LogDisconnect(current_char, ipid, hwid, showname, ooc_name, QString::number(id));
    server->freeUID(id);

    if (joined) {
        server->player_count--;
        server->areas[current_area]->clientLeftArea(server->getCharID(current_char));
        arup(ARUPType::PLAYER_COUNT, true);

        if (!sneaked) {
            if (showname.isEmpty() && current_char.isEmpty())
                sendServerMessageArea ("[" + QString::number(id) + "] " + "Spectator has disconnected.");
            else if (showname.isEmpty())
                sendServerMessageArea ("[" + QString::number(id) + "] " + current_char + " has disconnected.");
            else
                sendServerMessageArea ("[" + QString::number(id) + "] " + showname + " has disconnected.");
         }
    }

    if (current_char != "") {
        server->updateCharsTaken(server->areas[current_area]);
    }

    bool l_updateLocks = false;

    for (AreaData* area : server->areas) {
        l_updateLocks = l_updateLocks || area->removeOwner(id);
    }

    if (l_updateLocks)
        arup(ARUPType::LOCKED, true);
    arup(ARUPType::CM, true);
}

void AOClient::clientConnected()
{
    server->areas.value(current_area)->LogConnect(ipid);
    server->db_manager->ipidip(ipid, remote_ip.toString().replace("::ffff:",""), QDateTime::currentDateTimeUtc().toString("dd-MM-yyyy"));

    long haznumdate = server->db_manager->getHazNumDate(ipid);
    long currentdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (haznumdate == 0)
        return;

    if ((currentdate - haznumdate) > 604752) {
        int haznumnew = server->db_manager->getHazNum(ipid) - 1;

        if (haznumnew < 1)
            return;

        server->db_manager->updateHazNum(ipid, currentdate);
        server->db_manager->updateHazNum(ipid, haznumnew);
    }
}

void AOClient::webclientConnected(QString ipid, QString ip)
{
    server->db_manager->ipidip(ipid, remote_ip.toString().replace("::ffff:",""),  QDateTime::currentDateTimeUtc().toString("dd-MM-yyyy"));

    long haznumdate = server->db_manager->getHazNumDate(ipid);
    long currentdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (haznumdate == 0)
        return;

    if ((currentdate - haznumdate) > 604752) {
        int haznumnew = server->db_manager->getHazNum(ipid) - 1;

        if (haznumnew < 1)
            return;

        server->db_manager->updateHazNum(ipid, currentdate);
        server->db_manager->updateHazNum(ipid, haznumnew);
    }
}

bool AOClient::evidencePresent(QString id)
{
    AreaData* area = server->areas[current_area];

    if (area->eviMod() != AreaData::EvidenceMod::HIDDEN_CM)
        return false;

    int idvalid = id.toInt() - 1;

    if (idvalid < 0)
        return false;

    QList<AreaData::Evidence> area_evidence = area->evidence();
    QRegularExpression regex("<owner=(.*?)>");
    QRegularExpressionMatch match = regex.match(area_evidence[idvalid].description);

    if (match.hasMatch()) {
        QStringList owners = match.captured(1).split(",");
        QString description = area_evidence[idvalid].description.replace(owners[0], "all");
        AreaData::Evidence evi = {area_evidence[idvalid].name, description, area_evidence[idvalid].image};

        area->replaceEvidence(idvalid, evi);
        sendEvidenceList(area);
        return true;
    }

    return false;
}

void AOClient::handlePacket(AOPacket packet)
{
#ifdef NET_DEBUG
    qDebug() << "Received packet:" << packet.header << ":" << packet.contents << "args length:" << packet.contents.length();
#endif
    AreaData* area = server->areas[current_area];
    PacketInfo info = packets.value(packet.header, {false, 0, &AOClient::pktDefault});

    if (packet.contents.join("").size() > 16384) {
        return;
    }

    if (!checkAuth(info.acl_mask)) {
        return;
    }

    if (packet.header != "CH" && packet.header != "CT") {
        if (is_afk)
        {
            sendServerMessage("You are no longer AFK. Welcome back!");

            if (current_char.isEmpty() && showname.isEmpty())
                sendServerMessageArea("[" + QString::number(id) + "] Spectator are no longer AFK.");
            else if (showname.isEmpty())
                sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " are no longer AFK.");
            else
                sendServerMessageArea("[" + QString::number(id) + "] " + showname + " are no longer AFK.");
        }
        is_afk = false;
    }

    if (packet.contents.length() < info.minArgs) {
#ifdef NET_DEBUG
        qDebug() << "Invalid packet args length. Minimum is" << info.minArgs << "but only" << packet.contents.length() << "were given.";
#endif
        return;
    }

    (this->*(info.action))(area, packet.contents.length(), packet.contents, packet);
}

void AOClient::changeArea(int new_area)
{
    const int old_area = current_area;

    if (current_area == new_area) {
        sendServerMessage("You are already in area " + server->area_names[current_area]);
        return;
    }
    if (server->areas[new_area]->lockStatus() == AreaData::LockStatus::LOCKED && !server->areas[new_area]->invited.contains(id) && !checkAuth(ACLFlags.value("BYPASS_LOCKS"))) {
        sendServerMessage("Area " + server->area_names[new_area] + " is locked.");
        return;
    }    
    if (QDateTime::currentDateTime().toSecsSinceEpoch() - last_area_change_time <= 2) {
        sendServerMessage("You change area very often!");
        return;
    }

    last_area_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (!sneaked) {
        if (showname.isEmpty() && current_char.isEmpty())
            sendServerMessageArea ("[" + QString::number(id) + "] " + "Spectator moved to " + "[" + QString::number(new_area) + "] " + server->area_names[new_area]);
        else if (showname.isEmpty())
            sendServerMessageArea ("[" + QString::number(id) + "] " + current_char + " moved to " + "[" + QString::number(new_area) + "] " + server->area_names[new_area]);
        else
            sendServerMessageArea ("[" + QString::number(id) + "] " + showname + " moved to " + "[" + QString::number(new_area) + "] " + server->area_names[new_area]);
     }

    if (current_char != "") {
        server->areas[current_area]->changeCharacter(server->getCharID(current_char), -1);
        server->updateCharsTaken(server->areas[current_area]);
    }

    server->areas[current_area]->clientLeftArea(char_id);
    bool character_taken = false;

    if (server->areas[new_area]->charactersTaken().contains(server->getCharID(current_char))) {
        current_char = "";
        char_id = -1;
        character_taken = true;
    }

    server->areas[new_area]->clientJoinedArea(char_id);
    current_area = new_area;
    arup(ARUPType::PLAYER_COUNT, true);
    sendEvidenceList(server->areas[new_area]);
    sendPacket("HP", {"1", QString::number(server->areas[new_area]->defHP())});
    sendPacket("HP", {"2", QString::number(server->areas[new_area]->proHP())});
    sendPacket("BN", {server->areas[new_area]->background()});
    sendPacket("MS", {"chat", "-", " ", " ", "", "jud", "0", "0", "-1", "0", "0", "0", "0", "0", "0", " ", "-1", "0", "0", "100<and>100", "0", "0", "0", "0", "0", "", "", "", "0", "||"});

    if (character_taken && take_taked_char == false) {
        sendPacket("DONE");
    }

    for (QTimer* timer : server->areas[current_area]->timers()) {
        int timer_id = server->areas[current_area]->timers().indexOf(timer) + 1;
        if (timer->isActive()) {
            sendPacket("TI", {QString::number(timer_id), "2"});
            sendPacket("TI", {QString::number(timer_id), "0", QString::number(QTime(0,0).msecsTo(QTime(0,0).addMSecs(timer->remainingTime())))});
        }
        else {
            sendPacket("TI", {QString::number(timer_id), "3"});
        }
    }

    sendServerMessage("You moved to area " + server->area_names[current_area]);

    if (!sneaked) {
        if (showname.isEmpty() && current_char.isEmpty())
            sendServerMessageArea ("[" + QString::number(id) + "] " + "Spectator enters from " + "[" + QString::number(old_area) + "] " + server->area_names[old_area]);
        else if (showname.isEmpty())
            sendServerMessageArea ("[" + QString::number(id) + "] " + current_char + " enters from " + "[" + QString::number(old_area) + "] " + server->area_names[old_area]);
        else
            sendServerMessageArea ("[" + QString::number(id) + "] " + showname + " enters from " + "[" + QString::number(old_area) + "] " + server->area_names[old_area]);
     }

    server->areas.value(current_area)->LogChangeArea(current_char, ipid, hwid, showname, ooc_name, server->area_names[old_area] + " -> " + server->area_names[current_area], QString::number(id));

    if (server->areas[current_area]->lockStatus() == AreaData::LockStatus::SPECTATABLE)
        sendServerMessage("Area " + server->area_names[current_area] + " is spectate-only; to chat IC you will need to be invited by the CM.");
}

bool AOClient::changeCharacter(int char_id)
{
    AreaData* area = server->areas[current_area];

    if (char_id >= server->characters.length())
        return false;

    if (is_charcursed && !charcurse_list.contains(char_id)) {
        return false;
    }

    if (take_taked_char == true) {
        bool l_successfulChange = area->changeCharacter(server->getCharID(current_char), char_id, true);

        if (char_id < 0) {
            current_char = "";
        }

        if (l_successfulChange == true) {
            QString char_selected = server->characters[char_id];
            current_char = char_selected;
            pos = "";
            server->updateCharsTaken(area);
            sendPacket("PV", {QString::number(id), "CID", QString::number(char_id)});
            return true;
        }
    }

    bool l_successfulChange = area->changeCharacter(server->getCharID(current_char), char_id, false);

    if (char_id < 0) {
        current_char = "";
    }

    if (l_successfulChange == true) {
        QString char_selected = server->characters[char_id];
        current_char = char_selected;
        pos = "";
        server->updateCharsTaken(area);
        sendPacket("PV", {QString::number(id), "CID", QString::number(char_id)});
        return true;
    }

    return false;
}

void AOClient::changePosition(QString new_pos)
{
    if (new_pos == "hidden") {
        sendServerMessage("This position cannot be used.");
        return;
    }

    pos = new_pos;

    sendServerMessage("Position changed to " + pos + ".");
    sendPacket("SP", {pos});
}

void AOClient::handleCommand(QString command, int argc, QStringList argv)
{
    CommandInfo info = commands.value(command, {false, -1, &AOClient::cmdDefault});

    if (!checkAuth(info.acl_mask)) {
        sendServerMessage("You do not have permission to use that command.");
        return;
    }

    if (argc < info.minArgs) {
        sendServerMessage("Invalid command syntax.");
        return;
    }

    (this->*(info.action))(argc, argv);
}

void AOClient::arup(ARUPType type, bool broadcast)
{
    QStringList arup_data;
    arup_data.append(QString::number(type));

    for (AreaData* area : server->areas) {
        switch(type) {
            case ARUPType::PLAYER_COUNT: {
                arup_data.append(QString::number(area->playerCount()));
                break;
            }
            case ARUPType::STATUS: {
                QString area_status = QVariant::fromValue(area->status()).toString().replace("_", "-"); // LOOKING_FOR_PLAYERS to LOOKING-FOR-PLAYERS

                if (area_status == "IDLE") {
                    arup_data.append("");
                    break;
                }

                arup_data.append(area_status);
                break;
            }
        case ARUPType::CM: {
            if (area->owners.isEmpty())
                arup_data.append("");
            else {
                QStringList area_owners;
                for (int owner_id : area->owners) {
                    AOClient* owner = server->getClientByID(owner_id);

                    if (owner->showname.isEmpty() && owner->current_char.isEmpty())
                        area_owners.append("[" + QString::number(owner->id) + "] " + "Spectator");
                    else if (owner->showname.isEmpty())
                        area_owners.append("[" + QString::number(owner->id) + "] " + owner->current_char);
                    else
                        area_owners.append("[" + QString::number(owner->id) + "] " + owner->showname);
                }
                arup_data.append(area_owners.join(", "));
                }
                break;
            }
            case ARUPType::LOCKED: {
                QString lock_status = QVariant::fromValue(area->lockStatus()).toString();
                if (lock_status == "FREE") {
                    arup_data.append("");
                    break;
                }

                arup_data.append(lock_status);
                break;
            }
            default: {
                return;
            }
        }
    }

    if (broadcast)
        server->broadcast(AOPacket("ARUP", arup_data));
    else
        sendPacket("ARUP", arup_data);
}

void AOClient::fullArup() {
    arup(ARUPType::PLAYER_COUNT, false);
    arup(ARUPType::STATUS, false);
    arup(ARUPType::CM, false);
    arup(ARUPType::LOCKED, false);
}

void AOClient::sendPacket(AOPacket packet)
{
#ifdef NET_DEBUG
    qDebug() << "Sent packet:" << packet.header << ":" << packet.contents;
#endif

    packet.contents.replaceInStrings("#", "<num>")
                   .replaceInStrings("%", "<percent>")
                   .replaceInStrings("$", "<dollar>");

    if (packet.header != "LE")
        packet.contents.replaceInStrings("&", "<and>");

    socket->write(packet.toUtf8());
    socket->flush();
}

void AOClient::sendPacket(QString header, QStringList contents)
{
    sendPacket(AOPacket(header, contents));
}

void AOClient::sendPacket(QString header)
{
    sendPacket(AOPacket(header, {}));
}

void AOClient::calculateIpid()
{
    // TODO: add support for longer ipids?
    // This reduces the (fairly high) chance of
    // birthday paradox issues arising. However,
    // typing more than 8 characters might be a
    // bit cumbersome.

    QCryptographicHash hash(QCryptographicHash::Md5); // Don't need security, just hashing for uniqueness

    hash.addData(remote_ip.toString().toUtf8());

    ipid = hash.result().toHex().right(8); // Use the last 8 characters (4 bytes)
}

void AOClient::sendServerMessage(QString message)
{
    sendPacket("CT", {ConfigManager::serverName(), message, "1"});
}

void AOClient::sendServerMessageArea(QString message)
{
    server->broadcast(AOPacket("CT", {ConfigManager::serverName(), message, "1"}), current_area);
}

void AOClient::sendServerBroadcast(QString message)
{
    server->broadcast(AOPacket("CT", {ConfigManager::serverName(), message, "1"}));
}

void AOClient::autoMod()
{
    if (last5messagestime[0] == -5) {
       last5messagestime[0] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5messagestime[1] == -5) {
       last5messagestime[1] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5messagestime[2] == -5) {
       last5messagestime[2] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5messagestime[3] == -5) {
       last5messagestime[3] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5messagestime[4] == -5) {
       last5messagestime[4] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    long warnterm = parseTime(ConfigManager::autoModWarnTerm());

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - last_warn_time > warnterm && warn != 0) {
        warn--;
        last_warn_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    }

    qDebug() << warn;

    int calmdowntime = last5messagestime[4] - last5messagestime[0];
    int triggertime = ConfigManager::autoModTrigger();

    if (calmdowntime < triggertime && !first_message) {

        if (warn < 2) {
            warn++;
            qDebug() << warn;
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(3 - warn) + " more warn, then you will be punished.");
            last_warn_time = QDateTime::currentDateTime().toSecsSinceEpoch();
            last5messagestime[0] = -5;
            last5messagestime[1] = -5;
            last5messagestime[2] = -5;
            last5messagestime[3] = -5;
            last5messagestime[4] = -5;
            return;
        }

        AreaData* area = server->areas[current_area];
        int haznum = server->db_manager->getHazNum(ipid);

        if (haznum == 0 || haznum == 1) {
           AOClient* target = server->getClientByID(id);

           area->logCmdAdvanced("Auto Mod", "", "", "MUTE", "Muted UID: " + QString::number(target->id), "", "", "");

           target->is_muted = true;
           DBManager::automod num;
           num.ipid = ipid;
           num.date = QDateTime::currentDateTime().toSecsSinceEpoch();
           num.action = "MUTE";
           num.haznum = 2;

           if (haznum == 0)
              server->db_manager->addHazNum(num);
           else {
               long date = num.date;

               server->db_manager->updateHazNum(ipid, date);
               server->db_manager->updateHazNum(ipid, num.action);
               server->db_manager->updateHazNum(ipid, num.haznum);
           }
        }
      else if (haznum == 2) {
           autoKick();
        }

      else if (haznum == 3) {
          autoBan();
       }
  }

    if (first_message)
        first_message = !first_message;

    last5messagestime[0] = -5;
    last5messagestime[1] = -5;
    last5messagestime[2] = -5;
    last5messagestime[3] = -5;
    last5messagestime[4] = -5;
}

void AOClient::autoModOoc()
{
    if (last5oocmessagestime[0] == -5) {
       last5oocmessagestime[0] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5oocmessagestime[1] == -5) {
       last5oocmessagestime[1] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5oocmessagestime[2] == -5) {
       last5oocmessagestime[2] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5oocmessagestime[3] == -5) {
       last5oocmessagestime[3] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    if (last5oocmessagestime[4] == -5) {
       last5oocmessagestime[4] = QDateTime::currentDateTime().toSecsSinceEpoch();
       return;
    }

    long warnterm = parseTime(ConfigManager::autoModWarnTerm());

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - last_warn_time > warnterm && warn != 0) {
        warn--;
        last_warn_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    }

    int calmdowntime = last5oocmessagestime[4] - last5oocmessagestime[0];
    int triggertime = ConfigManager::autoModTrigger();

    if (calmdowntime < triggertime && !first_oocmessage) {

        if (warn < 2) {
            warn++;
            qDebug() << warn;
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(3 - warn) + " more warn, then you will be punished.");
            last_warn_time = QDateTime::currentDateTime().toSecsSinceEpoch();
            last5oocmessagestime[0] = -5;
            last5oocmessagestime[1] = -5;
            last5oocmessagestime[2] = -5;
            last5oocmessagestime[3] = -5;
            last5oocmessagestime[4] = -5;
            return;
        }

        AreaData* area = server->areas[current_area];
        int haznum = server->db_manager->getHazNum(ipid);

        if (haznum == 0) {
           AOClient* target = server->getClientByID(id);

           area->logCmdAdvanced("Auto Mod", "", "", "OOC MUTE", "Muted UID: " + QString::number(target->id), "", "", "");

           target->is_ooc_muted = true;
           DBManager::automod num;
           num.ipid = ipid;
           num.date = QDateTime::currentDateTime().toSecsSinceEpoch();
           num.action = "OOC MUTE";
           num.haznum = 2;

           if (haznum == 0)
              server->db_manager->addHazNum(num);
           else {
               long date = num.date;

               server->db_manager->updateHazNum(ipid, date);
               server->db_manager->updateHazNum(ipid, num.action);
               server->db_manager->updateHazNum(ipid, num.haznum);
           }
        }
      else if (haznum == 2) {
           autoKick();
        }

      else if (haznum == 3) {
          autoBan();
       }
  }

    if (first_oocmessage)
        first_oocmessage = !first_oocmessage;

    last5oocmessagestime[0] = -5;
    last5oocmessagestime[1] = -5;
    last5oocmessagestime[2] = -5;
    last5oocmessagestime[3] = -5;
    last5oocmessagestime[4] = -5;
}

void AOClient::autoKick()
{
    AreaData* area = server->areas[current_area];

    for (AOClient* client : server->getClientsByIpid(ipid)) {
        client->sendPacket("KK", {"You were kicked by a Auto Mod."});
        client->socket->close();
    }

    area->logCmdAdvanced("Auto Mod", "", "", "KICK", "Kicked IPID: " + ipid, "", "", "");

    long date = QDateTime::currentDateTime().toSecsSinceEpoch();
    QString action = "KICK";
    int haznum = 3;

    server->db_manager->updateHazNum(ipid, date);
    server->db_manager->updateHazNum(ipid, action);
    server->db_manager->updateHazNum(ipid, haznum);
}

void AOClient::autoBan()
{
    AreaData* area = server->areas[current_area];

    DBManager::BanInfo ban;

    long long duration_seconds = 0;
    QString duration_ban = ConfigManager::autoModBanDuration();

    if (duration_ban == "perma")
        duration_seconds = -2;
    else
        duration_seconds = parseTime(duration_ban);

    if (duration_seconds == -1) {
        qDebug() << "ERROR: Invalid ban time format for Auto Mod! Format example: 1h30m";
        return;
    }

    ban.duration = duration_seconds;
    ban.ipid = ipid;
    ban.reason = "You were banned by a Auto Mod.";
    ban.moderator = "Auto Mod";
    ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
    bool ban_logged = false;

    for (AOClient* client : server->getClientsByIpid(ban.ipid)) {
        if (!ban_logged) {
            ban.ip = client->remote_ip;
            ban.hdid = client->hwid;
            server->db_manager->addBan(ban);
            ban_logged = true;
        }

        QString ban_duration;

        if (!(ban.duration == -2)) {
            ban_duration = QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            ban_duration = "The heat death of the universe.";
        }

        int ban_id = server->db_manager->getBanID(ban.ip);
        client->sendPacket("KB", {ban.reason + "\nID: " + QString::number(ban_id) + "\nUntil: " + ban_duration});
        client->socket->close();

        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(ban.ipid, ban.moderator, ban_duration, ban.reason, ban_id);

        area->logCmdAdvanced("Auto Mod", "", "", "BAN", "Ban ID: " + QString::number(ban_id), "", "", "");

        long date = QDateTime::currentDateTime().toSecsSinceEpoch();
        QString action = "BAN";

        server->db_manager->updateHazNum(ipid, date);
        server->db_manager->updateHazNum(ipid, action);
    }
}

bool AOClient::checkAuth(unsigned long long acl_mask)
{
#ifdef SKIP_AUTH
    return true;
#endif
    if (acl_mask != ACLFlags.value("NONE")) {
        if (acl_mask == ACLFlags.value("CM")) {
            AreaData* area = server->areas[current_area];
            if (area->owners.contains(id))
                return true;
        }
        else if (!authenticated) {
            return false;
        }
        switch (ConfigManager::authType()) {
        case DataTypes::AuthType::SIMPLE:
            return authenticated;
            break;
        case DataTypes::AuthType::ADVANCED:
            unsigned long long user_acl = server->db_manager->getACL(moderator_name);
            return (user_acl & acl_mask) != 0;
            break;
        }
    }
    return true;
}


QString AOClient::getIpid() const { return ipid; }

Server* AOClient::getServer() { return server; }

AOClient::~AOClient() {
    socket->deleteLater();
}
