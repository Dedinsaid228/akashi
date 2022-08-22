#include "include/aoclient.h"
#include "include/config_manager.h"
#include "include/db_manager.h"
#include "include/server.h"


void AOClient::autoMod(bool ic_chat)
{
    int l_warn = server->getDatabaseManager()->getWarnNum(m_ipid);
    long l_currentdate = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (QDateTime::currentDateTime().toSecsSinceEpoch() - server->getDatabaseManager()->getWarnDate(m_ipid)
            > parseTime(ConfigManager::autoModWarnTerm()) && l_warn != 0 && l_warn != 1) {
        long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
        server->getDatabaseManager()->updateWarn(m_ipid, l_warn - 1);
        server->getDatabaseManager()->updateWarn(m_ipid, l_date);
    }

    if (l_currentdate - m_lastmessagetime < ConfigManager::autoModTrigger()) {

        if (l_warn == 0) {
            DBManager::automodwarns l_warn;
            l_warn.ipid = m_ipid;
            l_warn.date = QDateTime::currentDateTime().toSecsSinceEpoch();
            l_warn.warns = 2;

            server->getDatabaseManager()->addWarn(l_warn);
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(5) + " more warn, then you will be punished.");
            return;
        }

        if (l_warn <= 5) {
            long date = QDateTime::currentDateTime().toSecsSinceEpoch();

            server->getDatabaseManager()->updateWarn(m_ipid, l_warn + 1);
            server->getDatabaseManager()->updateWarn(m_ipid, date);
            sendServerMessage("You got a warn from an automoderator! If you get " + QString::number(6 - l_warn) + " more warn, then you will be punished.");
            return;
        }

        int l_haznum = server->getDatabaseManager()->getHazNum(m_ipid);

        switch (l_haznum) {
            case 0:
            case 1: autoMute(ic_chat, l_haznum); break;
            case 2: autoKick(); break;
            case 3: autoBan(); break;
        }
    }

    m_lastmessagetime = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

void AOClient::autoMute(bool ic_chat, int haznum)
{
    AOClient* target = server->getClientByID(m_id);

    if (ic_chat)
        target->m_is_muted = true;
    else
        target->m_is_ooc_muted = true;

    DBManager::automod l_num;
    l_num.ipid = m_ipid;
    l_num.date = QDateTime::currentDateTime().toSecsSinceEpoch();
    l_num.action = "MUTE";
    l_num.haznum = 2;

    if (haznum == 0)
       server->getDatabaseManager()->addHazNum(l_num);
    else {
        long date = l_num.date;

        emit logCMD("Automoderator","", "","MUTE","Muted UID: " + QString::number(target->m_id),server->getAreaById(m_current_area)->name(), "", "");
        server->getDatabaseManager()->updateHazNum(m_ipid, date);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_num.action);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_num.haznum);
    }
}

void AOClient::autoKick()
{
    const QList<AOClient*> l_targets = server->getClientsByIpid(m_ipid);
    for (AOClient* l_client : l_targets) {
        l_client->sendPacket("KK", {"You were kicked by a automoderator."});
        l_client->m_socket->close();
    }

    emit logKick("Automoderator", m_ipid, "You were kicked by a automoderator.", "", "");
    long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
    QString l_action = "KICK";
    int l_haznum = 3;

    server->getDatabaseManager()->updateHazNum(m_ipid, l_date);
    server->getDatabaseManager()->updateHazNum(m_ipid, l_action);
    server->getDatabaseManager()->updateHazNum(m_ipid, l_haznum);
}

void AOClient::autoBan()
{
    DBManager::BanInfo l_ban;

    long long l_duration_seconds = 0;
    QString l_duration_ban = ConfigManager::autoModBanDuration();

    if (l_duration_ban == "perma")
        l_duration_seconds = -2;
    else
        l_duration_seconds = parseTime(l_duration_ban);

    if (l_duration_seconds == -1) {
        qDebug() << "ERROR: Invalid ban time format for automoderator! Format example: 1h30m";
        return;
    }

    l_ban.duration = l_duration_seconds;
    l_ban.ipid = m_ipid;
    l_ban.reason = "You were banned by a automoderator.";
    l_ban.moderator = "Automoderator";
    l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
    bool ban_logged = false;

    const QList<AOClient*> l_targets = server->getClientsByIpid(l_ban.ipid);
    for (AOClient* l_client : l_targets) {
        if (!ban_logged) {
            l_ban.ip = l_client->m_remote_ip;
            l_ban.hdid = l_client->m_hwid;
            server->getDatabaseManager()->addBan(l_ban);
            ban_logged = true;
        }

        QString l_ban_duration;

        if (!(l_ban.duration == -2)) {
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        }
        else {
            l_ban_duration = "The heat death of the universe.";
        }

        int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        l_client->sendPacket("KB", {l_ban.reason + "\nID: " + QString::number(l_ban_id) + "\nUntil: " + l_ban_duration});
        l_client->m_socket->close();

        emit logBan(l_ban.moderator,l_ban.ipid,l_ban_duration,l_ban.reason, "","");
        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(l_ban.ipid, l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);

        long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
        QString l_action = "BAN";

        server->getDatabaseManager()->updateHazNum(m_ipid, l_date);
        server->getDatabaseManager()->updateHazNum(m_ipid, l_action);
    }
}
