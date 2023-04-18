#include "include/aoclient.h"
#include "include/config_manager.h"
#include "include/hub_data.h"
#include "include/packet/packet_factory.h"
#include "include/server.h"

void AOClient::cmdHub(int argc, QStringList argv)
{
    if (argc == 0) {
        QStringList hub_list;
        for (int i = 0; i < server->getHubsCount(); i++) {
            HubData *l_hub = server->getHubById(i);
            QString l_playercount;
            if (l_hub->getHidePlayerCount())
                l_playercount = "?";
            else
                l_playercount = QString::number(l_hub->getHubPlayerCount());

            QString l_hub_string = "[" + QString::number(i) + "] " + server->getHubName(i) + " with " + l_playercount + " players. [" + getHubLockStatus(i) + "]";
            QStringList l_hubs_owners;
            const QList<int> l_owner_ids = l_hub->hubOwners();
            for (int l_owner_id : l_owner_ids) {
                AOClient *l_owner = server->getClientByID(l_owner_id);
                QString l_sender_name = getSenderName(l_owner->m_id);
                l_hubs_owners.append("[" + QString::number(l_owner->m_id) + "] " + l_sender_name);
            }
            if (!l_hubs_owners.isEmpty())
                l_hub_string += " [GM: " + l_hubs_owners.join(", ") + "]";

            hub_list.append(l_hub_string);
        }

        sendServerMessage("You are in hub [" + QString::number(m_hub) + "] " + server->getHubName(m_hub) + "\nHub list:\n" + hub_list.join("\n"));
    }
    else {
        bool ok;
        int l_new_hub = argv[0].toInt(&ok);
        bool l_sneaked = m_sneaked;

        if (!ok || l_new_hub >= server->getHubsCount() || l_new_hub < 0) {
            sendServerMessage("That does not look like a valid hub ID.");
            return;
        }

        if (m_hub == l_new_hub) {
            sendServerMessage("You are already in hub [" + QString::number(m_hub) + "] " + server->getHubName(m_hub));
            return;
        }

        if (server->getHubById(l_new_hub)->hubLockStatus() == HubData::HubLockStatus::LOCKED && !server->getHubById(l_new_hub)->hubInvited().contains(m_id) && !checkPermission(ACLRole::BYPASS_LOCKS)) {
            sendServerMessage("Hub [" + QString::number(l_new_hub) + "] " + server->getHubName(l_new_hub) + " is locked.");
            return;
        }

        if (server->getHubById(m_hub)->hubOwners().contains(m_id)) {
            sendServerMessage("You cannot move to another hub while you are a GM.");
            return;
        }

        server->getHubById(m_hub)->clientLeftHub();
        m_hub = l_new_hub;
        getAreaList();
        sendPacket("FA", getServer()->getClientAreaNames(m_hub));
        server->getHubById(m_hub)->clientJoinedHub();

        if (!l_sneaked)
            m_sneaked = true;

        changeArea(m_area_list[0], true);

        if (!l_sneaked)
            m_sneaked = false;

        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients)
            l_client->fullArup();

        sendServerMessage("Hub changed.");

        if (server->getHubById(m_hub)->hubLockStatus() == HubData::HubLockStatus::SPECTATABLE)
            sendServerMessage("hub " + server->getHubName(m_hub) + " is spectate-only; to chat IC you will need to be invited by the GM.");
    }
}

void AOClient::cmdGm(int argc, QStringList argv)
{
    HubData *l_hub = server->getHubById(m_hub);

    if (l_hub->hubProtected()) {
        sendServerMessage("This hub is protected, you may not become GM.");
        return;
    }
    else if (l_hub->hubOwners().contains(m_id) && argc == 0) {
        sendServerMessage("You are already a GM in this hub.");
        return;
    }
    else if (l_hub->hubOwners().isEmpty()) {
        l_hub->addHubOwner(m_id);
        l_hub->hubInvite(m_id);
        sendServerMessage("You is now GM in this hub.");
        QString l_sender_name = getSenderName(m_id);
        sendServerMessageHub("[" + QString::number(m_id) + "] " + l_sender_name + " is now GM in this hub.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "NEW HUB OWNER", "Owner UID: " + QString::number(m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
    }
    else if (!l_hub->hubOwners().contains(m_id)) {
        sendServerMessage("You cannot become a GM in this hub.");
    }
    else if (argc == 1) {
        bool l_ok;
        AOClient *l_owner_candidate = server->getClientByID(argv[0].toInt(&l_ok));
        if (!l_ok) {
            sendServerMessage("That doesn't look like a valid ID.");
            return;
        }
        if (l_owner_candidate == nullptr) {
            sendServerMessage("Unable to find client with ID " + argv[0] + ".");
            return;
        }
        if (l_hub->hubOwners().contains(l_owner_candidate->m_id)) {
            sendServerMessage("This client are already a GM in this hub.");
            return;
        }
        l_hub->addHubOwner(l_owner_candidate->m_id);
        l_hub->hubInvite(l_owner_candidate->m_id);
        l_owner_candidate->sendServerMessage("You is now GM in this hub.");
        QString l_sender_name = getSenderName(l_owner_candidate->m_id);
        sendServerMessageHub("[" + QString::number(l_owner_candidate->m_id) + "] " + l_sender_name + " is now GM in this hub.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "NEW HUB OWNER", "Owner UID: " + QString::number(l_owner_candidate->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
    }
    else
        sendServerMessage("You are already a GM in this hub.");
}

void AOClient::cmdUnGm(int argc, QStringList argv)
{
    HubData *l_hub = server->getHubById(m_hub);
    int l_uid;

    if (l_hub->hubOwners().isEmpty()) {
        sendServerMessage("There are no GMs in this hub.");
        return;
    }
    else if (argc == 0) {
        l_uid = m_id;

        if (!l_hub->hubOwners().contains(l_uid)) {
            sendServerMessage("You are not the GM of this hub!");
            return;
        }

        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "REMOVE HUB OWNER", "Owner UID: " + QString::number(m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
        QString l_sender_name = getSenderName(m_id);
        sendServerMessageHub("[" + QString::number(m_id) + "] " + l_sender_name + " no longer GM in this hub.");
        sendServerMessage("You are no longer GM in this hub.");
        m_hub_listen = false;
    }
    else {
        bool l_conv_ok = false;
        l_uid = argv[0].toInt(&l_conv_ok);

        if (!l_conv_ok) {
            sendServerMessage("Invalid user ID.");
            return;
        }

        if (!l_hub->hubOwners().contains(l_uid)) {
            sendServerMessage("That user is not GMed.");
            return;
        }

        AOClient *target = server->getClientByID(l_uid);

        if (target == nullptr) {
            sendServerMessage("No client with that ID found.");
            return;
        }

        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "REMOVE AREA OWNER", "Owner UID: " + QString::number(target->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
        QString l_sender_name = getSenderName(target->m_id);
        sendServerMessageHub("[" + QString::number(target->m_id) + "] " + l_sender_name + " no longer GM in this hub.");
        target->sendServerMessage("You have been unGMed.");
        target->m_hub_listen = false;
    }

    l_hub->removeHubOwner(l_uid);
    sendEvidenceList(server->getAreaById(m_current_area));
}

void AOClient::cmdHubProtected(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    HubData *l_hub = server->getHubById(m_hub);

    l_hub->toggleHubProtected();

    QString l_state = l_hub->hubProtected() ? "protected." : "not protected.";

    sendServerMessage("This hub is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEHUBPROTECTED", l_state, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdHidePlayerCount(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    HubData *l_hub = server->getHubById(m_hub);

    l_hub->toggleHidePlayerCount();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        l_client->fullArup();

    QString l_state = l_hub->getHidePlayerCount() ? "hided." : "not hided.";

    sendServerMessage("Player count in this hub is now " + l_state);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "TOGGLEHUB PLAYERCOUNT HIDED", l_state, server->getAreaName(m_current_area), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdHubRename(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    for (int i = 0; i < server->getHubsCount(); i++) {
        QString l_hub = server->getHubName(i);
        QString l_hub_name = dezalgo(argv.join(" "));

        if (server->getAreaNames().contains(l_hub_name)) {
            sendServerMessage("An hub with that name already exists!");
            return;
        }

        if (l_hub == server->getHubName(m_hub)) {
            emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "RENAMEHUB", l_hub_name, server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
            server->renameHub(l_hub_name, i);
            sendServerMessage("Hub has been renamed!");
            return;
        }
    }

    sendServerMessage("For unknown reasons hub could not be renamed.");
}

void AOClient::cmdHubListening(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    m_hub_listen = !m_hub_listen;

    QString l_state = m_hub_listen ? "listening" : "not listening";

    sendServerMessage("You are " + l_state + " to this hub.");
}

void AOClient::cmdHubUnlock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    HubData *l_hub = server->getHubById(m_hub);

    if (l_hub->hubLockStatus() == HubData::HubLockStatus::FREE) {
        sendServerMessage("This hub is not locked.");
        return;
    }

    sendServerMessageArea("This hub is now unlocked.");
    l_hub->hubUnlock();
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "HUBUNLOCK", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdHubSpectate(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    HubData *l_hub = server->getHubById(m_hub);

    if (l_hub->hubLockStatus() == HubData::HubLockStatus::SPECTATABLE) {
        sendServerMessage("This hub is already in spectate mode.");
        return;
    }

    sendServerMessageArea("This hub is now spectatable.");
    l_hub->hubSpectatable();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_hub == m_hub) {
            l_hub->hubInvite(l_client->m_id);
        }
    }

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "HUBSPECTATABLE", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdHubLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    HubData *l_hub = server->getHubById(m_hub);

    if (l_hub->hubLockStatus() == HubData::HubLockStatus::LOCKED) {
        sendServerMessage("This hub is already locked.");
        return;
    }

    sendServerMessageArea("This hub is now locked.");
    l_hub->hubLock();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_hub == m_hub) {
            l_hub->hubInvite(l_client->m_id);
        }
    }

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "HUBLOCK", "", server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdHubInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    HubData *l_hub = server->getHubById(m_hub);
    bool l_ok;
    int l_invited_id = argv[0].toInt(&l_ok);

    if (!l_ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *target_client = server->getClientByID(l_invited_id);

    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (!l_hub->hubInvite(l_invited_id)) {
        sendServerMessage("That ID is already on the invite list.");
        return;
    }

    sendServerMessage("You invited ID " + argv[0]);
    target_client->sendServerMessage("You were invited and given access to hub " + server->getHubName(m_hub));
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "HUBIVNITE", "Invited UID: " + QString::number(target_client->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdHubUnInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    HubData *l_hub = server->getHubById(m_hub);
    bool l_ok;
    int l_uninvited_id = argv[0].toInt(&l_ok);

    if (!l_ok) {
        sendServerMessage("That does not look like a valid ID.");
        return;
    }

    AOClient *target_client = server->getClientByID(l_uninvited_id);

    if (target_client == nullptr) {
        sendServerMessage("No client with that ID found.");
        return;
    }
    else if (l_hub->hubOwners().contains(l_uninvited_id)) {
        sendServerMessage("You cannot uninvite a GM!");
        return;
    }
    else if (!l_hub->hubUninvite(l_uninvited_id)) {
        sendServerMessage("That ID is not on the invite list.");
        return;
    }

    sendServerMessage("You uninvited ID " + argv[0]);
    target_client->sendServerMessage("You were uninvited from hub " + server->getHubName(m_hub));
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "HUBUNIVNITE", "Uninvited UID: " + QString::number(target_client->m_id), server->getAreaById(m_current_area)->name(), QString::number(m_id), m_hwid, QString::number(m_hub));
}

void AOClient::cmdGHub(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_sender_name = m_ooc_name;
    QString l_sender_area = server->getAreaName(m_current_area);
    QString l_sender_message = argv.join(" ");
    bool l_sender_auth = m_authenticated;
    bool l_sender_sneak = m_sneak_mod;
    QString l_areaname = "[G][HUB MESSAGE][" + l_sender_area + "]";

    if (l_sender_message.size() > ConfigManager::maxCharacters()) {
        sendServerMessage("Your message is too long!");
        return;
    }

    if (l_sender_auth && !l_sender_sneak)
        l_areaname += "[M]";

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients) {
        if (l_client->m_global_enabled && l_client->m_hub == m_hub)
            l_client->sendPacket("CT", {l_areaname + l_sender_name, l_sender_message});
    }

    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "HUBGLOBALCHAT", l_sender_message, l_sender_area, QString::number(m_id), m_hwid, QString::number(m_hub));
    return;
}
