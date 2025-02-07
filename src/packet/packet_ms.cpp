#include "packet/packet_ms.h"
#include "config_manager.h"
#include "hub_data.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>

PacketMS::PacketMS(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketMS::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 15,
        .header = "MS"};
    return info;
}

void PacketMS::handlePacket(AreaData *area, AOClient &client) const
{
    if (client.m_is_muted) {
        client.sendServerMessage("You cannot speak while muted.");
        return;
    }

    if (client.m_wuso) {
        client.sendServerMessage("You cannot speak while you are affected by WUSO Mod.");
        return;
    }

    if (!area->isMessageAllowed() || !client.getServer()->isMessageAllowed())
        return;

    std::shared_ptr<AOPacket> validated_packet = validateIcPacket(client);
    if (validated_packet->getPacketInfo().header == "INVALID")
        return;

    if (client.m_pos != "" && client.m_pos.toLower() != "hidden")
        validated_packet->setContentField(5, client.m_pos);

    QList<std::shared_ptr<AOPacket>> l_packets;
    static QRegularExpression re("¨<(.*?)>¨");
    QRegularExpressionMatchIterator l_iter = re.globalMatch(validated_packet->toString().split("#")[5]);
    int l_capcon = 0;
    QString l_mes = validated_packet->toString().split("#")[5];
    QString l_lastpos;
    bool l_firstmes = true;
    while (l_iter.hasNext()) {
        QRegularExpressionMatch l_match = l_iter.next();
        if (l_match.hasMatch()) {
            QString l_part = l_mes.mid(l_capcon, l_match.capturedStart() - l_capcon);
            QString l_cap = l_match.captured(1);
            QStringList l_temp = validated_packet->toString().split("#");
            l_capcon = l_match.capturedEnd();
            l_temp[5] = l_part;
            if (!l_lastpos.isEmpty())
                l_temp[4] = l_lastpos;
            l_lastpos = l_cap;
            if (l_firstmes)
                l_firstmes = false;
            else
                l_temp[29] = "1";
            l_temp.removeFirst();
            l_packets.append(PacketFactory::createPacket("MS", l_temp));
            qDebug() << l_temp;
        }
    }

    if (!l_packets.isEmpty()) {
        QString l_part = l_mes.mid(l_capcon);
        QStringList l_temp = validated_packet->toString().split("#");
        l_temp[5] = l_part;
        l_temp[4] = l_lastpos;
        l_temp[29] = "1";
        l_temp.removeFirst();
        l_packets.append(PacketFactory::createPacket("MS", l_temp));
    }

    bool evipresent = client.evidencePresent(validated_packet->getContent()[11]);
    if (evipresent)
        client.sendEvidenceListHidCmNoCm(area);

    if (!client.m_blinded) {
        if (!l_packets.isEmpty())
            for (const std::shared_ptr<AOPacket> &l_packet : l_packets)
                client.getServer()->broadcast(l_packet, client.areaId());
        else
            client.getServer()->broadcast(validated_packet, client.areaId());
    }
    else {
        if (!l_packets.isEmpty())
            for (const std::shared_ptr<AOPacket> &l_packet : l_packets)
                client.sendPacket(l_packet);
        else
            client.sendPacket(validated_packet);
    }

    client.getServer()->hubListen(m_content[4], client.areaId(), client.getSenderName(client.clientId()), client.clientId());

    if (evipresent)
        client.sendEvidenceList(area);

    emit client.logIC((client.character() + " " + client.characterName()), client.name(), client.m_ipid, area->name(), client.m_last_message, QString::number(client.clientId()), client.m_hwid, client.getServer()->getHubName(client.hubId()));
    area->updateLastICMessage(validated_packet->getContent());
    area->updateLastICMessageOwner(client.m_ipid);

    bool floodguard = area->floodguardActive();
    if (floodguard)
        area->startMessageFloodguard(ConfigManager::messageFloodguard());
}

std::shared_ptr<AOPacket> PacketMS::validateIcPacket(AOClient &client) const
{
    // Welcome to the super cursed server-side IC chat validation hell

    // I wanted to use enums or #defines here to make the
    // indicies of the args arrays more readable. But,
    // in typical AO fasion, the indicies for the incoming
    // and outgoing packets are different. Just RTFM.

    // This packet can be sent with a minimum required args of 15.
    // 2.6+ extensions raise this to 19, and 2.8 further raises this to 26.

    std::shared_ptr<AOPacket> l_invalid = PacketFactory::createPacket("INVALID", {});
    QStringList l_args;
    if (client.isSpectator() || client.character().isEmpty() || !client.m_joined)
        // Spectators cannot use IC
        return l_invalid;
    AreaData *area = client.getServer()->getAreaById(client.areaId());
    HubData *hub = client.getServer()->getHubById(client.hubId());

    if (((area->lockStatus() == AreaData::LockStatus::SPECTATABLE && !area->invited().contains(client.clientId())) || (hub->hubLockStatus() == HubData::HubLockStatus::SPECTATABLE && !hub->hubInvited().contains(client.clientId()))) && !client.checkPermission(ACLRole::BYPASS_LOCKS))
        // Non-invited players cannot speak in spectatable areas
        return l_invalid;

    QList<QVariant> l_incoming_args;
    for (const QString &l_arg : m_content)
        l_incoming_args.append(QVariant(l_arg));

    // desk modifier
    QStringList allowed_desk_mods;
    allowed_desk_mods << "chat"
                      << "0"
                      << "1"
                      << "2"
                      << "3"
                      << "4"
                      << "5";
    QString l_incoming_deskmod = l_incoming_args[0].toString();
    if (allowed_desk_mods.contains(l_incoming_deskmod))
        // **WARNING : THIS IS A HACK!**
        // A proper solution would be to deprecate chat as an argument on the clientside
        // instead of overwriting correct netcode behaviour on the serverside.
        if (l_incoming_deskmod == "chat")
            l_args.append("1");
        else
            l_args.append(l_incoming_args[0].toString());
    else
        return l_invalid;

    // preanim
    l_args.append(l_incoming_args[1].toString());

    // char name
    if (client.character().toLower() != l_incoming_args[2].toString().toLower())
        // Selected char is different from supplied folder name
        // This means the user is INI-swapped
        if (!area->iniswapAllowed())
            if (!client.getServer()->getCharacters().contains(l_incoming_args[2].toString(), Qt::CaseInsensitive))
                return l_invalid;

    client.m_current_iniswap = l_incoming_args[2].toString();
    l_args.append(l_incoming_args[2].toString());

    // emote
    client.m_emote = l_incoming_args[3].toString();

    if (client.m_first_person)
        client.m_emote = "";

    l_args.append(client.m_emote);

    bool l_chillmod = area->chillMod();
    // message text
    if (l_incoming_args[4].toString().size() > ConfigManager::maxCharacters() || (l_chillmod && l_incoming_args[4].toString().size() > ConfigManager::maxCharactersChillMod())) {
        client.sendServerMessage("Your message is too long!");
        return l_invalid;
    }

    QString l_incoming_msg = client.dezalgo(l_incoming_args[4].toString().trimmed());
    if (!area->lastICMessage().isEmpty() && l_incoming_msg == area->lastICMessage()[4] && client.m_ipid == area->lastICMessageOwner() && l_incoming_msg != "")
        return l_invalid;

    if (l_incoming_msg == "" && area->blankpostingAllowed() == false) {
        client.sendServerMessage("Blankposting has been forbidden in this area.");
        return l_invalid;
    }

    if (!ConfigManager::filterList().isEmpty()) {
        foreach (const QString &regex, ConfigManager::filterList()) {
            QRegularExpression re(regex, QRegularExpression::CaseInsensitiveOption);
            l_incoming_msg.replace(re, "❌");
        }
    }

    if (client.m_is_gimped) {
        QString l_gimp_message = ConfigManager::gimpList().at((client.genRand(1, ConfigManager::gimpList().size() - 1)));
        l_incoming_msg = l_gimp_message;
    }

    if (client.m_is_shaken) {
        QStringList l_parts = l_incoming_msg.split(" ");

        std::random_device rng;
        std::mt19937 urng(rng());
        std::shuffle(l_parts.begin(), l_parts.end(), urng);

        l_incoming_msg = l_parts.join(" ");
    }

    if (client.m_is_disemvoweled) {
        static QRegularExpression re("[AEIOUaeioЁУЕЫАОЭЯёуеыаоэя]");
        QString l_disemvoweled_message = l_incoming_msg.remove(re);
        l_incoming_msg = l_disemvoweled_message;
    }

    client.m_last_message = l_incoming_msg;
    l_args.append(l_incoming_msg);

    // side
    // this is validated clientside so w/e
    l_args.append(l_incoming_args[5].toString());
    if (client.m_pos != l_incoming_args[5].toString()) {
        if (l_incoming_args[5].toString() == "hidden") {
            client.sendServerMessage("This position cannot be used.");
            return l_invalid;
        }

        client.m_pos = l_incoming_args[5].toString();
        client.updateEvidenceList(client.getServer()->getAreaById(client.areaId()));
    }

    // sfx name
    l_args.append(l_incoming_args[6].toString());

    // emote modifier
    // Now, gather round, y'all. Here is a story that is truly a microcosm of the AO dev experience.
    // If this value is a 4, it will crash the client. Why? Who knows, but it does.
    // Now here is the kicker: in certain versions, the client would incorrectly send a 4 here
    // For a long time, by configuring the client to do a zoom with a preanim, it would send 4
    // This would crash everyone else's client, and the feature had to be disabled
    // But, for some reason, nobody traced the cause of this issue for many many years.
    // The serverside fix is needed to ensure invalid values are not sent, because the client sucks
    int emote_mod = l_incoming_args[7].toInt();

    if (emote_mod == 4)
        emote_mod = 6;
    if (emote_mod != 0 && emote_mod != 1 && emote_mod != 2 && emote_mod != 5 && emote_mod != 6)
        return l_invalid;
    l_args.append(QString::number(emote_mod));

    // char id
    if (l_incoming_args[8].toInt() != client.m_char_id)
        return l_invalid;
    l_args.append(l_incoming_args[8].toString());

    // sfx delay
    l_args.append(l_incoming_args[9].toString());

    // objection modifier
    if (area->isShoutAllowed())
        if (l_incoming_args[10].toString().contains("4"))
            // custom shout includes text metadata
            l_args.append(l_incoming_args[10].toString());
        else {
            int l_obj_mod = l_incoming_args[10].toInt();
            if ((l_obj_mod < 0) || (l_obj_mod > 4))
                return l_invalid;

            l_args.append(QString::number(l_obj_mod));
        }
    else {
        if (l_incoming_args[10].toString() != "0")
            client.sendServerMessage("Shouts have been disabled in this area.");

        l_args.append("0");
    }

    // evidence
    int evi_idx = client.m_evi_list[l_incoming_args[11].toInt()];
    if (evi_idx > area->evidence().length())
        return l_invalid;

    l_args.append(QString::number(evi_idx));

    // flipping
    int l_flip = l_incoming_args[12].toInt();
    if (l_flip != 0 && l_flip != 1)
        return l_invalid;

    client.m_flipping = QString::number(l_flip);
    l_args.append(client.m_flipping);

    // realization
    int realization = l_incoming_args[13].toInt();
    if (realization != 0 && realization != 1)
        return l_invalid;

    l_args.append(QString::number(realization));

    // text color
    int text_color = l_incoming_args[14].toInt();
    if (text_color < 0 || text_color > 11)
        return l_invalid;

    l_args.append(QString::number(text_color));

    // 2.6 packet extensions
    if (l_incoming_args.length() >= 19) {
        // showname
        QString l_incoming_showname = client.dezalgo(l_incoming_args[15].toString().trimmed());
        if (!(l_incoming_showname == client.character() || l_incoming_showname.isEmpty()) && !area->shownameAllowed()) {
            client.sendServerMessage("Shownames are not allowed in this area!");
            return l_invalid;
        }

        if (l_incoming_showname.length() > 30) {
            client.sendServerMessage("Your showname is too long! Please limit it to under 30 characters");
            return l_invalid;
        }

        // if the raw input is not empty but the trimmed input is, use a single space
        if (l_incoming_showname.isEmpty() && !l_incoming_args[15].toString().isEmpty())
            l_incoming_showname = " ";

        l_args.append(l_incoming_showname);
        client.setCharacterName(l_incoming_showname);

        // other char id
        // things get a bit hairy here
        // don't ask me how this works, because i don't know either
        QStringList l_pair_data = l_incoming_args[16].toString().split("^");
        client.m_pairing_with = l_pair_data[0].toInt();
        QString l_front_back = "";
        if (l_pair_data.length() > 1)
            l_front_back = "^" + l_pair_data[1];

        int l_other_charid = client.m_pairing_with;
        bool l_pairing = false;
        QString l_other_name = "0";
        QString l_other_emote = "0";
        QString l_other_offset = "0";
        QString l_other_flip = "0";
        for (int l_client_id : area->joinedIDs()) {
            AOClient *l_client = client.getServer()->getClientByID(l_client_id);
            if (l_client->m_pairing_with == client.m_char_id && l_other_charid != client.m_char_id && l_client->m_char_id == client.m_pairing_with && l_client->m_pos == client.m_pos) {
                l_other_name = l_client->m_current_iniswap;
                l_other_emote = l_client->m_emote;
                l_other_offset = l_client->m_offset;
                l_other_flip = l_client->m_flipping;
                l_pairing = true;
            }
        }

        if (!l_pairing) {
            l_other_charid = -1;
            l_front_back = "";
        }

        l_args.append(QString::number(l_other_charid) + l_front_back);
        l_args.append(l_other_name);
        l_args.append(l_other_emote);

        // self offset
        client.m_offset = l_incoming_args[17].toString();
        // versions 2.6-2.8 cannot validate y-offset so we send them just the x-offset
        if ((client.m_version.release == 2) && (client.m_version.major == 6 || client.m_version.major == 7 || client.m_version.major == 8)) {
            QString l_x_offset = client.m_offset.split("&")[0];
            l_args.append(l_x_offset);
            QString l_other_x_offset = l_other_offset.split("&")[0];
            l_args.append(l_other_x_offset);
        }
        else {
            l_args.append(client.m_offset);
            l_args.append(l_other_offset);
        }

        l_args.append(l_other_flip);

        // immediate text processing
        int l_immediate = l_incoming_args[18].toInt();
        if (area->forceImmediate()) {
            if (l_args[7] == "1" || l_args[7] == "2") {
                l_args[7] = "0";
                l_immediate = 1;
            }
            else if (l_args[7] == "6") {
                l_args[7] = "5";
                l_immediate = 1;
            }
        }

        if (l_immediate != 1 && l_immediate != 0)
            return l_invalid;

        l_args.append(QString::number(l_immediate));
    }

    // 2.8 packet extensions
    if (l_incoming_args.length() >= 26) {
        // sfx looping
        int l_sfx_loop = l_incoming_args[19].toInt();
        if (l_sfx_loop != 0 && l_sfx_loop != 1)
            return l_invalid;

        l_args.append(QString::number(l_sfx_loop));

        // screenshake
        int l_screenshake = l_incoming_args[20].toInt();
        if (l_screenshake != 0 && l_screenshake != 1)
            return l_invalid;

        l_args.append(QString::number(l_screenshake));

        // frames shake
        l_args.append(l_incoming_args[21].toString());

        // frames realization
        l_args.append(l_incoming_args[22].toString());

        // frames sfx
        l_args.append(l_incoming_args[23].toString());

        // additive
        int l_additive = l_incoming_args[24].toInt();
        if (l_additive != 0 && l_additive != 1)
            return l_invalid;

        else if (area->lastICMessage().isEmpty())
            l_additive = 0;

        else if (!(client.m_char_id == area->lastICMessage()[8].toInt()))
            l_additive = 0;

        else if (l_additive == 1)
            l_args[4].insert(0, " ");

        l_args.append(QString::number(l_additive));

        // effect
        l_args.append(l_incoming_args[25].toString());
    }

    if (l_incoming_args.size() >= 27) {
        // blips
        l_args.append(l_incoming_args[26].toString());
    }
    if (l_incoming_args.size() >= 28) {
        // slide toggle
        l_args.append(l_incoming_args[27].toString());
    }

    // Testimony playback
    if (area->testimonyRecording() == AreaData::TestimonyRecording::RECORDING || area->testimonyRecording() == AreaData::TestimonyRecording::ADD) {
        if (!l_args[5].startsWith("wit"))
            return PacketFactory::createPacket("MS", l_args);

        if (area->statement() == -1) {
            l_args[4] = "~~-- " + l_args[4] + " --";
            l_args[14] = "3";
            client.getServer()->broadcast(PacketFactory::createPacket("RT", {"testimony1"}), client.areaId());
        }

        client.addStatement(l_args);
    }
    else if (area->testimonyRecording() == AreaData::TestimonyRecording::UPDATE)
        l_args = client.updateStatement(l_args);

    else if (area->testimonyRecording() == AreaData::TestimonyRecording::PLAYBACK) {
        AreaData::TestimonyProgress l_progress;
        if (l_args[4] == ">") {
            auto l_statement = area->jumpToStatement(area->statement() + 1);
            l_args = l_statement.first;
            l_progress = l_statement.second;
            client.m_pos = l_args[5];
            client.sendServerMessageArea(client.getSenderName(client.clientId()) + " moved to the next statement.");
            if (l_progress == AreaData::TestimonyProgress::LOOPED)
                client.sendServerMessageArea("Last statement reached. Looping to first statement.");
        }
        if (l_args[4] == "<") {
            auto l_statement = area->jumpToStatement(area->statement() - 1);
            l_args = l_statement.first;
            l_progress = l_statement.second;
            client.m_pos = l_args[5];
            client.sendServerMessageArea(client.getSenderName(client.clientId()) + " moved to the previous statement.");
            if (l_progress == AreaData::TestimonyProgress::STAYED_AT_FIRST)
                client.sendServerMessage("First statement reached.");
        }

        QString l_decoded_message = client.decodeMessage(l_args[4]); // Get rid of that pesky encoding first.
        static QRegularExpression jump("(?<arrow>>)(?<int>[0,1,2,3,4,5,6,7,8,9]+)");
        QRegularExpressionMatch match = jump.match(l_decoded_message);
        if (match.hasMatch()) {
            client.m_pos = "wit";
            int jump_idx = match.captured("int").toInt();
            auto l_statement = area->jumpToStatement(jump_idx);
            l_args = l_statement.first;
            l_progress = l_statement.second;
            client.sendServerMessageArea(client.getSenderName(client.clientId()) + " jumped to statement number " + QString::number(jump_idx) + ".");

            switch (l_progress) {
            case AreaData::TestimonyProgress::LOOPED:
            {
                client.sendServerMessageArea("Last statement reached. Looping to first statement.");
                break;
            }
            case AreaData::TestimonyProgress::STAYED_AT_FIRST:
            {
                client.sendServerMessage("First statement reached.");
                Q_FALLTHROUGH();
            }
            case AreaData::TestimonyProgress::OK:
            default:
                // No need to handle.
                break;
            }
        }
    }

    if (area->autoMod()) {
        if (l_incoming_msg.isEmpty() && client.m_blankposts_row < 3)
            client.m_blankposts_row++;
        else if (!l_incoming_msg.isEmpty()) {
            client.autoMod(true, l_incoming_args[4].toString().size());
            client.m_blankposts_row = 0;
        }
        else
            client.autoMod(true);
    }

    if (area->autoCap() && !l_args[4].isEmpty()) {
        QStringList temp = l_args[4].split("");
        temp[1] = temp[1].toUpper();
        if (temp[temp.count() - 2] != "." && temp[temp.count() - 2] != "?" && temp[temp.count() - 2] != "!")
            temp.append(".");

        l_args[4] = temp.join("");
    }

    return PacketFactory::createPacket("MS", l_args);
}
