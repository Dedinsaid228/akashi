#include "packet/packet_ct.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

#include <QDebug>

PacketCT::PacketCT(QStringList &contents) :
    AOPacket(contents)
{}

PacketInfo PacketCT::getPacketInfo() const
{
    PacketInfo info{
        .acl_permission = ACLRole::Permission::NONE,
        .min_args = 2,
        .header = "CT"};
    return info;
}

void PacketCT::handlePacket(AreaData *area, AOClient &client) const
{
    if (client.m_is_ooc_muted && !m_content[1].startsWith("/")) {
        client.sendServerMessage("You are OOC muted, and cannot speak.");
        return;
    }

    if (client.m_wuso) {
        client.sendServerMessage("You cannot speak while you are affected by WUSO.");
        return;
    }

    if (((area->oocType() == AreaData::OocType::INVITED && !area->invited().contains(client.clientId()) && !client.checkPermission(ACLRole::GM)) || (area->oocType() == AreaData::OocType::CM && !client.checkPermission(ACLRole::CM))) && !m_content[1].startsWith("/")) {
        client.sendServerMessage("Only invited players or CMs can speak in this area.");
        return;
    }

    static QRegularExpression re("\\[|\\]|\\{|\\}|\\#|\\$|\\%|\\&");
    client.setName(client.dezalgo(m_content[0]).replace(re, ""));                                                               // no fucky wucky shit here
    if (client.name().isEmpty() || client.name() == ConfigManager::serverName() || client.name() == ConfigManager::serverTag()) // impersonation & empty name protection
        return;

    if (client.name().length() > 30) {
        client.sendServerMessage("Your name is too long! Please limit it to under 30 characters.");
        return;
    }

    QString l_message = client.dezalgo(m_content[1]);
    QString l_ooc_name = client.name();
    if (l_message.length() == 0)
        return;

    if (!ConfigManager::filterList().isEmpty()) {
        foreach (const QString &regex, ConfigManager::filterList()) {
            QRegularExpression re(regex, QRegularExpression::CaseInsensitiveOption);
            l_message.replace(re, "❌");
        }
    }

    std::shared_ptr<AOPacket> final_packet = PacketFactory::createPacket("CT", {l_ooc_name, l_message, "0"});
    if (l_message.at(0) == '/') {
        QStringList l_cmd_argv = l_message.split(" ", Qt::SkipEmptyParts);
        QString l_command = l_cmd_argv[0].trimmed().toLower();
        l_command = l_command.right(l_command.length() - 1);
        l_cmd_argv.removeFirst();
        int l_cmd_argc = l_cmd_argv.length();
        client.handleCommand(l_command, l_cmd_argc, l_cmd_argv);

        if (l_command == "g" || l_command == "g_hub" || l_command == "need")
            client.autoMod();

        return;
    }
    else {
        if (l_message.size() > ConfigManager::maxCharacters() || (area->chillMod() && l_message.size() > ConfigManager::maxCharactersChillMod())) {
            client.sendServerMessage("Your message is too long!");
            return;
        }

        if (!client.m_blinded)
            client.getServer()->broadcast(final_packet, client.areaId());
        else
            client.sendPacket(final_packet);

        if (area->autoMod())
            client.autoMod();

        emit client.logOOC((client.character() + " " + client.characterName()), client.name(), client.m_ipid, area->name(), l_message, QString::number(client.clientId()), client.m_hwid, client.getServer()->getHubName(client.hubId()));
    }
}
