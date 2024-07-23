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
#include "music_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

// This file is for commands under the music category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdPlay(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv);
}

void AOClient::cmdPlayOnce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, false, true);
}

void AOClient::cmdPlayAmbience(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, false, false, false, true);
}

void AOClient::cmdPlayAmbienceGgl(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, false, false, true, true);
}

void AOClient::cmdCurrentMusic(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->currentMusic() != "" && l_area->currentMusic() != "~stop.mp3") // dummy track for stopping music
        sendServerMessage("The current song is " + l_area->currentMusic() + " played by " + l_area->musicPlayerBy());
    else
        sendServerMessage("There is no music playing.");
}

void AOClient::cmdBlockDj(int argc, QStringList argv)
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

    if (l_target->m_is_dj_blocked)
        sendServerMessage("That player is already DJ blocked.");
    else
        sendServerMessage("That player has been DJ blocked.");

    l_target->m_is_dj_blocked = true;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "BLOCKDJ", "Blocked UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdUnBlockDj(int argc, QStringList argv)
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

    if (!l_target->m_is_dj_blocked)
        sendServerMessage("That player is not DJ blocked!");
    else {
        sendServerMessage("DJ permissions restored to the player.");
        l_target->sendServerMessage("A moderator restored your music permissions.");
    }

    l_target->m_is_dj_blocked = false;
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "UNBLOCKDJ", "Unblocked UID: " + QString::number(l_target->clientId()), server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdToggleMusic(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->toggleMusic();
    QString l_state = l_area->isMusicAllowed() ? "allowed." : "disallowed.";
    sendServerMessage("Music in this area is now " + l_state);
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "TOGGLEMUSIC", l_state, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdAddSong(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    // This needs some explanation.
    // Akashi has no concept of argument count,so any space is interpreted as a new element
    // in the QStringList. This works fine until someone enters something with a space.
    // Since we can't preencode those elements, we join all as a string and use a delimiter
    // that does not exist in file and URL paths. I decided on the ol' reliable ','.
    QString l_argv_string = argv.join(" ");
    QStringList l_argv = l_argv_string.split(",");

    bool l_success = false;
    if (l_argv.size() == 1) {
        QString l_song_name = l_argv.value(0);
        l_success = m_music_manager->addCustomSong(l_song_name, areaId());
    }

    if (l_argv.size() >= 2) {
        sendServerMessage("Too many arguments. Addition of the song has failed.");
        return;
    }

    QString l_message = l_success ? "succeeded." : "failed.";
    sendServerMessage("The addition of the song has " + l_message);
}

void AOClient::cmdAddCategory(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    bool l_success = m_music_manager->addCustomCategory(argv.join(" "), areaId());
    QString l_message = l_success ? "succeeded." : "failed.";
    sendServerMessage("The addition of the category has " + l_message);
}

void AOClient::cmdRemoveCategorySong(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    bool l_success = m_music_manager->removeCategorySong(argv.join(" "), areaId());
    QString l_message = l_success ? "succeeded." : "failed.";
    sendServerMessage("The removal of the entry has " + l_message);
}

void AOClient::cmdToggleRootlist(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    bool l_status = m_music_manager->toggleRootEnabled(areaId());
    QString l_message = (l_status) ? "enabled." : "disabled.";
    sendServerMessage("Global musiclist has been " + l_message);
}

void AOClient::cmdClearCustom(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    m_music_manager->clearCustomList(areaId());
    sendServerMessage("Custom songs have been cleared.");
}

void AOClient::cmdPlayHub(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, true);
}

void AOClient::cmdPlayHubOnce(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, true, true);
}

void AOClient::cmdPlayGgl(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, false, false, true);
}

void AOClient::cmdPlayOnceGgl(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, false, true, true);
}

void AOClient::cmdPlayHubGgl(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, true, false, true);
}

void AOClient::cmdPlayHubOnceGgl(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    playMusic(argv, true, true, true);
}
