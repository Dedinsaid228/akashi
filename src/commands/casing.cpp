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
#include "packet/packet_factory.h"
#include "server.h"

// This file is for commands under the casing category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdDoc(int argc, QStringList argv)
{
    AreaData *l_area = server->getAreaById(areaId());
    if (argc == 0)
        sendServerMessage("Document: " + l_area->document());
    else {
        l_area->changeDoc(argv.join(" "));
        sendServerMessageArea("[" + QString::number(clientId()) + "] " + getSenderName(clientId()) + " has changed the document.");
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "CHANGEDOC", argv.join(" "), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
    }
}

void AOClient::cmdClearDoc(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->changeDoc("No document.");
    sendServerMessageArea("[" + QString::number(clientId()) + "] " + getSenderName(clientId()) + " has cleared the document.");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "CLEARDOC", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdEvidenceMod(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    argv[0] = argv[0].toLower();
    if (argv[0] == "cm")
        l_area->setEviMod(AreaData::EvidenceMod::CM);
    else if (argv[0] == "mod")
        l_area->setEviMod(AreaData::EvidenceMod::MOD);
    else if (argv[0] == "hiddencm")
        l_area->setEviMod(AreaData::EvidenceMod::HIDDEN_CM);
    else if (argv[0] == "ffa")
        l_area->setEviMod(AreaData::EvidenceMod::FFA);
    else {
        sendServerMessage("Invalid evidence mod.");
        return;
    }

    sendServerMessage("Evidence mod in this area is changed to " + argv[0].toUpper() + ".");

    // Resend evidence lists to everyone in the area
    sendEvidenceList(l_area);
}

void AOClient::cmdEvidence_Swap(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    int l_ev_size = l_area->evidence().size() - 1;
    if (l_ev_size < 0) {
        sendServerMessage("No evidence in the area.");
        return;
    }

    bool ok, ok2;
    int l_ev_id1 = argv[0].toInt(&ok), l_ev_id2 = argv[1].toInt(&ok2);

    if ((!ok || !ok2)) {
        sendServerMessage("Invalid evidence ID.");
        return;
    }

    if ((l_ev_id1 < 0) || (l_ev_id2 < 0)) {
        sendServerMessage("Evidence ID cannot be negative.");
        return;
    }

    if ((l_ev_id2 <= l_ev_size) && (l_ev_id1 <= l_ev_size)) {
        l_area->swapEvidence(l_ev_id1, l_ev_id2);
        sendEvidenceList(l_area);
        sendServerMessage("The evidence " + QString::number(l_ev_id1) + " and " + QString::number(l_ev_id2) + " have been swapped.");
    }
    else
        sendServerMessage("Unable to swap the evidence. Evidence ID out of range.");
}

void AOClient::cmdTestify(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->testimonyRecording() == AreaData::TestimonyRecording::RECORDING)
        sendServerMessage("Testimony recording is already in progress. Please stop it before starting a new one.");
    else {
        clearTestimony();
        l_area->setTestimonyRecording(AreaData::TestimonyRecording::RECORDING);
        sendServerMessage("Testimony recording is started.");
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "STARTTESTIMONY", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
    }
}

void AOClient::cmdExamine(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->testimony().size() - 1 > 0) {
        l_area->restartTestimony();
        server->broadcast(PacketFactory::createPacket("RT", {"testimony2", "0"}), areaId());
        server->broadcast(PacketFactory::createPacket("MS", {l_area->testimony()[0]}), areaId());
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "PLAYBACKTESTIMONY", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
        return;
    }

    if (l_area->testimonyRecording() == AreaData::TestimonyRecording::PLAYBACK)
        sendServerMessage("Unable to examine while another examination is running");
    else
        sendServerMessage("Unable to start replay without prior examination.");
}

void AOClient::cmdTestimony(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->testimony().size() - 1 < 1) {
        sendServerMessage("Unable to display empty testimony.");
        return;
    }

    QString l_ooc_message;
    for (int i = 1; i <= l_area->testimony().size() - 1; i++) {
        QStringList l_packet = l_area->testimony().at(i);
        QString l_ic_message = l_packet[4];
        l_ooc_message.append("[" + QString::number(i) + "] " + l_ic_message + "\n");
    }

    sendServerMessage(l_ooc_message);
}

void AOClient::cmdDeleteStatement(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->testimony().size() - 1 == 0)
        sendServerMessage("Unable to delete statement. No statements saved in this area.");

    int l_c_statement = l_area->statement();
    if (l_c_statement > 0 && l_area->testimony().size() > 2) {
        l_area->removeStatement(l_c_statement);
        sendServerMessage("The statement with id " + QString::number(l_c_statement) + " has been deleted from the testimony.");
    }
}

void AOClient::cmdUpdateStatement(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    server->getAreaById(areaId())->setTestimonyRecording(AreaData::TestimonyRecording::UPDATE);
    sendServerMessage("The next IC-Message will replace the last displayed replay message.");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "UPDATESTATEMENT", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdPauseTestimony(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->setTestimonyRecording(AreaData::TestimonyRecording::STOPPED);
    server->broadcast(PacketFactory::createPacket("RT", {"testimony1", "1"}), areaId());
    sendServerMessage("Testimony recording has been stopped.");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "STOPTESTIMONY", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}

void AOClient::cmdAddStatement(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (server->getAreaById(areaId())->statement() < ConfigManager::maxStatements()) {
        server->getAreaById(areaId())->setTestimonyRecording(AreaData::TestimonyRecording::ADD);
        sendServerMessage("The next IC-Message will be inserted into the testimony.");
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "ADDSTATEMENT", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
    }
    else
        sendServerMessage("Unable to add anymore statements. Please remove any unused ones.");
}

void AOClient::cmdSaveTestimony(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    bool l_permission_found = false;
    if (checkPermission(ACLRole::SAVETEST))
        l_permission_found = true;

    if (m_testimony_saving == true)
        l_permission_found = true;

    if (l_permission_found) {
        AreaData *l_area = server->getAreaById(areaId());
        if (l_area->testimony().size() - 1 <= 0) {
            sendServerMessage("Cannot save an empty testimony.");
            return;
        }

        QDir l_dir_testimony("storage/testimony");
        if (!l_dir_testimony.exists())
            l_dir_testimony.mkpath(".");

        QString l_testimony_name = argv[0].trimmed().toLower().replace("..", ""); // :)
        QFile l_file("storage/testimony/" + l_testimony_name + ".txt");
        if (l_file.exists()) {
            sendServerMessage("Unable to save testimony. Testimony name already exists.");
            return;
        }

        QTextStream l_out(&l_file);
        if (l_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            for (int i = 0; i <= l_area->testimony().size() - 1; i++)
                l_out << l_area->testimony().at(i).join("#") << "\n";

            sendServerMessage("Testimony saved. To load it use /loadtestimony " + l_testimony_name);
            emit logCMD((character() + " " + characterName()), m_ipid, name(), "SAVETESTIMONY", l_testimony_name, server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
            m_testimony_saving = false;
        }
    }
    else {
        sendServerMessage("You do not have permission to save a testimony. Please contact a moderator for permission.");
        return;
    }
}

void AOClient::cmdLoadTestimony(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QDir l_dir_testimony("storage/testimony");
    if (!l_dir_testimony.exists()) {
        sendServerMessage("Unable to load testimonies. Testimony storage not found.");
        return;
    }

    QString l_testimony_name = argv[0].trimmed().toLower().replace("..", ""); // :)
    QFile l_file("storage/testimony/" + l_testimony_name + ".txt");

    if (!l_file.exists()) {
        sendServerMessage("Unable to load testimony. Testimony name not found.");
        return;
    }

    if (!l_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sendServerMessage("Unable to load testimony. Permission denied.");
        return;
    }

    clearTestimony();
    int l_testimony_lines = 0;
    QTextStream l_in(&l_file);
    AreaData *l_area = server->getAreaById(areaId());
    while (!l_in.atEnd()) {
        if (l_testimony_lines <= ConfigManager::maxStatements()) {
            QString l_line = l_in.readLine();
            QStringList packet = l_line.split("#");
            l_area->addStatement(l_area->testimony().size(), packet);
            l_testimony_lines = l_testimony_lines + 1;
        }
        else {
            sendServerMessage("Testimony too large to be loaded.");
            clearTestimony();
            return;
        }
    }

    sendServerMessage("Testimony loaded successfully. Use /examine to start playback.");
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "LOADTESTIMONY", l_testimony_name, server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, server->getHubName(hubId()));
}