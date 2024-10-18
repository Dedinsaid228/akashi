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
#include "server.h"
#include "acl_roles_handler.h"
#include "aoclient.h"
#include "area_data.h"
#include "command_extension.h"
#include "config_manager.h"
#include "db_manager.h"
#include "discord.h"
#include "hub_data.h"
#include "logger/u_logger.h"
#include "music_manager.h"
#include "network/network_socket.h"
#include "packet/packet_factory.h"
#include "serverpublisher.h"

Server::Server(int p_ws_port, QObject *parent) :
    QObject(parent),
    m_port(p_ws_port),
    m_player_count(0)
{
    timer = new QTimer(this);
    db_manager = new DBManager;

    acl_roles_handler = new ACLRolesHandler(this);
    acl_roles_handler->loadFile("config/acl_roles.ini");

    command_extension_collection = new CommandExtensionCollection;
    command_extension_collection->setCommandNameWhitelist(AOClient::COMMANDS.keys());
    command_extension_collection->loadFile("config/command_extensions.ini");

    // We create it, even if its not used later on.
    discord = new Discord(this);
    logger = new ULogger(this);
    connect(this, &Server::logConnectionAttempt,
            logger, &ULogger::logConnectionAttempt);

    http = new QNetworkAccessManager(this);

    AOPacket::registerPackets();
}

void Server::start()
{
    QString bind_ip = ConfigManager::bindIP();
    QHostAddress bind_addr;

    if (bind_ip == "all")
        bind_addr = QHostAddress::Any;
    else
        bind_addr = QHostAddress(bind_ip);

    if (bind_addr.protocol() != QAbstractSocket::IPv4Protocol && bind_addr.protocol() != QAbstractSocket::IPv6Protocol && bind_addr != QHostAddress::Any)
        qDebug() << bind_ip << "is an invalid IP address to listen on! Server not starting, check your config.";

    server = new QWebSocketServer("Akashi", QWebSocketServer::NonSecureMode, this);
    if (!server->listen(bind_addr, m_port))
        qDebug() << "Server error:" << server->errorString();
    else {
        connect(server, &QWebSocketServer::newConnection,
                this, &Server::clientConnected);
        qInfo() << "Server listening on" << server->serverPort();
    }

    // Checks if any Discord webhooks are enabled.
    handleDiscordIntegration();

    // Construct modern advertiser if enabled in config
    server_publisher = new ServerPublisher(server->serverPort(), &m_player_count, this);

    // Get characters from config file
    m_characters = ConfigManager::charlist();

    // Get backgrounds from config file
    m_backgrounds = ConfigManager::backgrounds();

    // Build our music manager.
    QStringList l_musiclist = ConfigManager::musiclist();
    music_manager = new MusicManager(ConfigManager::cdnList(), l_musiclist, ConfigManager::ordered_songs(), this);
    connect(music_manager, &MusicManager::sendFMPacket, this, &Server::unicast);
    connect(music_manager, &MusicManager::sendAreaFMPacket, this, QOverload<std::shared_ptr<AOPacket>, int>::of(&Server::broadcast));

    // Get musiclist from config file
    m_music_list = music_manager->rootMusiclist();

    // Assembles the area list
    m_area_names = ConfigManager::sanitizedAreaNames();
    for (int i = 0; i < m_area_names.length(); i++) {
        QString area_name = QString::number(i) + ":" + m_area_names[i];
        AreaData *l_area = new AreaData(area_name, i, music_manager);
        m_areas.insert(i, l_area);
        connect(l_area, &AreaData::sendAreaPacket,
                this, QOverload<std::shared_ptr<AOPacket>, int>::of(&Server::broadcast));
        connect(l_area, &AreaData::userJoinedArea,
                music_manager, &MusicManager::userJoinedArea);
        connect(l_area, &AreaData::sendAreaPacketClient,
                this, &Server::unicast);
        music_manager->registerArea(i);
        QSettings *areas_ini = ConfigManager::areaData();
        areas_ini->beginGroup(area_name);
        music_manager->setCustomMusicList(areas_ini->value("musiclist", "").toStringList(), i);
        areas_ini->endGroup();
    }

    // Assembles the hub list
    m_hub_names = ConfigManager::sanitizedHubNames();
    QStringList raw_hub_names = ConfigManager::rawHubNames();
    for (int i = 0; i < raw_hub_names.length(); i++) {
        QString hub_name = raw_hub_names[i];
        HubData *l_hub = new HubData(hub_name, i);
        m_hubs.insert(i, l_hub);
    }

    // Get IP bans
    m_ipban_list = ConfigManager::iprangeBans();
    m_ipignore_list = ConfigManager::ipignoreBans();

    // Rate-Limiter for IC-Chat
    m_message_floodguard_timer = new QTimer(this);
    connect(m_message_floodguard_timer, &QTimer::timeout, this, &Server::allowMessage);

    // Prepare player IDs and reference hash.
    for (int i = ConfigManager::maxPlayers() - 1; i >= 0; i--) {
        m_available_ids.push(i);
        m_clients_ids.insert(i, nullptr);
    }

    request_version([this](QString version) { m_latest_version = version; });
}

QVector<AOClient *> Server::getClients() { return m_clients; }

void Server::renameArea(QString f_areaNewName, int f_areaIndex) { m_area_names[f_areaIndex] = f_areaNewName; }

void Server::addArea(QString f_areaName, int f_areaIndex, QString f_hubIndex)
{
    AreaData *l_area = new AreaData(QString::number(f_areaIndex) + ":" + f_hubIndex + ":" + f_areaName, f_areaIndex, music_manager);
    m_areas.insert(f_areaIndex, l_area);
    m_area_names.insert(f_areaIndex, f_areaName);
    connect(l_area, &AreaData::sendAreaPacket, this,
            QOverload<std::shared_ptr<AOPacket>, int>::of(&Server::broadcast));
    connect(l_area, &AreaData::userJoinedArea,
            music_manager, &MusicManager::userJoinedArea);
    music_manager->registerArea(f_areaIndex);
}

void Server::removeArea(int f_areaNumber)
{
    delete m_areas[f_areaNumber];
    m_areas[f_areaNumber] = nullptr;
    m_areas.removeAll(m_areas[f_areaNumber]);
    m_area_names.removeAll(m_area_names[f_areaNumber]);
}

void Server::swapAreas(int f_area1, int f_area2)
{
    m_areas.swapItemsAt(f_area1, f_area2);
    m_area_names.swapItemsAt(f_area1, f_area2);
}

void Server::renameHub(QString f_hubNewName, int f_hubIndex) { m_hub_names[f_hubIndex] = f_hubNewName; }

void Server::clientConnected()
{
    QWebSocket *socket = server->nextPendingConnection();
    NetworkSocket *l_socket = new NetworkSocket(socket, socket);
    // Too many players. Reject connection!
    // This also enforces the maximum playercount.
    if (m_available_ids.empty()) {
        std::shared_ptr<AOPacket> disconnect_reason = PacketFactory::createPacket("BD", {"Maximum playercount has been reached."});
        l_socket->write(disconnect_reason);
        l_socket->close();
        l_socket->deleteLater();
        return;
    }

    int user_id = m_available_ids.pop();
    AOClient *client = new AOClient(this, l_socket, l_socket, user_id, music_manager);
    m_clients_ids.insert(user_id, client);
    m_player_state_observer.registerClient(client);
    client->calculateIpid();
    client->clientConnected();

    int multiclient_count = 1;
    for (AOClient *joined_client : std::as_const(m_clients))
        if (client->m_remote_ip.isEqual(joined_client->m_remote_ip))
            multiclient_count++;

    bool is_at_multiclient_limit = false;
    if (multiclient_count > ConfigManager::multiClientLimit() && !client->m_remote_ip.isLoopback())
        is_at_multiclient_limit = true;

    auto ban = db_manager->isIPBanned(client->getIpid());
    bool is_banned = ban.first;
    if (is_banned) {
        QString ban_duration;
        if (!(ban.second.duration == -2))
            ban_duration = QDateTime::fromSecsSinceEpoch(ban.second.time).addSecs(ban.second.duration).toString("MM/dd/yyyy, hh:mm");
        else
            ban_duration = "Permanently.";

        std::shared_ptr<AOPacket> ban_reason = PacketFactory::createPacket("BD", {"Reason: " + ban.second.reason + "\nBan ID: " + QString::number(ban.second.id) + "\nUntil: " + ban_duration});
        socket->sendTextMessage(ban_reason->toUtf8());
    }

    if (is_banned || is_at_multiclient_limit) {
        client->deleteLater();
        l_socket->close(QWebSocketProtocol::CloseCodeNormal);
        markIDFree(user_id);
        return;
    }

    QHostAddress l_remote_ip = client->m_remote_ip;
    if (l_remote_ip.protocol() == QAbstractSocket::IPv6Protocol)
        l_remote_ip = parseToIPv4(l_remote_ip);

    if (isIPBanned(l_remote_ip)) {
        QString l_reason = "Your IP has been banned by a moderator.";
        std::shared_ptr<AOPacket> l_ban_reason = PacketFactory::createPacket("BD", {l_reason});
        l_socket->write(l_ban_reason);
        client->deleteLater();
        l_socket->close(QWebSocketProtocol::CloseCodeNormal);
        markIDFree(user_id);
        return;
    }

    m_clients.append(client);
    connect(l_socket, &NetworkSocket::clientDisconnected, this, [=, this] {
        if (client->hasJoined())
            decreasePlayerCount();

        m_clients.removeAll(client);
        l_socket->deleteLater();
    });

    connect(l_socket, &NetworkSocket::handlePacket, client, &AOClient::handlePacket);

    // This is the infamous workaround for
    // tsuserver4. It should disable fantacrypt
    // completely in any client 2.4.3 or newer
    std::shared_ptr<AOPacket> decryptor = PacketFactory::createPacket("decryptor", {"NOENCRYPT"});
    client->sendPacket(decryptor);
    hookupAOClient(client);
}

void Server::updateCharsTaken(AreaData *area)
{
    QStringList chars_taken;
    for (const QString &cur_char : std::as_const(m_characters))
        chars_taken.append(area->charactersTaken().contains(getCharID(cur_char))
                               ? QStringLiteral("-1")
                               : QStringLiteral("0"));

    std::shared_ptr<AOPacket> response_cc = PacketFactory::createPacket("CharsCheck", chars_taken);
    for (AOClient *client : std::as_const(m_clients))
        if (client->areaId() == area->index()) {
            if (!client->m_is_charcursed)
                client->sendPacket(response_cc);
            else {
                QStringList chars_taken_cursed = getCursedCharsTaken(client, chars_taken);
                std::shared_ptr<AOPacket> response_cc_cursed = PacketFactory::createPacket("CharsCheck", chars_taken_cursed);
                client->sendPacket(response_cc_cursed);
            }
        }
}

QStringList Server::getCursedCharsTaken(AOClient *client, QStringList chars_taken)
{
    QStringList chars_taken_cursed;
    for (int i = 0; i < chars_taken.length(); i++)
        if (!client->m_charcurse_list.contains(i))
            chars_taken_cursed.append("-1");
        else
            chars_taken_cursed.append(chars_taken.value(i));

    return chars_taken_cursed;
}

bool Server::isMessageAllowed() const { return m_can_send_ic_messages; }

QHostAddress Server::parseToIPv4(QHostAddress f_remote_ip)
{
    bool l_ok;
    QHostAddress l_remote_ip = f_remote_ip;
    QHostAddress l_temp_remote_ip = QHostAddress(f_remote_ip.toIPv4Address(&l_ok));
    if (l_ok)
        l_remote_ip = l_temp_remote_ip;

    return l_remote_ip;
}

void Server::reloadSettings(int f_uid)
{
    ConfigManager::reloadSettings();
    emit reloadRequest(ConfigManager::serverName(), ConfigManager::serverDescription());
    emit updateHTTPConfiguration();
    logger->loadLogtext();
    m_ipban_list = ConfigManager::iprangeBans();
    m_ipignore_list = ConfigManager::ipignoreBans();
    acl_roles_handler->loadFile("config/acl_roles.ini");
    command_extension_collection->loadFile("config/command_extensions.ini");
    m_backgrounds = ConfigManager::backgrounds();
    music_manager->reloadRequest();
    m_music_list = music_manager->rootMusiclist();
    QStringList l_characters = m_characters;
    m_characters = ConfigManager::charlist();

    const QVector<AOClient *> l_clients = getClients();
    for (AOClient *l_client : l_clients) {
        l_client->sendPacket("FM", music_manager->musiclist(l_client->areaId()));

        if (m_characters != l_characters) {
            l_client->sendPacket("SC", getCharacters());
            l_client->changeCharacter(l_client->SPECTATOR_ID);
            l_client->sendPacket("DONE");
        }
    }

    for (int i = 0; i < getAreaCount(); i++)
        updateCharsTaken(getAreaById(i));

    AOClient *l_client = getClientByID(f_uid);
    l_client->sendServerMessage("Configurations is reloaded.");
}

void Server::hubListen(QString message, int area_index, QString sender_name, int sender_id)
{
    for (AOClient *client : std::as_const(m_clients))
        if (!client->m_blinded && client->m_hub_listen && client->hubId() == getAreaById(area_index)->getHub())
            client->sendServerMessage("[" + QString::number(sender_id) + "] " + sender_name + " in the area [" + QString::number(client->m_area_list.indexOf(area_index)) + "] " + getAreaName(area_index) + ": " + message);
}

void Server::broadcast(std::shared_ptr<AOPacket> packet, int area_index)
{
    QVector<int> l_client_ids = m_areas.value(area_index)->joinedIDs();
    for (const int l_client_id : std::as_const(l_client_ids))
        if (!getClientByID(l_client_id)->m_blinded)
            getClientByID(l_client_id)->sendPacket(packet);
}

void Server::broadcast(std::shared_ptr<AOPacket> packet)
{
    for (AOClient *client : std::as_const(m_clients))
        if (!client->m_blinded)
            client->sendPacket(packet);
}

void Server::broadcast(std::shared_ptr<AOPacket> packet, TARGET_TYPE target)
{
    for (AOClient *l_client : std::as_const(m_clients))
        switch (target) {
        case TARGET_TYPE::MODCHAT:
            if (l_client->checkPermission(ACLRole::MODCHAT))
                l_client->sendPacket(packet);
            break;
        case TARGET_TYPE::ADVERT:
            if (l_client->m_advert_enabled)
                l_client->sendPacket(packet);
            break;
        default:
            break;
        }
}

void Server::broadcast(std::shared_ptr<AOPacket> packet, std::shared_ptr<AOPacket> other_packet, TARGET_TYPE target)
{
    switch (target) {
    case TARGET_TYPE::AUTHENTICATED:
        for (AOClient *client : std::as_const(m_clients))
            if (client->isAuthenticated())
                client->sendPacket(other_packet);
            else
                client->sendPacket(packet);
    default:
        // Unimplemented, so not handled.
        break;
    }
}

void Server::broadcast(int hub_index, std::shared_ptr<AOPacket> packet)
{
    for (AOClient *client : std::as_const(m_clients))
        if (!client->m_blinded && client->hubId() == hub_index)
            client->sendPacket(packet);
}

void Server::unicast(std::shared_ptr<AOPacket> f_packet, int f_client_id)
{
    AOClient *l_client = getClientByID(f_client_id);
    if (l_client != nullptr) { // This should never happen, but safety first.
        l_client->sendPacket(f_packet);
        return;
    }
}

QList<AOClient *> Server::getClientsByIpid(QString ipid)
{
    QList<AOClient *> return_clients;
    for (AOClient *client : std::as_const(m_clients))
        if (client->getIpid() == ipid)
            return_clients.append(client);

    return return_clients;
}

QList<AOClient *> Server::getClientsByHwid(QString f_hwid)
{
    QList<AOClient *> return_clients;
    for (AOClient *l_client : std::as_const(m_clients))
        if (l_client->getHwid() == f_hwid)
            return_clients.append(l_client);

    return return_clients;
}

AOClient *Server::getClientByID(int id) { return m_clients_ids.value(id); }

int Server::getPlayerCount() { return m_player_count; }

QStringList Server::getCharacters() { return m_characters; }

int Server::getCharacterCount() { return m_characters.length(); }

QString Server::getCharacterById(int f_chr_id)
{
    QString l_chr;
    if (f_chr_id >= 0 && f_chr_id < m_characters.length())
        l_chr = m_characters.at(f_chr_id);

    return l_chr;
}

int Server::getCharID(QString char_name)
{
    for (const QString &character : std::as_const(m_characters))
        if (character.toLower() == char_name.toLower()) {
            QString escaped_character = QRegularExpression::escape(character);
            return m_characters.indexOf(QRegularExpression(escaped_character, QRegularExpression::CaseInsensitiveOption));
        }

    return -1; // character does not exist
}

QVector<AreaData *> Server::getAreas() { return m_areas; }

QVector<HubData *> Server::getHubs() { return m_hubs; }

QVector<AreaData *> Server::getClientAreas(int f_hub)
{
    QVector<AreaData *> l_areas;
    for (int i = 0; i < m_areas.size(); i++) {
        AreaData *l_area = getAreaById(i);
        if (l_area->getHub() == f_hub)
            l_areas.append(l_area);
    }

    return l_areas;
}

int Server::getAreaCount() { return m_areas.length(); }

int Server::getHubsCount() { return m_hubs.length(); }

AreaData *Server::getAreaById(int f_area_id)
{
    AreaData *l_area = nullptr;
    if (f_area_id >= 0 && f_area_id < m_areas.length())
        l_area = m_areas.at(f_area_id);

    return l_area;
}

QQueue<QString> Server::getAreaBuffer(const QString &f_areaName) { return logger->buffer(f_areaName); }

QStringList Server::getAreaNames() { return m_area_names; }

QStringList Server::getClientAreaNames(int f_hub)
{
    QStringList l_area_names;
    for (int i = 0; i < m_areas.size(); i++) {
        AreaData *l_area = getAreaById(i);
        if (l_area->getHub() == f_hub)
            l_area_names.append(getAreaName(i));
    }

    return l_area_names;
}

QString Server::getAreaName(int f_area_id)
{
    QString l_name;
    if (f_area_id >= 0 && f_area_id < m_area_names.length())
        l_name = m_area_names.at(f_area_id);

    return l_name;
}

HubData *Server::getHubById(int f_hub_id)
{
    HubData *l_hub = nullptr;
    if (f_hub_id >= 0 && f_hub_id < m_areas.length())
        l_hub = m_hubs.at(f_hub_id);

    return l_hub;
}

QString Server::getHubName(int f_hub_id)
{
    QString l_name;
    if (f_hub_id >= 0 && f_hub_id < m_hub_names.length())
        l_name = m_hub_names.at(f_hub_id);

    return l_name;
}

QStringList Server::getMusicList() { return m_music_list; }

QStringList Server::getBackgrounds() { return m_backgrounds; }

DBManager *Server::getDatabaseManager() { return db_manager; }

ACLRolesHandler *Server::getACLRolesHandler() { return acl_roles_handler; }

CommandExtensionCollection *Server::getCommandExtensionCollection() { return command_extension_collection; }

void Server::allowMessage() { m_can_send_ic_messages = true; }

void Server::handleDiscordIntegration()
{
    // Prevent double connecting by preemtively disconnecting them.
    disconnect(this, nullptr, discord, nullptr);

    if (ConfigManager::discordWebhookEnabled()) {
        if (ConfigManager::discordModcallWebhookEnabled())
            connect(this, &Server::modcallWebhookRequest,
                    discord, &Discord::onModcallWebhookRequested);

        if (ConfigManager::discordBanWebhookEnabled())
            connect(this, &Server::banWebhookRequest,
                    discord, &Discord::onBanWebhookRequested);

        if (ConfigManager::discordUptimeEnabled())
            discord->startUptimeTimer();
        else
            discord->stopUptimeTimer();
    }

    return;
}

void Server::markIDFree(const int &f_user_id)
{
    m_player_state_observer.unregisterClient(m_clients_ids[f_user_id]);
    m_clients_ids.insert(f_user_id, nullptr);
    m_available_ids.push(f_user_id);
}

void Server::hookupAOClient(AOClient *client)
{
    connect(client, &AOClient::logIC, logger, &ULogger::logIC);
    connect(client, &AOClient::logOOC, logger, &ULogger::logOOC);
    connect(client, &AOClient::logLogin, logger, &ULogger::logLogin);
    connect(client, &AOClient::logCMD, logger, &ULogger::logCMD);
    connect(client, &AOClient::logBan, logger, &ULogger::logBan);
    connect(client, &AOClient::logKick, logger, &ULogger::logKick);
    connect(client, &AOClient::logModcall, logger, &ULogger::logModcall);
    connect(client, &AOClient::clientSuccessfullyDisconnected, this, &Server::markIDFree);
    connect(client, &AOClient::logDisconnect, logger, &ULogger::logDisconnect);
    connect(client, &AOClient::logMusic, logger, &ULogger::logMusic);
    connect(client, &AOClient::logChangeChar, logger, &ULogger::logChangeChar);
    connect(client, &AOClient::logChangeArea, logger, &ULogger::logChangeArea);
}

void Server::increasePlayerCount()
{
    m_player_count++;
    emit playerCountUpdated(m_player_count);
}

void Server::decreasePlayerCount()
{
    m_player_count--;
    emit playerCountUpdated(m_player_count);
}

bool Server::isIPBanned(QHostAddress f_remote_IP)
{
    bool l_match_found = false;
    for (const QString &l_ipban : std::as_const(m_ipban_list))
        if (f_remote_IP.isInSubnet(QHostAddress::parseSubnet(l_ipban)))
            if (!isIPignored(f_remote_IP.toString().replace(":ffff:", ""))) {
                l_match_found = true;
                break;
            }

    return l_match_found;
}

bool Server::isIPignored(QString ip)
{
    bool l_match_found = false;
    for (const QString &l_ip : std::as_const(m_ipignore_list))
        if (l_ip == ip) {
            l_match_found = true;
            break;
        }

    return l_match_found;
}

void Server::request_version(const std::function<void(QString)> &cb)
{
    QString version_url = "https://sshapeshifter.ru/kakashiversion";
    QNetworkRequest req(version_url);
    QNetworkReply *reply = http->get(req);
    connect(reply, &QNetworkReply::finished, this, [cb, reply] {
        QString content = QString::fromUtf8(reply->readAll()).replace("\n", "");
        int http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (content.isEmpty() || http_status != 200)
            content = QString();

        cb(content);
        reply->deleteLater();
    });
}

void Server::check_version()
{
    request_version([this](QString version) {if (version != m_latest_version) { m_latest_version = version;} });
}

Server::~Server()
{
    for (AOClient *client : std::as_const(m_clients))
        client->deleteLater();

    server->deleteLater();
    discord->deleteLater();
    acl_roles_handler->deleteLater();
    delete db_manager;
}
