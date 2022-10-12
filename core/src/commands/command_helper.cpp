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
#include "include/packet/packet_factory.h"
#include "include/server.h"

// This file is for functions used by various commands, defined in the command helper function category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdDefault(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("Invalid command.");
    return;
}

QStringList AOClient::buildAreaList(int area_idx)
{
    QStringList entries;
    QString area_name = server->getAreaName(area_idx);
    AreaData *area = server->getAreaById(area_idx);

    if (area->playerCount() == 0)
        return entries;

    entries.append("=== " + area_name + " ===");

    switch (area->lockStatus()) {
    case AreaData::LockStatus::LOCKED:
        entries.append("[LOCKED]");
        break;
    case AreaData::LockStatus::SPECTATABLE:
        entries.append("[SPECTATABLE]");
        break;
    case AreaData::LockStatus::FREE:
    default:
        break;
    }

    entries.append("[" + QString::number(area->playerCount()) + " users][" + QVariant::fromValue(area->status()).toString().replace("_", "-") + "]");

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == area_idx && l_client->hasJoined()) {
            QString char_entry = "[" + QString::number(l_client->m_id) + "] " + l_client->m_current_char;
            if (l_client->m_current_char == "")
                char_entry += "Spectator";
            if (area->owners().contains(l_client->m_id))
                char_entry.insert(0, "[CM]");
            if (l_client->m_showname != "")
                char_entry += " (" + l_client->m_showname + ")";
            if (l_client->m_pos != "" && l_client->m_current_char != "")
                char_entry += " <" + l_client->m_pos + "> ";
            if (m_authenticated)
                char_entry += " (" + l_client->getIpid() + "): " + l_client->m_ooc_name;
            if (!m_authenticated && area->owners().contains(m_id))
                char_entry += ": " + l_client->m_ooc_name;
            if (l_client->m_vote_candidate)
                char_entry.insert(0, "[VC]");
            if (l_client->m_authenticated && !l_client->m_sneak_mod)
                char_entry.insert(0, "[M]");
            if (l_client->m_is_afk)
                char_entry.insert(0, "[AFK]");
            entries.append(char_entry);
        }
    }

    return entries;
}

int AOClient::genRand(int min, int max)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    qsrand(QDateTime::currentMSecsSinceEpoch());
    quint32 random_number = (qrand() % (max - min + 1)) + min;
    return random_number;

#else
    return QRandomGenerator::system()->bounded(min, max + 1);
#endif
}

void AOClient::diceThrower(int argc, QStringList argv, bool p_roll)
{
    QString l_sender_name = getSenderName(m_id);
    int l_sides = 6;
    int l_dice = 1;
    QStringList results;

    if (argc >= 1)
        l_sides = qBound(1, argv[0].toInt(), ConfigManager::diceMaxValue());

    if (argc == 2)
        l_dice = qBound(1, argv[1].toInt(), ConfigManager::diceMaxDice());

    for (int i = 1; i <= l_dice; i++) {
        results.append(QString::number(AOClient::genRand(1, l_sides)));
    }

    QString total_results = results.join(" ");

    if (p_roll) {
        sendServerMessage("You rolled a " + QString::number(l_dice) + "d" + QString::number(l_sides) + ". Results: " + total_results);
        return;
    }

    sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " rolled a " + QString::number(l_dice) + "d" + QString::number(l_sides) + ". Results: " + total_results);
}

QString AOClient::getAreaTimer(int area_idx, int timer_idx)
{
    AreaData *l_area = server->getAreaById(area_idx);
    QTimer *l_timer;
    QString l_timer_name = (timer_idx == 0) ? "Global timer" : "Timer " + QString::number(timer_idx);

    if (timer_idx == 0)
        l_timer = server->timer;
    else if (timer_idx > 0 && timer_idx <= 4)
        l_timer = l_area->timers().at(timer_idx - 1);
    else
        return "Invalid timer ID.";

    if (l_timer->isActive()) {
        QTime current_time = QTime(0, 0).addMSecs(l_timer->remainingTime());

        return l_timer_name + " is at " + current_time.toString("hh:mm:ss.zzz");
    }
    else {
        return l_timer_name + " is inactive.";
    }
}

long long AOClient::parseTime(QString input)
{
    QRegularExpression l_regex("(?:(?:(?<year>.*?)y)*(?:(?<week>.*?)w)*(?:(?<day>.*?)d)*(?:(?<hr>.*?)h)*(?:(?<min>.*?)m)*(?:(?<sec>.*?)s)*)");
    QRegularExpressionMatch match = l_regex.match(input);
    QString str_year, str_week, str_hour, str_day, str_minute, str_second;
    int year, week, day, hour, minute, second;

    str_year = match.captured("year");
    str_week = match.captured("week");
    str_day = match.captured("day");
    str_hour = match.captured("hr");
    str_minute = match.captured("min");
    str_second = match.captured("sec");

    bool l_is_well_formed = false;
    QString concat_str(str_year + str_week + str_day + str_hour + str_minute + str_second);
    concat_str.toInt(&l_is_well_formed);

    if (!l_is_well_formed) {
        return -1;
    }

    year = str_year.toInt();
    week = str_week.toInt();
    day = str_day.toInt();
    hour = str_hour.toInt();
    minute = str_minute.toInt();
    second = str_second.toInt();

    long long l_total = 0;
    l_total += 31622400 * year;
    l_total += 604800 * week;
    l_total += 86400 * day;
    l_total += 3600 * hour;
    l_total += 60 * minute;
    l_total += second;

    if (l_total < 0)
        return -1;

    return l_total;
}

bool AOClient::checkPasswordRequirements(QString f_username, QString f_password)
{
    QString l_decoded_password = decodeMessage(f_password);

    if (!ConfigManager::passwordRequirements())
        return true;

    if (ConfigManager::passwordMinLength() > l_decoded_password.length())
        return false;

    if (ConfigManager::passwordMaxLength() < l_decoded_password.length() && ConfigManager::passwordMaxLength() != 0)
        return false;

    else if (ConfigManager::passwordRequireMixCase()) {
        if (l_decoded_password.toLower() == l_decoded_password)
            return false;

        if (l_decoded_password.toUpper() == l_decoded_password)
            return false;
    }
    else if (ConfigManager::passwordRequireNumbers()) {
        QRegularExpression regex("[0123456789]");
        QRegularExpressionMatch match = regex.match(l_decoded_password);

        if (!match.hasMatch())
            return false;
    }
    else if (ConfigManager::passwordRequireSpecialCharacters()) {
        QRegularExpression regex("[~!@#$%^&*_-+=`|\\(){}\[]:;\"'<>,.?/]");
        QRegularExpressionMatch match = regex.match(l_decoded_password);

        if (!match.hasMatch())
            return false;
    }
    else if (!ConfigManager::passwordCanContainUsername()) {

        if (l_decoded_password.contains(f_username))
            return false;
    }
    return true;
}

void AOClient::sendNotice(QString f_notice, bool f_global)
{
    QString l_message = "A moderator sent this ";
    l_message += (f_global ? "server-wide " : "");
    l_message += "notice:\n\n" + f_notice;

    sendServerMessageArea(l_message);
    AOPacket *l_packet = PacketFactory::createPacket("BB", {l_message});

    if (f_global)
        server->broadcast(l_packet);
    else
        server->broadcast(l_packet, m_current_area);

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "NOTICE", f_notice, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid);
}

void AOClient::playMusic(QStringList f_args, bool f_once)
{
    if (m_is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_music_change_time <= 2) {
        sendServerMessage("You change music a lot!");
        return;
    }

    m_last_music_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    AreaData *l_area = server->getAreaById(m_current_area);

    if (l_area->isMusicAllowed() == false && !checkPermission(ACLRole::CM)) {
        sendServerMessage("Music is disabled in this area.");
        return;
    }

    QString l_song = f_args.join(" ");

    if (l_song.startsWith("https://www.youtube.com/") || l_song.startsWith("https://www.youtu.be//")) {
        sendServerMessage("You cannot use YouTube links. You may use direct links to MP3, Ogg, or M3U streams.");
        return;
    }

    if (!l_song.startsWith("http")) {
        sendServerMessage("Unknown http source. Maybe you forgot to copy the link along with http:// or https://");
        return;
    }

    QString l_sender_name = getSenderName(m_id);
    QString l_play_once;

    if (f_once)
        l_play_once = "0";
    else
        l_play_once = "1";

    AOPacket *music_change = PacketFactory::createPacket("MC", {l_song, QString::number(server->getCharID(m_current_char)), m_showname, l_play_once, "0"});
    server->broadcast(music_change, m_current_area);
    l_area->changeMusic(l_sender_name, l_song);
    emit logMusic((m_current_char + " " + m_showname), m_ooc_name, m_ipid, server->getAreaById(m_current_area)->name(), l_song, QString::number(m_id), m_hwid);
}

QString AOClient::getSenderName(int f_uid)
{
    AOClient *l_target = server->getClientByID(f_uid);

    if (l_target->m_showname.isEmpty() && l_target->m_current_char.isEmpty())
        return "Spectator";
    else if (l_target->m_showname.isEmpty())
        return l_target->m_current_char;
    else
        return l_target->m_showname;
}

QString AOClient::getEviMod(int f_area)
{
    AreaData *l_area = server->getAreaById(f_area);

    switch (l_area->eviMod()) {
    case AreaData::EvidenceMod::FFA:
        return "FFA";
    case AreaData::EvidenceMod::CM:
        return "CM";
    case AreaData::EvidenceMod::HIDDEN_CM:
        return "HIDDEN_CM";
    case AreaData::EvidenceMod::MOD:
        return "MOD";
    }
    return "UNKNOWN";
}

void AOClient::endVote()
{
    QString l_message = "Results: \nWinner(-s) is ";
    QString l_winner;
    int l_winner_points;
    l_winner_points = 0;
    QString l_results;
    const QVector<AOClient *> l_clients = server->getClients();

    for (AOClient *l_client : l_clients) {
        if (l_client->m_vote_candidate) {
            l_client->m_can_vote = false;
            l_client->m_vote_candidate = false;

            if (l_client->m_vote_points > l_winner_points) {
                l_winner_points = l_client->m_vote_points;
                l_winner = "[" + QString::number(l_client->m_id) + "] " + getSenderName(l_client->m_id);
            }
            else if (l_client->m_vote_points == l_winner_points)
                l_winner = l_winner + ", [" + QString::number(l_client->m_id) + "] " + getSenderName(l_client->m_id);

            l_results += "[" + QString::number(l_client->m_id) + "] " + getSenderName(l_client->m_id) + " = " +  QString::number(l_client->m_vote_points) + "\n";
        }
    }

    sendServerMessageArea(l_message + l_winner + "\n" + l_results);
}
