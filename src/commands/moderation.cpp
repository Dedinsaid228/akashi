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
#include "db_manager.h"
#include "packet/packet_factory.h"
#include "server.h"
#include <QtConcurrent/QtConcurrent>

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

    long long l_duration_seconds = 0;
    if (argv[1] == "perma")
        l_duration_seconds = -2;
    else
        l_duration_seconds = parseTime(argv[1]);

    if (l_duration_seconds == -1) {
        sendServerMessage("Invalid time format. Format example: 1h30m");
        return;
    }

    DBManager::BanInfo l_ban;
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
        if (!(l_ban.duration == -2))
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        else
            l_ban_duration = "Permanently.";

        int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        sendServerMessage("Ban ID: " + QString::number(l_ban_id));
        l_client->sendPacket("KB", {l_ban.reason + "\nID: " + QString::number(l_ban_id) + "\nUntil: " + l_ban_duration});
        l_client->m_socket->close();
        l_kick_counter++;

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason, QString::number(clientId()), m_hwid);
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

        if (!(l_ban.duration == -2))
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        else
            l_ban_duration = "Permanently.";

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason, QString::number(clientId()), m_hwid);
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
        emit logKick(m_ipid, l_target_ipid, l_reason, QString::number(clientId()), m_hwid);
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
        if (l_client->m_authenticated) {
            l_entries << "---";

            if (ConfigManager::authType() != DataTypes::AuthType::SIMPLE) {
                l_entries << "Moderator: " + l_client->m_moderator_name;
                l_entries << "Role:" << l_client->m_acl_role_id;
            }

            l_entries << "OOC name: " + l_client->name();
            l_entries << "ID: " + QString::number(l_client->clientId());
            l_entries << "Hub: " + QString::number(l_client->hubId());
            l_entries << "Area: " + QString::number(l_client->m_area_list.indexOf(l_client->areaId()));
            l_entries << "Character: " + l_client->character();
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
        sendServerMessage("(!) It is better to use the wiki: https://github.com/Ddedinya/kakashi/wiki \n"
                          "/help area - commands relate to setting an area.\n"
                          "/help areaedit - commands relate to manage areas.\n"
                          "/help hubs - commands relate to manage hubs.\n"
                          "/help casing - useful for casing commands.\n"
                          "/help testimony - testimony bot commands.\n"
                          "/help roleplay - useful for roleplay and mini-games commands.\n"
                          "/help char - commands for changing a character or position.\n"
                          "/help messaging - commands related to special messaging (global or private messages and etc.)\n"
                          "/help music - commands related to manage music.\n"
                          "/help admin - commands for moderating and something like that.\n"
                          "/help other - others commands.");
        return;
    }

    if (argv[0] == "other")
        sendServerMessage("/about - get a very brief description of kakashi.\n"
                          "/mods - get a list about moderators on server.\n"
                          "/motd - get a server MOTD. If the user has [MOTD] permission, they can edit MOTD with this command.\n"
                          "/sneak - hide messages about your movements by areas.\n"
                          "/kickphantoms - kick other your clients.\n"
                          "/webfiles - get links to download iniswaps of every client in the area.");
    else if (argv[0] == "music")
        sendServerMessage("[brackets] mean [required arguments]. Actually, commands don't need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "/currentmusic - returns the currently playing music in the area, and who played it.\n"
                          "/play [musicname or URL] - play a music.\n"
                          "/play_once [musicname or URL] - play a music once.\n"
                          "/togglemusic - toggle whether music is allowed to be played in the area or not. CM users can play music always. [CM]\n"
                          "/addsong [URL] - add a song to the area's custom musiclist. [CM]\n"
                          "/addcategory [category name] - add a category to the area's custom musiclist. [CM]\n"
                          "/removeentry [URL or category name] - remove an entry (category or song) from the area's custom musiclist. [CM]\n"
                          "/toggleroot - hide or show server's musiclist in the area. [CM]\n"
                          "/clearcustom - remove all from the area's custom musiclist. [CM]\n"
                          "/play_hub [musicname or URL] - play a music in all areas of the hub. [GM]\n"
                          "/play_once_hub [musicname or URL] - play a music in all areas of the hub once. [GM]\n"
                          "/playggl [file id] - play a music from Google Drive.***\n"
                          "/playggl_once [file id] - play a music from Google Drive once.***\n"
                          "/playggl_hub [file id] - play a music from Google Drive in all areas of the hub. [GM]***\n"
                          "/playggl_once_hub [file id] - play a music from Google Drive in all areas of the hub once. [GM]***\n"
                          "/play_ambience - play the requested track in the ambience channel.\n"
                          "*** - To get the file id, share the file with everyone and copy the link. File id is in the middle of the link - https://drive.google.com/file/d/[FILE ID]/view?usp=sharing");
    else if (argv[0] == "messaging")
        sendServerMessage("[brackets] mean [required arguments]. Actually, commands don't need these brackets.\n"
                          "/afk - show that you are AFK.\n"
                          "/g [message] - send a global message to all players.\n"
                          "/toggleglobal - toggle whether will ignore global messages or not.\n"
                          "/pm [uid] [message] - send a private message to another player.\n"
                          "/mutepm - toggle whether will recieve private messages or not.\n"
                          "/need [message] - send a global message expressing that the area needs something, such as players for something.\n"
                          "/toggleadverts - toggle whether the caller will receive /need advertisements.\n"
                          "/g_hub [message] - send a message in all areas of the hub.");
    else if (argv[0] == "char")
        sendServerMessage("[brackets] mean [required arguments]. Actually, commands don't need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in an area.\n"
                          "/charselect - enter the character select screen.\n"
                          "/forcepos [pos] [uid] - set the position another client in the area forcibly. Use * instead of UID if you need to set the position all clients in the area. [CM]\n"
                          "/randomchar - pick a new random character.\n"
                          "/switch [name] - switch to a different character.\n"
                          "/pos [position] - set the own position in the area.");
    else if (argv[0] == "roleplay")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actually, commands don't need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/8ball [text] - ask the magic ball!\n"
                          "/coinflip - flip a coin. The result is shown publicly.\n"
                          "/roll (XdY) - roll a die. The result is shown publicly. X is the number of dice, Y is the maximum value of the die.\n"
                          "/rollp (XdY) - roll a die privately. Same as /roll but the result is only shown to you and the CMs.\n"
                          "/subtheme [subtheme name] - change the subtheme of all clients in the current area. [CM]\n"
                          "/notecard [message] - write a note card in the current area.\n"
                          "/notecard_reveal - reveal all note cards in the current area. [CM]\n"
                          "/notecard_clear - сlear a note card.\n"
                          "/vote (uid) - without arguments - start or end voting (requires [CM] permission), with arguments - vote for the candidate.\n"
                          "/scoreboard - get a list of scores for invited clients in the area.\n"
                          "/addscore [uid] (scores) - add score(-s) to the client.\n"
                          "/takescore [uid] (scores) - take away score(-s) to the client.");
    else if (argv[0] == "testimony")
        sendServerMessage("[brackets] mean [required arguments]. Actually, commands don't need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "Testimony are only recorded from players who have chosen the [wit] position.\n"
                          "Guide to use Testimony bot: https://github.com/Ddedinya/kakashi/wiki/(ENG)-Testimony-bot \n"
                          "/add - adds a statement to the recorded testimony. [CM]\n"
                          "/delete - deletes the currently displayed statement from the recorded testimony. [CM]\n"
                          "/examine - playback recorded testimony. [CM]\n"
                          "/loadtestimony [testimony name] - loads testimony from the server's storage. [CM]\n"
                          "/savetestimony [testimony name] - saves a testimony recording to the server's storage. [SAVETEST]\n"
                          "/testify - enable the testimony recording. [CM]\n"
                          "/testimony - display the currently recorded testimony.\n"
                          "/pause - pause testimony playback/recording. [CM]");
    else if (argv[0] == "casing")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actually, commands don't need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/evidence_mod [mod] - changes the evidence mod in the area.[CM]\n"
                          "/currentevimod - get the current evidence mode. [CM]\n"
                          "/evidence_swap [evidence id] [evidence id] - swap the positions of two evidence items on the evidence list. [CM]");
    else if (argv[0] == "area")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actually, commands don't need these brackets. "
                          "If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n"
                          "/area [area id] - go to another area.\n"
                          "/cm (uid) - promote yourself to CM status in the area. If UID is provided, the client will promoted to CM status. The number of CM is unlimited.\n"
                          "/uncm (uid) - remove CM status from yourself. If user with [UNCM] permission access mentions player UID his CM status will be removed too. [CM]\n"
                          "/area_kick [id] - kick the client from the area. [CM]\n"
                          "/area_lock - lock the area. It's open only for the invited players. [CM]\n"
                          "/area_unlock - allow anyone to freely join/speak the current area. [CM]\n"
                          "/area_spectate - mute the area, make every player who is not invited forbidden to write in IC chat. Invites all players who are in the area automatically. [CM]\n"
                          "/area_mute - same as /area_spectate, but doesn't invite players which were in area. [CM]\n"
                          "/allowiniswap - toggles whether iniswaps are allowed or not in the current area. [CM]\n"
                          "/bg [background name] - change a background.\n"
                          "/bgs - get a list of backgrounds.\n"
                          "/forceimmediate - toggles immediate text processing is enabled or not in the current area. [CM]\n"
                          "/cleardoc - clear the link for the current case document.\n"
                          "/currentbg - get the name of the current background in the area.\n"
                          "/doc (url) - get or change the link for the current case document.\n"
                          "/getarea - get information about the current area.\n"
                          "/getareas - get information about all areas in the hub.\n"
                          "/getareahubs - get information about all areas.\n"
                          "/judgelog - get list the last 10 uses of judge controls in the current area. [CM]\n"
                          "/invite [uid] - invite the client into the area, letting them access to write in IC if the area is locked/muted. [CM]\n"
                          "/uninvite [uid] - univite the client from area, forbidding them to write in IC if the area is locked/muted. [CM]\n"
                          "/status [idle/rp/casing/looking-for-players/lfp/recess/gaming/erp/yablachki] - change the current status of the area.\n"
                          "/togglechillmod - enable/disable Chill Mod in the area. [CM]\n"
                          "/toggleautomod - enable/disable Auto Mod in the area. [CM]\n"
                          "/togglefloodguard - enable/disable floodguard in the area. [CM]\n"
                          "/togglemessage - toggle whether the client shows the area message when joining the current area. [CM]\n"
                          "/clearmessage - clear the area's message and disable automatic sending. [CM]\n"
                          "/areamessage (message) - get the area message in OOC. If text is entered, it updates the current area message. [CM]\n"
                          "/bglock - lock the background of the current area, preventing it from being changed. [CM]\n"
                          "/bgunlock - unlock the background of the current area, allowing it to be changed. [CM]\n"
                          "/togglewtce - toggle whether the WTCE buttons can be used or not in the area. [CM]\n"
                          "/toggleshouts - toggle whether shouts can be used or not in the area. [CM]\n"
                          "/togglestatus - toggle whether status can be changed or not in the area. [CM]\n"
                          "/ooc_type [all/invited/cm] - allow everyone to speak in OOC chat, only invited clients or only CMs. [CM]\n"
                          "/toggleautocap - toggle whether all messages will start with a capital letter and end with a period automatically or not in the area. [CM]");
    else if (argv[0] == "areaedit")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actually, commands don't need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "/renamearea [new name] - rename the current area. [GM]\n"
                          "/createarea [name] - create a new area. [GM]\n"
                          "/removearea [area id] - delete selected area. [GM]\n"
                          "/swapareas [area id] [area id] - swap selected areas. [GM]\n"
                          "/toggleprotected - toggle whether it is possible in the current area to become CM or not. [GM]\n"
                          "/saveareas [file name] - save the area config file. [SAVEAREAS]");
    else if (argv[0] == "hubs")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actually, commands don't need these brackets. "
                          "The commands presented here require [GM] permission.\n"
                          "/hub (hub id) - view the list of hubs or go to another hub.\n"
                          "/gm - become a GM of hub.\n"
                          "/ungm - unbecome a GM of hub.\n"
                          "/hub_hideplayercount - show/hide the count of players in the hub and its areas.\n"
                          "/hub_rename [new name] - rename the hub.\n"
                          "/hub_listening - receive messages from all areas in the hub.\n"
                          "/hub_spectate - mute the hub, prohibit uninvited players from writing to IC chat in all areas of the hub.\n"
                          "/hub_lock - lock the hub. Only invited players can enter it.\n"
                          "/hub_unlock - unlock/unmute the hub.\n"
                          "/hub_invite - invite the specified client in hub.\n"
                          "/hub_uninvite - uninvite the specified client in hub.");
    else if (argv[0] == "admin")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actually, commands don't need these brackets. "
                          "The prefix after the command description means permission to use that command.\n"
                          "/allowblankposting - allow/deny blankposting in the area. [MODCHAT]\n"
                          "/ban [ipid] [time (examples - 1h30m, 1h, 30m, 30s)] [reason] - ban the client. [BAN]\n"
                          "/unban [ban id] - unban the client. [BAN]\n"
                          "/baninfo [ban id] - get information about the ban. [BAN]\n"
                          "/bans - get information about the last 5 bans. [BAN]\n"
                          "/blockdj [uid] - restrict the client from changing music. [MUTE]\n"
                          "/unblockdj [uid] - restore the client's ability to change music. [MUTE]\n"
                          "/blockwtce [uid] - forbid the client to use WTCE buttons. [MUTE]\n"
                          "/unblockwtce [uid] - allow the client to use WTCE buttons. [MUTE]\n"
                          "/blind [uid] - blind the client, makes them unable to see any messages in IC/OOC chat. [MUTE]\n"
                          "/unblind - allow the client to see messages in IC/OOC chat. [MUTE]\n"
                          "/charcurse [uid] (character1,character2,...) - restrict the client to only being able to switch between a set of characters. [MUTE]\n"
                          "/uncharcurse [uid] - allow the client to switch characters normally. [MUTE]\n"
                          "/clearcm - remove all CMs from the current area. [KICK]\n"
                          "/disemvowel [uid] - remove all vowels from the client's IC messages. [MUTE]\n"
                          "/undisemvowel [uid] - bring back all client's vowels. [MUTE]\n"
                          "/gimp [uid] - replace client's IC messages with strings randomly selected from gimp.txt. [MUTE]\n"
                          "/ungimp [uid] - bring back all client's IC messages. [MUTE]\n"
                          "/ipidinfo [ipid] - get some information about the IPID. [IPIDINFO]\n"
                          "/taketaken - allow or deny yourself take taked characters. [TAKETAKEN]\n"
                          "/notice [message] - send all clients in the area a message in a special window. [SEND_NOTICE]\n"
                          "/noticeg [message] - send all clients a message in a special window. [SEND_NOTICE]\n"
                          "/shake [uid] - shuffle the order of words in the client's IC messages. [MUTE]\n"
                          "/unshake [uid] - bring back the client's order of words in IC messages.\n"
                          "/kick [ipid] [reason] - kick the client [KICK].\n"
                          "/login [username (if auth is advanced)] [password] - login as a moderator.\n"
                          "/unmod - log out as moderator.\n"
                          "/m [message] - send a message to moderator-only chat. [MODCHAT]\n"
                          "/modarea_kick [uid] [area id] - kick the player into the specified area. [KICK]\n"
                          "/mute [uid] - mute the client. [MUTE]\n"
                          "/unmute [uid] - unmute the client. [MUTE]\n"
                          "/ooc_mute [uid] - mute the client in OOC. [MUTE]\n"
                          "/ooc_unmute [uid] - unmute the client in OOC. [MUTE]\n"
                          "/permitsaving [uid] - allow the client to save 1 testimony with /savetestimony. [MODCHAT]\n"
                          "/updateban [ban id] [duration/reason] [updated info] - update the ban with the specified ban ID. [BAN]\n"
                          "/removewuso [uid] - remove the WUSO action on the player. [WUSO]\n"
                          "/togglewuso - enable or disable WUSO. [WUSO]\n"
                          "/hub_protected - enable or disable the ability to become a GM in the hub. [MODCHAT]");
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
            emit logCMD((character() + " " + characterName()), m_ipid, name(), "CHANGEMOTD", l_MOTD, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
        }
        else
            sendServerMessage("You do not have permission to change the MOTD");
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
    else if (server->getDatabaseManager()->invalidateBan(l_target_ban))
        sendServerMessage("Successfully invalidated ban " + argv[0] + ".");
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
                      "\n Based on akashi jackfruit (Pre-release). "
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

    if (l_target->clientId() == clientId()) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_is_muted)
        sendServerMessage("That player is already muted!");
    else
        sendServerMessage("Muted player.");

    l_target->m_is_muted = true;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "MUTE", "Muted UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "UNMUTE", "Unmuted UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    if (l_target->clientId() == clientId()) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_is_ooc_muted)
        sendServerMessage("That player is already OOC muted!");
    else
        sendServerMessage("OOC muted player.");

    l_target->m_is_ooc_muted = true;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "OOCMUTE", "OOC muted UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "OOCUNMUTE", "OOC unmuted UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    if (l_target->clientId() == clientId()) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_is_wtce_blocked)
        sendServerMessage("That player is already judge blocked!");
    else
        sendServerMessage("Revoked player's access to judge controls.");

    l_target->m_is_wtce_blocked = true;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "BLOCKWTCE", "Blocked UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "UNBLOCKWTCE", "Unblocked UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdAllowBlankposting(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleBlankposting();
    QString l_state = l_area->forceImmediate() ? "allowed." : "forbidden.";
    sendServerMessage("[" + QString::number(clientId()) + "] " + getSenderName(clientId()) + " has set blankposting in the area to " + l_state);
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "TOGGLEBLANKPOST", "", server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    if (server->reload_watcher.isRunning()) {
        sendServerMessage("Another reloading configurations is still not finished.");
        return;
    }
    server->reload_watcher.setFuture(QtConcurrent::run(&Server::reloadSettings, server, clientId()));
    sendServerMessage("Reloading configurations...");
    server->handleDiscordIntegration(); // It's here to avoid uptime webhook breaking
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "RELOAD", "", server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdForceImmediate(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleImmediate();
    QString l_state = l_area->forceImmediate() ? "on." : "off.";
    sendServerMessage("Forced immediate text processing in this area is now " + l_state);
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "FORCEIMMEDIATE", l_state, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdAllowIniswap(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleIniswap();
    QString l_state = l_area->iniswapAllowed() ? "allowed." : "disallowed.";
    sendServerMessage("Iniswapping in this area is now " + l_state);
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "ALLOWINISWAP", l_state, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "ALLOWTESTIMONYSAVING", "Target UID: " + QString::number(l_client->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

        if (l_target->clientId() == clientId()) {
            sendServerMessage("Nope.");
            return;
        }

        l_target->sendPacket("KK", {l_reason});
        l_target->m_socket->close();
        sendServerMessage("Kicked client with UID " + argv[0] + " for a reason: " + l_reason);
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "KICKUID", "Kicked UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
    }
    else if (argv[0] == "*") { // kick all clients in the area
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "AREAKICK", "Kicked all players in the area", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients)
            if (l_client->areaId() == areaId() && l_client->clientId() != clientId()) {
                l_client->sendPacket("KK", {l_reason});
                l_client->m_socket->close();
            }

        sendServerMessage("Kicked all clients for a reason: " + l_reason);
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "KICKUID", "Kicked all clients in area from the server", server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    sendServerMessage("The ban has been updated.");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "UPDATEBAN", "Update info: " + argv[1] + " " + argv[2], server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    AreaData *l_area = server->getAreaById(areaId());
    foreach (int l_client_id, l_area->owners())
        l_area->removeOwner(l_client_id);

    arup(ARUPType::CM, true, hubId());
    sendServerMessage("Removed all CMs from this area.");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "CLEARCM", "", server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
    for (AOClient *l_target_candidate : std::as_const(l_targets_hwid))
        if (!l_target_clients.contains(l_target_candidate))
            l_target_clients.append(l_target_candidate);

    // The list is unique, we can only have on instance of the current client.
    l_target_clients.removeOne(this);
    for (AOClient *l_target_client : std::as_const(l_target_clients)) {
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
        l_ipid_info << "Last HWID: " + l_ipidip.hwid;
        l_ipid_info << "-----";
    }

    sendServerMessage(l_ipid_info.join("\n"));
}

void AOClient::cmdTakeTakenChar(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_take_taked_char = !m_take_taked_char;
    QString l_str_en = m_take_taked_char ? "now may take taken characters" : "no longer may take taken characters";
    sendServerMessage("You are " + l_str_en + ".");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "TAKETAKENCHAR", l_str_en, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    if (l_target->clientId() == clientId()) {
        sendServerMessage("Nope.");
        return;
    }

    if (l_target->m_blinded)
        sendServerMessage("That player is already blinded!");
    else
        sendServerMessage("That player has been blinded.");

    l_target->m_blinded = true;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "BLIND", "Blinded UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
        sendServerMessage("That player has been unblinded.");
        l_target->sendServerMessage("A moderator has unblinded you. ");
    }

    l_target->m_blinded = false;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "UNBLIND", "Unblinded UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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

    emit logCMD((character() + " " + characterName()), m_ipid, name(), "TOGGLEWUSO", l_str_en, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
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
        sendServerMessage("That player is not affected by WUSO.");
    else {
        sendServerMessage("Done.");
        l_target->sendServerMessage("Now the restrictions of the WUSO do not apply to you.");
    }

    l_target->m_wuso = false;
    l_target->m_usemodcall = true;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "REMOVEWUSO", "Target UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdKickPhantoms(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        if (l_client->m_ipid == m_ipid && l_client->clientId() != clientId()) {
            l_client->sendPacket("KK", {"Phantom client."});
            l_client->m_socket->close();
        }

    sendServerMessage("Kicked your phantom client(-s).");
}
