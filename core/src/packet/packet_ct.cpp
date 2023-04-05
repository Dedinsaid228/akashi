#include "include/packet/packet_ct.h"
#include "include/akashidefs.h"
#include "include/config_manager.h"
#include "include/packet/packet_factory.h"
#include "include/server.h"

#include <QDebug>

PacketCT::PacketCT(QStringList &contents) :
    AOPacket(contents)
{
}

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
    if (client.m_is_ooc_muted) {
        client.sendServerMessage("You are OOC muted, and cannot speak.");
        return;
    }

    if (client.m_wuso) {
        client.sendServerMessage("You cannot speak while you are affected by WUSO Mod.");
        return;
    }

    if (((area->oocType() == AreaData::OocType::INVITED && !area->invited().contains(client.m_id) && !client.checkPermission(ACLRole::GM)) || (area->oocType() == AreaData::OocType::CM && !client.checkPermission(ACLRole::CM))) && !m_content[1].startsWith("/")) {
        client.sendServerMessage("Only invited players or CMs can speak in this area.");
        return;
    }

    client.m_ooc_name = client.dezalgo(m_content[0]).replace(QRegularExpression("\\[|\\]|\\{|\\}|\\#|\\$|\\%|\\&"), ""); // no fucky wucky shit here
    if (client.m_ooc_name.isEmpty() || client.m_ooc_name == ConfigManager::serverName())                                 // impersonation & empty name protection
        return;

    if (client.m_ooc_name.length() > 30) {
        client.sendServerMessage("Your name is too long! Please limit it to under 30 characters.");
        return;
    }

    QString l_message = client.dezalgo(m_content[1]);
    QString l_ooc_name;

    if (client.m_authenticated && !client.m_sneak_mod)
        l_ooc_name = "[M]" + client.m_ooc_name;
    else
        l_ooc_name = client.m_ooc_name;

    if (l_message.length() == 0 || l_message.length() > ConfigManager::maxCharacters())
        return;
    std::shared_ptr<AOPacket> final_packet = PacketFactory::createPacket("CT", {l_ooc_name, l_message, "0"});
    if (l_message.at(0) == '/') {
        QStringList l_cmd_argv = l_message.split(" ", akashi::SkipEmptyParts);
        QString l_command = l_cmd_argv[0].trimmed().toLower();
        l_command = l_command.right(l_command.length() - 1);
        l_cmd_argv.removeFirst();
        int l_cmd_argc = l_cmd_argv.length();

        client.handleCommand(l_command, l_cmd_argc, l_cmd_argv);
        return;
    }
    else {
        if (l_message.size() > ConfigManager::maxCharacters()) {
            client.sendServerMessage("Your message is too long!");
            return;
        }

        if (area->chillMod() && l_message.size() > ConfigManager::maxCharactersChillMod()) {
            client.sendServerMessage("Your message is too long!");
            return;
        }

        client.getServer()->broadcast(final_packet, client.m_current_area);

        if (area->autoMod())
            client.autoMod();
    }
}

bool PacketCT::validatePacket() const
{
    // Nothing to validate.
    return true;
}
