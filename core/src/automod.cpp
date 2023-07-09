#include "include/aoclient.h"
#include "include/config_manager.h"
#include "include/db_manager.h"
#include "include/server.h"

void AOClient::autoMod(bool ic_chat, int chars)
{
    int l_warn = server->getDatabaseManager()->getWarnNum(m_ipid);
    long l_currentdate = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (QDateTime::currentDateTime().toSecsSinceEpoch() - server->getDatabaseManager()->getWarnDate(m_ipid) > parseTime(ConfigManager::autoModWarnTerm()) && l_warn > 0) {
        long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
        server->getDatabaseManager()->updateWarn(m_ipid, l_warn - 1);
        server->getDatabaseManager()->updateWarn(m_ipid, l_date);
    }

    if ((ic_chat && m_lastmessagetime == 0) || (!ic_chat && m_lastoocmessagetime == 0)) {
        updateLastTime(ic_chat, chars);
        return;
    }

    if ((l_currentdate - m_lastmessagetime < ConfigManager::autoModTrigger() + m_lastmessagechars * 0.046875 * 1000 && ic_chat) ||
        (l_currentdate - m_lastoocmessagetime < ConfigManager::autoModOocTrigger() && !ic_chat)) {
        if (l_warn < ConfigManager::autoModWarns()) {
            if (server->getDatabaseManager()->warnExist(m_ipid)) {
                long date = QDateTime::currentDateTime().toSecsSinceEpoch();
                server->getDatabaseManager()->updateWarn(m_ipid, l_warn + 1);
                server->getDatabaseManager()->updateWarn(m_ipid, date);
            }
            else {
                DBManager::automodwarns warn;
                warn.ipid = m_ipid;
                warn.date = QDateTime::currentDateTime().toSecsSinceEpoch();
                warn.warns = 2;
                server->getDatabaseManager()->addWarn(warn);
            }

            sendServerMessage("You got a warn from the Automod! If you get " + QString::number(3 - l_warn) + " warns, you will be punished.");
            updateLastTime(ic_chat, chars);
        }
        else {
            switch (server->getDatabaseManager()->getHazNum(m_ipid)) {
            case 0:
                autoMute(ic_chat);
                break;
            case 1:
                autoKick();
                break;
            case 2:
                autoBan();
                break;
            }
        }
    }

    updateLastTime(ic_chat, chars);
}

void AOClient::updateLastTime(bool ic_chat, int chars)
{
    if (ic_chat) {
        m_lastmessagetime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        m_lastmessagechars = chars;
    }
    else
        m_lastoocmessagetime = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

void AOClient::autoMute(bool ic_chat)
{
    AOClient *target = server->getClientByID(m_id);
    if (ic_chat)
        target->m_is_muted = true;
    else
        target->m_is_ooc_muted = true;

    emit logCMD("Automod", "", "", "MUTE", "Muted UID: " + QString::number(target->m_id), server->getAreaById(m_current_area)->name(), "", "", "");

    if (server->getDatabaseManager()->hazNumExist(m_ipid)) {
        long date = QDateTime::currentDateTime().toSecsSinceEpoch();
        server->getDatabaseManager()->updateHazNum(m_ipid, date);
        server->getDatabaseManager()->updateHazNum(m_ipid, "MUTE");
        server->getDatabaseManager()->updateHazNum(m_ipid, 1);
    }
    else {
        DBManager::automod l_num;
        l_num.ipid = m_ipid;
        l_num.date = QDateTime::currentDateTime().toSecsSinceEpoch();
        l_num.action = "MUTE";
        l_num.haznum = 1;
        server->getDatabaseManager()->addHazNum(l_num);
    }
}

void AOClient::autoKick()
{
    const QList<AOClient *> l_targets = server->getClientsByIpid(m_ipid);
    for (AOClient *l_client : l_targets) {
        l_client->sendPacket("KK", {"You were kicked by the Automod."});
        l_client->m_socket->close();
    }

    emit logKick("Automod", m_ipid, "You were kicked by the Automod.", "", "");

    long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
    server->getDatabaseManager()->updateHazNum(m_ipid, l_date);
    server->getDatabaseManager()->updateHazNum(m_ipid, "KICK");
    server->getDatabaseManager()->updateHazNum(m_ipid, 2);
}

void AOClient::autoBan()
{
    long long l_duration_seconds = 0;
    QString l_duration_ban = ConfigManager::autoModBanDuration();
    if (l_duration_ban == "perma")
        l_duration_seconds = -2;
    else
        l_duration_seconds = parseTime(l_duration_ban);

    if (l_duration_seconds == -1) {
        qDebug() << "ERROR: Invalid ban time format for the Automod! Format example: 1h30m";
        return;
    }

    DBManager::BanInfo l_ban;
    l_ban.duration = l_duration_seconds;
    l_ban.ipid = m_ipid;
    l_ban.reason = "You were banned by the Automod.";
    l_ban.moderator = "Automod";
    l_ban.time = QDateTime::currentDateTime().toSecsSinceEpoch();
    bool ban_logged = false;

    const QList<AOClient *> l_targets = server->getClientsByIpid(l_ban.ipid);
    for (AOClient *l_client : l_targets) {
        if (!ban_logged) {
            l_ban.ip = l_client->m_remote_ip;
            l_ban.hdid = l_client->m_hwid;
            server->getDatabaseManager()->addBan(l_ban);
            ban_logged = true;
        }

        QString l_ban_duration;
        if (!(l_ban.duration == -2))
            l_ban_duration = QDateTime::fromSecsSinceEpoch(l_ban.time).addSecs(l_ban.duration).toString("dd/MM/yyyy, hh:mm");
        else
            l_ban_duration = "The heat death of the universe.";

        int l_ban_id = server->getDatabaseManager()->getBanID(l_ban.ip);
        l_client->sendPacket("KB", {l_ban.reason + "\nID: " + QString::number(l_ban_id) + "\nUntil: " + l_ban_duration});
        l_client->m_socket->close();

        emit logBan(l_ban.moderator, l_ban.ipid, l_ban_duration, l_ban.reason, "", "");

        if (ConfigManager::discordBanWebhookEnabled())
            emit server->banWebhookRequest(l_ban.ipid, l_ban.moderator, l_ban_duration, l_ban.reason, l_ban_id);

        long l_date = QDateTime::currentDateTime().toSecsSinceEpoch();
        server->getDatabaseManager()->updateHazNum(m_ipid, l_date);
        server->getDatabaseManager()->updateHazNum(m_ipid, "BAN");
    }
}
