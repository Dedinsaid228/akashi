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
#include "aoclient.h"
#include "area_data.h"
#include "config_manager.h"
#include "hub_data.h"
#include "packet/packet_factory.h"
#include "qhttpmultipart.h"
#include "server.h"

#include "QtConcurrent/qtconcurrentrun.h"
#include <QProcess>
#include <QQueue>

// This file is for functions used by various commands, defined in the command helper function category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdDefault(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("Invalid command.");
    return;
}

QStringList AOClient::buildAreaList(int area_idx, bool ignore_hubs)
{
    QStringList entries;
    AreaData *area = server->getAreaById(area_idx);
    if (area->getHub() != hubId() && ignore_hubs)
        return entries;

    entries.append("=== " + server->getAreaName(area_idx) + " ===");

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

    if (server->getHubById(area->getHub())->getHidePlayerCount()) {
        entries.append("[???][" + area->status().replace("_", "-").toUpper() + "]");
        return entries;
    }

    entries.append("[" + QString::number(area->playerCount()) + " users][" + area->status().replace("_", "-").toUpper() + "]");

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->areaId() == area_idx && l_client->hasJoined()) {
            QString char_entry = "[" + QString::number(l_client->clientId()) + "] " + l_client->character();
            if (l_client->character() == "")
                char_entry += "Spectator";
            if (area->owners().contains(l_client->clientId()))
                char_entry.insert(0, "[CM]");
            if (server->getHubById(l_client->hubId())->hubOwners().contains(l_client->clientId()))
                char_entry.insert(0, "[GM]");
            if (l_client->characterName() != "")
                char_entry += " (" + l_client->characterName() + ")";
            if (l_client->m_pos != "" && l_client->character() != "")
                char_entry += " <" + l_client->m_pos + "> ";
            if (m_authenticated)
                char_entry += " (" + l_client->getIpid() + "): " + l_client->name();
            if (!m_authenticated && area->owners().contains(clientId()))
                char_entry += ": " + l_client->name();
            if (l_client->m_vote_candidate)
                char_entry.insert(0, "[VC]");
            if (l_client->m_is_afk)
                char_entry.insert(0, "[AFK]");

            entries.append(char_entry);
        }
    }

    return entries;
}

int AOClient::genRand(int min, int max) { return QRandomGenerator::system()->bounded(min, max + 1); }

void AOClient::diceThrower(int sides, int dice, bool p_roll, int roll_modifier)
{
    if (sides < 0 || dice < 0 || sides > ConfigManager::diceMaxValue() || dice > ConfigManager::diceMaxDice()) {
        sendServerMessage("Dice or side number out of bounds.");
        return;
    }

    QStringList results;
    for (int i = 1; i <= dice; i++)
        results.append(QString::number(AOClient::genRand(1, sides) + roll_modifier));

    QString total_results = results.join(" ");
    if (p_roll) {
        if (roll_modifier)
            sendServerMessage("You have rolled a " + QString::number(dice) + "d" + QString::number(sides) + "+" + QString::number(roll_modifier) + ". Results: " + total_results);
        else
            sendServerMessage("You have rolled a " + QString::number(dice) + "d" + QString::number(sides) + ". Results: " + total_results);
        return;
    }

    QString l_sender_name = getSenderName(clientId());
    if (roll_modifier)
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + l_sender_name + " has rolled a " + QString::number(dice) + "d" + QString::number(sides) + "+" + QString::number(roll_modifier) + ". Results: " + total_results);
    else
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + l_sender_name + " has rolled a " + QString::number(dice) + "d" + QString::number(sides) + ". Results: " + total_results);
}

QString AOClient::getAreaTimer(int area_idx, int timer_idx)
{
    AreaData *l_area = server->getAreaById(area_idx);
    QTimer *l_timer;
    if (timer_idx == 0)
        l_timer = server->timer;
    else if (timer_idx > 0 && timer_idx <= 4)
        l_timer = l_area->timers().at(timer_idx - 1);
    else
        return "Invalid timer ID.";

    QString l_timer_name = (timer_idx == 0) ? "Global timer" : "Timer " + QString::number(timer_idx);
    if (l_timer->isActive()) {
        QTime current_time = QTime(0, 0).addMSecs(l_timer->remainingTime());
        return l_timer_name + " is at " + current_time.toString("hh:mm:ss.zzz");
    }
    else
        return l_timer_name + " is inactive.";
}

long long AOClient::parseTime(QString input)
{
    static QRegularExpression l_regex("(?:(?:(?<year>.*?)y)*(?:(?<week>.*?)w)*(?:(?<day>.*?)d)*(?:(?<hr>.*?)h)*(?:(?<min>.*?)m)*(?:(?<sec>.*?)s)*)");
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
    if (!l_is_well_formed)
        return -1;

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
    if (!ConfigManager::passwordRequirements())
        return true;

    QString l_decoded_password = decodeMessage(f_password);
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
        static QRegularExpression regex("[0123456789]");
        QRegularExpressionMatch match = regex.match(l_decoded_password);

        if (!match.hasMatch())
            return false;
    }
    else if (ConfigManager::passwordRequireSpecialCharacters()) {
        static QRegularExpression regex("[~!@#$%^&*_-+=`|\\(){}\[]:;\"'<>,.?/]");
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
    std::shared_ptr<AOPacket> l_packet = PacketFactory::createPacket("BB", {l_message});

    if (f_global)
        server->broadcast(l_packet);
    else
        server->broadcast(l_packet, areaId());

    emit logCMD((character() + " " + characterName()), m_ipid, name(), "NOTICE", f_notice, server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::playMusic(QStringList f_args, bool f_hubbroadcast, bool f_once, bool f_gdrive, bool f_ambience)
{
    if (m_is_spectator) {
        sendServerMessage("Spectator are blocked from changing the music/ambience.");
        return;
    }

    if (m_is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music/ambience.");
        return;
    }

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_music_change_time <= 2) {
        sendServerMessage("You change the music/ambience very often!");
        return;
    }

    m_last_music_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->isMusicAllowed() == false && !checkPermission(ACLRole::CM)) {
        sendServerMessage("The music/ambience change is disabled in this area.");
        return;
    }

    QString l_song = f_args.join(" ");
    if (!l_song.startsWith("http") && !server->getMusicList().contains(l_song) && l_song != "~stop.mp3") {
        sendServerMessage("Unknown music file! You may have made a mistake in the filename or in the link.");
        return;
    }

    if (l_song.startsWith("https://youtube.com/") || l_song.startsWith("https://youtu.be/") || l_song.startsWith("https://www.youtube.com/") || l_song.startsWith("https://www.youtu.be/")) {
        if (ConfigManager::useYtdlp()) {
            sendServerMessage("Start loading... it takes some time.");
            QProcess *l_proc = new QProcess();
            QString l_path = "temp/" + QString::number(areaId()) + "/" + QUuid::createUuid().toString() + ".opus";

#ifdef Q_OS_WIN
            l_proc->start("./yt-dlp.exe", {"-x", "-q", "--audio-format", "opus", "-o", l_path, l_song});
#else
            l_proc->start("yt-dlp", {"-x", "-q", "--audio-format", "opus", "-o", l_path, l_song});
#endif

            QObject::connect(l_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                             [this, l_path, f_hubbroadcast, f_once, f_ambience](int l_code, QProcess::ExitStatus l_status) mutable {
                                 if (l_status == QProcess::NormalExit && l_code == 0) {
                                     auto l_future = QtConcurrent::run([this, l_path]() {
                                         return this->uploadToLetterBox(l_path);
                                     });

                                     QFutureWatcher<QString> *l_watch = new QFutureWatcher<QString>(this);
                                     connect(l_watch, &QFutureWatcher<QString>::finished, this, [l_watch, f_hubbroadcast, f_once, f_ambience, this]() {
                                         QString l_link = l_watch->future().result();
                                         if (!l_link.isEmpty())
                                             startMusicPlaying(l_link, f_hubbroadcast, f_once, f_ambience);
                                         else
                                             sendServerMessage("Error: the server got a empty link!");
                                         l_watch->deleteLater();
                                     });

                                     l_watch->setFuture(l_future);
                                 }
                                 else
                                     sendServerMessage("Audio download failed: code " + QString::number(l_code));
                             });

            return;
        }
        else {
            sendServerMessage("You cannot use a YouTube link.");
            return;
        }
    }

    if (f_gdrive)
        l_song = "https://drive.google.com/uc?export=download&id=" + f_args.join(" ");

    startMusicPlaying(l_song, f_hubbroadcast, f_once, f_ambience);
}

QString AOClient::uploadToLetterBox(QString f_path)
{
    QFile *l_file = new QFile(f_path);
    if (!l_file->open(QIODevice::ReadOnly)) {
        sendServerMessage("Error: Failed to open file.");
        delete l_file;
        return "";
    }

    QHttpPart l_filepart;
    l_filepart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant("form-data; name=\"fileToUpload\"; filename=\"" + QFileInfo(f_path).fileName() + "\""));

    QHttpMultiPart *l_multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    l_filepart.setBodyDevice(l_file);
    l_file->setParent(l_multipart);
    l_multipart->append(l_filepart);

    QHttpPart l_timepart;
    l_timepart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"time\""));
    l_timepart.setBody("1h");
    l_multipart->append(l_timepart);

    QHttpPart l_typepart;
    l_typepart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"reqtype\""));
    l_typepart.setBody("fileupload");
    l_multipart->append(l_typepart);

    QNetworkAccessManager *l_manager = new QNetworkAccessManager(this);
    QNetworkReply *l_reply = l_manager->post(QNetworkRequest(QUrl("https://litterbox.catbox.moe/resources/internals/api.php")), l_multipart);
    l_multipart->setParent(l_reply);

    QEventLoop l_eventloop;
    QObject::connect(l_reply, &QNetworkReply::finished, &l_eventloop, &QEventLoop::quit);
    l_eventloop.exec();

    QString l_response;
    if (l_reply->error() == QNetworkReply::NoError)
        l_response = l_reply->readAll();
    else {
        sendServerMessage("Error: " + l_reply->errorString());
        l_response = "";
    }

    l_reply->deleteLater();
    l_file->remove();
    l_file->deleteLater();

    return l_response;
}

void AOClient::startMusicPlaying(QString f_song, bool f_hubbroadcast, bool f_once, bool f_ambience)
{
    AreaData *l_area = server->getAreaById(areaId());
    QString l_play_once;
    if (f_once)
        l_play_once = "0";
    else
        l_play_once = "1";

    std::shared_ptr<AOPacket> music_change;
    if (f_ambience)
        music_change = PacketFactory::createPacket("MC", {f_song, "-1", characterName(), "1", "1"});
    else
        music_change = PacketFactory::createPacket("MC", {f_song, QString::number(server->getCharID(character())), characterName(), l_play_once, "0"});

    if (f_hubbroadcast) {
        server->broadcast(hubId(), music_change);

        for (int i = 0; i < server->getAreaCount(); i++) {
            AreaData *area = server->getAreaById(i);
            if (area->getHub() == hubId())
                area->changeMusic(getSenderName(clientId()), f_song);
        }
    }
    else {
        server->broadcast(music_change, areaId());
        if (f_ambience)
            l_area->changeAmbience(f_song);
        else
            l_area->changeMusic(getSenderName(clientId()), f_song);
    }

    emit logMusic((character() + " " + characterName()), name(), m_ipid, server->getAreaById(areaId())->name(), f_song, QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

QString AOClient::getSenderName(int f_uid)
{
    AOClient *l_target = server->getClientByID(f_uid);
    if (l_target->characterName().isEmpty() && l_target->character().isEmpty())
        return "Spectator";
    else if (l_target->characterName().isEmpty())
        return l_target->character();
    else
        return l_target->characterName();
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

QString AOClient::getLockStatus(int f_area)
{
    AreaData *l_area = server->getAreaById(f_area);
    switch (l_area->lockStatus()) {
    case AreaData::LockStatus::FREE:
        return "FREE";
    case AreaData::LockStatus::SPECTATABLE:
        return "SPECTATABLE";
    case AreaData::LockStatus::LOCKED:
        return "LOCKED";
    }

    return "UNKNOWN";
}

QString AOClient::getOocType(int f_area)
{
    AreaData *l_area = server->getAreaById(f_area);
    switch (l_area->oocType()) {
    case AreaData::OocType::ALL:
        return "ALL";
    case AreaData::OocType::CM:
        return "CM";
    case AreaData::OocType::INVITED:
        return "INVITED";
    }

    return "UNKNOWN";
}

QString AOClient::getHubLockStatus(int f_hub)
{
    HubData *l_hub = server->getHubById(f_hub);
    switch (l_hub->hubLockStatus()) {
    case HubData::HubLockStatus::FREE:
        return "FREE";
    case HubData::HubLockStatus::SPECTATABLE:
        return "SPECTATABLE";
    case HubData::HubLockStatus::LOCKED:
        return "LOCKED";
    }

    return "UNKNOWN";
}

void AOClient::endVote()
{
    QString l_message = "Results: \nWinner(-s) is/are ";
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
                l_winner = "[" + QString::number(l_client->clientId()) + "] " + getSenderName(l_client->clientId());
            }
            else if (l_client->m_vote_points == l_winner_points)
                l_winner = l_winner + ", [" + QString::number(l_client->clientId()) + "] " + getSenderName(l_client->clientId());

            l_results += "\n[" + QString::number(l_client->clientId()) + "] " + getSenderName(l_client->clientId()) + " = " + QString::number(l_client->m_vote_points);
        }
    }

    if (l_winner_points == 0)
        l_winner = "Draw, maybe?";

    sendServerMessageArea(l_message + l_winner + l_results);
}
