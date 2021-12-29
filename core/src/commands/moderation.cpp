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

// This file is for commands under the moderation category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdBan(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString args_str = argv[2];

    if (argv[0] == ipid) {
        sendServerMessage("Nope.");
        return;
    }

    if (argc > 3) {
        for (int i = 3; i < argc; i++)
            args_str += " " + argv[i];
    }

    DBManager::BanInfo ban;

    long long duration_seconds = 0;

    if (argv[1] == "perma")
        duration_seconds = -2;
    else
        duration_seconds = parseTime(argv[1]);

    if (duration_seconds == -1) {
        sendServerMessage("Invalid time format. Format example: 1h30m");
        return;
    }

    ban.duration = duration_seconds;
    ban.ipid = argv[0];
    ban.reason = args_str;
    ban.moderator = ipid;
    ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
    bool ban_logged = false;
    int kick_counter = 0;

    for (AOClient* client : server->getClientsByIpid(ban.ipid)) {
        if (!ban_logged) {
            ban.ip = client->remote_ip;
            ban.hdid = client->hwid;
            server->db_manager->addBan(ban);
            sendServerMessage("Banned user with ipid " + ban.ipid + " for reason: " + ban.reason);
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
        sendServerMessage("Ban ID: " + QString::number(ban_id));
        client->sendPacket("KB", {ban.reason + "\nID: " + QString::number(ban_id) + "\nUntil: " + ban_duration});
        client->socket->close();
        kick_counter++;

        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(ban.ipid, ban.moderator, ban_duration, ban.reason, ban_id);

        area->logCmdAdvanced(current_char, ipid, hwid, "BAN", "Ban ID: " + QString::number(ban_id), showname, ooc_name, QString::number(id));
    }

    if (kick_counter > 1)
        sendServerMessage("Kicked " + QString::number(kick_counter) + " clients with matching ipids.");

    // We're banning someone not connected.
    if (!ban_logged) {
        ban.ip = remote_ip;
        server->db_manager->addBan(ban);     
        QString ban_duration;
        int ban_id = server->db_manager->getBanID(ban.ip);

        sendServerMessage("Banned " + ban.ipid + " for reason: " + ban.reason);
        sendServerMessage("Ban ID: " + QString::number(ban_id));

        if (!(ban.duration == -2)) {
            ban_duration = QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            ban_duration = "The heat death of the universe.";
        } 

        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(ban.ipid + " (Сlient was not connected to the server)", ban.moderator, ban_duration, ban.reason, ban_id);

        area->logCmdAdvanced(current_char, ipid, hwid, "BAN", "Ban ID: " + QString::number(ban_id), showname, ooc_name, QString::number(id));
    }
}

void AOClient::cmdKick(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString target_ipid = argv[0];
    QString reason = argv[1];
    int kick_counter = 0;

    if (target_ipid == ipid) {
        sendServerMessage("Nope.");
        return;
    }

    if (argc > 2) {
        for (int i = 2; i < argv.length(); i++) {
            reason += " " + argv[i];
        }
    }

    for (AOClient* client : server->getClientsByIpid(target_ipid)) {
        client->sendPacket("KK", {reason});
        client->socket->close();
        kick_counter++;
    }

    if (kick_counter > 0) {
        sendServerMessage("Kicked " + QString::number(kick_counter) + " client(s) with ipid " + target_ipid + " for reason: " + reason);
        area->logCmdAdvanced(current_char, ipid, hwid, "KICK", "Kicked IPID: " + target_ipid + ". Reason: " + reason, showname, ooc_name, QString::number(id));
    }
    else
        sendServerMessage("User with ipid not found!");
}

void AOClient::cmdMods(int argc, QStringList argv)
{
    QStringList entries;
    int online_count = 0;

    for (AOClient* client : server->clients) {
        if (client->authenticated && !client->slient_mod) {
            entries << "---";

            if (ConfigManager::authType() != DataTypes::AuthType::SIMPLE)
                entries << "Moderator: " + client->moderator_name;

            entries << "OOC name: " + client->ooc_name;
            entries << "ID: " + QString::number(client->id);
            entries << "Area: " + QString::number(client->current_area);
            entries << "Character: " + client->current_char;
            online_count++;
        }
    }

    entries << "---";
    entries << "Total online: " << QString::number(online_count);
    sendServerMessage(entries.join("\n"));
}

void AOClient::cmdHelp(int argc, QStringList argv)
{
    if (argc == 0) {
        sendServerMessage("/help area - commands relate to area management.\n/help casing - commands relate to casing.\n/help testimony - testimony recording commands.\n/help roleplay - commands are related to various kinds of roleplay actions.\n/help char - commands related to characters.\n/help messaging - commands related to messaging.\n/help music - commands related to music.\n/help other - others commands.");
        return;
    }

    if (argv[0] == "other")
        sendServerMessage("/about - gives a very brief description of kakashi.\n/mods - lists the currently logged-in moderators on the server.\n/motd - gets the server's Message Of The Day.\n/sneak - begin sneaking a.k.a. hide your area moving messages from the OOC.");
    else if (argv[0] == "music")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n/currentmusic - show the current music playing.\n/play [http://direct.link/to/music.mp3] - play a track.\n/play_once [http://direct.link/to/music.mp3] - play a track once.\n/togglemusic - enable/disable change music in area. [CM]");
    else if (argv[0] == "messaging")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets.\n/afk - sets your player as AFK in player listings.\n/g [message] - sends a global message (i.e., all clients in the server will be able to see it).\n/toggleglobal - toggles whether  will ignore  global messages or not.\n/pm [id] [message] - send a private message to another online user.\n/mutepm - toggles whether will recieve private messages or not.\n/need [message] - sends a global message expressing that the area needs something, such as players for something.");
    else if (argv[0] == "char")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets.\n/charselect - enter the character select screen.\n/forcepos [pos] [id] - Set the place another character resides in the area.\n/randomchar - picks a new random character.\n/switch [name] - Switches to a different character.\n/pos [position] - set the place your character resides in the area.");
    else if (argv[0] == "roleplay")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n/8ball [text] - answers a question.\n/coinflip - flip a coin.\n/roll (value/X Y) - roll a die. The result is shown publicly. X is the number of dice, Y is the maximum value on the die.\n/rollp (value/X Y) - roll a die privately. Same as /roll but the result is only shown to you and the CMs.\n/subtheme [subtheme name] - changes the subtheme of all clients in the current area. [CM]\n/notecard [message] - writes a note card in the current area. The note card is not readable until all note cards in the area are revealed by a CM. A message will appear to all clients in the area indicating that a note card has been written.\n/notecard_reveal - reveals all note cards in the current area. [CM]\n/notecard_clear - сlears a client's note card. A message will appear to all clients in the area indicating that a note card has been cleared.");
    else if (argv[0] == "testimony")
        sendServerMessage("[brackets] mean [required arguments]. Actual commands do not need these brackets. If there is a [CM] prefix after the command description, then to use the command you need to be CM in area, and if there is a [MOD] prefix, then you need to be a moderator to use the command. Testimony are only recorded from players who have chosen the wit position.\n/add - adds a statement to an existing testimony. [CM]\n/delete [id statement] - deletes a statement from the testimony. [CM]\n/examine - playback recorded testimony. [CM]\n/loadtestimony [testimony name] - loads testimony for the testimony replay. [CM]\n/savetestimony [testimony name] - saves a testimony recording to the servers storage. [MOD]\n/testify - enables the testimony recording. [CM]\n/testimony - display the currently recorded testimony.\n/pause - pauses testimony playback/recording. [CM]");
    else if (argv[0] == "casing")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n/cm (id) - add a case manager for the current area. Leave id blank to promote yourself if there are no CMs.\n/uncm - remove a case manager from the current area. Leave id blank to demote yourself. [CM]\n/evidence_mod [mod] - changes the evidence mod in an area.[CM]\n/currentevimod - find out current evidence mod.[CM]\n/evidence_swap [evidence id] [evidence id] - Swap the positions of two evidence items on the evidence list. The ID of each evidence can be displayed by mousing over it in 2.8 client, or simply its number starting from 1. [CM]\n");
    else if (argv[0] == "area")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. If there is a [CM] prefix after the command description, then to use the command you need to be CM in area.\n/area [area id] - go to another area.\n/area_kick [id] - remove a user from the current area and move them to another area. [CM]\n/area_lock - prevent users from joining the current area unlesss /invite is used. [CM]\n/area_unlock - allow anyone to freely join/speak the current area. [CM]\n/area_spectate - sets the current area to spectatable, where anyone may join but only invited users may communicate in-character. Anyone currently in the area is considered invited. [CM]\n/area_mute - Makes this area impossible to speak for normal users unlesss /invite is used. [CM]\n/allowiniswap - toggles whether iniswaps are allowed in the current area. [CM]\n/bg [background name] - change background.\n/bgs - get a list of backgrounds.\n/forceimmediate - toggles immediate text processing in the current area. [CM]\n/cleardoc - clear the link for the current case document.\n/currentbg - find out the name of the current background in the area.\n/doc (url) - show or change the link for the current case document.\n/getarea - show information about the current area.\n/getareas - show information about all areas.\n/judgelog - list the last 10 uses of judge controls in the current area. [CM]\n/invite [id] - allow a particular user to join a locked or speak in spectator-only area. [CM]\n/uninvite [id] - revoke an invitation for a particular user. [CM]\n/status [idle/rp/casing/looking-for-players/lfp/recess/gaming/erp/yablachki] - modify the current status of an area.\n/togglechillmod - enable/disable Chill Mod in area. [CM]\n/toogleautomod - enable/disable Auto Mod in area. [CM]\n/togglefloodguard - enable/disable floodguard in area. [CM]");
    else if (argv[0] == "admin")
        sendServerMessage("[brackets] mean [required arguments], (brackets) mean (optional arguments). Actual commands do not need these brackets. The prefix after the command description means permission to use that command.\n/allowblankposting - allow/deny blankposting in the area. [MODCHAT]\n/ban [ipid] [time (examples - 1h30m, 1h, 30m, 30s)] [reason] - ban player. [BAN]\n/unban [ban id] - unban player. [BAN]\n/baninfo [ban id] - get information about the ban. [BAN]\n/bans - get information about the last 5 bans. [BAN]\n/bglock - locks the background of the current area, preventing it from being changed. [BGLOCK]\n/bgunlock - unlocks the background of the current area, allowing it to be changed. [BGLOCK]\n/blockdj [uid] - restricts a target client from changing music. [MUTE]\n/unblockdj [uid] - restores a client's DJ controls. [MUTE]\n/blockwtce [uid] - revokes judge controls from a client, preventing them from using WT/CE buttons or updating penalties. [MUTE]\n/unblockwtce [uid] - Grants judge controls back to a client. [MUTE]\n/blind [uid] - blind the targeted player from being able to see or talk IC/OOC. [MUTE]\n/unblind - undo effects of the /blind command. [MUTE]\n/charcurse [uid] (character1,character2,...) - restricts a target client into only being able to switch between a set of characters. If no characters are given, the client is locked to only their current character. Characters must be given in a comma-separated list. [MUTE]\n/uncharcurse [uid] - uncharcurses a target client, allowing them to switch normally again. [MUTE]\n/disemvowel [uid] - removes all vowels from a target client's IC messages. [MUTE]\n/undisemvowel [uid] - undisemvowels a client. [MUTE]\n/gimp [uid] - replaces a target client's IC messages with strings randomly selected from gimp.txt. [MUTE]\n/ungimp [uid] - ungimps a client. [MUTE]\n/ipidinfo [ipid] - find out the ip address and date of ipid creation. [IPIDINFO]\n/taketaked - allow/deny yourself take taked characters. [TAKETAKED]\n/notice [message] - send all players in area a message in a special window. [SEND_NOTICE]\n/noticeg [message] - send all players a message in a special window. [SEND_NOTICE]\n/shake [uid] - shuffles the order of words in a target client's IC messages. [MUTE]\n/unshake [uid] - unshakes a client.\n/kick [ipid] [reason] - kick player [KICK].\n/login [username] [password] - enters the login prompt to login as a moderator.\n/unmod - logs a user out as a moderator.\n/m [message] - sends a message to the server-wide, moderator-only chat. [MODCHAT]\n/modarea_kick [uid] [area id] - kick the player into the specified area. [KICK]\n/mute [uid] - mute player. [MUTE]\n/unmute [uid] - unmute player. [MUTE]\n/ooc_mute [uid] - mute player in OOC. [MUTE]\n/ooc_unmute [uid] - unmute player in OOC. [MUTE]\n/permitsaving [uid] - allows the target client to save 1 testimony with /savetestimony. [MODCHAT]\n/updateban [ban id] [duration/reason] [updated info] - updates the ban with the specified ban ID. [BAN]\n/removewuso [uid] - remove the WUSO action on the player. [WUSO]\n/togglewuso - allow/deny WUSO. [WUSO]");
    else
        sendServerMessage("/help area - commands relate to area management.\n/help casing - commands relate to casing.\n/help testimony - testimony recording commands.\n/help roleplay - commands are related to various kinds of roleplay actions.\n/help char - commands related to characters.\n/help_messaging - commands related to messaging.\n/help music - commands related to music.\n/help other - others commands.");
}

void AOClient::cmdMOTD(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (argc == 0) {
        sendServerMessage("=== MOTD ===\r\n" + ConfigManager::motd() + "\r\n=============");
    }

    else if (argc > 0) {
        if (checkAuth(ACLFlags.value("MOTD"))) {
            QString MOTD = argv.join(" ");

            ConfigManager::setMotd(MOTD);
            sendServerMessage("MOTD has been changed.");
            area->logCmdAdvanced(current_char, ipid, hwid, "CHANGE MOTD", MOTD, showname, ooc_name, QString::number(id));
        }
        else {
            sendServerMessage("You do not have permission to change the MOTD");
        }
    }
}

void AOClient::cmdBans(int argc, QStringList argv)
{
    QStringList recent_bans;
    recent_bans << "Last 5 bans:";
    recent_bans << "-----";

    for (DBManager::BanInfo ban : server->db_manager->getRecentBans()) {
        QString banned_until;

        if (ban.duration == -2)
            banned_until = "The heat death of the universe";
        else
            banned_until = QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toString("dd/MM/yyyy, hh:mm");
        recent_bans << "Ban ID: " + QString::number(ban.id);
        recent_bans << "Affected IPID: " + ban.ipid;
        recent_bans << "Affected HDID: " + ban.hdid;
        recent_bans << "Reason for ban: " + ban.reason;
        recent_bans << "Date of ban: " + QDateTime::fromSecsSinceEpoch(ban.time).toString("dd/MM/yyyy, hh:mm");
        recent_bans << "Ban lasts until: " + banned_until;
        recent_bans << "Moderator: " + ban.moderator;
        recent_bans << "-----";
    }

    sendServerMessage(recent_bans.join("\n"));
}

void AOClient::cmdUnBan(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool ok;
    int target_ban = argv[0].toInt(&ok);

    if (!ok) {
        sendServerMessage("Invalid ban ID.");
        return;
    }
    else if (server->db_manager->invalidateBan(target_ban)) {
        sendServerMessage("Successfully invalidated ban " + argv[0] + ".");
        area->logCmdAdvanced(current_char, ipid, hwid, "UNBAN", "Ban ID: " + QString::number(target_ban), showname, ooc_name, QString::number(id));
    }
    else
        sendServerMessage("Couldn't invalidate ban " + argv[0] + ", are you sure it exists?");
}

void AOClient::cmdAbout(int argc, QStringList argv)
{
    sendServerMessage("akashi by: \n scatterflower \n Salanto \n in1tiate \n mangosarentliterature \n Github: https://github.com/AttorneyOnline/akashi \n kakashi by Dedinsaid228. \n Based on akashi coconut. \n Github: https://github.com/Dedinsaid228/kakashi");
}

void AOClient::cmdMute(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (target->id == id) {
        sendServerMessage("Nope.");
        return;
    }

    if (target->is_muted)
        sendServerMessage("That player is already muted!");
    else {
        sendServerMessage("Muted player.");
        area->logCmdAdvanced(current_char, ipid, hwid, "MUTE", "Muted UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->is_muted = true;
}

void AOClient::cmdUnMute(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!target->is_muted)
        sendServerMessage("That player is not muted!");
    else {
        sendServerMessage("Unmuted player.");
        target->sendServerMessage("You were unmuted by a moderator. ");
        area->logCmdAdvanced(current_char, ipid, hwid, "UNMUTE", "Unmuted UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->is_muted = false;
}

void AOClient::cmdOocMute(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (target->id == id) {
        sendServerMessage("Nope.");
        return;
    }

    if (target->is_ooc_muted)
        sendServerMessage("That player is already OOC muted!");
    else {
        sendServerMessage("OOC muted player.");
        area->logCmdAdvanced(current_char, ipid, hwid, "OOCMUTE", "OOC muted UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->is_ooc_muted = true;
}

void AOClient::cmdOocUnMute(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!target->is_ooc_muted)
        sendServerMessage("That player is not OOC muted!");
    else {
        sendServerMessage("OOC unmuted player.");
        target->sendServerMessage("You were OOC unmuted by a moderator. ");
        area->logCmdAdvanced(current_char, ipid, hwid, "OOCUNMUTE", "OOC unmuted UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->is_ooc_muted = false;
}

void AOClient::cmdBlockWtce(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (target->id == id) {
        sendServerMessage("Nope.");
        return;
    }

    if (target->is_wtce_blocked)
        sendServerMessage("That player is already judge blocked!");
    else {
        sendServerMessage("Revoked player's access to judge controls.");
    }

    target->is_wtce_blocked = true;
    area->logCmdAdvanced(current_char, ipid, hwid, "BLOCKWTCE", "WTCE blocked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUnBlockWtce(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!target->is_wtce_blocked)
        sendServerMessage("That player is not judge blocked!");
    else {
        sendServerMessage("Restored player's access to judge controls.");
        target->sendServerMessage("A moderator restored your judge controls access. ");
    }

    target->is_wtce_blocked = false;
    area->logCmdAdvanced(current_char, ipid, hwid, "UNBLOCKWTCE", "WTCE unblocked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdAllowBlankposting(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    area->toggleBlankposting();

    if (area->blankpostingAllowed() == false) {
        if (showname.isEmpty() && current_char.isEmpty())
            sendServerMessageArea("[" + QString::number(id) + "] " + "Spectator has set blankposting in the area to forbidden.");
        else if (showname.isEmpty())
            sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " has set blankposting in the area to forbidden.");
        else
            sendServerMessageArea("[" + QString::number(id) + "] " + showname + " has set blankposting in the area to forbidden.");

        area->logCmdAdvanced(current_char, ipid, hwid, "BLANKPOST", "Forbidden", showname, ooc_name, QString::number(id));
    }
    else {
        if (showname.isEmpty() && current_char.isEmpty())
            sendServerMessageArea("[" + QString::number(id) + "] " + "Spectator has set blankposting in the area to allowed.");
        else if (showname.isEmpty())
            sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " has set blankposting in the area to allowed.");
        else
            sendServerMessageArea("[" + QString::number(id) + "] " + showname + " has set blankposting in the area to allowed.");

        area->logCmdAdvanced(current_char, ipid, hwid, "BLANKPOST", "Allowed", showname, ooc_name, QString::number(id));
    }
}

void AOClient::cmdBanInfo(int argc, QStringList argv)
{
    QStringList ban_info;
    ban_info << ("Ban Info for " + argv[0]);
    ban_info << "-----";
    QString lookup_type;

    if (argc == 1) {
       lookup_type = "banid";
    }
    else if (argc == 2) {
        lookup_type = argv[1];
        if (!((lookup_type == "banid") || (lookup_type == "ipid") || (lookup_type == "hdid"))) {
            sendServerMessage("Invalid ID type.");
            return;
        }
    }
    else {
        sendServerMessage("Invalid command.");
        return;
    }

    QString id = argv[0];

    for (DBManager::BanInfo ban : server->db_manager->getBanInfo(lookup_type, id)) {
        QString banned_until;
        if (ban.duration == -2)
            banned_until = "The heat death of the universe";
        else
            banned_until = QDateTime::fromSecsSinceEpoch(ban.time).addSecs(ban.duration).toString("dd/MM/yyyy, hh:mm");

        ban_info << "Ban ID: " + QString::number(ban.id);
        ban_info << "Affected IPID: " + ban.ipid;
        ban_info << "Affected HDID: " + ban.hdid;
        ban_info << "Reason for ban: " + ban.reason;
        ban_info << "Date of ban: " + QDateTime::fromSecsSinceEpoch(ban.time).toString("dd/MM/yyyy, hh:mm");
        ban_info << "Ban lasts until: " + banned_until;
        ban_info << "Moderator: " + ban.moderator;
        ban_info << "-----";
    }

    sendServerMessage(ban_info.join("\n"));
}

void AOClient::cmdReload(int argc, QStringList argv)
{
    ConfigManager::reloadSettings();
    server->resizeUIDs();
    emit server->reloadRequest(ConfigManager::serverName(), ConfigManager::serverDescription());
    server->updateHTTPAdvertiserConfig();
    server->m_ipban_list = ConfigManager::iprangeBans();
    sendServerMessage("Reloaded configurations");
}

void AOClient::cmdForceImmediate(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    area->toggleImmediate();
    QString state = area->forceImmediate() ? "on." : "off.";

    sendServerMessage("Forced immediate text processing in this area is now " + state);
    area->logCmdAdvanced(current_char, ipid, hwid, "FORCE IMMEDIATE", state, showname, ooc_name, QString::number(id));
}

void AOClient::cmdAllowIniswap(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    area->toggleIniswap();
    QString state = area->iniswapAllowed() ? "allowed." : "disallowed.";

    sendServerMessage("Iniswapping in this area is now " + state);
    area->logCmdAdvanced(current_char, ipid, hwid, "INISWAP", state, showname, ooc_name, QString::number(id));
}

void AOClient::cmdPermitSaving(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    AOClient* client = server->getClientByID(argv[0].toInt());

    if (client == nullptr) {
        sendServerMessage("Invalid ID.");
        return;
    }

    client->testimony_saving = true;
    area->logCmdAdvanced(current_char, ipid, hwid, "ALLOW PERMIT SAVING", "UID: " + QString::number(client->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdKickUid(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString reason = argv[1];

    if (argc > 2) {
        for (int i = 2; i < argv.length(); i++) {
            reason += " " + argv[i];
        }
    }

    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);
    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (target->id == id) {
        sendServerMessage("Nope.");
        return;
    }

    target->sendPacket("KK", {reason});
    target->socket->close();
    sendServerMessage("Kicked client with UID " + argv[0] + " for reason: " + reason);
    area->logCmdAdvanced(current_char, ipid, hwid, "KICKUID", "Kicked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUpdateBan(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int ban_id = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid ban ID.");
        return;
    }

    QVariant updated_info;

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

        updated_info = QVariant(duration_seconds);

    }
    else if (argv[1] == "reason") {
        QString args_str = argv[2];

        if (argc > 3) {
            for (int i = 3; i < argc; i++)
                args_str += " " + argv[i];
        }

        updated_info = QVariant(args_str);
    }
    else {
        sendServerMessage("Invalid update type.");
        return;
    }
    if (!server->db_manager->updateBan(ban_id, argv[1], updated_info)) {
        sendServerMessage("There was an error updating the ban. Please confirm the ban ID is valid.");
        return;
    }

    sendServerMessage("Ban updated.");
    area->logCmdAdvanced(current_char, ipid, hwid, "UPDATE BAN", "Updated ban: " + QString::number(ban_id) + ". Updated info: " + argv[1] + " " + updated_info.toString(), showname, ooc_name, QString::number(id));
}

void AOClient::cmdNotice(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    sendNotice(argv.join(" "));
    area->logCmdAdvanced(current_char, ipid, hwid, "NOTICE", argv.join(" "), showname, ooc_name, QString::number(id));
}

void AOClient::cmdNoticeGlobal(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    sendNotice(argv.join(" "), true);
    area->logCmdAdvanced(current_char, ipid, hwid, "GLOBAL NOTICE", argv.join(" "), showname, ooc_name, QString::number(id));
}

void AOClient::cmdIpidInfo(int argc, QStringList argv)
{
    QStringList ipid_info;
    ipid_info << "-----";
    QString ipid = argv[0];

    for (DBManager::idipinfo ipidip : server->db_manager->getIpidInfo(ipid)) {
        ipid_info << "IPID: " + ipidip.ipid;
        ipid_info << "IP: " + ipidip.ip;
        ipid_info << "Created: " + ipidip.date;
        ipid_info << "-----";
    }

    sendServerMessage(ipid_info.join("\n"));
}

void AOClient::cmdTakeTakedChar(int argc, QStringList argv)
{
    take_taked_char = !take_taked_char;
    QString str_en = take_taked_char ? "now take taked characters" : "no longer take taked characters";

    sendServerMessage("You are " + str_en + ".");
}

void AOClient::cmdBlind(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (target->id == id) {
        sendServerMessage("Nope.");
        return;
    }

    if (target->blinded)
        sendServerMessage("That player is already blinded!");
    else {
        sendServerMessage("Blind player.");
        area->logCmdAdvanced(current_char, ipid, hwid, "BLIND", "Blinded UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->blinded = true;
}

void AOClient::cmdUnBlind(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!target->blinded)
        sendServerMessage("That player is not blinded!");
    else {
        sendServerMessage("Unblind player.");
        target->sendServerMessage("A moderator unblinded you. ");
    }

    target->blinded = false;
    area->logCmdAdvanced(current_char, ipid, hwid, "UNBLIND", "Unblinded UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdSneakMod(int argc, QStringList argv)
{
    slient_mod = !slient_mod;

    QString str_en = slient_mod ? "invisible" : "visible";
    sendServerMessage("Your moderator status is now " + str_en + ".");
}

void AOClient::cmdToggleWebUsersSpectateOnly(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    ConfigManager::webUsersSpectableOnlyToggle();

    QString str_en = ConfigManager::webUsersSpectableOnly() ? "actived." : "disabled.";

    sendServerMessage("WUSO Mod is now " + str_en);

    if (ConfigManager::webUsersSpectableOnly() == false) {
        for (AOClient* client : server->clients) {
            if (client->wuso == true) {
                wuso = false;
                usemodcall = true;
            }
        }
    }

    area->logCmdAdvanced(current_char, ipid, hwid, "TOGGLE WUSO", str_en, showname, ooc_name, QString::number(id));
}

void AOClient::cmdRemoveWebUsersSpectateOnly(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    bool conv_ok = false;
    int uid = argv[0].toInt(&conv_ok);

    if (!conv_ok) {
        sendServerMessage("Invalid user ID.");
        return;
    }

    AOClient* target = server->getClientByID(uid);

    if (target == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }

    if (!target->wuso)
        sendServerMessage("This player is not affected by WUSO Mod!");
    else {
        sendServerMessage("Done.");
        target->sendServerMessage("Now the restrictions of the WUSO Mod do not apply to you.");
    }

    target->wuso = false;
    target->usemodcall = true;
    area->logCmdAdvanced(current_char, ipid, hwid, "REMOVE WUSO", "UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}
