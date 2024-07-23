#include "playerstateobserver.h"

PlayerStateObserver::PlayerStateObserver(QObject *parent) :
    QObject{parent}
{}

PlayerStateObserver::~PlayerStateObserver() {}

void PlayerStateObserver::registerClient(AOClient *client)
{
    Q_ASSERT(!m_client_list.contains(client));

    std::shared_ptr<AOPacket> packet = std::make_shared<PacketPR>(client->clientId(), PacketPR::ADD);
    sendToClientList(packet);

    m_client_list.append(client);

    connect(client, &AOClient::nameChanged, this, &PlayerStateObserver::notifyNameChanged);
    connect(client, &AOClient::characterChanged, this, &PlayerStateObserver::notifyCharacterChanged);
    connect(client, &AOClient::characterNameChanged, this, &PlayerStateObserver::notifyCharacterNameChanged);
    connect(client, &AOClient::areaIdChanged, this, &PlayerStateObserver::notifyAreaIdChanged);

    QList<std::shared_ptr<AOPacket>> packets;
    for (AOClient *i_client : std::as_const(m_client_list)) {
        packets.append(std::make_shared<PacketPR>(i_client->clientId(), PacketPR::ADD));
        packets.append(std::make_shared<PacketPU>(i_client->clientId(), PacketPU::NAME, i_client->name()));
        packets.append(std::make_shared<PacketPU>(i_client->clientId(), PacketPU::CHARACTER, i_client->character()));
        packets.append(std::make_shared<PacketPU>(i_client->clientId(), PacketPU::CHARACTER_NAME, i_client->characterName()));
        packets.append(std::make_shared<PacketPU>(i_client->clientId(), PacketPU::AREA_ID, i_client->areaId()));
    }

    for (std::shared_ptr<AOPacket> packet : std::as_const(packets)) {
        client->sendPacket(packet);
        // delete packet;
    }
}

void PlayerStateObserver::unregisterClient(AOClient *client)
{
    Q_ASSERT(m_client_list.contains(client));

    disconnect(client, nullptr, this, nullptr);

    m_client_list.removeAll(client);

    std::shared_ptr<AOPacket> packet = std::make_shared<PacketPR>(client->clientId(), PacketPR::REMOVE);
    sendToClientList(packet);
}

void PlayerStateObserver::sendToClientList(std::shared_ptr<AOPacket> packet)
{
    for (AOClient *client : std::as_const(m_client_list)) {
        client->sendPacket(packet);
    }
}

void PlayerStateObserver::notifyNameChanged(const QString &name) { sendToClientList(std::make_shared<PacketPU>(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::NAME, name)); }

void PlayerStateObserver::notifyCharacterChanged(const QString &character) { sendToClientList(std::make_shared<PacketPU>(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::CHARACTER, character)); }

void PlayerStateObserver::notifyCharacterNameChanged(const QString &characterName) { sendToClientList(std::make_shared<PacketPU>(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::CHARACTER_NAME, characterName)); }

void PlayerStateObserver::notifyAreaIdChanged(int areaId) { sendToClientList(std::make_shared<PacketPU>(qobject_cast<AOClient *>(sender())->clientId(), PacketPU::AREA_ID, areaId)); }
