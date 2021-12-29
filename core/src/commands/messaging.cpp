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

// This file is for commands under the messaging category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdPos(int argc, QStringList argv)
{
    changePosition(argv[0]);
    updateEvidenceList(server->areas[current_area]);
}

void AOClient::cmdForcePos(int argc, QStringList argv)
{
    bool ok;
    QList<AOClient*> targets;
    AreaData* area = server->areas[current_area];
    int target_id = argv[1].toInt(&ok);
    int forced_clients = 0;

    if (!ok && argv[1] != "*") {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }
    else if (ok) {
        AOClient* target_client = server->getClientByID(target_id);

        if (target_client != nullptr)
            targets.append(target_client);
        else {
            sendServerMessage("Target ID not found!");
            return;
        }
    }

    else if (argv[1] == "*") { // force all clients in the area
        for (AOClient* client : server->clients) {
            if (client->current_area == current_area)
                targets.append(client);
        }
    }

    for (AOClient* target : targets) {
        target->sendServerMessage("Position forcibly changed.");
        target->changePosition(argv[0]);
        forced_clients++;
    }

    sendServerMessage("Forced " + QString::number(forced_clients) + " into pos " + argv[0] + ".");
    area->logCmdAdvanced(current_char, ipid, hwid, "FORCE POS", "Client(-s): " + QString::number(forced_clients) + ". Pos: " + argv[0], showname, ooc_name, QString::number(id));
}

void AOClient::cmdG(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString sender_name = ooc_name;
    QString sender_area = server->area_names.value(current_area);
    bool sender_auth = authenticated;
    bool sender_slientmod = slient_mod;
    QString sender_message = argv.join(" ");

    for (AOClient* client : server->clients) {
        if (client->global_enabled && sender_auth == true && sender_slientmod == false)
            client->sendPacket("CT", {"[G][" + sender_area + "]" "[M]" + sender_name, sender_message});
        else if (client->global_enabled)
            client->sendPacket("CT", {"[G][" + sender_area + "]" + sender_name, sender_message});
    }
    area->logCmdAdvanced(current_char, ipid, hwid, "GLOBAL MESSAGE", sender_message, showname, ooc_name, QString::number(id));
    return;
}

void AOClient::cmdNeed(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString sender_area = server->area_names.value(current_area);
    QString sender_message = argv.join(" ");

    for (AOClient* client : server->clients) {
        if (client->advert_enabled) {
            client->sendServerMessage({"=== Advert ===\n[" + sender_area + "] needs " + sender_message+ "."});
        }
    }
    area->logCmdAdvanced(current_char, ipid, hwid, "NEED MESSAGE", sender_message, showname, ooc_name, QString::number(id));
}

void AOClient::cmdSwitch(int argc, QStringList argv)
{
    int selected_char_id = server->getCharID(argv.join(" "));

    if (selected_char_id == -1) {
        sendServerMessage("That does not look like a valid character.");
        return;
    }

    if (changeCharacter(selected_char_id)) {
        char_id = selected_char_id;
    }
    else {
        sendServerMessage("The character you picked is either taken or invalid.");
    }
}

void AOClient::cmdRandomChar(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    int selected_char_id;
    bool taken = true;

    while (taken) {
        selected_char_id = genRand(0, server->characters.size() - 1);
        if (!area->charactersTaken().contains(selected_char_id)) {
            taken = false;
        }
    }

    if (changeCharacter(selected_char_id)) {
        char_id = selected_char_id;
    }
}

void AOClient::cmdToggleGlobal(int argc, QStringList argv)
{
    global_enabled = !global_enabled;
    QString str_en = global_enabled ? "shown" : "hidden";

    sendServerMessage("Global chat set to " + str_en);
}

void AOClient::cmdPM(int arc, QStringList argv)
{
    const int sender_area = current_area;
    bool ok;
    int target_id = argv.takeFirst().toInt(&ok); // using takeFirst removes the ID from our list of arguments...

    if (!ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient* target_client = server->getClientByID(target_id);

    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    if (target_client->pm_mute) {
        sendServerMessage("That user is not recieving PMs.");
        return;
    }

    QString message = argv.join(" "); //...which means it will not end up as part of the message
    QString final_message = "PM from " + ooc_name;

    if (!showname.isEmpty())
        final_message += " (" + showname + ") ";

    final_message += " (ID: " + QString::number(id) + ") " + "in " + server->area_names[sender_area] + ". Message: " + message;

    target_client->sendServerMessage(final_message);
    sendServerMessage("PM sent to " + QString::number(target_client->id) + ". Message: " + message);
}

void AOClient::cmdAnnounce(int argc, QStringList argv)
{
    sendServerBroadcast("=== Announcement ===\r\n" + argv.join(" ") + "\r\n=============");
}

void AOClient::cmdM(int argc, QStringList argv)
{
    AreaData* area = server->areas[current_area];
    QString sender_name = ooc_name;
    int sender_area = current_area;
    QString sender_message = argv.join(" ");

    for (AOClient* client : server->clients) {
        if (client->checkAuth(ACLFlags.value("MODCHAT")))
            client->sendPacket("CT", {"$M[" + QString::number(sender_area) + "]" + sender_name, sender_message});
    }

    area->logCmdAdvanced(current_char, ipid, hwid, "MODCHAT", sender_message, showname, ooc_name, QString::number(id));

    return;
}

void AOClient::cmdGimp(int argc, QStringList argv)
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

    if (target->is_gimped)
        sendServerMessage("That player is already gimped!");
    else {
        sendServerMessage("Gimped player.");
    }

    target->is_gimped = true;
    area->logCmdAdvanced(current_char, ipid, hwid, "GIMP", "Gimped UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUnGimp(int argc, QStringList argv)
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

    if (!(target->is_gimped))
        sendServerMessage("That player is not gimped!");
    else {
        sendServerMessage("Ungimped player.");
        target->sendServerMessage("A moderator has ungimped you! ");
    }

    target->is_gimped = false;
    area->logCmdAdvanced(current_char, ipid, hwid, "UNGIMP", "Ungimped UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdDisemvowel(int argc, QStringList argv)
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

    if (target->is_disemvoweled)
        sendServerMessage("That player is already disemvoweled!");
    else {
        sendServerMessage("Disemvoweled player.");
    }

    target->is_disemvoweled = true;
    area->logCmdAdvanced(current_char, ipid, hwid, "DISEMVOWEL", "Disemvoweled UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUnDisemvowel(int argc, QStringList argv)
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

    if (!(target->is_disemvoweled))
        sendServerMessage("That player is not disemvoweled!");
    else {
        sendServerMessage("Undisemvoweled player.");
        target->sendServerMessage("A moderator has undisemvoweled you! ");
    }

    target->is_disemvoweled = false;
    area->logCmdAdvanced(current_char, ipid, hwid, "DISEMVOWEL", "Undisemvoweled UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdShake(int argc, QStringList argv)
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

    if (target->is_shaken)
        sendServerMessage("That player is already shaken!");
    else {
        sendServerMessage("Shook player.");
    }

    target->is_shaken = true;
    area->logCmdAdvanced(current_char, ipid, hwid, "SHAKE", "Shaked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUnShake(int argc, QStringList argv)
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

    if (!(target->is_shaken))
        sendServerMessage("That player is not shaken!");
    else {
        sendServerMessage("Unshook player.");
        target->sendServerMessage("A moderator has unshook you! ");
    }

    target->is_shaken = false;
    area->logCmdAdvanced(current_char, ipid, hwid, "UNSHAKE", "Unshaked UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdMutePM(int argc, QStringList argv)
{
    pm_mute = !pm_mute;
    QString str_en = pm_mute ? "muted" : "unmuted";

    sendServerMessage("PM's are now " + str_en);
}

void AOClient::cmdAfk(int argc, QStringList argv)
{
    if (is_afk == true) {
        is_afk = false;

        sendServerMessage("You are no longer AFK. Welcome back!");

        if (current_char.isEmpty() && showname.isEmpty())
           sendServerMessageArea("[" + QString::number(id) + "] Spectator are no longer AFK.");
        else if (showname.isEmpty())
           sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " are no longer AFK.");
        else
           sendServerMessageArea("[" + QString::number(id) + "] " + showname + " are no longer AFK.");

        return;
    }

    is_afk = true;
    sendServerMessage("You are now AFK. Have a good day!");

    if (showname.isEmpty() && current_char.isEmpty())
        sendServerMessageArea("[" + QString::number(id) + "] " + "Spectator" + " is now AFK.");
    else if (showname.isEmpty())
        sendServerMessageArea("[" + QString::number(id) + "] " + current_char + " is now AFK.");
    else
        sendServerMessageArea("[" + QString::number(id) + "] " + showname + " is now AFK.");
}

void AOClient::cmdCharCurse(int argc, QStringList argv)
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

    if (target->is_charcursed) {
        sendServerMessage("That player is already charcursed!");
        return;
    }

    if (argc == 1) {
        target->charcurse_list.append(server->getCharID(target->current_char));
    }
    else {
        argv.removeFirst();
        QStringList char_names = argv.join(" ").split(",");

        target->charcurse_list.clear();

        for (QString char_name : char_names) {
            int char_id = server->getCharID(char_name);
            if (char_id == -1) {
                sendServerMessage("Could not find character: " + char_name);
                return;
            }

            target->charcurse_list.append(char_id);
        }
    }

    target->is_charcursed = true;

    //Kick back to char select screen
    if (!target->charcurse_list.contains(server->getCharID(target->current_char))) {
        target->changeCharacter(-1);
        server->updateCharsTaken(server->areas.value(current_area));
        target->sendPacket("DONE");
    }
    else {
        server->updateCharsTaken(server->areas.value(current_area));
    }

    target->sendServerMessage("You have been charcursed!");
    sendServerMessage("Charcursed player.");
    area->logCmdAdvanced(current_char, ipid, hwid, "CHARCURSE", "Charcursed UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdUnCharCurse(int argc, QStringList argv)
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

    if (!target->is_charcursed) {
        sendServerMessage("That player is not charcursed!");
        return;
    }

    target->is_charcursed = false;
    target->charcurse_list.clear();
    server->updateCharsTaken(server->areas.value(current_area));
    sendServerMessage("Uncharcursed player.");
    target->sendServerMessage("You were uncharcursed.");
    area->logCmdAdvanced(current_char, ipid, hwid, "UNCHARCURSE", "Uncharcursed UID: " + QString::number(target->id), showname, ooc_name, QString::number(id));
}

void AOClient::cmdCharSelect(int argc, QStringList argv)
{
    if (argc == 0) {
        changeCharacter(-1);
        sendPacket("DONE");
    }
    else {
        if (!checkAuth(ACLFlags.value("FORCE_CHARSELECT"))) {
            sendServerMessage("You do not have permission to force another player to character select!");
            return;
        }

        bool ok = false;
        int target_id = argv[0].toInt(&ok);
        if (!ok)
            return;

        AOClient* target = server->getClientByID(target_id);

        if (target == nullptr)
            return;

        target->changeCharacter(-1);
        target->sendPacket("DONE");
    }
}

void AOClient::cmdFirstPerson(int argc, QStringList argv)
{
    first_person = !first_person;
    QString str_en = first_person ? "enabled" : "disabled";

    sendServerMessage("First person mode " + str_en + ".");
}
