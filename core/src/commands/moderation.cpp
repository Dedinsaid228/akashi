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
#include "include/db_manager.h"
#include "include/packet/packet_factory.h"
#include "include/server.h"

// This file is for commands under the moderation category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdBan(int argc, QStringList argv)
{
    QString l_args_str = argv[2];

    if (argv[0] == m_ipid) {
        sendServerMessage("Nope.");
        return;
    }

    if (argc > 3) {
        for (int i = 3; i < argc; i++)
            l_args_str += " " + argv[i];
    }

    DBManager::BanInfo l_ban;

    long long l_duration_seconds = 0;

    if (argv[1] == "perma")
        l_duration_seconds = -2;
    else
        l_duration_seconds = parseTime(argv[1]);

    if (l_duration_seconds == -1) {
        sendServerMessage("Invalid time format. Format example: 1h30m");
        return;
    }

    l_ban.duration = l_duration_seconds;
    l_ban.ipid = argv[0];
    l_ban.reason = l_args_str;
    l_ban.moderator = m_ipid;
    l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
    bool l_ban_logged = false;
    int l_kick_counter = 0;

    const QList<AOClient *> l_targets = server->getClientsByIpid(l_ban.ipid);
    for (AOClient *l_client : l_targets) {
        if (!l_ban_logged) {
            l_ban.ip = l_client->m_remote_ip;
            l_ban.hdid = l_client->m_hwid;
            server->getDatabaseManager()->addBan(l_ban);
            sendServerMessage("Banned user with ipid " + l_ban.ipid + " for reason: " + l_ban.reason);
            l_ban_logged = true;
        }

        QString l_ban_duration;

        if (!(l_ban.duration == -2)) {
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            l_ban_duration = "The heat death of the universe.";
        }

        int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        sendServerMessage("Ban ID: " + QString::number(l_ban_id));
        l_client->sendPacket("KB", {l_ban.reason + "\nID: " + QString::number(l_ban_id) + "\nUntil: " + l_ban_duration});
        l_client->m_socket->close();
        l_kick_counter++;

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason, QString::number(m_id), m_hwid);
        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(l_ban.ipid, l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);
    }

    if (l_kick_counter > 1)
        sendServerMessage("Kicked " + QString::number(l_kick_counter) + " clients with matching ipids.");

    // We're banning someone not connected.
    if (!l_ban_logged) {
        l_ban.ip = m_remote_ip;
        server->getDatabaseManager()->addBan(l_ban);
        QString l_ban_duration;
        int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);

        sendServerMessage("Banned " + l_ban.ipid + " for reason: " + l_ban.reason);
        sendServerMessage("Ban ID: " + QString::number(l_ban_id));

        if (!(l_ban.duration == -2)) {
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            l_ban_duration = "The heat death of the universe.";
        }

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason, QString::number(m_id), m_hwid);
        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(l_ban.ipid + " (Сlient was not connected to the server)", l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);
    }
}

void AOClient::cmdKick(int argc, QStringList argv)
{
    QString l_target_ipid = argv[0];
    QString l_reason = argv[1];
    int l_kick_counter = 0;

    if (l_target_ipid == m_ipid) {
        sendServerMessage("Nope.");
        return;
    }

    if (argc > 2) {
        for (int i = 2; i < argv.length(); i++) {
            l_reason += " " + argv[i];
        }
    }

    const QList<AOClient *> targets = server->getClientsByIpid(l_target_ipid);
    for (AOClient *l_client : targets) {
        l_client->sendPacket("KK", {l_reason});
        l_client->m_socket->close();
        l_kick_counter++;
    }

    if (l_kick_counter > 0) {
        emit logKick(m_ipid, l_target_ipid, l_reason, QString::number(m_id), m_hwid);
        sendServerMessage("Kicked " + QString::number(l_kick_counter) + " client(s) with ipid " + l_target_ipid + " for reason: " + l_reason);
    }
    else
        sendServerMessage("User with ipid not found!");
}

void AOClient::cmdMods(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_entries;
    int l_online_count = 0;

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_authenticated && !l_client->m_sneak_mod) {
            l_entries << "---";

            if (ConfigManager::authType() != DataTypes::AuthType::SIMPLE) {
                l_entries << "Moderator: " + l_client->m_moderator_name;
                l_entries << "Role:" << l_client->m_acl_role_id;
            }

            l_entries << "OOC name: " + l_client->m_ooc_name;
            l_entries << "ID: " + QString::number(l_client->m_id);
            l_entries << "Area: " + QString::number(l_client->m_current_area);
            l_entries << "Character: " + l_client->m_current_char;
            l_online_count++;
        }
    }

    l_entries << "---";
    l_entries << "Total online: " << QString::number(l_online_count);
    sendServerMessage(l_entries.join("\n"));
}

void AOClient::cmdHelp(int argc, QStringList argv)
{
    if (argc == 0) {
        sendServerMessage("/help area - commands relate to area management.\n"
                          "/help areaedit - commands to manage areas.\n"
                          "/help hubs - commands to manage hubs.\n"
                          "/help casing - commands relate to casing.\n"
                          "/help testimony - testimony recording commands.\n"
                          "/help roleplay - commands are related to various kinds of roleplay actions.\n"
                          "/help char - commands related to characters.\n"
                          "/help messaging - commands related to messaging.\n"
                          "/help music - commands related to music.\n"
                          "/help other - others commands.");
        return;
    }

    if (argv[0] == "other")
        sendServerMessage("/about - gives a very brief description of kakashi.\n"
                          "/mods - lists the currently logged-in moderators on the server.\n"
                          "/motd - gets the server's Message Of The Day.\n"
                          "/sneak - hide messages about your movements by areas.");
    else if (argv[0] == "music")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "/currentmusic - show the current music playing.\n"
                          "/play [musicname or URL] - play a music.\n"
                          "/play_once [musicname or URL] - play a music once.\n"
                          "/togglemusic - enable/disable change music in area. [CM]\n"
                          "/addsong [URL] - adds a song to the custom musiclist. [CM]\n"
                          "/addcategory [category name] - adds a category to the custom musiclist. [CM]\n"
                          "/removeentry [URL or category name] - removes an entry from the custom musiclist. [CM]\n"
                          "/toggleroot - changes the behaviour of prepending the server root musiclist to the custom lists of the area. [CM]\n"
                          "/clearcustom - removes all custom songs from the area. [CM]\n"
                          "/play_hub [musicname or URL] - play a music in all areas of the hub. [GM]\n"
                          "/play_once_hub [musicname or URL] - play a music in all areas of the hub once. [GM]\n"
                          "/playggl [file id] - play a music from Google Drive.***\n"
                          "/playggl_once [file id] - play a music from Google Drive once.***\n"
                          "/playggl_hub [file id] - play a music from Google Drive in all areas of the hub. [GM]***\n"
                          "/playggl_once_hub [file id] - play a music from Google Drive in all areas of the hub once. [GM]***\n"
                          "*** - To get the file id, share the file with everyone and copy the link. File id is in the middle of the link - https://drive.google.com/file/d/[FILE ID]/view?usp=sharing");
    else if (argv[0] == "messaging")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets.\n"
                          "/afk - sets your player as AFK in player listings.\n"
                          "/g [message] - sends a global message to all players.\n"
                          "/toggleglobal - toggles whether will ignore global messages or not.\n"
                          "/pm [id] [message] - send a private message to another online user.\n"
                          "/mutepm - toggles whether will recieve private messages or not.\n"
                          "/need [message] - sends a global message expressing that the area needs something, such as players for something.\n"
                          "/toggleadverts - toggles whether the caller will receive /need advertisements.\n"
                          "/g_hub [message] - sends a message in all areas of hub.");
    else if (argv[0] == "char")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/charselect - enter the character select screen.\n"
                          "/forcepos [pos] [id] - Set the place another character resides in the area. [CM]\n"
                          "/randomchar - picks a new random character.\n"
                          "/switch [name] - Switches to a different character.\n"
                          "/pos [position] - set the place your character resides in the area.");
    else if (argv[0] == "roleplay")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/8ball [text] - answers a question.\n/coinflip - flip a coin.\n"
                          "/roll (value/X Y) - roll a die. The result is shown publicly. X is the number of dice, Y is the maximum value on the die.\n"
                          "/rollp (value/X Y) - roll a die privately. Same as /roll but the result is only shown to you and the CMs.\n"
                          "/subtheme [subtheme name] - changes the subtheme of all clients in the current area. [CM]\n"
                          "/notecard [message] - writes a note card in the current area.\n"
                          "/notecard_reveal - reveals all note cards in the current area. [CM]\n"
                          "/notecard_clear - сlears a note card.\n"
                          "/vote (id) - without arguments - start/end voting (requires permission CM), with arguments - vote for the candidate.");
    else if (argv[0] == "testimony")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area, "
                          "and if there is a [MOD] prefix, then you need to be a moderator to use the command. "
                          "Testimony are only recorded from players who have chosen the wit position.\n"
                          "/add - adds a statement to an existing testimony. [CM]\n"
                          "/delete - deletes the currently displayed statement from the testimony recorder. [CM]\n"
                          "/examine - playback recorded testimony. [CM]\n"
                          "/loadtestimony [testimony name] - loads testimony for the testimony replay. [CM]\n"
                          "/savetestimony [testimony name] - saves a testimony recording to the servers storage. [MOD]\n"
                          "/testify - enables the testimony recording. [CM]\n"
                          "/testimony - display the currently recorded testimony.\n"
                          "/pause - pauses testimony playback/recording. [CM]");
    else if (argv[0] == "casing")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/cm (id) - add a case manager for the current area. Leave id blank to promote yourself if there are no CMs.\n"
                          "/uncm - remove a case manager from the current area. Leave id blank to demote yourself. [CM]\n"
                          "/evidence_mod [mod] - changes the evidence mod in an area.[CM]\n/currentevimod - find out current evidence mod. [CM]\n"
                          "/evidence_swap [evidence id] [evidence id] - Swap the positions of two evidence items on the evidence list. [CM]");
    else if (argv[0] == "area")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/area [area id] - go to another area.\n/area_kick [id] - remove a user from the current area and move them to another area. [CM]\n"
                          "/area_lock - prevent users from joining the current area unlesss /invite is used. [CM]\n"
                          "/area_unlock - allow anyone to freely join/speak the current area. [CM]\n"
                          "/area_spectate - sets the current area to spectatable, where anyone may join but only invited users may communicate in-character. [CM]\n"
                          "/area_mute - Makes this area impossible to speak for normal users unlesss /invite is used. [CM]\n"
                          "/allowiniswap - toggles whether iniswaps are allowed in the current area. [CM]\n"
                          "/bg [background name] - change background.\n"
                          "/bgs - get a list of backgrounds.\n"
                          "/forceimmediate - toggles immediate text processing in the current area. [CM]\n"
                          "/cleardoc - clear the link for the current case document.\n"
                          "/currentbg - find out the name of the current background in the area.\n"
                          "/doc (url) - show or change the link for the current case document.\n"
                          "/getarea - show information about the current area.\n"
                          "/getareas - show information about all areas in hub.\n"
                          "/getareahubs - show information about all areas.\n"
                          "/judgelog - list the last 10 uses of judge controls in the current area. [CM]\n"
                          "/invite [id] - allow a particular user to join a locked or speak in spectator-only area. [CM]\n"
                          "/uninvite [id] - revoke an invitation for a particular user. [CM]\n"
                          "/status [idle/rp/casing/looking-for-players/lfp/recess/gaming/erp/yablachki] - modify the current status of an area.\n"
                          "/togglechillmod - enable/disable Chill Mod in area. [CM]\n"
                          "/toogleautomod - enable/disable automoderator in area. [CM]\n"
                          "/togglefloodguard - enable/disable floodguard in area. [CM]\n"
                          "/togglemessage - toggles wether the client shows the area message when joining the current area. [CM]\n"
                          "/clearmessage - clears the areas message and disables automatic sending. [CM]\n"
                          "/areamessage (message) - returns the area message in OOC. If text is entered it updates the current area message. [CM]\n"
                          "/areapassword (new password) - when given arguments, sets the password that the client needs to set to enter the area. If no arguments are given, the command will return the current password in area.\n"
                          "/password (new password) - when given arguments, sets the password that the client needs to set to enter the passworded area. If no arguments are given, the command will return the current password of the client.\n"
                          "/bglock - locks the background of the current area, preventing it from being changed. [CM]\n"
                          "/bgunlock - unlocks the background of the current area, allowing it to be changed. [CM]\n"
                          "/togglewtce - toggles wether WTCE can be used in the area. [CM]\n"
                          "/toggleshouts - toggles wether shouts can be used in the area. [CM]\n"
                          "/togglestatus - toggles wether status can be changed in the area. [CM]\n"
                          "/ooc_type [all/invited/cm] - allow everyone to speak in OOC chat, only invited clients or only CMs. [CM]");
    else if (argv[0] == "areaedit")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "/renamearea [new name] - rename the current area.\n"
                          "/createarea [name] - create a new area.\n"
                          "/removearea [area id] - delete selected area.\n"
                          "/swapareas [area id] [area id] - swap selected areas.\n"
                          "/toggleprotected - toggle whether it is possible in the current area to become CM or not.\n"
                          "/saveareas [file name] - save the area config file. [SAVEAREAS]");
    else if (argv[0] == "hubs")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. "
                          "The commands presented here require [GM] permission.\n"
                          "/hub (hub id) - view the list of hubs or go to another hub.\n"
                          "/gm - become a GM of hub.\n"
                          "/ungm - unbecome a GM of hub.\n"
                          "/hub_hideplayercount - show/hide the number of players in the hub and its areas.\n"
                          "/hub_rename [new name] - rename hub.\n"
                          "/hub_listening - receive messages from all areas in the hub.\n"
                          "/hub_spectate - sets the current hub to spectatable.\n"
                          "/hub_lock - prevent users from joining the current hub unlesss /hub_invite is used.\n"
                          "/hub_unlock - allow anyone to freely join/speak the current hub.\n"
                          "/hub_invite - allow a particular player to join a locked or speak in spectator-only hub.\n"
                          "/hub_uninvite - revoke an invitation for a particular user.");
    else if (argv[0] == "admin")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "/allowblankposting - allow/deny blankposting in the area. [MODCHAT]\n"
                          "/ban [ipid] [time (examples - 1h30m, 1h, 30m, 30s)] [reason] - ban player. [BAN]\n"
                          "/unban [ban id] - unban player. [BAN]\n/baninfo [ban id] - get information about the ban. [BAN]\n"
                          "/bans - get information about the last 5 bans. [BAN]\n"
                          "/blockdj [uid] - restricts a target client from changing music. [MUTE]\n"
                          "/unblockdj [uid] - restores a client's DJ controls. [MUTE]\n"
                          "/blockwtce [uid] - revokes judge controls from a client, preventing them from using WT/CE buttons or updating penalties. [MUTE]\n"
                          "/unblockwtce [uid] - Grants judge controls back to a client. [MUTE]\n"
                          "/blind [uid] - blind the targeted player from being able to see or talk IC/OOC. [MUTE]\n"
                          "/unblind - undo effects of the /blind command. [MUTE]\n"
                          "/charcurse [uid] (character1,character2,...) - restricts a target client into only being able to switch between a set of characters. [MUTE]\n"
                          "/uncharcurse [uid] - uncharcurses a target client, allowing them to switch normally again. [MUTE]\n"
                          "/clearcm - removes all CMs from the current area. [KICK]\n"
                          "/disemvowel [uid] - removes all vowels from a target client's IC messages. [MUTE]\n"
                          "/undisemvowel [uid] - undisemvowels a client. [MUTE]\n"
                          "/gimp [uid] - replaces a target client's IC messages with strings randomly selected from gimp.txt. [MUTE]\n"
                          "/ungimp [uid] - ungimps a client. [MUTE]\n"
                          "/ipidinfo [ipid] - find out the ip address and date of ipid creation. [IPIDINFO]\n"
                          "/taketaked - allow/deny yourself take taked characters. [TAKETAKED]\n"
                          "/notice [message] - send all players in area a message in a special window. [SEND_NOTICE]\n"
                          "/noticeg [message] - send all players a message in a special window. [SEND_NOTICE]\n"
                          "/shake [uid] - shuffles the order of words in a target client's IC messages. [MUTE]\n"
                          "/unshake [uid] - unshakes a client.\n"
                          "/kick [ipid] [reason] - kick player [KICK].\n"
                          "/login [username] [password] - enters the login prompt to login as a moderator.\n"
                          "/unmod - logs a user out as a moderator.\n"
                          "/m [message] - sends a message to moderator-only chat. [MODCHAT]\n"
                          "/modarea_kick [uid] [area id] - kick the player into the specified area. [KICK]\n"
                          "/mute [uid] - mute player. [MUTE]\n"
                          "/unmute [uid] - unmute player. [MUTE]\n"
                          "/ooc_mute [uid] - mute player in OOC. [MUTE]\n"
                          "/ooc_unmute [uid] - unmute player in OOC. [MUTE]\n"
                          "/permitsaving [uid] - allows the target player to save 1 testimony with /savetestimony. [MODCHAT]\n"
                          "/updateban [ban id] [duration/reason] [updated info] - updates the ban with the specified ban ID. [BAN]\n"
                          "/removewuso [uid] - remove the WUSO action on the player. [WUSO]\n"
                          "/togglewuso - allow/deny WUSO. [WUSO]"
                          "/hub_protected - enable/disable the ability to become a GM in the hub. [MODCHAT]");
    else
        sendServerMessage("Wrong category! Type /help for category list.");
}

void AOClient::cmdMOTD(int argc, QStringList argv)
{
    if (argc == 0) {
        sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");
    }

    else if (argc > 0) {
        if (checkPermission(ACLRole::MOTD)) {
            QString l_MOTD = argv.join(" ");

            ConfigManager::setMotd(l_MOTD);
            sendServerMessage("MOTD has been changed.");
            emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "CHANGEMOTD", l_MOTD, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
        }
        else {
            sendServerMessage("You do not have permission to change the MOTD");
        }
    }
}

void AOClient::cmdBans(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_recent_bans;
    l_recent_bans << "Last 5 bans:";
    l_recent_bans << "-----";

    const QList<DBManager::BanInfo> l_bans_list = server->getDatabaseManager()->getRecentBans();
    for (const DBManager::BanInfo &l_ban : l_bans_list) {
        QString l_banned_until;

        if (l_ban.duration == -2)
            l_banned_until = "The heat death of the universe";
        else
            l_banned_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        l_recent_bans << "Ban ID: " + QString::number(l_ban.id);
        l_recent_bans << "Affected IPID: " + l_ban.ipid;
        l_recent_bans << "Affected HDID: " + l_ban.hdid;
        l_recent_bans << "Reason for ban: " + l_ban.reason;
        l_recent_bans << "Date of ban: " + QDateTime::fromSecsSinceEpoch(l_ban.time).toString("dd/MM/yyyy, hh:mm");
        l_recent_bans << "Ban lasts until: " + l_banned_until;
        l_recent_bans << "Moderator: " + l_ban.moderator;
        l_recent_bans << "-----";
    }

    sendServerMessage(l_recent_bans.join("\n"));
}

void AOClient::cmdUnBan(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool ok;
    int l_target_ban = argv[0].toInt(&ok);

    if (!ok) {
        sendServerMessage("Invalid ban ID.");
        return;
    }
    else if (server->getDatabaseManager()->invalidateBan(l_target_ban)) {
        sendServerMessage("Successfully invalidated ban " + argv[0] + ".");
    }
    else
        sendServerMessage("Couldn't invalidate ban " + argv[0] + ", are you sure it exists?");
}

void AOClient::cmdAbout(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("akashi by:"
                      "\n scatterflower"
                      "\n Salanto"
                      "\n in1tiate"
                      "\n MangosArentLiterature"
                      "\n and other cool guys!"
                      "\n Github: https://github.com/AttorneyOnline/akashi "
                      "\n kakashi by Ddedinya."
                      "\n Based on akashi Grapefruit. "
                      "\n Github: https://github.com/Ddedinya/kakashi");
}

void AOClient::cmdMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_id == m_id) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_is_muted)
        sendServerMessage("That player is already muted!");
    else {
        sendServerMessage("Muted player.");
    }

    l_target->m_is_muted = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "MUTE", "Muted UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdUnMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!l_target->m_is_muted)
        sendServerMessage("That player is not muted!");
    else {
        sendServerMessage("Unmuted player.");
        l_target->sendServerMessage("You were unmuted by a moderator. ");
    }

    l_target->m_is_muted = false;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "UNMUTE", "Unmuted UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdOocMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_id == m_id) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_is_ooc_muted)
        sendServerMessage("That player is already OOC muted!");
    else {
        sendServerMessage("OOC muted player.");
    }

    l_target->m_is_ooc_muted = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "OOCMUTE", "OOC muted UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdOocUnMute(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!l_target->m_is_ooc_muted)
        sendServerMessage("That player is not OOC muted!");
    else {
        sendServerMessage("OOC unmuted player.");
        l_target->sendServerMessage("You were OOC unmuted by a moderator. ");
    }

    l_target->m_is_ooc_muted = false;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "OOCUNMUTE", "OOC unmuted UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdBlockWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_id == m_id) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_is_wtce_blocked)
        sendServerMessage("That player is already judge blocked!");
    else {
        sendServerMessage("Revoked player's access to judge controls.");
    }

    l_target->m_is_wtce_blocked = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "BLOCKWTCE", "Blocked UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdUnBlockWtce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!l_target->m_is_wtce_blocked)
        sendServerMessage("That player is not judge blocked!");
    else {
        sendServerMessage("Restored player's access to judge controls.");
        l_target->sendServerMessage("A moderator restored your judge controls access. ");
    }

    l_target->m_is_wtce_blocked = false;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "UNBLOCKWTCE", "Unblocked UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdAllowBlankposting(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QString l_sender_name = getSenderName(m_id);
    AreaData *l_area = server->getAreaById(m_current_area);

    l_area->toggleBlankposting();

    if (l_area->blankpostingAllowed() == false) {
        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " has set blankposting in the area to forbidden.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "FORBIDBLANKPOST", "", server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
    }
    else {
        sendServerMessageArea("[" + QString::number(m_id) + "] " + l_sender_name + " has set blankposting in the area to allowed.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "ALLOWBLANKPOST", "", server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
    }
}

void AOClient::cmdBanInfo(int argc, QStringList argv)
{
    QStringList l_ban_info;
    l_ban_info << ("Ban Info for " + argv[0]);
    l_ban_info << "-----";
    QString l_lookup_type;

    if (argc == 1) {
        l_lookup_type = "banid";
    }
    else if (argc == 2) {
        l_lookup_type = argv[1];
        if (!((l_lookup_type == "banid") || (l_lookup_type == "ipid") || (l_lookup_type == "hdid"))) {
            sendServerMessage("Invalid ID type.");
            return;
        }
    }
    else {
        sendServerMessage("Invalid command.");
        return;
    }

    QString id = argv[0];

    const QList<DBManager::BanInfo> l_bans = server->getDatabaseManager()->getBanInfo(l_lookup_type, id);
    for (const DBManager::BanInfo &l_ban : l_bans) {
        QString l_banned_until;
        if (l_ban.duration == -2)
            l_banned_until = "The heat death of the universe";
        else
            l_banned_until = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");

        l_ban_info << "Ban ID: " + QString::number(l_ban.id);
        l_ban_info << "Affected IPID: " + l_ban.ipid;
        l_ban_info << "Affected HDID: " + l_ban.hdid;
        l_ban_info << "Reason for ban: " + l_ban.reason;
        l_ban_info << "Date of ban: " + QDateTime::fromSecsSinceEpoch(l_ban.time).toString("dd/MM/yyyy, hh:mm");
        l_ban_info << "Ban lasts until: " + l_banned_until;
        l_ban_info << "Moderator: " + l_ban.moderator;
        l_ban_info << "-----";
    }

    sendServerMessage(l_ban_info.join("\n"));
}

void AOClient::cmdReload(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        l_client->m_befrel_char_id = server->getCharID(l_client->m_current_char);

    // Todo: Make this a signal when splitting AOClient and Server.
    server->reloadSettings();
    server->broadcast(PacketFactory::createPacket("SC", server->getCharacters()));
    server->broadcast(PacketFactory::createPacket("FM", server->getMusicList()));

    for (AOClient *l_client : l_clients)
        server->getAreaById(l_client->m_current_area)->changeCharacter(l_client->m_befrel_char_id, server->getCharID(l_client->m_current_char));

    for (int i = 0; i < server->getAreaCount(); i++)
        server->updateCharsTaken(server->getAreaById(i));

    sendServerMessage("Reloaded configurations");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "RELOAD", "", server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdForceImmediate(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);
    l_area->toggleImmediate();
    QString l_state = l_area->forceImmediate() ? "on." : "off.";

    sendServerMessage("Forced immediate text processing in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "FORCEIMMEDIATE", l_state, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdAllowIniswap(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);
    l_area->toggleIniswap();
    QString l_state = l_area->iniswapAllowed() ? "allowed." : "disallowed.";

    sendServerMessage("Iniswapping in this area is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "ALLOWINISWAP", l_state, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdPermitSaving(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AOClient *l_client = server->getClientByID(argv[0].toInt());

    if (l_client == nullptr) {
        sendServerMessage("Invalid ID.");
        return;
    }

    l_client->m_testimony_saving = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "ALLOWTESTIMONYSAVING", "Target UID: " + QString::number(l_client->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdKickUid(int argc, QStringList argv)
{
    QString l_reason = argv[1];

    if (argc > 2) {
        for (int i = 2; i < argv.length(); i++) {
            l_reason += " " + argv[i];
        }
    }

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);
    if (!conv_ok && argv[0] != "*") {
        sendServerMessage("Invalid user ID.");
        return;
    }
    else if (conv_ok) {
        AOClient *l_target = server->getClientByID(l_uid);

        if (l_target == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }

        if (l_target->m_id == m_id) {
            sendServerMessage("Nope.");
            return;
        }

        l_target->sendPacket("KK", {l_reason});
        l_target->m_socket->close();
        sendServerMessage("Kicked client with UID " + argv[0] + " for reason: " + l_reason);
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "KICKUID", "Kicked UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
    }
    else if (argv[0] == "*") { // kick all clients in the area
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "AREAKICK", "Kicked all players from area", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, server->getHubName(m_hub));
        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients) {
            if (l_client->m_current_area == m_current_area && l_client->m_id != m_id) {
                l_client->sendPacket("KK", {l_reason});
                l_client->m_socket->close();
            }
        }
        sendServerMessage("Kicked all clients for reason: " + l_reason);
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "KICKUID", "Kicked all clients in area from the server", server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
    }
}

void AOClient::cmdUpdateBan(int argc, QStringList argv)
{
    bool conv_ok = false;
    int l_ban_id = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid ban ID.");
        return;
    }

    QVariant l_updated_info;

    if (argv[1] == "duration") {
        long long duration_seconds = 0;

        if (argv[2] == "perma")
            duration_seconds = -2;
        else
            duration_seconds = parseTime(argv[2]);

        if (duration_seconds == -1) {
            sendServerMessage("Invalid time format. Format example: 1h30m");
            return;
        }

        l_updated_info = QVariant(duration_seconds);
    }
    else if (argv[1] == "reason") {
        QString l_args_str = argv[2];

        if (argc > 3) {
            for (int i = 3; i < argc; i++)
                l_args_str += " " + argv[i];
        }

        l_updated_info = QVariant(l_args_str);
    }
    else {
        sendServerMessage("Invalid update type.");
        return;
    }
    if (!server->getDatabaseManager()->updateBan(l_ban_id, argv[1], l_updated_info)) {
        sendServerMessage("There was an error updating the ban. Please confirm the ban ID is valid.");
        return;
    }

    sendServerMessage("Ban updated.");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "UPDATEBAN", "Update info: " + argv[1] + " " + argv[2], server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdNotice(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    sendNotice(argv.join(" "));
}

void AOClient::cmdNoticeGlobal(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    sendNotice(argv.join(" "), true);
}

void AOClient::cmdClearCM(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(m_current_area);
    foreach (int l_client_id, l_area->owners()) {
        l_area->removeOwner(l_client_id);
    }
    arup(ARUPType::CM, true, m_hub);
    sendServerMessage("Removed all CMs from this area.");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "CLEARCM", "", server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdKickOther(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    int l_kick_counter = 0;

    QList<AOClient *> l_target_clients;
    const QList<AOClient *> l_targets_hwid = server->getClientsByHwid(m_hwid);
    l_target_clients = server->getClientsByIpid(m_ipid);

    // Merge both lookups into one single list.)
    for (AOClient *l_target_candidate : qAsConst(l_targets_hwid)) {
        if (!l_target_clients.contains(l_target_candidate)) {
            l_target_clients.append(l_target_candidate);
        }
    }

    // The list is unique, we can only have on instance of the current client.
    l_target_clients.removeOne(this);
    for (AOClient *l_target_client : qAsConst(l_target_clients)) {
        l_target_client->m_socket->close();
        l_kick_counter++;
    }
    sendServerMessage("Kicked " + QString::number(l_kick_counter) + " multiclients from the server.");
}

void AOClient::cmdIpidInfo(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QStringList l_ipid_info;
    l_ipid_info << "-----";
    QString l_ipid = argv[0];

    const QList<DBManager::idipinfo> l_ipidinfo = server->getDatabaseManager()->getIpidInfo(l_ipid);
    for (const DBManager::idipinfo &l_ipidip : l_ipidinfo) {
        l_ipid_info << "IPID: " + l_ipidip.ipid;
        l_ipid_info << "IP: " + l_ipidip.ip;
        l_ipid_info << "Created: " + l_ipidip.date;
        l_ipid_info << "-----";
    }

    sendServerMessage(l_ipid_info.join("\n"));
}

void AOClient::cmdTakeTakedChar(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_take_taked_char = !m_take_taked_char;
    QString l_str_en = m_take_taked_char ? "now take taked characters" : "no longer take taked characters";

    sendServerMessage("You are " + l_str_en + ".");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TAKETAKEDCHAR", l_str_en, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdBlind(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (l_target->m_id == m_id) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_blinded)
        sendServerMessage("That player is already blinded!");
    else {
        sendServerMessage("Blind player.");
    }

    l_target->m_blinded = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "BLIND", "Blinded UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdUnBlind(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!l_target->m_blinded)
        sendServerMessage("That player is not blinded!");
    else {
        sendServerMessage("Unblind player.");
        l_target->sendServerMessage("A moderator unblinded you. ");
    }

    l_target->m_blinded = false;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "UNBLIND", "Unblinded UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdSneakMod(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_sneak_mod = !m_sneak_mod;

    QString l_str_en = m_sneak_mod ? "invisible" : "visible";
    sendServerMessage("Your moderator status is now " + l_str_en + ".");
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "SNEAKMOD", l_str_en, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdToggleWebUsersSpectateOnly(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    ConfigManager::webUsersSpectableOnlyToggle();

    QString l_str_en = ConfigManager::webUsersSpectableOnly() ? "actived." : "disabled.";

    sendServerMessage("WUSO Mod is now " + l_str_en);

    if (ConfigManager::webUsersSpectableOnly() == false) {
        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients) {
            if (l_client->m_wuso == true) {
                l_client->m_wuso = false;
                l_client->m_usemodcall = true;
            }
        }
    }
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEWUSO", l_str_en, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}

void AOClient::cmdRemoveWebUsersSpectateOnly(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool conv_ok = false;
    int l_uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient *l_target = server->getClientByID(l_uid);

    if (l_target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!l_target->m_wuso)
        sendServerMessage("This player is not affected by WUSO Mod!");
    else {
        sendServerMessage("Done.");
        l_target->sendServerMessage("Now the restrictions of the WUSO Mod do not apply to you.");
    }

    l_target->m_wuso = false;
    l_target->m_usemodcall = true;
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "REMOVEWUSO", "Target UID: " + QString::number(l_target->m_id), server->getAreaName(m_current_area), QString::number(m_id), m_hwid, server->getHubName(m_hub));
}
