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
#include "include/command_extension.h"
#include "include/config_manager.h"
#include "include/db_manager.h"
#include "include/network/aopacket.h"
#include "include/server.h"

const QMap<QString, AOClient::CommandInfo> AOClient::COMMANDS{
        {"login",              {{ACLRole::NONE},         1, &AOClient::cmdLogin}},
        {"getareas",           {{ACLRole::NONE},         0, &AOClient::cmdGetAreas}},
        {"getarea",            {{ACLRole::NONE},         0, &AOClient::cmdGetArea}},
        {"ban",                {{ACLRole::BAN},          3, &AOClient::cmdBan}},
        {"kick",               {{ACLRole::KICK},         2, &AOClient::cmdKick}},
        {"changeauth",         {{ACLRole::SUPER},        0, &AOClient::cmdChangeAuth}},
        {"rootpass",           {{ACLRole::SUPER},        1, &AOClient::cmdSetRootPass}},
        {"bg",                 {{ACLRole::NONE},         1, &AOClient::cmdSetBackground}},
        {"bglock",             {{ACLRole::CM},           0, &AOClient::cmdBgLock}},
        {"bgunlock",           {{ACLRole::CM},           0, &AOClient::cmdBgUnlock}},
        {"adduser",            {{ACLRole::MODIFY_USERS}, 2, &AOClient::cmdAddUser}},
        {"listperms",          {{ACLRole::MODIFY_USERS}, 0, &AOClient::cmdListPerms}},
        {"addperm",            {{ACLRole::MODIFY_USERS}, 2, &AOClient::cmdSetPerms}},
        {"removeperm",         {{ACLRole::MODIFY_USERS}, 2, &AOClient::cmdRemovePerms}},
        {"listusers",          {{ACLRole::MODIFY_USERS}, 0, &AOClient::cmdListUsers}},
        {"unmod",              {{ACLRole::NONE},         0, &AOClient::cmdLogout}},
        {"pos",                {{ACLRole::NONE},         1, &AOClient::cmdPos}},
        {"g",                  {{ACLRole::NONE},         1, &AOClient::cmdG}},
        {"need",               {{ACLRole::NONE},         1, &AOClient::cmdNeed}},
        {"coinflip",           {{ACLRole::NONE},         0, &AOClient::cmdFlip}},
        {"roll",               {{ACLRole::NONE},         0, &AOClient::cmdRoll}},
        {"rollp",              {{ACLRole::NONE},         0, &AOClient::cmdRollP}},
        {"doc",                {{ACLRole::NONE},         0, &AOClient::cmdDoc}},
        {"cleardoc",           {{ACLRole::NONE},         0, &AOClient::cmdClearDoc}},
        {"cm",                 {{ACLRole::NONE},         0, &AOClient::cmdCM}},
        {"uncm",               {{ACLRole::CM},           0, &AOClient::cmdUnCM}},
        {"invite",             {{ACLRole::CM},           1, &AOClient::cmdInvite}},
        {"uninvite",           {{ACLRole::CM},           1, &AOClient::cmdUnInvite}},
        {"area_lock",          {{ACLRole::CM},           0, &AOClient::cmdLock}},
        {"area_spectate",      {{ACLRole::CM},           0, &AOClient::cmdSpectatable}},
        {"area_mute",          {{ACLRole::CM},           0, &AOClient::cmdAreaMute}},
        {"area_unlock",        {{ACLRole::CM},           0, &AOClient::cmdUnLock}},
        {"timer",              {{ACLRole::CM},           0, &AOClient::cmdTimer}},
        {"area",               {{ACLRole::NONE},         1, &AOClient::cmdArea}},
        {"play",               {{ACLRole::NONE},         1, &AOClient::cmdPlay}},
        {"area_kick",          {{ACLRole::CM},           1, &AOClient::cmdAreaKick}},
        {"modarea_kick",       {{ACLRole::KICK},         2, &AOClient::cmdModAreaKick}},
        {"randomchar",         {{ACLRole::NONE},         0, &AOClient::cmdRandomChar}},
        {"switch",             {{ACLRole::NONE},         1, &AOClient::cmdSwitch}},
        {"toggleglobal",       {{ACLRole::NONE},         0, &AOClient::cmdToggleGlobal}},
        {"mods",               {{ACLRole::NONE},         0, &AOClient::cmdMods}},
        {"help",               {{ACLRole::NONE},         0, &AOClient::cmdHelp}},
        {"status",             {{ACLRole::NONE},         1, &AOClient::cmdStatus}},
        {"forcepos",           {{ACLRole::CM},           2, &AOClient::cmdForcePos}},
        {"currentmusic",       {{ACLRole::NONE},         0, &AOClient::cmdCurrentMusic}},
        {"pm",                 {{ACLRole::NONE},         2, &AOClient::cmdPM}},
        {"evidence_mod",       {{ACLRole::CM},           1, &AOClient::cmdEvidenceMod}},
        {"motd",               {{ACLRole::NONE},         0, &AOClient::cmdMOTD}},
        {"m",                  {{ACLRole::MODCHAT},      1, &AOClient::cmdM}},
        {"mute",               {{ACLRole::MUTE},         1, &AOClient::cmdMute}},
        {"unmute",             {{ACLRole::MUTE},         1, &AOClient::cmdUnMute}},
        {"bans",               {{ACLRole::BAN},          0, &AOClient::cmdBans}},
        {"unban",              {{ACLRole::BAN},          1, &AOClient::cmdUnBan}},
        {"removeuser",         {{ACLRole::MODIFY_USERS}, 1, &AOClient::cmdRemoveUser}},
        {"subtheme",           {{ACLRole::CM},           1, &AOClient::cmdSubTheme}},
        {"about",              {{ACLRole::NONE},         0, &AOClient::cmdAbout}},
        {"evidence_swap",      {{ACLRole::CM},           2, &AOClient::cmdEvidence_Swap}},
        {"notecard",           {{ACLRole::NONE},         1, &AOClient::cmdNoteCard}},
        {"notecardreveal",     {{ACLRole::CM},           0, &AOClient::cmdNoteCardReveal}},
        {"notecard_reveal",    {{ACLRole::CM},           0, &AOClient::cmdNoteCardReveal}},
        {"notecardclear",      {{ACLRole::NONE},         0, &AOClient::cmdNoteCardClear}},
        {"notecard_clear",     {{ACLRole::NONE},         0, &AOClient::cmdNoteCardClear}},
        {"8ball",              {{ACLRole::NONE},         1, &AOClient::cmd8Ball}},
        {"judgelog",           {{ACLRole::CM},           0, &AOClient::cmdJudgeLog}},
        {"allowblankposting",  {{ACLRole::MODCHAT},      0, &AOClient::cmdAllowBlankposting}},
        {"gimp",               {{ACLRole::MUTE},         1, &AOClient::cmdGimp}},
        {"ungimp",             {{ACLRole::MUTE},         1, &AOClient::cmdUnGimp}},
        {"baninfo",            {{ACLRole::BAN},          1, &AOClient::cmdBanInfo}},
        {"testify",            {{ACLRole::CM},           0, &AOClient::cmdTestify}},
        {"testimony",          {{ACLRole::NONE},         0, &AOClient::cmdTestimony}},
        {"examine",            {{ACLRole::CM},           0, &AOClient::cmdExamine}},
        {"pause",              {{ACLRole::CM},           0, &AOClient::cmdPauseTestimony}},
        {"delete",             {{ACLRole::CM},           0, &AOClient::cmdDeleteStatement}},
        {"update",             {{ACLRole::CM},           0, &AOClient::cmdUpdateStatement}},
        {"add",                {{ACLRole::CM},           0, &AOClient::cmdAddStatement}},
        {"reload",             {{ACLRole::SUPER},        0, &AOClient::cmdReload}},
        {"disemvowel",         {{ACLRole::MUTE},         1, &AOClient::cmdDisemvowel}},
        {"undisemvowel",       {{ACLRole::MUTE},         1, &AOClient::cmdUnDisemvowel}},
        {"shake",              {{ACLRole::MUTE},         1, &AOClient::cmdShake}},
        {"unshake",            {{ACLRole::MUTE},         1, &AOClient::cmdUnShake}},
        {"forceimmediate",     {{ACLRole::CM},           0, &AOClient::cmdForceImmediate}},
        {"allowiniswap",       {{ACLRole::CM},           0, &AOClient::cmdAllowIniswap}},
        {"afk",                {{ACLRole::NONE},         0, &AOClient::cmdAfk}},
        {"savetestimony",      {{ACLRole::NONE},         1, &AOClient::cmdSaveTestimony}},
        {"loadtestimony",      {{ACLRole::CM},           1, &AOClient::cmdLoadTestimony}},
        {"permitsaving",       {{ACLRole::MODCHAT},      1, &AOClient::cmdPermitSaving}},
        {"mutepm",             {{ACLRole::NONE},         0, &AOClient::cmdMutePM}},
        {"toggleadverts",      {{ACLRole::NONE},         0, &AOClient::cmdToggleAdverts}},
        {"ooc_mute",           {{ACLRole::MUTE},         1, &AOClient::cmdOocMute}},
        {"ooc_unmute",         {{ACLRole::MUTE},         1, &AOClient::cmdOocUnMute}},
        {"blockwtce",          {{ACLRole::MUTE},         1, &AOClient::cmdBlockWtce}},
        {"unblockwtce",        {{ACLRole::MUTE},         1, &AOClient::cmdUnBlockWtce}},
        {"blockdj",            {{ACLRole::MUTE},         1, &AOClient::cmdBlockDj}},
        {"unblockdj",          {{ACLRole::MUTE},         1, &AOClient::cmdUnBlockDj}},
        {"charcurse",          {{ACLRole::MUTE},         1, &AOClient::cmdCharCurse}},
        {"uncharcurse",        {{ACLRole::MUTE},         1, &AOClient::cmdUnCharCurse}},
        {"charselect",         {{ACLRole::NONE},         0, &AOClient::cmdCharSelect}},
        {"togglemusic",        {{ACLRole::CM},           0, &AOClient::cmdToggleMusic}},
        {"kickuid",            {{ACLRole::KICK},         2, &AOClient::cmdKickUid}},
        {"firstperson",        {{ACLRole::NONE},         0, &AOClient::cmdFirstPerson}},
        {"updateban",          {{ACLRole::BAN},          3, &AOClient::cmdUpdateBan}},
        {"changepass",         {{ACLRole::SUPER},        1, &AOClient::cmdChangePassword}},
        {"ignorebglist",       {{ACLRole::IGNORE_BGLIST},0, &AOClient::cmdIgnoreBgList}},
        {"togglemessage",      {{ACLRole::CM},           0, &AOClient::cmdToggleAreaMessageOnJoin}},
        {"clearmessage",       {{ACLRole::CM},           0, &AOClient::cmdClearAreaMessage}},
        {"areamessage",        {{ACLRole::CM},           0, &AOClient::cmdAreaMessage}},
        {"notice",             {{ACLRole::SEND_NOTICE},  1, &AOClient::cmdNotice}},
        {"noticeg",            {{ACLRole::SEND_NOTICE},  1, &AOClient::cmdNoticeGlobal}},
        {"clearcm",            {{ACLRole::KICK},         0, &AOClient::cmdClearCM}},
        {"areamessage",        {{ACLRole::CM},           0, &AOClient::cmdAreaMessage}},
        {"addsong",            {{ACLRole::CM},           1, &AOClient::cmdAddSong}},
        {"addcategory",        {{ACLRole::CM},           1, &AOClient::cmdAddCategory}},
        {"removeentry",        {{ACLRole::CM},           1, &AOClient::cmdRemoveCategorySong}},
        {"toggleroot",         {{ACLRole::CM},           0, &AOClient::cmdToggleRootlist}},
        {"clearcustom",        {{ACLRole::CM},           0, &AOClient::cmdClearCustom}},
        {"togglewtce",         {{ACLRole::CM},           0, &AOClient::cmdToggleWtce}},
        {"toggleshouts",       {{ACLRole::CM},           0, &AOClient::cmdToggleShouts}},
        {"kick_other",         {{ACLRole::NONE},         0, &AOClient::cmdKickOther}},
        {"ipidinfo",           {{ACLRole::IPIDINFO},     1, &AOClient::cmdIpidInfo}},
        {"sneak",              {{ACLRole::NONE},         0, &AOClient::cmdSneak}},
        {"taketaked",          {{ACLRole::TAKETAKED},    0, &AOClient::cmdTakeTakedChar}},
        {"currentevimod",      {{ACLRole::CM},           0, &AOClient::cmdCurrentEvimod}},
        {"bgs",                {{ACLRole::NONE},         0, &AOClient::cmdBgs}},
        {"blind",              {{ACLRole::MUTE},         1, &AOClient::cmdBlind}},
        {"unblind",            {{ACLRole::MUTE},         1, &AOClient::cmdUnBlind}},
        {"currentbg",          {{ACLRole::NONE},         0, &AOClient::cmdCurBg}},
        {"sneak_mod",          {{ACLRole::SNEAK},        0, &AOClient::cmdSneakMod}},
        {"togglefloodguard",   {{ACLRole::CM},           0, &AOClient::cmdToggleFloodguardActuve}},
        {"togglechillmod",     {{ACLRole::CM},           0, &AOClient::cmdToggleChillMod}},
        {"toggleautomod",      {{ACLRole::CM},           0, &AOClient::cmdToggleAutoMod}},
        {"togglewuso",         {{ACLRole::WUSO},         0, &AOClient::cmdToggleWebUsersSpectateOnly}},
        {"removewuso",         {{ACLRole::WUSO},         1, &AOClient::cmdRemoveWebUsersSpectateOnly}},
        {"play_once",          {{ACLRole::NONE},         1, &AOClient::cmdPlayOnce}},
        {"areapassword",       {{ACLRole::CM},           0, &AOClient::cmdSetAreaPassword}},
        {"password",           {{ACLRole::NONE},         1, &AOClient::cmdSetClientPassword}},
        {"renamearea",         {{ACLRole::GM},           1, &AOClient::cmdRenameArea}},
        {"createarea",         {{ACLRole::GM},           1, &AOClient::cmdCreateArea}},
        {"removearea",         {{ACLRole::GM},           1, &AOClient::cmdRemoveArea}},
        {"saveareas",          {{ACLRole::NONE},         1, &AOClient::cmdSaveAreas}},
        {"permitareasaving",   {{ACLRole::MODCHAT},      1, &AOClient::cmdPermitAreaSaving}},
        {"swapareas",          {{ACLRole::GM},           2, &AOClient::cmdSwapAreas}},
        {"toggleprotected",    {{ACLRole::GM},           0, &AOClient::cmdToggleProtected}}
};

void AOClient::clientDisconnected()
{
#ifdef NET_DEBUG
    qDebug() << m_remote_ip.toString() << "disconnected";
#endif

    if (m_joined) {
        server->getAreaById(m_current_area)->clientLeftArea(server->getCharID(m_current_char), m_id);
        arup(ARUPType::PLAYER_COUNT, true);

        QString l_sender_name = getSenderName(m_id);

        if (!m_sneaked)
            sendServerMessageArea ("[" + QString::number(m_id) + "] " + l_sender_name + " has disconnected.");

        emit logDisconnect(m_current_char, m_ipid, m_ooc_name, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
    }

    if (m_current_char != "") {
        server->updateCharsTaken(server->getAreaById(m_current_area));
    }

    bool l_updateLocks = false;

    const QVector<AreaData *> l_areas = server->getAreas();
    for (AreaData *l_area : l_areas) {
        l_updateLocks = l_updateLocks || l_area->removeOwner(m_id);
    }

    if (l_updateLocks)
        arup(ARUPType::LOCKED, true);

    arup(ARUPType::CM, true);
    emit clientSuccessfullyDisconnected(m_id);
}

void AOClient::clientConnected()
{
    server->getDatabaseManager()->ipidip(m_ipid, m_remote_ip.toString().replace("::ffff:",""), QDateTime::currentDateTime().toString("dd-MM-yyyy"));

    long l_haznumdate = server->getDatabaseManager()->getHazNumDate(m_ipid);
    long l_currentdate = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (l_haznumdate == 0)
        return;

    if ((l_currentdate - l_haznumdate) > 604752) { // If about a week or more has passed since the last punishment from the automoderator.
        int l_haznumnew = server->getDatabaseManager()->getHazNum(m_ipid) - 1;

        if (l_haznumnew < 1)
            return;

        server->getDatabaseManager()->updateHazNum(m_ipid, l_currentdate);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_haznumnew);
    }
}

bool AOClient::evidencePresent(QString id)
{
    AreaData *l_area = server->getAreaById(m_current_area);

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
    qDebug() << "Received packet:" << packet.getHeader() << ":" << packet.getContent() << "args length:" << packet.getContent().length();
#endif
    AreaData *l_area = server->getAreaById(m_current_area);
    PacketInfo l_info = packets.value(packet.getHeader(), {ACLRole::NONE, 0, &AOClient::pktDefault});

    if (packet.getContent().join("").size() > 16384) {
        return;
    }

    if (!checkPermission(l_info.acl_permission)) {
        return;
    }

    if (packet.getHeader() != "CH" && m_joined) {
        if (m_is_afk)
        {
            QString l_sender_name = getSenderName(m_id);
            sendServerMessage("You are no longer AFK. Welcome back!");
            sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " are no longer AFK.");
        }
        m_is_afk = false;
    }

    if (packet.getContent().length() < l_info.minArgs) {
#ifdef NET_DEBUG
        qDebug() << "Invalid packet args length. Minimum is" << l_info.minArgs << "but only" << packet.getContent().length() << "were given.";
#endif
        return;
    }

    (this->*(l_info.action))(l_area, packet.getContent().length(), packet.getContent(), packet);
}

void AOClient::changeArea(int new_area, bool ignore_cooldown)
{
    const int l_old_area = m_current_area;

    if (m_current_area == new_area) {
        sendServerMessage("You are already in area " + server->getAreaName(m_current_area));
        return;
    }

    if (server->getAreaById(new_area)->lockStatus() == AreaData::LockStatus::LOCKED && !server->getAreaById(new_area)->invited().contains(m_id) && !checkPermission(ACLRole::BYPASS_LOCKS)) {
        sendServerMessage("Area " + server->getAreaName(m_current_area) + " is locked.");
        return;
    }

    if (!server->getAreaById(new_area)->areaPassword().isEmpty() && m_password != server->getAreaById(new_area)->areaPassword() && !checkPermission(ACLRole::BYPASS_LOCKS)) {
        sendServerMessage("Area " + server->getAreaName(m_current_area) + " is passworded.");
        return;
    }

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_area_change_time <= 2 && !ignore_cooldown) {
        sendServerMessage("You change area very often!");
        return;
    }

    m_last_area_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();

    QString l_sender_name = getSenderName(m_id);

    if (!m_sneaked)
        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " moved to " + "[" + QString::number(new_area) + "] " + server->getAreaName(new_area));

    if (m_current_char != "") {
        server->getAreaById(m_current_area)->changeCharacter(server->getCharID(m_current_char), -1);
        server->updateCharsTaken(server->getAreaById(m_current_area));
    }

    server->getAreaById(m_current_area)->clientLeftArea(m_char_id, m_id);
    bool l_character_taken = false;

    if (server->getAreaById(new_area)->charactersTaken().contains(server->getCharID(m_current_char))) {
        m_current_char = "";
        m_char_id = -1;
        l_character_taken = true;
    }

    server->getAreaById(new_area)->clientJoinedArea(m_char_id, m_id);
    m_current_area = new_area;
    arup(ARUPType::PLAYER_COUNT, true);
    sendEvidenceList(server->getAreaById(new_area));
    sendPacket("HP", {"1", QString::number(server->getAreaById(new_area)->defHP())});
    sendPacket("HP", {"2", QString::number(server->getAreaById(new_area)->proHP())});
    sendPacket("BN", {server->getAreaById(new_area)->background(), m_pos});

    if (l_character_taken && m_take_taked_char == false) {
        sendPacket("DONE");
    }

    const QList<QTimer*> l_timers = server->getAreaById(m_current_area)->timers();
    for (QTimer* l_timer : l_timers) {
        int l_timer_id = server->getAreaById(m_current_area)->timers().indexOf(l_timer) + 1;
        if (l_timer->isActive()) {
            sendPacket("TI", {QString::number(l_timer_id), "2"});
            sendPacket("TI", {QString::number(l_timer_id), "0", QString::number(QTime(0,0).msecsTo(QTime(0,0).addMSecs(l_timer->remainingTime())))});
        }
        else {
            sendPacket("TI", {QString::number(l_timer_id), "3"});
        }
    }

    sendServerMessage("You moved to area " + server->getAreaName(m_current_area));

    if (server->getAreaById(m_current_area)->sendAreaMessageOnJoin())
        sendServerMessage(server->getAreaById(m_current_area)->areaMessage());

    if (!m_sneaked)
        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " enters from " + "[" + QString::number(l_old_area) + "] " + server->getAreaName(l_old_area));

    if (server->getAreaById(m_current_area)->lockStatus() == AreaData::LockStatus::SPECTATABLE)
        sendServerMessage("Area " + server->getAreaName(m_current_area) + " is spectate-only; to chat IC you will need to be invited by the CM.");

    emit logChangeArea((m_current_char + " " + m_showname), m_ooc_name,m_ipid,server->getAreaById(m_current_area)->name(),server->getAreaName(l_old_area) + " -> " + server->getAreaName(new_area), QString::number(m_id), m_hwid);
}

bool AOClient::changeCharacter(int char_id)
{
    QString const l_old_char = m_current_char;
    AreaData* l_area = server->getAreaById(m_current_area);

    if (char_id >= server->getCharacterCount())
        return false;

    if (m_is_charcursed && !m_charcurse_list.contains(char_id)) {
        return false;
    }

    bool l_successfulChange = l_area->changeCharacter(server->getCharID(m_current_char), char_id, m_take_taked_char);

    if (char_id < 0) {
        m_current_char = "";
        m_char_id = char_id;
        setSpectator(true);
    }

    if (l_successfulChange == true) {
        QString char_selected = server->getCharacterById(char_id);
        m_current_char = char_selected;
        m_pos = "";
        server->updateCharsTaken(l_area);
        sendPacket("PV", {QString::number(m_id), "CID", QString::number(char_id)});
        emit logChangeChar((m_current_char + " " + m_showname), m_ooc_name,m_ipid,server->getAreaById(m_current_area)->name(),l_old_char + " -> " + m_current_char, QString::number(m_id), m_hwid);
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
    command = command.toLower();
    QString l_target_command = command;
    QVector<ACLRole::Permission> l_permissions;

    // check for aliases
    const QList<CommandExtension> l_extensions = server->getCommandExtensionCollection()->getExtensions();
    for (const CommandExtension &i_extension : l_extensions) {
        if (i_extension.checkCommandNameAndAlias(command)) {
            l_target_command = i_extension.getCommandName();
            l_permissions = i_extension.getPermissions();
            break;
        }
    }

    CommandInfo l_command = COMMANDS.value(l_target_command, {{ACLRole::NONE}, -1, &AOClient::cmdDefault});
    if (l_permissions.isEmpty()) {
        l_permissions.append(l_command.acl_permissions);
    }

    bool l_has_permissions = false;
    for (const ACLRole::Permission i_permission : qAsConst(l_permissions)) {
        if (checkPermission(i_permission)) {
            l_has_permissions = true;
            break;
        }
    }
    if (!l_has_permissions) {
        sendServerMessage("You do not have permission to use that command.");
        return;
    }

    if (argc < l_command.minArgs) {
        sendServerMessage("Invalid command syntax.");
        return;
    }

    (this->*(l_command.action))(argc, argv);
}

void AOClient::arup(ARUPType type, bool broadcast)
{
    QStringList l_arup_data;
    l_arup_data.append(QString::number(type));

    const QVector<AreaData *> l_areas = server->getAreas();
    for (AreaData *l_area : l_areas) {
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

void AOClient::fullArup()
{
    arup(ARUPType::PLAYER_COUNT, false);
    arup(ARUPType::STATUS, false);
    arup(ARUPType::CM, false);
    arup(ARUPType::LOCKED, false);
}

void AOClient::sendPacket(AOPacket packet)
{
#ifdef NET_DEBUG
    qDebug() << "Sent packet:" << packet.getHeader() << ":" << packet.getContent();
#endif

    m_socket->write(packet);
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
    for (int i = 0; i <= 4; i++) {
         if (m_last5messagestime[i] == -5) {
             m_last5messagestime[i] = QDateTime::currentDateTime().toSecsSinceEpoch();
             return;
         }
    }

    int l_warn = server->getDatabaseManager()->getWarnNum(m_ipid);

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - server->getDatabaseManager()->getWarnDate(m_ipid)
            > parseTime(ConfigManager::autoModWarnTerm()) && l_warn != 0 && l_warn != 1) {
    long date = QDateTime::currentDateTime().toSecsSinceEpoch();
    server->getDatabaseManager()->updateWarn(m_ipid, l_warn - 1);
    server->getDatabaseManager()->updateWarn(m_ipid, date);
    }

    if (m_last5messagestime[4] - m_last5messagestime[0] < ConfigManager::autoModTrigger() && !m_first_message) {

        if (l_warn == 0) {
            DBManager::automodwarns l_warn;
            l_warn.ipid = m_ipid;
            l_warn.date = QDateTime::currentDateTime().toSecsSinceEpoch();
            l_warn.warns = 2;

            server->getDatabaseManager()->addWarn(l_warn);
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(2) + " more warn, then you will be punished.");
            clearLastMessages();
            return;
        }

        if (l_warn < 4) {
            long date = QDateTime::currentDateTime().toSecsSinceEpoch();

            server->getDatabaseManager()->updateWarn(m_ipid, l_warn + 1);
            server->getDatabaseManager()->updateWarn(m_ipid, date);
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(5 - (l_warn + 1)) + " more warn, then you will be punished.");
            clearLastMessages();
            return;
        }

        int l_haznum = server->getDatabaseManager()->getHazNum(m_ipid);

        switch (l_haznum) {
        case 0:
        case 1: autoMute(ic_chat, l_haznum); break;
        case 2: autoKick(); break;
        case 3: autoBan(); break;
        }
  }

    if (m_first_message)
        m_first_message = !m_first_message;

    clearLastMessages();
}

void AOClient::clearLastMessages()
{
    for (int i = 0; i <= 4; i++)
         m_last5messagestime[i] = -5;
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
       server->getDatabaseManager()->addHazNum(l_num);
    else {
        long date = l_num.date;

        emit logCMD("Automoderator","", "","MUTE","Muted UID: " + QString::number(target->m_id),server->getAreaById(m_current_area)->name(), "", "");
        server->getDatabaseManager()->updateHazNum(m_ipid, date);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_num.action);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_num.haznum);
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

    server->getDatabaseManager()->updateHazNum(m_ipid, l_date);
    server->getDatabaseManager()->updateHazNum(m_ipid, l_action);
    server->getDatabaseManager()->updateHazNum(m_ipid, l_haznum);
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
            server->getDatabaseManager()->addBan(l_ban);
            ban_logged = true;
        }

        QString l_ban_duration;

        if (!(l_ban.duration == -2)) {
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            l_ban_duration = "The heat death of the universe.";
        }

        int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        l_client->sendPacket("KB", {l_ban.reason + "\nID: " + QString::number(l_ban_id) + "\nUntil: " + l_ban_duration});
        l_client->m_socket->close();

        emit logBan(l_ban.moderator,l_ban.ipid,l_ban_duration,l_ban.reason, "","");
        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(l_ban.ipid, l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);

        long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
        QString l_action = "BAN";

        server->getDatabaseManager()->updateHazNum(m_ipid, l_date);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_action);
    }
}

bool AOClient::checkPermission(ACLRole::Permission f_permission) const
{
    if (f_permission == ACLRole::NONE) {
        return true;
    }

    if ((f_permission == ACLRole::CM) && server->getAreaById(m_current_area)->owners().contains(m_id)) {
        return true; // I'm sorry for this hack.
    }

    if (!isAuthenticated()) {
        return false;
    }

    if (ConfigManager::authType() == DataTypes::AuthType::SIMPLE) {
        return true;
    }

    const ACLRole l_role = server->getACLRolesHandler()->getRoleById(m_acl_role_id);
    return l_role.checkPermission(f_permission);
}


QString AOClient::getIpid() const { return m_ipid; }

QString AOClient::getHwid() const { return m_hwid; }

bool AOClient::hasJoined() const { return m_joined; }

bool AOClient::isAuthenticated() const { return m_authenticated; }

Server *AOClient::getServer() { return server; }

void AOClient::setSpectator(bool f_spectator)
{
    m_is_spectator = f_spectator;
}

bool AOClient::isSpectator() const
{
    return m_is_spectator;
}

AOClient::AOClient(Server *p_server, NetworkSocket *socket, QObject *parent, int user_id, MusicManager *p_manager) :
     QObject(parent),
     m_id(user_id),
     m_remote_ip(socket->peerAddress()),
     m_password(""),
     m_joined(false),
     m_current_area(0),
     m_current_char(""),
     m_socket(socket),
     server(p_server),
     is_partial(false),
     m_last_wtce_time(0),
     m_last_area_change_time(0),
     m_last_music_change_time(0),
     m_last5messagestime(),
     m_music_manager(p_manager)
{
    m_afk_timer = new QTimer;
    m_afk_timer->setSingleShot(true);
}

AOClient::~AOClient()
{
    clientDisconnected();
    m_socket->deleteLater();
}
