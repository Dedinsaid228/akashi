#ifndef HUB_DATA_H
#define HUB_DATA_H

#include <QSettings>
#include <QString>

class HubData : public QObject
{
    Q_OBJECT

  public:
    HubData(QString p_name, int p_index);

    enum HubLockStatus
    {
        FREE,
        LOCKED,
        SPECTATABLE
    };
    Q_ENUM(HubLockStatus);

    QList<int> hubOwners() const;

    void addHubOwner(int f_clientId);

    QList<int> findHubOwner() const;

    bool removeHubOwner(int f_clientId);

    bool hubProtected() const;

    void toggleHubProtected();

    void clientLeftHub();

    void clientJoinedHub();

    int getHubPlayerCount() const;

    bool getHidePlayerCount() const;

    void toggleHidePlayerCount();

    HubLockStatus hubLockStatus() const;

    void hubLock();

    void hubUnlock();

    void hubSpectatable();

    QList<int> hubInvited() const;

    bool hubInvite(int f_clientId);

    bool hubUninvite(int f_clientId);

  private:
    QString m_hub_name;

    int m_hub_index;

    QList<int> m_hub_owners;

    bool m_hub_protected;

    int m_hub_player_count = 0;

    bool m_hub_player_count_hide;

    HubLockStatus m_hub_locked;

    QList<int> m_hub_invited;
};

#endif // HUB_DATA_H
