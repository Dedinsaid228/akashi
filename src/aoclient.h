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
#ifndef AOCLIENT_H
#define AOCLIENT_H

#include <algorithm>
#include <memory>

#include <QDateTime>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTimer>
#include <QtGlobal>

#include "acl_roles_handler.h"
#include "network/aopacket.h"
#include "network/network_socket.h"

class AreaData;
class DBManager;
class MusicManager;
class Server;
class NetworkSocket;
class AOPacket;

/**
 * @brief Represents a client connected to the server running Attorney Online 2 or one of its derivatives.
 */
class AOClient : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief Describes a command's details.
     */
    struct CommandInfo
    {
        QVector<ACLRole::Permission> acl_permissions; //!< The permissions necessary to be able to run the command. @see ACLRole::Permission.
        int minArgs;                                  //!< The minimum mandatory arguments needed for the command to function.
        void (AOClient::*action)(int, QStringList);
    };

    /**
     * @property CommandInfo::action
     *
     * @brief A function reference that contains what the command actually does.
     *
     * @param int When called, this parameter will be filled with the argument count. @anchor commandArgc
     * @param QStringList When called, this parameter will be filled the list of arguments. @anchor commandArgv
     */

    /**
     * @brief The list of commands available on the server.
     *
     * @details Generally called with the format of `/command parameters` in the out-of-character chat.
     * @showinitializer
     *
     * @tparam QString The name of the command, without the leading slash.
     * @tparam CommandInfo The details of the command.
     * See @ref CommandInfo "the type's documentation" for more details.
     */
    static const QMap<QString, CommandInfo> COMMANDS;

    /**
     * @brief Creates an instance of the AOClient class.
     *
     * @param p_server A pointer to the Server instance where the client is joining to.
     * @param p_socket The socket associated with the AOClient.
     * @param user_id The user ID of the client.
     * @param parent Qt-based parent, passed along to inherited constructor from QObject.
     */
    AOClient(Server *p_server, NetworkSocket *socket, QObject *parent = nullptr, int user_id = 0, MusicManager *p_manager = nullptr);

    /**
     * @brief Destructor for the AOClient instance.
     *
     * @details Sets the socket to delete later.
     */
    ~AOClient();

    /**
     * @brief Getter for the client's IPID.
     *
     * @return The IPID.
     *
     * @see #m_ipid
     */
    QString getIpid() const;

    /**
     * @brief Getter for the client's HWID.
     *
     * @return The HWID.
     *
     * @see #m_hwid
     */
    QString getHwid() const;

    /**
     * @brief Returns true if the client has completed the participation handshake. False otherwise.
     *
     * @return True if the client has completed the participation handshake. False otherwise.
     */
    bool hasJoined() const;

    /**
     * @brief Returns true if the client has logged-in as a role.
     *
     * @return True if loggged-in, false otherwise.
     */
    bool isAuthenticated() const;

    /**
     * @brief Calculates the client's IPID based on a hashed version of its IP.
     */
    void calculateIpid();

    /**
     * @brief Getter for the pointer to the server.
     *
     * @return See brief description.
     *
     * @note Unused. There isn't really a point to this existing, either.
     *
     * @see #server
     */
    Server *getServer();

    int clientId() const;

    QString name() const;
    void setName(const QString &f_name);

    QString character() const;
    void setCharacter(const QString &f_character);

    QString characterName() const;
    void setCharacterName(const QString &f_showname);

    int areaId() const;
    void setAreaId(const int f_area_id);

    int hubId() const;
    void setHubId(const int f_hub_id);

    /**
     * @brief The IP address of the client.
     */
    QHostAddress m_remote_ip;

    /**
     * @brief The stored character password for the client, used to be able to select passworded characters.
     */
    QString m_password;

    /**
     * @brief True if the client is actually in the server.
     *
     * @details To explain: In AO, clients immediately establish connection to the server when the user clicks on the server's name in the server
     * browser. Thus, as the user browses servers, they constantly connect and disconnect to and from servers.
     *
     * The purpose of this variable is to determine if the user isn't just doing that, but has actually double-clicked the server, and
     * its client has sent the standard handshake packets, which does signify that the client intended to 'join' this server.
     */
    bool m_joined;

    /**
     * @brief The internal name of the character the client is iniswapped to.
     *
     * @note This will be the same as current_char if the client is not iniswapped.
     */
    QString m_current_iniswap;

    /**
     * @brief If using advanced authentication, this is the moderator name that the client has logged in with.
     */
    QString m_moderator_name = "";

    /**
     * @brief The out-of-character name of the client, generally the nickname of the user themself.
     */
    QString m_ooc_name = "";

    /**
     * @brief The custom showname of the client, used when "renaming" already existing characters in-character.
     */
    QString m_showname = "";

    /**
     * @brief If true, the client is willing to receive global messages.
     *
     * @see AOClient::cmdG and AOClient::cmdToggleGlobal
     */
    bool m_global_enabled = true;

    /**
     * @brief If true, the client's messages will be sent in first-person mode.
     *
     * @see AOClient::cmdFirstPerson
     */
    bool m_first_person = false;

    /**
     * @brief If true, the client may not use in-character chat.
     */
    bool m_is_muted = false;

    /**
     * @brief If true, the client may not use out-of-character chat.
     */
    bool m_is_ooc_muted = false;

    /**
     * @brief If true, the client may not use the music list.
     */
    bool m_is_dj_blocked = false;

    /**
     * @brief If true, the client may not use the judge controls.
     */
    bool m_is_wtce_blocked = false;

    /**
     * @brief If true, client movement in areas will be hidden.
     */
    bool m_sneaked = false;

    /**
     * @brief If true, the client may take taked characters.
     */
    bool m_take_taked_char = false;

    /**
     * @brief If true, the client will not be able to receive and send messages.
     */
    bool m_blinded = false;

    /**
     * @brief If true, the client will receive restrictions from WUSO.
     */
    bool m_wuso = false;

    /**
     * @brief If true, the client can use Modcall.
     */
    bool m_usemodcall = true;

    bool m_can_vote = false;

    bool m_vote_candidate = false;

    int m_vote_points = 0;

    QList<int> m_evi_list;

    QList<int> m_area_list;

    bool m_hub_listen = false;

    int m_score = 0;

    /**
     * @brief Represents the client's client software, and its version.
     *
     * @note Though the version number and naming scheme looks vaguely semver-like,
     * do not be misled into thinking it is that.
     */
    struct ClientVersion
    {
        int release = -1;
        int major = -1;
        int minor = -1;
    };

    /**
     * @brief The software and version of the client.
     *
     * @see The struct itself for more details.
     */
    ClientVersion m_version;

    /**
     * @brief A list of 5 casing preferences (def, pro, judge, jury, steno)
     */
    QList<bool> m_casing_preferences = {false, false, false, false, false};

    /**
     * @brief If true, the client's in-character messages will have their word order randomised.
     */
    bool m_is_shaken = false;

    /**
     * @brief If true, the client's in-character messages will have their vowels (English alphabet only) removed.
     */
    bool m_is_disemvoweled = false;

    /**
     * @brief If true, the client's in-character messages will be overwritten by a randomly picked predetermined message.
     */
    bool m_is_gimped = false;

    /**
     * @brief If true, the client will be marked as AFK in /getarea. Automatically applied when a configurable
     * amount of time has passed since the last interaction, or manually applied by /afk.
     */
    bool m_is_afk = false;

    /**
     * @brief If true, the client will not recieve PM messages.
     */
    bool m_pm_mute = false;

    /**
     * @brief If true, the client will recieve advertisements.
     */
    bool m_advert_enabled = true;

    /**
     * @brief If true, the client is restricted to only changing into certain characters.
     */
    bool m_is_charcursed = false;

    /**
     * @brief The list of char IDs a charcursed player is allowed to switch to.
     */
    QList<int> m_charcurse_list;

    /**
     * @brief Password for entering in area, which contains the same password.
     */
    QString m_userpassword = "";

    /**
     * @brief Temporary client permission if client is allowed to save a testimony to server storage.
     */
    bool m_testimony_saving = false;

    /**
     * @brief If true, the client is a spectator and his IC interactions will be limtied.
     */
    bool m_is_spectator = true;

    /**
     * @brief Temporary client permission if client is allowed to save a area config to server storage.
     */
    bool m_area_saving = false;

    /**
     * @brief The hardware ID of the client.
     * @details Generated based on the client's own supplied hardware ID.
     * The client supplied hardware ID is generally a machine unique ID.
     */
    QString m_hwid;

    /**
     * @brief The network socket used by the client.
     */
    NetworkSocket *m_socket;

    bool m_web_client = false;

    /**
     * @brief The IPID of the client.
     *
     * @details Generated based on the client's IP, but cannot be reversed to identify the client's IP.
     */
    QString m_ipid;

    /**
     * @brief The type of area update, used for area update (ARUP) packets.
     */
    enum ARUPType
    {
        PLAYER_COUNT, //!< The packet contains player count updates.
        STATUS,       //!< The packet contains area status updates.
        CM,           //!< The packet contains updates about who's the CM of what area.
        LOCKED        //!< The packet contains updates about what areas are locked.
    };

    /**
     * @brief Checks if the client's ACL role has permission for the given permission.
     * @param f_permission The permission flags.
     * @return True if the client has permission, false otherwise.
     */
    bool checkPermission(ACLRole::Permission f_permission) const;

    /**
     * @brief Returns if the client is a spectator.
     *
     * @return True if the client is a spectator, false otherwise.
     */
    bool isSpectator() const;

    /**
     * @brief Sets the spectator state for the client.
     *
     * @param f_spectator
     */
    void setSpectator(bool f_spectator);

    /**
     * @brief Sends or announces an ARUP update.
     *
     * @param type The type of ARUP to send.
     * @param broadcast If true, the update is sent out to all clients on the server. If false, it is only sent to this client.
     *
     * @see AOClient::ARUPType
     */
    void arup(ARUPType type, bool broadcast, int hub);

    /**
     * @brief Sends all four types of ARUP to the client.
     */
    void fullArup();
    /**
     * @brief Sends an out-of-character message originating from the server to the client.
     *
     * @param message The text of the message to send.
     */
    void sendServerMessage(QString message);

    /**
     * @brief Like with AOClient::sendServerMessage(), but to every client in the client's area.
     *
     * @param message The text of the message to send.
     */
    void sendServerMessageArea(QString message);

    /**
     * @brief Like with AOClient::sendServerMessage(), but to every client in the server.
     *
     * @param message The text of the message to send.
     */
    void sendServerBroadcast(QString message);

    void sendServerMessageHub(QString message);

    /**
     * @brief The main function of the automoderator.
     */
    void autoMod(bool ic_chat = false, int chars = 0);

    void updateLastTime(bool ic_chat, int chars);

    /**
     * @brief An auxiliary function of the automoderator, which is responsible for the mute of the player.
     */
    void autoMute(bool ic_chat);

    /**
     * @brief An auxiliary function of the automoderator, which is responsible for the kick of the player.
     */
    void autoKick();

    /**
     * @brief An auxiliary function of the automoderator, which is responsible for the ban of the player.
     */
    void autoBan();

    /**
     * @brief Calls AOClient::updateEvidenceList() for every client in the current client's area.
     *
     * @param area The current client's area.
     */
    void sendEvidenceList(AreaData *area) const;

    /**
     * @brief Calls AOClient::updateEvidenceListHidCmNoCm() for every client in the current client's area.
     *
     * @param area The current client's area.
     */
    void sendEvidenceListHidCmNoCm(AreaData *area) const;

    /**
     * @brief Updates the evidence list in the area for the client.
     *
     * @param area The client's area.
     */
    void updateEvidenceList(AreaData *area);

    /**
     * @brief Updates the evidence list for better performance of evidencePresent();
     *
     * @param area The client's area.
     */
    void updateEvidenceListHidCmNoCm(AreaData *area);

    bool evidencePresent(QString id);

    void getAreaList();

    /**
     * @brief Removes excessive combining characters from a text.
     *
     * @return See brief description.
     *
     * @see https://en.wikipedia.org/wiki/Zalgo_text
     */
    QString dezalgo(QString p_text);

    /**
     * @brief Checks if the client can modify the evidence in the area.
     *
     * @param area The client's area.
     *
     * @return True if the client can modify the evidence, false if not.
     */
    bool checkEvidenceAccess(AreaData *area);

    /**
     * @brief Changes the client's character.
     *
     * @param char_id The character ID of the client's new character.
     */
    bool changeCharacter(int char_id);

    /**
     * @brief A helper function for logging in a client as moderator.
     *
     * @param message The OOC message the client has sent.
     */
    void loginAttempt(QString message);

    /**
     * @brief Changes the area the client is in.
     *
     * @param new_area The ID of the new area.
     */
    void changeArea(int new_area, bool ignore_cooldown = false);

    /**
     * @brief Handles an incoming command, checking for authorisation and minimum argument count.
     *
     * @param command The incoming command.
     * @param argc The amount of arguments the command was called with. Equivalent to `argv.size()`.
     * @param argv The arguments the command was called with.
     */
    void handleCommand(QString command, int argc, QStringList argv);

    /**
     * @brief A helper function for decoding AO encoding from a QString.
     *
     * @param incoming_message QString to be decoded.
     */
    QString decodeMessage(QString incoming_message);

    /**
     * @brief Adds the last send IC-Message to QVector of the respective area.
     *
     * @details This one pulls double duty to both append IC-Messages to the QVector or insert them, depending on the current recorder enum.
     *
     * @param packet The MS-Packet being recorded with their color changed to green.
     */
    void addStatement(QStringList packet);

    /**
     * @brief Updates the currently displayed IC-Message with the next one send
     * @param packet The IC-Message that will overwrite the currently stored one.
     * @return Returns the updated IC-Message to be send to the other users. It also changes the color to green.
     */
    QStringList updateStatement(QStringList packet);

    /**
     * @brief Convenience function to generate a random integer number between the given minimum and maximum values.
     *
     * @param min The minimum possible value for the random integer, inclusive.
     * @param max The maximum possible value for the random integer, exclusive.
     *
     * @warning `max` must be greater than `min`.
     *
     * @return A randomly generated integer within the bounds given.
     */
    int genRand(int min, int max);

    /**
     * @brief Returns the name of the character or showname of the client.
     *
     * @param f_uid The UID of the desired client.
     */
    QString getSenderName(int f_uid);

    /**
     * @brief A helper function to add recorded packets to an area's judgelog.
     *
     * @param area Pointer to the area where the packet was sent.
     *
     * @param client Pointer to the client that sent the packet.
     *
     * @param action String containing the info that is being recorded.
     */
    void updateJudgeLog(AreaData *area, AOClient *client, QString action);

    /**
     * @brief Pointer to the servers music manager instance.
     */
    MusicManager *m_music_manager;

    /**
     * @brief The text of the last in-character message that was sent by the client.
     *
     * @details Used to determine if the incoming message is a duplicate.
     */
    QString m_last_message;

    /**
     * @brief The time in seconds since the client last sent a Witness Testimony / Cross Examination
     * popup packet.
     *
     * @details Used to filter out potential spam.
     */
    long m_last_wtce_time;

    /**
     * @brief The time in seconds since the client last moved to another area.
     *
     * @details Used to filter out potential spam.
     */
    long m_last_area_change_time;

    /**
     * @brief The time in seconds since the client changed music for the last time.
     *
     * @details Used to filter out potential spam.
     */
    long m_last_music_change_time;

    /**
     * @brief The time in seconds since the client last changed status in the area.
     *
     * @details Used to filter out potential spam.
     */
    long m_last_status_change_time;

    /**
     * @brief The time in seconds since the client sent last message to the IC chat.
     *
     * @details Used by an automoderator.
     */
    long m_lastmessagetime;

    long m_lastoocmessagetime;

    int m_lastmessagechars;

    int m_blankposts_row;

    /**
     * @name Packet helper global variables
     */
    ///@{

    /**
     * @brief If true, the client is a logged-in moderator.
     */
    bool m_authenticated = false;

    /**
     * @brief The ACL role identifier, used to determine what ACL role the client is linked to.
     */
    QString m_acl_role_id;

    /**
     * @brief The character ID of the other character that the client wants to pair up with.
     *
     * @details Though this uses character ID, a client with *that* character ID must exist in the area for the pairing to work.
     * Furthermore, the owner of that character ID must also do the reverse to this client, making their `pairing_with` equal
     * to this client's character ID.
     */
    int m_pairing_with = -1;

    /**
     * @brief The name of the emote last used by the client. No extension.
     *
     * @details This is used for pairing mainly, for the server to be able to craft a smooth-looking transition from one
     * paired-up client talking to the next.
     */
    QString m_emote = "";

    /**
     * @brief The amount the client was last offset by.
     *
     * @details This used to be just a plain number ranging from -100 to 100, but then Crystal mangled it by building some extra data into it.
     * Cheers, love.
     */
    QString m_offset = "";

    /**
     * @brief The last flipped state of the client.
     */
    QString m_flipping = "";

    /**
     * @brief The last reported position of the client.
     */
    QString m_pos = "";

    ///@}

    /**
     * @brief The client's character ID.
     *
     * @details A character ID is just the character's index in the server's character list.
     *
     * In general, the client assumes that this is a continuous block starting from 0.
     */
    int m_char_id = -1;

    /**
     * @brief The spectator character ID
     *
     * @details You may assume that AO has a sane way to determine if a user is a spectator
     * or an actual player. Well, to nobodys surprise, this is not the case, so the character id -1 is used
     * to determine if a client has entered spectator or user mode. I am making this a const mostly
     * for the case this could change at some point in the future, but don't count on it.
     */
    const int SPECTATOR_ID = -1;

  public slots:
    /**
     * @brief Handles an incoming packet, checking for authorisation and minimum argument count.
     *
     * @param packet The incoming packet.
     */
    void handlePacket(std::shared_ptr<AOPacket> packet);

    /**
     * @brief A slot for when the client disconnects from the server.
     */
    void clientDisconnected(int f_hub);

    void clientConnected();

    /**
     * @brief A slot for sending a packet to the client.
     *
     * @param packet The packet to send.
     */
    void sendPacket(std::shared_ptr<AOPacket> packet);

    /**
     * @overload
     */
    void sendPacket(QString header, QStringList contents);

    /**
     * @overload
     */
    void sendPacket(QString header);

  signals:
    /**
     * @brief This signal is emitted when the client has completed the participation handshake.
     */
    void joined();

    void nameChanged(const QString &);
    void characterChanged(const QString &);
    void characterNameChanged(const QString &);
    void areaIdChanged(int);
    void hubIdChanged(int);

  private:
    /**
     * @brief The user ID of the client.
     */
    int m_id;

    /**
     * @brief The ID of the area the client is currently in.
     */
    int m_current_area;

    /**
     * @brief The internal name of the character the client is currently using.
     */
    QString m_current_char;

    int m_hub = 0;

    /**
     * @brief A pointer to the Server, used for updating server variables that depend on the client (e.g. amount of players in an area).
     */
    Server *server;

    /**
     * @brief Changes the client's in-character position.
     *
     * @param new_pos The new position of the client.
     */
    void changePosition(QString new_pos);

    /**
     * @name Packet helper functions
     */
    ///@{

    ///@}

    /**
     * @property PacketInfo::action
     *
     * @brief A function reference that contains what the packet actually does.
     *
     * @param AreaData This is always just a reference to the data of the area the sender client is in. Used by some packets.
     * @param int When called, this parameter will be filled with the argument count.
     * @param QStringList When called, this parameter will be filled the list of arguments.
     * @param AOPacket This is a duplicated version of the QStringList above, containing the same data.
     */

    /**
     * @name Authentication
     */
    ///@{

    /**
     * @brief Sets the client to be in the process of logging in, setting is_logging_in to **true**.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdLogin(int argc, QStringList argv);

    /**
     * @brief Starts the authorisation type change from `"simple"` to `"advanced"`.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdChangeAuth(int argc, QStringList argv);

    /**
     * @brief Sets the root user's password.
     *
     * @details Accepts a single argument that will be the **root user's password**.
     *
     * @iscommand
     *
     * @pre AOClient::cmdChangeAuth()
     */
    void cmdSetRootPass(int argc, QStringList argv);

    /**
     * @brief Adds a user to the moderators in `"advanced"` authorisation type.
     *
     * @details The first argument is the **user's name**, the second is their **password**.
     *
     * @iscommand
     */
    void cmdAddUser(int argc, QStringList argv);

    /**
     * @brief Removes a user from the moderators in `"advanced"` authorisation type.
     *
     * @details Takes the **targer user's name** as the argument.
     *
     * @iscommand
     */
    void cmdRemoveUser(int argc, QStringList argv);

    /**
     * @brief Lists the permission of a given user.
     *
     * @details If called without argument, lists the caller's permissions.
     *
     * If called with one argument, **a username**, lists that user's permissions.
     *
     * @iscommand
     */
    void cmdListPerms(int argc, QStringList argv);

    /**
     * @brief Sets the role of the user.
     *
     * @details The first argument is the **target user**, the second is the **role** (in string form) to set to that user.
     *
     * @iscommand
     */
    void cmdSetPerms(int argc, QStringList argv);

    /**
     * @brief Removes the role from a given user.
     *
     * @details The first argument is the **target user**, the second is the **permission** (in string form) to remove from that user.
     *
     * @iscommand
     */
    void cmdRemovePerms(int argc, QStringList argv);

    /**
     * @brief Lists all users in the server's database.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdListUsers(int argc, QStringList argv);

    /**
     * @brief Logs the caller out from their moderator user.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdLogout(int argc, QStringList argv);

    /**
     * @brief Changes a moderator's password.
     *
     * @details If it is called with **one argument**, that argument is the **new password** to change to.
     *
     * If it is called with **two arguments**, the first argument is the **new password** to change to,
     * and the second argument is the **username** of the moderator to change the password of.
     */
    void cmdChangePassword(int argc, QStringList argv);

    ///@}

    /**
     * @name Areas
     *
     * @brief All functions that detail the actions of commands,
     * that are also related to area management.
     */
    ///@{

    /**
     * @brief Promotes a client to CM status.
     *
     * @details If called without arguments, promotes the caller.
     *
     * If called with a **user ID** as an argument, and the caller is a CM, promotes the target client to CM status.
     *
     * @iscommand
     */
    void cmdCM(int argc, QStringList argv);

    /**
     * @brief Removes the CM status from the caller.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdUnCM(int argc, QStringList argv);

    /**
     * @brief Invites a client to the area.
     *
     * @details Needs a **user ID** as an argument.
     *
     * @iscommand
     *
     * @see AreaData::LOCKED and AreaData::SPECTATABLE for the benefits of being invited.
     */
    void cmdInvite(int argc, QStringList argv);

    /**
     * @brief Uninvites a client to the area.
     *
     * @details Needs a **user ID** as an argument.
     *
     * @iscommand
     *
     * @see AreaData::LOCKED and AreaData::SPECTATABLE for the benefits of being invited.
     */
    void cmdUnInvite(int argc, QStringList argv);

    /**
     * @brief Locks the area.
     *
     * @details No arguments.
     *
     * @iscommand
     *
     * @see AreaData::LOCKED
     */
    void cmdLock(int argc, QStringList argv);

    /**
     * @brief Sets the area to spectatable.
     *
     * @details No arguments.
     *
     * @iscommand
     *
     * @see AreaData::SPECTATABLE
     */
    void cmdSpectatable(int argc, QStringList argv);

    /**
     * @brief Sets the area to spectatable without inviting everyone in area.
     *
     * @details No arguments.
     *
     * @iscommand
     *
     * @see AreaData::SPECTATABLE
     */
    void cmdAreaMute(int argc, QStringList argv);

    /**
     * @brief Unlocks the area.
     *
     * @details No arguments.
     *
     * @iscommand
     *
     * @see AreaData::FREE
     */
    void cmdUnLock(int argc, QStringList argv);

    /**
     * @brief Lists all clients in all areas.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdGetAreas(int argc, QStringList argv);

    void cmdGetAreaHubs(int argc, QStringList argv);

    /**
     * @brief Lists all clients in the area the caller is in.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdGetArea(int argc, QStringList argv);

    /**
     * @brief Moves the caller to the area with the given ID.
     *
     * @details Takes an **area ID** as an argument.
     *
     * @iscommand
     */
    void cmdArea(int argc, QStringList argv);

    /**
     * @brief Kicks a client from the area, moving them back to the default area.
     *
     * @details Takes one argument, the **client's ID** to kick.
     *
     * @iscommand
     */
    void cmdAreaKick(int argc, QStringList argv);

    /**
     * @brief Kicks a client from the area, moving them to the desired area.
     *
     * @details Takes two argument, the **client's ID** to kick and desired area.
     *
     * @iscommand
     */
    void cmdModAreaKick(int argc, QStringList argv);

    /**
     * @brief Changes the background of the current area.
     *
     * @details Takes the **background's internal name** (generally the background's directory's name for the clients) as the only argument.
     *
     * @iscommand
     */
    void cmdSetBackground(int argc, QStringList argv);

    /**
     * @brief Locks the background, preventing it from being changed.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdBgLock(int argc, QStringList argv);

    /**
     * @brief Unlocks the background, allowing it to be changed again.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdBgUnlock(int argc, QStringList argv);

    /**
     * @brief Changes the status of the current area.
     *
     * @details Takes a **status** as an argument. See AreaData::Status for permitted values.
     *
     * @iscommand
     */
    void cmdStatus(int argc, QStringList argv);

    /**
     * @brief Sends an out-of-character message with the judgelog of an area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdJudgeLog(int argc, QStringList argv);

    /**
     * @brief Toggles whether the BG list is ignored in an area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdIgnoreBgList(int argc, QStringList argv);

    /**
     * @brief Returns the area message in OOC. Double to set the current area message.
     *
     * @details See short description.
     *
     * @iscommand
     */
    void cmdAreaMessage(int argc, QStringList argv);

    /**
     * @brief Clears the areas message and disables automatic sending.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdClearAreaMessage(int argc, QStringList argv);

    /**
     * @brief Toggles whether the client shows the area message when joining the current area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleAreaMessageOnJoin(int argc, QStringList argv);

    /**
     * @brief Toggles whether the client can use testimony animations in the area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleWtce(int argc, QStringList argv);

    /**
     * @brief Toggles whether the client can send game shouts in the area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleShouts(int argc, QStringList argv);

    /**
     * @brief Generates a download link for characters who are iniswapping
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdWebfiles(int argc, QStringList argv);

    /**
     * @brief Disable/Enable messages when you move to areas or disconnect.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdSneak(int argc, QStringList argv);

    /**
     * @brief Find out the current Evidence Mod in area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdCurrentEvimod(int argc, QStringList argv);

    /**
     * @brief Get a list of backgrounds.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdBgs(int argc, QStringList argv);

    /**
     * @brief Find out the current background in area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdCurBg(int argc, QStringList argv);

    /**
     * @brief Enable/disable floodguard in area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleFloodguardActuve(int argc, QStringList argv);

    /**
     * @brief Enable/disable Chill Mod in area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleChillMod(int argc, QStringList argv);

    /**
     * @brief Enable/disable automoderator in area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleAutoMod(int argc, QStringList argv);

    /**
     * @brief Rename the area in which the client is located.
     *
     * @details The only argument is the new name of the area.
     *
     * @iscommand
     */
    void cmdRenameArea(int argc, QStringList argv);

    /**
     * @brief Create a new area.
     *
     * @details The only argument is the name of the new area.
     *
     * @iscommand
     */
    void cmdCreateArea(int argc, QStringList argv);

    /**
     * @brief Delete the specified area.
     *
     * @details The only argument is the ID of the area to be removed.
     *
     * @iscommand
     */
    void cmdRemoveArea(int argc, QStringList argv);

    /**
     * @brief Save the areas.ini file.
     *
     * @details The only argument is an additional name for the file.
     *
     * @iscommand
     */
    void cmdSaveAreas(int argc, QStringList argv);

    /**
     * @brief Give permission to save areas.ini file.
     *
     * @details The only argument is the client ID.
     *
     * @iscommand
     */
    void cmdPermitAreaSaving(int argc, QStringList argv);

    /**
     * @brief Swap selected areas.
     *
     * @details This command needs two arguments is ID of the areas to be swapped.
     *
     * @iscommand
     */
    void cmdSwapAreas(int argc, QStringList argv);

    /**
     * @brief Toggle whether it is possible in the current area to become CM or not.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleProtected(int argc, QStringList argv);

    void cmdToggleStatus(int argc, QStringList argv);

    void cmdOocType(int argc, QStringList argv);

    void cmdToggleAutoCap(int argc, QStringList argv);

    ///@}

    /**
     * @name Moderation
     *
     * @brief All functions that detail the actions of commands,
     * that are also related to the moderation and administration of the server.
     */
    ///@{

    /**
     * @brief Lists all the commands that the caller client has the permissions to use.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdHelp(int argc, QStringList argv);

    /**
     * @brief Gets or sets the server's Message Of The Day.
     *
     * @details If called without arguments, gets the MOTD.
     *
     * If it has any number of arguments, it is set as the **MOTD**.
     *
     * @iscommand
     */
    void cmdMOTD(int argc, QStringList argv);

    /**
     * @brief Gives a very brief description of Akashi.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdAbout(int argc, QStringList argv);

    /**
     * @brief Lists the currently logged-in moderators on the server.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdMods(int argc, QStringList argv);

    /**
     * @brief Bans a client from the server, forcibly severing its connection to the server,
     * and disallowing their return.
     *
     * @details The first argument is the **target's IPID**, the second is the **duration**,
     * and the third is the **reason** why the client was banned.
     *
     * The duration can be `perma`, meaning a forever ban, otherwise, it must be given in the format of `YYyWWwDDdHHhMMmSSs` to
     * mean a YY years, WW weeks, DD days, HH hours, MM minutes and SS seconds long ban. Any of these may be left out, for example,
     * `1h30m` for a 1.5 hour long ban.
     *
     * Besides banning, this command kicks all clients having the given IPID,
     * thus a multiclienting user will have all their clients be kicked from the server.
     *
     * The target's hardware ID is also recorded in a ban, so users with dynamic IPs will not be able to
     * cirvumvent the ban without also changing their hardware ID.
     *
     * @iscommand
     */
    void cmdBan(int argc, QStringList argv);

    /**
     * @brief Removes a ban from the database.
     *
     * @details Takes a single argument, the **ID** of the ban.
     *
     * @iscommand
     */
    void cmdUnBan(int argc, QStringList argv);

    /**
     * @brief Kicks a client from the server, forcibly severing its connection to the server.
     *
     * @details The first argument is the **target's IPID**, while the remaining arguments are the **reason**
     * the client was kicked. Both arguments are mandatory.
     *
     * This command kicks all clients having the given IPID, thus a multiclienting user will have all
     * their clients be kicked from the server.
     *
     * @iscommand
     */
    void cmdKick(int argc, QStringList argv);

    /**
     * @brief Mutes a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_muted
     */
    void cmdMute(int argc, QStringList argv);

    /**
     * @brief Removes the muted status from a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_muted
     */
    void cmdUnMute(int argc, QStringList argv);

    /**
     * @brief OOC-mutes a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_ooc_muted
     */
    void cmdOocMute(int argc, QStringList argv);

    /**
     * @brief Removes the OOC-muted status from a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_ooc_muted
     */
    void cmdOocUnMute(int argc, QStringList argv);

    /**
     * @brief WTCE-blocks a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_wtce_blocked
     */
    void cmdBlockWtce(int argc, QStringList argv);

    /**
     * @brief Removes the WTCE-blocked status from a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_wtce_blocked
     */
    void cmdUnBlockWtce(int argc, QStringList argv);

    /**
     * @brief Lists the last five bans made on the server.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdBans(int argc, QStringList argv);

    /**
     * @brief Toggle whether or not in-character messages purely consisting of spaces are allowed.
     *
     * @details Takes no arguments. Against all common sense this also allows you to disable blankposting.
     *
     * @iscommand
     */
    void cmdAllowBlankposting(int argc, QStringList argv);

    /**
     * @brief Looks up info on a ban.
     *
     * @details If it is called with **one argument**, that argument is the ban ID to look up.
     *
     * If it is called with **two arguments**, then the first argument is either a ban ID, an IPID,
     * or an HDID, and the the second argument specifies the ID type.
     *
     * @iscommand
     */
    void cmdBanInfo(int argc, QStringList argv);

    /**
     * @brief Reloads all server configuration files.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdReload(int argc, QStringList argv);

    /**
     * @brief Toggles immediate text processing in the current area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdForceImmediate(int argc, QStringList argv);

    /**
     * @brief Toggles whether iniswaps are allowed in the current area.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdAllowIniswap(int argc, QStringList argv);

    /**
     * @brief Grants a client the temporary permission to save a testimony.
     *
     * @details clientId() as the target of permission
     *
     * @iscommand
     *
     */
    void cmdPermitSaving(int argc, QStringList argv);

    /**
     * @brief Kicks a client from the server, forcibly severing its connection to the server.
     *
     * @details The first argument is the **target's UID**, while the remaining arguments are the **reason**
     * the client was kicked. Both arguments are mandatory.
     *
     * Unlike cmdKick, this command will only kick a single client, thus a multiclienting user will not have all their clients kicked.
     *
     * @iscommand
     *
     * @see #cmdKick
     */
    void cmdKickUid(int argc, QStringList argv);

    /**
     * @brief Updates a ban in the database, changing either its reason or duration.
     *
     * @details The first argument is the **ID** of the ban to update. The second argument is the **field** to update, either `reason` or `duration`
     *
     * and the remaining arguments are the **duration** or the **reason** to update to.
     *
     * @iscommand
     */
    void cmdUpdateBan(int argc, QStringList argv);

    /**
     * @brief Pops up a notice for all clients in the targeted area with a given message.
     *
     * @details The only argument is the message to send. This command only works on clients with support for the BB#%
     *
     * generic message box packet (i.e. Attorney Online versions 2.9 and up). To support earlier versions (as well as to make the message
     *
     * re-readable if a user clicks out of it too fast) the message will also be sent in OOC to all affected clients.
     *
     * @iscommand
     */
    void cmdNotice(int argc, QStringList argv);

    /**
     * @brief Pops up a notice for all clients in the server with a given message.
     *
     * @details Unlike cmdNotice, this command will send its notice to every client connected to the server.
     *
     * @see #cmdNotice
     *
     * @iscommand
     */
    void cmdNoticeGlobal(int argc, QStringList argv);

    /**
     * @brief Removes all CMs from the current area.
     *
     * @details This command is a bandaid fix to the issue that clients may end up ghosting when improperly disconnected from the server.
     *
     * @iscommand
     */
    void cmdClearCM(int argc, QStringList argv);

    /**
     * @brief Removes all multiclient instances of a client on the server, excluding the one using the command.
     *
     * @details This command gracefully removes all multiclients from the server, disconnecting them and freeing their used character.
     *
     * @arg argv This command allows the user to specify if it should look for IPID or HWID matches. This is useful when a client mayb
     * have been connected over IPv6 and the second connection is made over IPv4
     *
     * @iscommand
     */
    void cmdKickOther(int argc, QStringList argv);

    /**
     * @brief Get IPID information.
     *
     * @details The argument to this command is one - the client's IPID. This command gives the IP address and the date of "creation" of the IPID.
     *
     * @iscommand
     */
    void cmdIpidInfo(int argc, QStringList argv);

    /**
     * @brief Allow/deny yourself to take the taken characters.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdTakeTakenChar(int argc, QStringList argv);

    /**
     * @brief Blind the targeted player from being able to see or talk IC/OOC.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #blinded
     */
    void cmdBlind(int argc, QStringList argv);

    /**
     * @brief Undo effects of the /blind command.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #blinded
     */
    void cmdUnBlind(int argc, QStringList argv);

    /**
     * @brief Allow/deny WUSO Mod.
     *
     * @details No arguments.
     *
     * @see #wuso
     *
     * @iscommand
     */
    void cmdToggleWebUsersSpectateOnly(int argc, QStringList argv);

    /**
     * @brief Remove the WUSO action on the client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @see #wuso
     *
     * @iscommand
     */
    void cmdRemoveWebUsersSpectateOnly(int argc, QStringList argv);

    void cmdKickPhantoms(int argc, QStringList argv);

    ///@}

    /**
     * @name Roleplay
     *
     * @brief All functions that detail the actions of commands,
     * that are also related to various kinds of roleplay actions in some way.
     */
    ///@{

    /**
     * @brief Flips a coin, returning heads or tails.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdFlip(int argc, QStringList argv);

    /**
     * @brief Rolls dice and sends the results.
     *
     * @details The first argument is the **amount of faces** each die should have.
     * The second argument is the **amount of dice** that should be rolled.
     *
     * Both arguments are optional.
     *
     * @iscommand
     *
     * @see AOClient::diceThrower
     */
    void cmdRoll(int argc, QStringList argv);

    /**
     * @brief Rolls dice, but sends the results in private to the roller.
     *
     * @copydetails AOClient::cmdRoll
     */
    void cmdRollP(int argc, QStringList argv);

    /**
     * @brief Gets or sets the global or one of the area-specific timers.
     *
     * @details If called without arguments, sends an out-of-character message listing the statuses of both
     * the global timer and the area-specific timers.
     *
     * If called with **one argument**, and that argument is between `0` and `4` (inclusive on both ends),
     * sends an out-of-character message about the status of the given timer, where `0` is the global timer,
     * and the remaining numbers are the first, second, third and fourth timers in the current area.
     *
     * If called with **two arguments**, and the second argument is
     * * in the format of `hh:mm:ss`, then it starts the given timer,
     *   with `hh` hours, `mm` minutes, and `ss` seconds on it, making it appear if needed.
     * * `start`, it (re)starts the given timer, making it appear if needed.
     * * `pause` or `stop`, it pauses the given timer.
     * * `hide` or `unset`, it stops the timer and hides it.
     *
     * @iscommand
     */
    void cmdTimer(int argc, QStringList argv);

    /**
     * @brief Changes the subtheme of the clients in the current area.
     *
     * @details The only argument is the **name of the subtheme**. Reloading is always forced.
     *
     * @iscommand
     */
    void cmdSubTheme(int argc, QStringList argv);

    void cmdVote(int argc, QStringList argv);

    void cmdScoreboard(int argc, QStringList argv);

    void cmdAddScore(int argc, QStringList argv);

    void cmdRemoveScore(int argc, QStringList argv);

    /**
     * @brief Writes a "note card" in the current area.
     *
     * @details The note card is not readable until all note cards in the area are revealed by a **CM**.
     * A message will appear to all clients in the area indicating that a note card has been written.
     *
     * @iscommand
     */
    void cmdNoteCard(int argc, QStringList argv);

    /**
     * @brief Reveals all note cards in the current area.
     *
     * @iscommand
     */
    void cmdNoteCardReveal(int argc, QStringList argv);

    /**
     * @brief Erases the client's note card from the area's list of cards.
     *
     * @details A message will appear to all clients in the area indicating that a note card has been erased.
     *
     * @iscommand
     */
    void cmdNoteCardClear(int argc, QStringList argv);

    /**
     * @brief Randomly selects an answer from 8ball.txt to a question.
     *
     * @details The only argument is the question the client wants answered.
     *
     * @iscommand
     */
    void cmd8Ball(int argc, QStringList argv);

    ///@}

    /**
     * @name Messaging
     *
     * @brief All functions that detail the actions of commands,
     * that are also related to messages or the client's self-management in some way.
     */
    ///@{

    /**
     * @brief Changes the client's position.
     *
     * @details The only argument is the **target position** to move the client to.
     *
     * @iscommand
     */
    void cmdPos(int argc, QStringList argv);

    /**
     * @brief Forces a client, or all clients in the area, to a specific position.
     *
     * @details The first argument is the **client's ID**, or `\*` if the client wants to force all
     * clients in their area into the position.
     * If a specific client ID is given, this command can reach across areas, i.e., the caller and target
     * clients don't have to share areas.
     *
     * The second argument is the **position** to force the clients to.
     * This is not checked for nonsense values.
     *
     * @iscommand
     */
    void cmdForcePos(int argc, QStringList argv);

    /**
     * @brief Switches to a different character based on character ID.
     *
     * @details The only argument is the **character's ID** that the client wants to switch to.
     *
     * @iscommand
     */
    void cmdSwitch(int argc, QStringList argv);

    /**
     * @brief Picks a new random character for the client.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdRandomChar(int argc, QStringList argv);

    /**
     * @brief Sends a global message (i.e., all clients in the server will be able to see it).
     *
     * @details The arguments are **the message** that the client wants to send.
     *
     * @iscommand
     */
    void cmdG(int argc, QStringList argv);

    /**
     * @brief Toggles whether the client will ignore @ref cmdG "global" messages or not.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleGlobal(int argc, QStringList argv);

    /**
     * @brief Sends a direct message to another client on the server based on ID.
     *
     * @details Has two arguments, the first is **the target's ID** whom the client wants to send the message to,
     * while the second is **the message** the client wants to send.
     *
     * The "second" part is technically everything that isn't the first argument.
     *
     * @iscommand
     */
    void cmdPM(int argc, QStringList argv);

    /**
     * @brief A global message expressing that the client needs something (generally: players for something).
     *
     * @details The arguments are **the message** that the client wants to send.
     *
     * @iscommand
     */
    void cmdNeed(int argc, QStringList argv);

    /**
     * @brief Sends out a decorated global message, for announcements.
     *
     * @details The arguments are **the message** that the client wants to send.
     *
     * @iscommand
     *
     * @see AOClient::cmdG()
     */
    void cmdAnnounce(int argc, QStringList argv);

    /**
     * @brief Sends a message in the server-wide, moderator only chat.
     *
     * @details The arguments are **the message** that the client wants to send.
     *
     * @iscommand
     */
    void cmdM(int argc, QStringList argv);

    /**
     * @brief Replaces a target client's in-character messages with strings randomly selected from gimp.txt.
     *
     * @details The only argument is the **the target's ID** whom the client wants to gimp.
     *
     * @iscommand
     */
    void cmdGimp(int argc, QStringList argv);

    /**
     * @brief Allows a gimped client to speak normally.
     *
     * @details The only argument is **the target's ID** whom the client wants to ungimp.
     *
     * @iscommand
     */
    void cmdUnGimp(int argc, QStringList argv);

    /**
     * @brief Removes all vowels from a target client's in-character messages.
     *
     * @details The only argument is **the target's ID** whom the client wants to disemvowel.
     *
     * @iscommand
     */
    void cmdDisemvowel(int argc, QStringList argv);

    /**
     * @brief Allows a disemvoweled client to speak normally.
     *
     * @details The only argument is **the target's ID** whom the client wants to undisemvowel.
     *
     * @iscommand
     */
    void cmdUnDisemvowel(int argc, QStringList argv);

    /**
     * @brief Scrambles the words of a target client's in-character messages.
     *
     * @details The only argument is **the target's ID** whom the client wants to shake.
     *
     * @iscommand
     */
    void cmdShake(int argc, QStringList argv);

    /**
     * @brief Allows a shaken client to speak normally.
     *
     * @details The only argument is **the target's ID** whom the client wants to unshake.
     *
     * @iscommand
     */
    void cmdUnShake(int argc, QStringList argv);

    /**
     * @brief Toggles whether a client will recieve @ref cmdPM private messages or not.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdMutePM(int argc, QStringList argv);

    /**
     * @brief Toggles whether a client will recieve @ref cmdNeed "advertisement" messages.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdToggleAdverts(int argc, QStringList argv);

    /**
     * @brief Toggles whether this client is considered AFK.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdAfk(int argc, QStringList argv);

    /**
     * @brief Restricts a target client to a set of characters that they can switch from, blocking them from other characters.
     *
     * @details The first argument is the **target's ID** whom the client wants to charcurse.
     *
     * The second argument is one or more character names the client wants to restrict to, comma separated.
     *
     * @iscommand
     */
    void cmdCharCurse(int argc, QStringList argv);

    /**
     * @brief Removes the charcurse status from a client.
     *
     * @details The only argument is the **target's ID** whom the client wants to uncharcurse.
     *
     * @iscommand
     */
    void cmdUnCharCurse(int argc, QStringList argv);
    void cmdCharSelect(int argc, QStringList argv);

    /**
     * @brief Toggle whether the client's messages will be sent in first person mode.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdFirstPerson(int argc, QStringList argv);

    ///@}

    /**
     * @name Casing
     *
     * @brief All functions that detail the actions of commands,
     * that are related to casing.
     */
    ///@{

    /**
     * @brief Sets the `/doc` to a custom text.
     *
     * @details The arguments are **the text** that the client wants to set the doc to.
     *
     * @iscommand
     */
    void cmdDoc(int argc, QStringList argv);

    /**
     * @brief Sets the `/doc` to `"No document."`.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdClearDoc(int argc, QStringList argv);

    /**
     * @brief Changes the evidence mod in the area.
     *
     * @details The only argument is the **evidence mod** to change to.
     *
     * @iscommand
     *
     * @see AreaData::EvidenceMod
     */
    void cmdEvidenceMod(int argc, QStringList argv);

    /**
     * @brief Changes position of two pieces of evidence in the area.
     *
     * @details The two arguments are the indices of the evidence items you want to swap the position of.
     *
     * @iscommand
     *
     * @see Area::Evidence_Swap
     *
     */
    void cmdEvidence_Swap(int argc, QStringList argv);

    /**
     * @brief Sets are to PLAYBACK mode
     *
     * @details Enables control over the stored testimony, prevent new messages to be added and
     * allows people to navigate trough it using > and <.
     */
    void cmdExamine(int argc, QStringList argv);

    /**
     * @brief Enables the testimony recording functionality.
     *
     * @details Any IC-Message send after this command is issues will be recorded by the testimony recorder.
     */
    void cmdTestify(int argc, QStringList argv);

    /**
     * @brief Allows user to update the currently displayed IC-Message from the testimony replay.
     *
     * @details Using this command replaces the content of the current statement entirely. It does not append information.
     */
    void cmdUpdateStatement(int argc, QStringList argv);

    /**
     * @brief Deletes a statement from the testimony.
     *
     * @details Using this deletes the entire entry in the QVector and resizes it appropriately to prevent empty record indices.
     */
    void cmdDeleteStatement(int argc, QStringList argv);

    /**
     * @brief Pauses testimony playback.
     *
     * @details Disables the testimony playback controls.
     */
    void cmdPauseTestimony(int argc, QStringList argv);

    /**
     * @brief Adds a statement to an existing testimony.
     *
     * @details Inserts new statement after the currently displayed recorded message. Increases the index by 1.
     *
     */
    void cmdAddStatement(int argc, QStringList argv);

    /**
     * @brief Sends a list of the testimony to OOC of the requesting client
     *
     * @details Retrieves all stored IC-Messages of the area and dumps them into OOC with some formatting.
     *
     */
    void cmdTestimony(int argc, QStringList argv);

    /**
     * @brief Saves a testimony recording to the servers storage.
     *
     * @details Saves a titled text file which contains the edited packets into a text file.
     *          The filename will always be lowercase.
     *
     */
    void cmdSaveTestimony(int argc, QStringList argv);

    /**
     * @brief Loads testimony for the testimony replay. Argument is the testimony name.
     *
     * @details Loads a titled text file which contains the edited packets to be loaded into the QVector.
     *          Validates the size of the testimony to ensure the entire testimony can be replayed.
     *          Testimony name will always be converted to lowercase.
     *
     */
    void cmdLoadTestimony(int argc, QStringList argv);

    ///@}

    /**
     * @name Music
     *
     * @brief All functions that detail the actions of commands,
     * that are also related to music in some way.
     */
    ///@{

    /**
     * @brief Plays music in the area.
     *
     * @details The arguments are **the song's filepath** originating from `base/sounds/music/`,
     * or **the song's URL** if it's a stream.
     *
     * As described above, this command can be used to play songs by URL (for clients at and above version 2.9),
     * but it can also be used to play songs locally available for the clients but not listed in the music list.
     *
     * @iscommand
     */
    void cmdPlay(int argc, QStringList argv);

    /**
     * @brief Plays ambience in the area.
     *
     * @details The arguments are **the song's filepath** originating from `base/sounds/music/`,
     * or **the song's URL** if it's a stream.
     *
     * As described above, this command can be used to play ambience by URL (for clients at and above version 2.9),
     * but it can also be used to play ambience locally available for the clients.
     *
     * @iscommand
     */
    void cmdPlayAmbience(int argc, QStringList argv);

    void cmdPlayAmbienceGgl(int argc, QStringList argv);

    /**
     * @brief Plays music in the area once.
     *
     * @copydetails AOClient::cmdPlay
     *
     * @iscommand
     */
    void cmdPlayOnce(int argc, QStringList argv);

    void cmdPlayHub(int argc, QStringList argv);

    void cmdPlayHubOnce(int argc, QStringList argv);

    void cmdPlayGgl(int argc, QStringList argv);

    void cmdPlayOnceGgl(int argc, QStringList argv);

    void cmdPlayHubGgl(int argc, QStringList argv);

    void cmdPlayHubOnceGgl(int argc, QStringList argv);

    /**
     * @brief DJ-blocks a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_dj_blocked
     */
    void cmdBlockDj(int argc, QStringList argv);

    /**
     * @brief Removes the DJ-blocked status from a client.
     *
     * @details The only argument is the **target client's user ID**.
     *
     * @iscommand
     *
     * @see #is_dj_blocked
     */
    void cmdUnBlockDj(int argc, QStringList argv);

    /**
     * @brief Returns the currently playing music in the area, and who played it.
     *
     * @details No arguments.
     *
     * @iscommand
     */
    void cmdCurrentMusic(int argc, QStringList argv);

    /**
     * @brief Toggles music playing in the current area.
     *
     * @details No arguments.
     */
    void cmdToggleMusic(int argc, QStringList argv);

    /**
     * @brief Adds a song to the custom list.
     */
    void cmdAddSong(int argc, QStringList argv);

    /**
     * @brief Adds a category to the areas custom music list.
     */
    void cmdAddCategory(int argc, QStringList argv);

    /**
     * @brief Removes any matching song or category from the custom area.
     */
    void cmdRemoveCategorySong(int argc, QStringList argv);

    /**
     * @brief Toggles the prepending behaviour of the servers root musiclist.
     */
    void cmdToggleRootlist(int argc, QStringList argv);

    /**
     * @brief Clears the entire custom list of this area.
     */
    void cmdClearCustom(int argc, QStringList argv);

    ///@}

    /**
     * @name Command helper functions
     *
     * @brief A collection of functions of shared behaviour between command functions,
     * allowing the abstraction of technical details in the command function definition,
     * or the avoidance of repetition over multiple definitions.
     */
    ///@{

    /**
     * @brief Literally just an invalid default command. That's it.
     *
     * @note Can be used as a base for future commands.
     *
     * @iscommand
     */
    void cmdDefault(int argc, QStringList argv);

    /**
     * @brief Returns a textual representation of the time left in an area's Timer.
     *
     * @param area_idx The ID of the area whose timer to grab.
     * @param timer_idx The ID of the timer to grab
     *
     * @return A textual representation of the time left over on the Timer,
     * or `"Timer is inactive"` if the timer wasn't started.
     */
    QString getAreaTimer(int area_idx, int timer_idx);

    /**
     * @brief Generates a tsuserver3-style area list to be displayed to the user in the out-of-character chat.
     *
     * @details This list shows general details about the area the caller is currently in,
     * like who is the owner, what players are in there, the status of the area, etc.
     *
     * @param area_idx The index of the area whose details should be generated.
     *
     * @return A QStringList of details about the given area, with every entry in the string list amounting to
     * roughly a separate line.
     */
    QStringList buildAreaList(int area_idx, bool ignore_hubs = true);

    /**
     * @brief A convenience function for rolling dice.
     *
     * @param sides The number of sides the dice to be rolled have
     *
     * @param dice The number of dice to be rolled
     *
     * @param p_roll Bool to determine of a roll is private or not.
     *
     * @param roll_modifier Option parameter to add or subtract from each
     * rolled value
     */
    void diceThrower(int sides, int dice, bool p_roll, int roll_modifier = 0);

    /**
     * @brief Interprets an expression of time into amount of seconds.
     *
     * @param input A string in the format of `"XXyXXwXXdXXhXXmXXs"`, where every `XX` is some integer.
     * There is no limit on the length of the integers, the `XX` text is just a placeholder, and is not intended to
     * indicate a limit of two digits maximum.
     *
     * The string gets interpreted as follows:
     * * `XXy` is parsed into `XX` amount of years,
     * * `XXw` is parsed into `XX` amount of weeks,
     * * `XXd` is parsed into `XX` amount of days,
     * * `XXh` is parsed into `XX` amount of hours,
     * * `XXm` is parsed into `XX` amount of minutes, and
     * * `XXs` is parsed into `XX` amount of seconds.
     *
     * Any of these may be left out, but the order must be kept (i.e., `"10s5y"` is a malformed text).
     *
     * @return The parsed text, converted into their respective durations, summed up, then converted into seconds.
     */
    long long parseTime(QString input);

    /**
     * @brief Clears QVector of the current area.
     *
     * @details It clears both its content and trims it back to size 0
     *
     */
    void clearTestimony();

    /**
     * @brief Called when area enum is set to PLAYBACK. Sends the IC-Message stored at the current statement.
     * @return IC-Message stored in the QVector.
     */
    QStringList playTestimony();

    /**
     * @brief Checks if a password meets the server's password requirements.
     *
     * @param username The chosen username.
     *
     * @param password The password to check.
     *
     * @return True if the password meets the requirements, otherwise false.
     */
    bool checkPasswordRequirements(QString f_username, QString f_password);

    /**
     * @brief Sends a server notice.
     *
     * @param notice The notice to send out.
     *
     * @param global Whether or not the notice should be server-wide.
     */
    void sendNotice(QString f_notice, bool f_global = false);

    /**
     * @brief Plays music in the area.
     *
     * @param f_args Audio file name or link to it.
     *
     * @param f_once Loop music or not.
     *
     * @see #cmdPlay(int argc, QStringList argv)
     *
     * @see #cmdPlayOnce(int argc, QStringList argv)
     */
    void playMusic(QStringList f_args, bool f_hubbroadcast = false, bool f_once = false, bool f_gdrive = false, bool f_ambience = false);

    QString uploadToLetterBox(QString f_path);

    void startMusicPlaying(QString f_song, bool f_hubbroadcast, bool f_once, bool f_ambience);

    QString getEviMod(int f_area);

    QString getLockStatus(int f_area);

    QString getOocType(int f_area);

    QString getHubLockStatus(int f_hub);

    void endVote();

    /**
     * @brief Checks if a testimony contains '<' or '>'.
     *
     * @param message The IC Message that might contain unproper symbols.
     *
     * @return True if it contains '<' or '>' symbols, otherwise false.
     */
    bool checkTestimonySymbols(const QString &message);
    ///@}

    /**
     * @name Hubs
     *
     * @brief All functions that detail the actions of commands,
     * that are also related to hub management.
     */
    ///@{

    void cmdHub(int argc, QStringList argv);

    void cmdGm(int argc, QStringList argv);

    void cmdUnGm(int argc, QStringList argv);

    void cmdHubProtected(int argc, QStringList argv);

    void cmdHidePlayerCount(int argc, QStringList argv);

    void cmdHubRename(int argc, QStringList argv);

    void cmdHubListening(int argc, QStringList argv);

    void cmdHubUnlock(int argc, QStringList argv);

    void cmdHubSpectate(int argc, QStringList argv);

    void cmdHubLock(int argc, QStringList argv);

    void cmdHubInvite(int argc, QStringList argv);

    void cmdHubUnInvite(int argc, QStringList argv);

    void cmdGHub(int argc, QStringList argv);
    ///@}

    /**
     * @brief A helper variable that is used to direct the called of the `/changeAuth` command through the process
     * of changing the authorisation method from simple to advanced.
     *
     * @see cmdChangeAuth and cmdSetRootPass
     */
    bool change_auth_started = false;

    /**
     * @property CommandInfo::action
     *
     * @brief A function reference that contains what the command actually does.
     *
     * @param int When called, this parameter will be filled with the argument count. @anchor commandArgc
     * @param QStringList When called, this parameter will be filled the list of arguments. @anchor commandArgv
     */

    /**
     * @brief Filled with part of a packet if said packet could not be read fully from the client's socket.
     *
     * @details Per AO2's network protocol, a packet is finished with the character `%`.
     *
     * @see #is_partial
     */
    QString partial_packet;

    /**
     * @brief True when the previous `readAll()` call from the client's socket returned an unfinished packet.
     *
     * @see #partial_packet
     */
    bool is_partial;

    /**
     * @brief The size, in bytes, of the last data the client sent to the server.
     */
    int last_read = 0;

  signals:

    /**
     * @brief Signal connected to universal logger. Sends IC chat usage to the logger.
     */
    void logIC(const QString &f_charName, const QString &f_oocName, const QString &f_ipid,
               const QString &f_areaName, const QString &f_message, const QString &f_uid,
               const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends OOC chat usage to the logger.
     */
    void logOOC(const QString &f_charName, const QString &f_oocName, const QString &f_ipid,
                const QString &f_areaName, const QString &f_message, const QString &f_uid,
                const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends login attempt to the logger.
     */
    void logLogin(const QString &f_charName, const QString &f_oocName, const QString &f_moderatorName,
                  const QString &f_ipid, const QString &f_areaName, const bool &f_success, const QString &f_uid,
                  const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends command usage to the logger.
     */
    void logCMD(const QString &f_charName, const QString &f_ipid, const QString &f_oocName, const QString f_command,
                const QString f_args, const QString f_areaName, const QString &f_uid, const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends player kick information to the logger.
     */
    void logKick(const QString &f_moderator, const QString &f_targetIPID, const QString &f_reason, const QString &f_uid,
                 const QString &f_hwid);

    /**
     * @brief Signal connected to universal logger. Sends ban information to the logger.
     */
    void logBan(const QString &f_moderator, const QString &f_targetIPID, const QString &f_duration, const QString &f_reason,
                const QString &f_uid, const QString &f_hwid);

    /**
     * @brief Signal connected to universal logger. Sends modcall information to the logger, triggering a write of the buffer
     *        when modcall logging is used.
     */
    void logModcall(const QString &f_charName, const QString &f_ipid, const QString &f_oocName, const QString &f_areaName,
                    const QString &f_uid, const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends disconnect information to the logger.
     */
    void logDisconnect(const QString &f_char_name, const QString &f_ipid, const QString &f_ooc_name, const QString &f_area_name,
                       const QString &f_uid, const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends change music information to the logger.
     */
    void logMusic(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                  const QString &f_area_name, const QString &f_music, const QString &f_uid,
                  const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends change character information to the logger.
     */
    void logChangeChar(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                       const QString &f_area_name, const QString &f_changechar, const QString &f_uid,
                       const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signal connected to universal logger. Sends change area information to the logger.
     */
    void logChangeArea(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                       const QString &f_area_name, const QString &f_changearea, const QString &f_uid,
                       const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Signals the server that the client has disconnected and marks its userID as free again.
     */
    void clientSuccessfullyDisconnected(const int &f_user_id);
};

#endif // AOCLIENT_H
