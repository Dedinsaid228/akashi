//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                           //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY{} without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "include/aoclient.h"

#include <QQueue>

#include "include/akashidefs.h"
#include "include/area_data.h"
#include "include/config_manager.h"
#include "include/db_manager.h"
#include "include/music_manager.h"
#include "include/packet/packet_factory.h"
#include "include/server.h"

void AOClient::sendEvidenceList(AreaData *area) const
{
    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == m_current_area)
            l_client->updateEvidenceList(area);
    }
}

void AOClient::sendEvidenceListHidCmNoCm(AreaData *area) const
{
    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_current_area == m_current_area)
            l_client->updateEvidenceListHidCmNoCm(area);
    }
}

void AOClient::updateEvidenceList(AreaData *area)
{
    QStringList l_evidence_list;
    QString l_evidence_format("%1&%2&%3");
    int l_evidence_id = 0;
    m_evi_list = {0};
    const QList<AreaData::Evidence> l_area_evidence = area->evidence();

    for (const AreaData::Evidence &evidence : l_area_evidence) {
        if (!checkPermission(ACLRole::CM) && area->eviMod() == AreaData::EvidenceMod::HIDDEN_CM) {
            QRegularExpression l_regex("<owner=(.*?)>");
            QRegularExpressionMatch l_match = l_regex.match(evidence.description);

            if (l_match.hasMatch()) {
                QStringList owners = l_match.captured(1).split(",");

                if (!owners.contains("all", Qt::CaseSensitivity::CaseInsensitive) && !owners.contains(m_pos, Qt::CaseSensitivity::CaseInsensitive)) {
                    l_evidence_id++;
                    continue;
                }
            }
            // no match = show it to all
        }
        l_evidence_list.append(l_evidence_format.arg(evidence.name, evidence.description, evidence.image));
        l_evidence_id++;
        m_evi_list.append(l_evidence_id);
    }

    sendPacket(PacketFactory::createPacket("LE", l_evidence_list));
}

void AOClient::updateEvidenceListHidCmNoCm(AreaData *area)
{
    QStringList l_evidence_list;
    QString l_evidence_format("%1&%2&%3");
    const QList<AreaData::Evidence> l_area_evidence = area->evidence();

    for (const AreaData::Evidence &evidence : l_area_evidence) {
        if (!checkPermission(ACLRole::CM) && area->eviMod() == AreaData::EvidenceMod::HIDDEN_CM) {
            QRegularExpression l_regex("<owner=(.*?)>");
            QRegularExpressionMatch l_match = l_regex.match(evidence.description);

            if (l_match.hasMatch()) {
                QStringList owners = l_match.captured(1).split(",");

                if (!owners.contains("all", Qt::CaseSensitivity::CaseInsensitive) && !owners.contains(m_pos, Qt::CaseSensitivity::CaseInsensitive)) {
                    l_evidence_list.append(l_evidence_format.arg("", "", ""));
                    continue;
                }
            }
            // no match = show it to all
        }
        l_evidence_list.append(l_evidence_format.arg(evidence.name, evidence.description, evidence.image));
    }

    sendPacket(PacketFactory::createPacket("LE", l_evidence_list));
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
        return true;
    }

    return false;
}

QString AOClient::dezalgo(QString p_text)
{
    QRegularExpression rxp("([̴̵̶̷̸̡̢̧̨̛̖̗̘̙̜̝̞̟̠̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̀́̂̃̄̅̆̇̈̉̊̋̌̍̎̏̐̑̒̓̔̽̾̿̀́͂̓̈́͆͊͋͌̕̚ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡])");
    QString filtered = p_text.replace(rxp, "");

    return filtered;
}

bool AOClient::checkEvidenceAccess(AreaData *area)
{
    switch (area->eviMod()) {
    case AreaData::EvidenceMod::FFA:
        return true;
    case AreaData::EvidenceMod::CM:
    case AreaData::EvidenceMod::HIDDEN_CM:
        return checkPermission(ACLRole::CM);
    case AreaData::EvidenceMod::MOD:
        return m_authenticated;
    default:
        return false;
    }
}

void AOClient::updateJudgeLog(AreaData *area, AOClient *client, QString action)
{
    QString l_timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString l_uid = QString::number(client->m_id);
    QString l_char_name = client->m_current_char;
    QString l_ipid = client->getIpid();
    QString l_message = action;
    QString l_logmessage = QString("[%1]: [%2] %3 (%4) %5").arg(l_timestamp, l_uid, l_char_name, l_ipid, l_message);
    area->appendJudgelog(l_logmessage);
}

QString AOClient::decodeMessage(QString incoming_message)
{
    QString decoded_message = incoming_message.replace("<num>", "#")
                                  .replace("<percent>", "%")
                                  .replace("<dollar>", "$")
                                  .replace("<and>", "&");
    return decoded_message;
}
