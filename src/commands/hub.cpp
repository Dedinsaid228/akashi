#include "aoclient.h"
#include "config_manager.h"
#include "hub_data.h"
#include "packet/packet_factory.h"
#include "server.h"

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
                l_hubs_owners.append("[" + QString::number(l_owner->clientId()) + "] " + getSenderName(l_owner->clientId()));
            }

            if (!l_hubs_owners.isEmpty())
                l_hub_string += " [GM: " + l_hubs_owners.join(", ") + "]";

            hub_list.append(l_hub_string);
        }

        sendServerMessage("You are in the hub [" + QString::number(hubId()) + "] " + server->getHubName(hubId()) + "\nHub list:\n" + hub_list.join("\n"));
    }
    else {
        if (QDateTime::currentDateTime().toSecsSinceEpoch() - m_last_area_change_time <= 2) {
            sendServerMessage("You change an area or a hub very often!");
            return;
        }

        m_last_area_change_time = QDateTime::currentDateTime().toSecsSinceEpoch();
        bool ok;
        int l_new_hub = argv[0].toInt(&ok);
        bool l_sneaked = m_sneaked;
        if (!ok || l_new_hub >= server->getHubsCount() || l_new_hub < 0) {
            sendServerMessage("That does not look like a valid hub ID.");
            return;
        }

        if (hubId() == l_new_hub) {
            sendServerMessage("You are already in the hub [" + QString::number(hubId()) + "] " + server->getHubName(hubId()));
            return;
        }

        if (server->getHubById(l_new_hub)->hubLockStatus() == HubData::HubLockStatus::LOCKED && !server->getHubById(l_new_hub)->hubInvited().contains(clientId()) && !checkPermission(ACLRole::BYPASS_LOCKS)) {
            sendServerMessage("The hub [" + QString::number(l_new_hub) + "] " + server->getHubName(l_new_hub) + " is locked.");
            return;
        }

        if (server->getHubById(hubId())->hubOwners().contains(clientId())) {
            sendServerMessage("You cannot move to another hub while you are a GM.");
            return;
        }

        server->getHubById(hubId())->removeClient();
        setHubId(l_new_hub);
        getAreaList();
        QStringList l_areas = getServer()->getClientAreaNames(hubId());
        if (m_version.release == 2 && m_version.major >= 10) {
            for (int i = 0; i < l_areas.length(); i++)
                l_areas[i] = "[" + QString::number(i) + "] " + l_areas[i];
        }
        sendPacket("FA", l_areas);
        server->getHubById(hubId())->addClient();

        if (!l_sneaked)
            m_sneaked = true;

        changeArea(m_area_list[0], true);
        fullArup();

        if (!l_sneaked)
            m_sneaked = false;

        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients)
            l_client->arup(ARUPType::PLAYER_COUNT, false, l_client->hubId());

        sendServerMessage("Hub is changed to [" + QString::number(hubId()) + "] " + server->getHubName(hubId()) + ".");

        if (server->getHubById(hubId())->hubLockStatus() == HubData::HubLockStatus::SPECTATABLE)
            sendServerMessage("The hub [" + QString::number(hubId()) + "] " + server->getHubName(hubId()) + " is spectate-only; to chat IC you will need to be invited by the GM.");
    }
}

void AOClient::cmdGm(int argc, QStringList argv)
{
    HubData *l_hub = server->getHubById(hubId());
    if (l_hub->hubProtected()) {
        sendServerMessage("This hub is protected, you may not become GM.");
        return;
    }
    else if (l_hub->hubOwners().contains(clientId()) && argc == 0) {
        sendServerMessage("You are already a GM in this hub.");
        return;
    }
    else if (l_hub->hubOwners().isEmpty()) {
        l_hub->addHubOwner(clientId());
        l_hub->hubInvite(clientId());
        sendServerMessage("You is now GM in this hub.");
        sendServerMessageHub("[" + QString::number(clientId()) + "] " + getSenderName(clientId()) + " is now GM in this hub.");
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "NEW HUB OWNER", "Owner UID: " + QString::number(clientId()), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
    }
    else if (!l_hub->hubOwners().contains(clientId()))
        sendServerMessage("You cannot become a GM in this hub.");
    else if (argc == 1) {
        bool l_ok;
        AOClient *l_owner_candidate = server->getClientByID(argv[0].toInt(&l_ok));
        if (!l_ok) {
            sendServerMessage("That does not look like a valid ID.");
            return;
        }

        if (l_owner_candidate == nullptr) {
            sendServerMessage("Unable to find client with ID " + argv[0] + ".");
            return;
        }

        if (l_hub->hubOwners().contains(l_owner_candidate->clientId())) {
            sendServerMessage("This client are already a GM in this hub.");
            return;
        }

        l_hub->addHubOwner(l_owner_candidate->clientId());
        l_hub->hubInvite(l_owner_candidate->clientId());
        l_owner_candidate->sendServerMessage("You is now GM in this hub.");
        sendServerMessageHub("[" + QString::number(l_owner_candidate->clientId()) + "] " + getSenderName(l_owner_candidate->clientId()) + " is now GM in this hub.");
        emit logCMD((character() + " " + characterName()), m_ipid, name(), "NEW HUB OWNER", "Owner UID: " + QString::number(l_owner_candidate->clientId()), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
    }
    else
        sendServerMessage("You are already a GM in this hub.");
}

void AOClient::cmdUnGm(int argc, QStringList argv)
{
    HubData *l_hub = server->getHubById(hubId());
    int l_uid;

    if (l_hub->hubOwners().isEmpty()) {
        sendServerMessage("There are no GMs in this hub.");
        return;
    }
    else if (argc == 0) {
        l_uid = clientId();
        if (!l_hub->hubOwners().contains(l_uid)) {
            sendServerMessage("You are not the GM of this hub!");
            return;
        }

        emit logCMD((character() + " " + characterName()), m_ipid, name(), "REMOVE HUB OWNER", "Owner UID: " + QString::number(clientId()), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
        sendServerMessageHub("[" + QString::number(clientId()) + "] " + getSenderName(clientId()) + " no longer GM in this hub.");
        sendServerMessage("You are no longer a GM in this hub.");
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

        emit logCMD((character() + " " + characterName()), m_ipid, name(), "REMOVE AREA OWNER", "Owner UID: " + QString::number(target->clientId()), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
        sendServerMessageHub("[" + QString::number(target->clientId()) + "] " + getSenderName(target->clientId()) + " no longer GM in this hub.");
        target->sendServerMessage("You have been unGMed.");
        target->m_hub_listen = false;
    }

    l_hub->removeHubOwner(l_uid);
    sendEvidenceList(server->getAreaById(areaId()));
}

void AOClient::cmdHubProtected(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    HubData *l_hub = server->getHubById(hubId());
    l_hub->toggleHubProtected();
    QString l_state = l_hub->hubProtected() ? "protected." : "not protected.";
    sendServerMessage("This hub is now " + l_state);
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "TOGGLEHUBPROTECTED", l_state, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdHidePlayerCount(int argc, QStringList argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    HubData *l_hub = server->getHubById(hubId());
    l_hub->toggleHidePlayerCount();
    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        l_client->fullArup();

    QString l_state = l_hub->getHidePlayerCount() ? "hid." : "not hid.";
    sendServerMessage("Player count in this hub is now " + l_state);
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "TOGGLEHUB PLAYERCOUNT HID", l_state, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdHubRename(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    QString l_hub_name = dezalgo(argv.join(" "));
    if (server->getAreaNames().contains(l_hub_name)) {
        sendServerMessage("A hub with that name already exists.");
        return;
    }

    emit logCMD((character() + " " + characterName()), m_ipid, name(), "RENAMEHUB", l_hub_name, server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
    server->renameHub(l_hub_name, hubId());
    sendServerMessage("This hub has been renamed.");
    return;
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

    HubData *l_hub = server->getHubById(hubId());
    if (l_hub->hubLockStatus() == HubData::HubLockStatus::FREE) {
        sendServerMessage("This hub is not locked.");
        return;
    }

    sendServerMessageHub("This hub is now unlocked.");
    l_hub->hubUnlock();
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "HUBUNLOCK", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdHubSpectate(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    HubData *l_hub = server->getHubById(hubId());
    if (l_hub->hubLockStatus() == HubData::HubLockStatus::SPECTATABLE) {
        sendServerMessage("This hub is already in spectate mode.");
        return;
    }

    sendServerMessageHub("This hub is now spectatable.");
    l_hub->hubSpectatable();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        if (l_client->hubId() == hubId())
            l_hub->hubInvite(l_client->clientId());

    emit logCMD((character() + " " + characterName()), m_ipid, name(), "HUBSPECTATABLE", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdHubLock(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    HubData *l_hub = server->getHubById(hubId());
    if (l_hub->hubLockStatus() == HubData::HubLockStatus::LOCKED) {
        sendServerMessage("This hub is already locked.");
        return;
    }

    sendServerMessageHub("This hub is now locked.");
    l_hub->hubLock();

    const QVector<AOClient *> l_clients = server->getClients();
    for (AOClient *l_client : l_clients)
        if (l_client->hubId() == hubId())
            l_hub->hubInvite(l_client->clientId());

    emit logCMD((character() + " " + characterName()), m_ipid, name(), "HUBLOCK", "", server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdHubInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    HubData *l_hub = server->getHubById(hubId());
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
    target_client->sendServerMessage("You were invited and given access to the hub " + server->getHubName(hubId()));
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "HUBIVNITE", "Invited UID: " + QString::number(target_client->clientId()), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdHubUnInvite(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    HubData *l_hub = server->getHubById(hubId());
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
    target_client->sendServerMessage("You were uninvited from hub " + server->getHubName(hubId()));
    emit logCMD((character() + " " + characterName()), m_ipid, name(), "HUBUNIVNITE", "Uninvited UID: " + QString::number(target_client->clientId()), server->getAreaById(areaId())->name(), QString::number(clientId()), m_hwid, QString::number(hubId()));
}

void AOClient::cmdGHub(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    if (m_is_ooc_muted) {
        sendServerMessage("You are OOC muted, and cannot speak.");
        return;
    }

    QString l_sender_message = argv.join(" ");
    if (l_sender_message.size() > ConfigManager::maxCharacters()) {
        sendServerMessage("Your message is too long!");
        return;
    }

    if (!m_blinded) {
        const QVector<AOClient *> l_clients = server->getClients();
        for (AOClient *l_client : l_clients)
            if (l_client->m_global_enabled && !l_client->m_blinded && l_client->hubId() == hubId())
                l_client->sendPacket("CT", {"[HUB MESSAGE][" + server->getAreaName(areaId()) + "]" + name(), l_sender_message});
    }
    else
        sendPacket("CT", {"[HUB MESSAGE][" + server->getAreaName(areaId()) + "]" + name(), l_sender_message});

    emit logCMD((character() + " " + characterName()), m_ipid, name(), "HUBGLOBALCHAT", l_sender_message, server->getAreaName(areaId()), QString::number(clientId()), m_hwid, QString::number(hubId()));
    return;
}
