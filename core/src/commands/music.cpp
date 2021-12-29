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

// This file is for commands under the music category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdPlay(int argc, QStringList argv)
{
    if (is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - last_music_change_time <= 2) {
        sendServerMessage("You change music a lot!");
        return;
    }

    last_music_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    AreaData* area = server->areas[current_area];

    if (area->toggle_music == false && !checkAuth(ACLFlags.value("CM"))) {
        sendServerMessage("Music is disabled in this area.");
        return;
    }

    QString song = argv.join(" ");

    if (song.startsWith("https://www.youtube.com/") || song.startsWith("https://www.youtu.be//")) {
        sendServerMessage("You cannot use YouTube links. You may use direct links to MP3, Ogg, or M3U streams.");
        return;
    }

    if (!song.startsWith("http")) {
        sendServerMessage("Unknown http source. Maybe you forgot to copy the link along with http:// or https://");
        return;
    }

    AOPacket music_change("MC", {song, QString::number(server->getCharID(current_char)), showname, "1", "0"});
    server->broadcast(music_change, current_area);
    area->current_music = song;

    if (!showname.isEmpty())
        area->music_played_by = showname;
    else
        area->music_played_by = current_char;

    server->areas.value(current_area)->LogMusic(current_char, ipid, hwid, showname, ooc_name, song, QString::number(id));
}

void AOClient::cmdPlayOnce(int argc, QStringList argv)
{
    if (is_dj_blocked) {
        sendServerMessage("You are blocked from changing the music.");
        return;
    }

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - last_music_change_time <= 2) {
        sendServerMessage("You change music a lot!");
        return;
    }

    last_music_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();
    AreaData* area = server->areas[current_area];

    if (area->toggle_music == false && !checkAuth(ACLFlags.value("CM"))) {
        sendServerMessage("Music is disabled in this area.");
        return;
    }

    QString song = argv.join(" ");

    if (song.startsWith("https://www.youtube.com/") || song.startsWith("https://www.youtu.be//")) {
        sendServerMessage("You cannot use YouTube links. You may use direct links to MP3, Ogg, or M3U streams.");
        return;
    }

    if (!song.startsWith("http")) {
        sendServerMessage("Unknown http source. Maybe you forgot to copy the link along with http:// or https://");
        return;
    }

    AOPacket music_change("MC", {song, QString::number(server->getCharID(current_char)), showname, "0", "0"});
    server->broadcast(music_change, current_area);

    if (!showname.isEmpty())
        area->music_played_by = showname;
    else
        area->music_played_by = current_char;

    server->areas.value(current_area)->LogMusic(current_char, ipid, hwid, showname, ooc_name, song, QString::number(id));
}

void AOClient::cmdCurrentMusic(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];

    if (area->current_music != "" && area->current_music != "~stop.mp3") // dummy track for stopping music
        sendServerMessage("The current song is " + area->current_music + " played by " + area->music_played_by);
    else
        sendServerMessage("There is no music playing.");
}

void AOClient::cmdBlockDj(int argc, QStringList argv)
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

    if (target->is_dj_blocked)
        sendServerMessage("That player is already DJ blocked!");
    else {
        sendServerMessage("DJ blocked player.");
        area->logCmdAdvanced(current_char, ipid, hwid, "BLOCKDJ", "Blocked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->is_dj_blocked = true;
}

void AOClient::cmdUnBlockDj(int argc, QStringList argv)
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

    if (!target->is_dj_blocked)
        sendServerMessage("That player is not DJ blocked!");
    else {
        sendServerMessage("DJ permissions restored to player.");
        target->sendServerMessage("A moderator restored your music permissions. ");
        area->logCmdAdvanced(current_char, ipid, hwid, "UNBLOCKDJ", "Unblocked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
    }

    target->is_dj_blocked = false;
}

void AOClient::cmdToggleMusic(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    area->toggle_music = !area->toggle_music;
    QString state = area->toggle_music ? "allowed." : "disallowed.";

    sendServerMessage("Music in this area is now " + state);
    area->logCmdAdvanced(current_char, ipid, hwid, "TOGGLEMUSIC", state, showname, ooc_name, QString::number(id));
}
