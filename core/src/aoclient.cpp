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
    if (last_read + m_socket->bytesAvailable() > 30720) { // Client can send a max of 30KB to the server over two sequential reads
        m_socket->close();
    }

    if (last_read == 0) { // i.e. this is the first packet we've been sent
        if (!m_socket->waitForConnected(1000)) {
            m_socket->close();
        }
    }

    QString l_data = QString::fromUtf8(m_socket->readAll());
    last_read = l_data.size();

    if (is_partial) {
        l_data = partial_packet + l_data;
    }
    if (!l_data.endsWith("%")) {
        is_partial = true;
    }

    QStringList l_all_packets = l_data.split("%");
    l_all_packets.removeLast(); // Remove the entry after the last delimiter

    for (const QString &l_single_packet : qAsConst(l_all_packets)) {
        AOPacket l_packet(l_single_packet);
        handlePacket(l_packet);
    }
}
void AOClient::clientDisconnected()
{
#ifdef NET_DEBUG
    qDebug() << m_remote_ip.toString() << "disconnected";
#endif

    server->freeUID(m_id);

    if (m_joined) {
        server->m_player_count--;
        emit server->updatePlayerCount(server->m_player_count);
        server->m_areas[m_current_area]->clientLeftArea(server->getCharID(m_current_char));
        arup(ARUPType::PLAYER_COUNT, true);

        QString l_sender_name = getSenderName(m_id);

        if (!m_sneaked)
            sendServerMessageArea ("[" + QString::number(m_id) + "] " + l_sender_name + " has disconnected.");

        emit logDisconnect(m_current_char, m_ipid, m_ooc_name, server->m_area_names[m_current_area], QString::number(m_id), m_hwid);
    }

    if (m_current_char != "") {
        server->updateCharsTaken(server->m_areas[m_current_area]);
    }

    bool l_updateLocks = false;

    for (AreaData* area : qAsConst(server->m_areas)) {
        l_updateLocks = l_updateLocks || area->removeOwner(m_id);
    }

    if (l_updateLocks)
        arup(ARUPType::LOCKED, true);
    arup(ARUPType::CM, true);
}

void AOClient::clientConnected()
{
    server->db_manager->ipidip(m_ipid, m_remote_ip.toString().replace("::ffff:",""), QDateTime::currentDateTime().toString("dd-MM-yyyy"));

    long l_haznumdate = server->db_manager->getHazNumDate(m_ipid);
    long l_currentdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (l_haznumdate == 0)
        return;

    if ((l_currentdate - l_haznumdate) > 604752) { // If about a week or more has passed since the last punishment from the automoderator.
        int l_haznumnew = server->db_manager->getHazNum(m_ipid) - 1;

        if (l_haznumnew < 1)
            return;

        server->db_manager->updateHazNum(m_ipid, l_currentdate);
        server->db_manager->updateHazNum(m_ipid, l_haznumnew);
    }
}

bool AOClient::evidencePresent(QString id)
{
    AreaData* l_area = server->m_areas[m_current_area];

    if (l_area->eviMod() != AreaData::EvidenceMod::HIDDEN_CM)
        return false;

    int l_idvalid = id.toInt() - 1;

    if (l_idvalid < 0)
        return false;

    QList<AreaData::Evidence> l_area_evidence = l_area->evidence();
    QRegularExpression l_regex("<owner=(.*?)>");
    QRegularExpressionMatch l_match = l_regex.match(l_area_evidence[l_idvalid].description);

    if (l_match.hasMatch()) {
        QStringList l_owners = l_match.captured(1).split(",");
        QString l_description = l_area_evidence[l_idvalid].description.replace(l_owners[0], "all");
        AreaData::Evidence l_evi = {l_area_evidence[l_idvalid].name, l_description, l_area_evidence[l_idvalid].image};

        l_area->replaceEvidence(l_idvalid, l_evi);
        sendEvidenceList(l_area);
        return true;
    }

    return false;
}

void AOClient::handlePacket(AOPacket packet)
{
#ifdef NET_DEBUG
    qDebug() << "Received packet:" << packet.header << ":" << packet.contents << "args length:" << packet.contents.length();
#endif
    QString l_sender_name = getSenderName(m_id);
    AreaData* l_area = server->m_areas[m_current_area];
    PacketInfo l_info = packets.value(packet.header, {false, 0, &AOClient::pktDefault});

    if (packet.contents.join("").size() > 16384) {
        return;
    }

    if (!checkAuth(l_info.acl_mask)) {
        return;
    }

    if (packet.header != "CH" && packet.header != "CT") {
        if (m_is_afk)
        {
            sendServerMessage("You are no longer AFK. Welcome back!");
            sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " are no longer AFK.");
        }
        m_is_afk = false;
    }

    if (packet.contents.length() < l_info.minArgs) {
#ifdef NET_DEBUG
        qDebug() << "Invalid packet args length. Minimum is" << l_info.minArgs << "but only" << packet.contents.length() << "were given.";
#endif
        return;
    }

    (this->*(l_info.action))(l_area, packet.contents.length(), packet.contents, packet);
}

void AOClient::changeArea(int new_area)
{
    const int l_old_area = m_current_area;

    if (m_current_area == new_area) {
        sendServerMessage("You are already in area " + server->m_area_names[m_current_area]);
        return;
    }

    if (server->m_areas[new_area]->lockStatus() == AreaData::LockStatus::LOCKED && !server->m_areas[new_area]->invited().contains(m_id) && !checkAuth(ACLFlags.value("BYPASS_LOCKS"))) {
        sendServerMessage("Area " + server->m_area_names[new_area] + " is locked.");
        return;
    }

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_area_change_time <= 2) {
        sendServerMessage("You change area very often!");
        return;
    }

    m_last_area_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();

    QString l_sender_name = getSenderName(m_id);

    if (!m_sneaked)
        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " moved to " + "[" + QString::number(new_area) + "] " + server->m_area_names[new_area]);

    if (m_current_char != "") {
        server->m_areas[m_current_area]->changeCharacter(server->getCharID(m_current_char), -1);
        server->updateCharsTaken(server->m_areas[m_current_area]);
    }

    server->m_areas[m_current_area]->clientLeftArea(m_char_id);
    bool l_character_taken = false;

    if (server->m_areas[new_area]->charactersTaken().contains(server->getCharID(m_current_char))) {
        m_current_char = "";
        m_char_id = -1;
        l_character_taken = true;
    }

    server->m_areas[new_area]->clientJoinedArea(m_char_id);
    m_current_area = new_area;
    arup(ARUPType::PLAYER_COUNT, true);
    sendEvidenceList(server->m_areas[new_area]);
    sendPacket("HP", {"1", QString::number(server->m_areas[new_area]->defHP())});
    sendPacket("HP", {"2", QString::number(server->m_areas[new_area]->proHP())});
    sendPacket("BN", {server->m_areas[new_area]->background()});
    sendPacket("MS", {"chat", "-", " ", " ", "", "jud", "0", "0", "-1", "0", "0", "0", "0", "0", "0", " ", "-1", "0", "0", "100<and>100", "0", "0", "0", "0", "0", "", "", "", "0", "||"});

    if (l_character_taken && m_take_taked_char == false) {
        sendPacket("DONE");
    }

    const QList<QTimer*> l_timers = server->m_areas[m_current_area]->timers();
    for (QTimer* l_timer : l_timers) {
        int l_timer_id = server->m_areas[m_current_area]->timers().indexOf(l_timer) + 1;
        if (l_timer->isActive()) {
            sendPacket("TI", {QString::number(l_timer_id), "2"});
            sendPacket("TI", {QString::number(l_timer_id), "0", QString::number(QTime(0,0).msecsTo(QTime(0,0).addMSecs(l_timer->remainingTime())))});
        }
        else {
            sendPacket("TI", {QString::number(l_timer_id), "3"});
        }
    }

    sendServerMessage("You moved to area " + server->m_area_names[m_current_area]);

    if (server->m_areas[m_current_area]->sendAreaMessageOnJoin())
        sendServerMessage(server->m_areas[m_current_area]->areaMessage());

    if (!m_sneaked)
        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " enters from " + "[" + QString::number(l_old_area) + "] " + server->m_area_names[l_old_area]);

    if (server->m_areas[m_current_area]->lockStatus() == AreaData::LockStatus::SPECTATABLE)
        sendServerMessage("Area " + server->m_area_names[m_current_area] + " is spectate-only; to chat IC you will need to be invited by the CM.");

    emit logChangeArea((m_current_char + " " + m_showname), m_ooc_name,m_ipid,server->m_areas[m_current_area]->name(),server->m_area_names[l_old_area] + " -> " + server->m_area_names[new_area], QString::number(m_id), m_hwid);
}

bool AOClient::changeCharacter(int char_id)
{
    QString const l_old_char = m_current_char;
    AreaData* l_area = server->m_areas[m_current_area];

    if (char_id >= server->m_characters.length())
        return false;

    if (m_is_charcursed && !m_charcurse_list.contains(char_id)) {
        return false;
    }

    bool l_successfulChange = l_area->changeCharacter(server->getCharID(m_current_char), char_id, m_take_taked_char);

    if (char_id < 0) {
        m_current_char = "";
    }

    if (l_successfulChange == true) {
        QString char_selected = server->m_characters[char_id];
        m_current_char = char_selected;
        m_pos = "";
        server->updateCharsTaken(l_area);
        sendPacket("PV", {QString::number(m_id), "CID", QString::number(char_id)});
        emit logChangeChar((m_current_char + " " + m_showname), m_ooc_name,m_ipid,server->m_areas[m_current_area]->name(),l_old_char + " -> " + m_current_char, QString::number(m_id), m_hwid);
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

    m_pos = new_pos;

    sendServerMessage("Position changed to " + m_pos + ".");
    sendPacket("SP", {m_pos});
}

void AOClient::handleCommand(QString command, int argc, QStringList argv)
{
    CommandInfo l_info = commands.value(command, {false, -1, &AOClient::cmdDefault});

    if (!checkAuth(l_info.acl_mask)) {
        sendServerMessage("You do not have permission to use that command.");
        return;
    }

    if (argc < l_info.minArgs) {
        sendServerMessage("Invalid command syntax.");
        return;
    }

    (this->*(l_info.action))(argc, argv);
}

void AOClient::arup(ARUPType type, bool broadcast)
{
    QStringList l_arup_data;
    l_arup_data.append(QString::number(type));

    for (AreaData* l_area : qAsConst(server->m_areas)) {
        switch(type) {
            case ARUPType::PLAYER_COUNT: {
                l_arup_data.append(QString::number(l_area->playerCount()));
                break;
            }
            case ARUPType::STATUS: {
                QString l_area_status = QVariant::fromValue(l_area->status()).toString().replace("_", "-"); // LOOKING_FOR_PLAYERS to LOOKING-FOR-PLAYERS

                if (l_area_status == "IDLE") {
                    l_arup_data.append("");
                    break;
                }

                l_arup_data.append(l_area_status);
                break;
            }
        case ARUPType::CM: {
            if (l_area->owners().isEmpty())
                l_arup_data.append("");
            else {
                QStringList l_area_owners;
                const QList<int> l_owner_ids = l_area->owners();
                for (int l_owner_id : l_owner_ids) {
                    AOClient* l_owner = server->getClientByID(l_owner_id);
                    QString l_sender_name = getSenderName(l_owner->m_id);

                    l_area_owners.append("[" + QString::number(l_owner->m_id) + "] " + l_sender_name);
                }
                l_arup_data.append(l_area_owners.join(", "));
                }
                break;
            }
            case ARUPType::LOCKED: {
                QString l_lock_status = QVariant::fromValue(l_area->lockStatus()).toString();
                if (l_lock_status == "FREE") {
                    l_arup_data.append("");
                    break;
                }

                l_arup_data.append(l_lock_status);
                break;
            }
            default: {
                return;
            }
        }
    }

    if (broadcast)
        server->broadcast(AOPacket("ARUP", l_arup_data));
    else
        sendPacket("ARUP", l_arup_data);
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

    m_socket->write(packet.toUtf8());
    m_socket->flush();
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

    hash.addData(m_remote_ip.toString().toUtf8());

    m_ipid = hash.result().toHex().right(8); // Use the last 8 characters (4 bytes)
}

void AOClient::sendServerMessage(QString message)
{
    sendPacket("CT", {ConfigManager::serverName(), message, "1"});
}

void AOClient::sendServerMessageArea(QString message)
{
    server->broadcast(AOPacket("CT", {ConfigManager::serverName(), message, "1"}), m_current_area);
}

void AOClient::sendServerBroadcast(QString message)
{
    server->broadcast(AOPacket("CT", {ConfigManager::serverName(), message, "1"}));
}

void AOClient::autoMod(bool ic_chat)
{
    if (ic_chat) {
        if (m_last5messagestime[0] == -5) {
            m_last5messagestime[0] = QDateTime::currentDateTime().toSecsSinceEpoch();
            return;
        }

        if (m_last5messagestime[1] == -5) {
            m_last5messagestime[1] = QDateTime::currentDateTime().toSecsSinceEpoch();
            return;
        }

         if (m_last5messagestime[2] == -5) {
             m_last5messagestime[2] = QDateTime::currentDateTime().toSecsSinceEpoch();
             return;
        }

         if (m_last5messagestime[3] == -5) {
             m_last5messagestime[3] = QDateTime::currentDateTime().toSecsSinceEpoch();
             return;
        }

        if (m_last5messagestime[4] == -5) {
            m_last5messagestime[4] = QDateTime::currentDateTime().toSecsSinceEpoch();
            return;
        }
}
    else {
        if (m_last5oocmessagestime[0] == -5) {
           m_last5oocmessagestime[0] = QDateTime::currentDateTime().toSecsSinceEpoch();
           return;
        }

        if (m_last5oocmessagestime[1] == -5) {
           m_last5oocmessagestime[1] = QDateTime::currentDateTime().toSecsSinceEpoch();
           return;
        }

        if (m_last5oocmessagestime[2] == -5) {
           m_last5oocmessagestime[2] = QDateTime::currentDateTime().toSecsSinceEpoch();
           return;
        }

        if (m_last5oocmessagestime[3] == -5) {
           m_last5oocmessagestime[3] = QDateTime::currentDateTime().toSecsSinceEpoch();
           return;
        }

        if (m_last5oocmessagestime[4] == -5) {
           m_last5oocmessagestime[4] = QDateTime::currentDateTime().toSecsSinceEpoch();
           return;
        }
    }

    long l_warnterm = parseTime(ConfigManager::autoModWarnTerm());

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_warn_time > l_warnterm && m_warn != 0) {
        m_warn--;
        m_last_warn_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    }

    int l_calmdowntime = m_last5messagestime[4] - m_last5messagestime[0];
    int l_ooccalmdowntime = m_last5oocmessagestime[4] - m_last5oocmessagestime[0];
    int l_triggertime = ConfigManager::autoModTrigger();
    bool l_punishment = false;

    if (ic_chat) {
        if (l_calmdowntime < l_triggertime && !m_first_message)
            l_punishment = true;
    }
    else {
        if (l_ooccalmdowntime < l_triggertime && !m_first_oocmessage)
            l_punishment = true;
    }

    if (l_punishment) {

        if (m_warn < 2) {
            m_warn++;
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(3 - m_warn) + " more warn, then you will be punished.");
            m_last_warn_time = QDateTime::currentDateTime().toSecsSinceEpoch();
            clearLastMessages(ic_chat);
            return;
        }

        int l_haznum = server->db_manager->getHazNum(m_ipid);

        if (l_haznum == 0 || l_haznum == 1)
           autoMute(ic_chat, l_haznum);
      else if (l_haznum == 2)
           autoKick();
      else if (l_haznum == 3)
           autoBan();
  }

    if (ic_chat) {
        if (m_first_message)
            m_first_message = !m_first_message;
    }
    else {
        if (m_first_oocmessage)
            m_first_oocmessage = !m_first_oocmessage;
    }

    clearLastMessages(ic_chat);
}

void AOClient::clearLastMessages(bool ic_chat)
{
    if (ic_chat) {
        m_last5messagestime[0] = -5;
        m_last5messagestime[1] = -5;
        m_last5messagestime[2] = -5;
        m_last5messagestime[3] = -5;
        m_last5messagestime[4] = -5;
    }
    else {
        m_last5oocmessagestime[0] = -5;
        m_last5oocmessagestime[1] = -5;
        m_last5oocmessagestime[2] = -5;
        m_last5oocmessagestime[3] = -5;
        m_last5oocmessagestime[4] = -5;
    }
}

void AOClient::autoMute(bool ic_chat, int haznum)
{
    AOClient* target = server->getClientByID(m_id);

    if (ic_chat)
        target->m_is_muted = true;
    else
        target->m_is_ooc_muted = true;

    DBManager::automod l_num;
    l_num.ipid = m_ipid;
    l_num.date = QDateTime::currentDateTime().toSecsSinceEpoch();
    l_num.action = "MUTE";
    l_num.haznum = 2;

    if (haznum == 0)
       server->db_manager->addHazNum(l_num);
    else {
        long date = l_num.date;

        emit logCMD("Automoderator","", "","MUTE","Muted UID: " + QString::number(target->m_id),server->m_areas[m_current_area]->name(), "", "");
        server->db_manager->updateHazNum(m_ipid, date);
        server->db_manager->updateHazNum(m_ipid, l_num.action);
        server->db_manager->updateHazNum(m_ipid, l_num.haznum);
    }
}

void AOClient::autoKick()
{
    const QList<AOClient*> l_targets = server->getClientsByIpid(m_ipid);
    for (AOClient* l_client : l_targets) {
        l_client->sendPacket("KK", {"You were kicked by a automoderator."});
        l_client->m_socket->close();
    }

    emit logKick("Automoderator", m_ipid, "You were kicked by a automoderator.", "", "");
    long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
    QString l_action = "KICK";
    int l_haznum = 3;

    server->db_manager->updateHazNum(m_ipid, l_date);
    server->db_manager->updateHazNum(m_ipid, l_action);
    server->db_manager->updateHazNum(m_ipid, l_haznum);
}

void AOClient::autoBan()
{
    DBManager::BanInfo l_ban;

    long long l_duration_seconds = 0;
    QString l_duration_ban = ConfigManager::autoModBanDuration();

    if (l_duration_ban == "perma")
        l_duration_seconds = -2;
    else
        l_duration_seconds = parseTime(l_duration_ban);

    if (l_duration_seconds == -1) {
        qDebug() << "ERROR: Invalid ban time format for automoderator! Format example: 1h30m";
        return;
    }

    l_ban.duration = l_duration_seconds;
    l_ban.ipid = m_ipid;
    l_ban.reason = "You were banned by a automoderator.";
    l_ban.moderator = "Automoderator";
    l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
    bool ban_logged = false;

    const QList<AOClient*> l_targets = server->getClientsByIpid(l_ban.ipid);
    for (AOClient* l_client : l_targets) {
        if (!ban_logged) {
            l_ban.ip = l_client->m_remote_ip;
            l_ban.hdid = l_client->m_hwid;
            server->db_manager->addBan(l_ban);
            ban_logged = true;
        }

        QString l_ban_duration;

        if (!(l_ban.duration == -2)) {
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            l_ban_duration = "The heat death of the universe.";
        }

        int l_ban_id = server->db_manager->getBanID(l_ban.ip);
        l_client->sendPacket("KB", {l_ban.reason + "\nID: " + QString::number(l_ban_id) + "\nUntil: " + l_ban_duration});
        l_client->m_socket->close();

        emit logBan(l_ban.moderator,l_ban.ipid,l_ban_duration,l_ban.reason, "","");
        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(l_ban.ipid, l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);

        long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
        QString l_action = "BAN";

        server->db_manager->updateHazNum(m_ipid, l_date);
        server->db_manager->updateHazNum(m_ipid, l_action);
    }
}

bool AOClient::checkAuth(unsigned long long acl_mask)
{
#ifdef SKIP_AUTH
    return true;
#endif
    if (acl_mask != ACLFlags.value("NONE")) {
        if (acl_mask == ACLFlags.value("CM")) {
            AreaData* l_area = server->m_areas[m_current_area];
            if (l_area->owners().contains(m_id))
                return true;
        }
        else if (!m_authenticated) {
            return false;
        }
        switch (ConfigManager::authType()) {
        case DataTypes::AuthType::SIMPLE:
            return m_authenticated;
            break;
        case DataTypes::AuthType::ADVANCED:
            unsigned long long l_user_acl = server->db_manager->getACL(m_moderator_name);
            return (l_user_acl & acl_mask) != 0;
            break;
        }
    }
    return true;
}


QString AOClient::getIpid() const { return m_ipid; }

QString AOClient::getHwid() const { return m_hwid; }

Server* AOClient::getServer() { return server; }

AOClient::~AOClient() {
    m_socket->deleteLater();
}
