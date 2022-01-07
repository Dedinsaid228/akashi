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
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "include/db_manager.h"

DBManager::DBManager() :
    DRIVER("QSQLITE")
{
    const QString db_filename = "logs/akashi.db";
    QFileInfo db_info(db_filename);
    if (!db_info.exists()) {
        qWarning().noquote() << tr("Database Info: Database not found. Attempting to create new database.");
    }
    else {
        //We should only check if a file is readable/writeable when it actually exists.
        if(!db_info.isReadable() || !db_info.isWritable())
            qCritical() << tr("Database Error: Missing permissions. Check if \"%1\" is writable.").arg(db_filename);
    }

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName("logs/akashi.db");

    if (!db.open())
        qCritical() << "Database Error:" << db.lastError();

    db_version = checkVersion();

    QSqlQuery create_ban_table("CREATE TABLE IF NOT EXISTS bans ('ID' INTEGER, 'IPID' TEXT, 'HDID' TEXT, 'IP' TEXT, 'TIME' INTEGER, 'REASON' TEXT, 'DURATION' INTEGER, 'MODERATOR' TEXT, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_user_table("CREATE TABLE IF NOT EXISTS users ('ID' INTEGER, 'USERNAME' TEXT, 'SALT' TEXT, 'PASSWORD' TEXT, 'ACL' TEXT, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_ipidip_table("CREATE TABLE IF NOT EXISTS ipidip ('ID' INTEGER, 'IPID' TEXT, 'IP' TEXT, 'CREATED' TEXT, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_automod_table("CREATE TABLE IF NOT EXISTS automod ('ID' INTEGER, 'IPID' TEXT, 'DATE' TEXT, 'ACTION' TEXT, 'HAZNUM' INTEGER, PRIMARY KEY('ID' AUTOINCREMENT))");

    create_ban_table.exec();
    create_user_table.exec();
    create_ipidip_table.exec();
    create_automod_table.exec();

    if (db_version != DB_VERSION)
        updateDB(db_version);
}

QPair<bool, QString> DBManager::isIPBanned(QString ipid)
{
    QSqlQuery query;

    query.prepare("SELECT TIME,REASON,DURATION FROM BANS WHERE IPID = ? ORDER BY TIME DESC");
    query.addBindValue(ipid);
    query.exec();

    if (query.first()) {
        long long ban_time = query.value(0).toLongLong();
        QString reason = query.value(1).toString();
        long long duration = query.value(2).toLongLong();

        if (duration == -2)
            return {true, reason};

        long long current_time = QDateTime::currentDateTime().toSecsSinceEpoch();

        if (ban_time + duration > current_time)
            return {true, reason};
        else return {false, nullptr};
    }
    else return {false, nullptr};
}

QPair<bool, QString> DBManager::isHDIDBanned(QString hdid)
{
    QSqlQuery query;

    query.prepare("SELECT TIME,REASON,DURATION FROM BANS WHERE HDID = ? ORDER BY TIME DESC");
    query.addBindValue(hdid);
    query.exec();

    if (query.first()) {
        long long ban_time = query.value(0).toLongLong();
        QString reason = query.value(1).toString();
        long long duration = query.value(2).toLongLong();

        if (duration == -2)
            return {true, reason};

        long long current_time = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

        if (ban_time + duration > current_time)
            return {true, reason};
        else return {false, nullptr};
    }
    else return {false, nullptr};
}

int DBManager::getBanID(QString hdid)
{
    QSqlQuery query;

    query.prepare("SELECT ID FROM BANS WHERE HDID = ? ORDER BY TIME DESC");
    query.addBindValue(hdid);
    query.exec();

    if (query.first()) {
        return query.value(0).toInt();
    }
    else {
        return -1;
    }
}


int DBManager::getBanID(QHostAddress ip)
{
    QSqlQuery query;

    query.prepare("SELECT ID FROM BANS WHERE IP = ? ORDER BY TIME DESC");
    query.addBindValue(ip.toString());
    query.exec();

    if (query.first()) {
        return query.value(0).toInt();
    }
    else {
        return -1;
    }
}

QList<DBManager::BanInfo> DBManager::getRecentBans()
{
    QList<BanInfo> return_list;
    QSqlQuery query;

    query.prepare("SELECT * FROM BANS ORDER BY TIME DESC LIMIT 5");
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        BanInfo ban;
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        return_list.append(ban);
    }

    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

void DBManager::addBan(BanInfo ban)
{
    QSqlQuery query;

    query.prepare("INSERT INTO BANS(IPID, HDID, IP, TIME, REASON, DURATION, MODERATOR) VALUES(?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(ban.ipid);
    query.addBindValue(ban.hdid);
    query.addBindValue(ban.ip.toString());
    query.addBindValue(QString::number(ban.time));
    query.addBindValue(ban.reason);
    query.addBindValue(ban.duration);
    query.addBindValue(ban.moderator);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

bool DBManager::invalidateBan(int id)
{
    QSqlQuery ban_exists;

    ban_exists.prepare("SELECT DURATION FROM bans WHERE ID = ?");
    ban_exists.addBindValue(id);
    ban_exists.exec();

    if (!ban_exists.first())
        return false;

    QSqlQuery query;

    query.prepare("UPDATE bans SET DURATION = 0 WHERE ID = ?");
    query.addBindValue(id);
    query.exec();

    return true;
}

bool DBManager::createUser(QString username, QString salt, QString password, unsigned long long acl)
{
    QSqlQuery username_exists;

    username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();

    if (username_exists.first())
        return false;

    QSqlQuery query;

    QString salted_password;
    QMessageAuthenticationCode hmac(QCryptographicHash::Sha256);
    hmac.setKey(salt.toUtf8());
    hmac.addData(password.toUtf8());
    salted_password = hmac.result().toHex();

    query.prepare("INSERT INTO users(USERNAME, SALT, PASSWORD, ACL) VALUES(?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(salt);
    query.addBindValue(salted_password);
    query.addBindValue(acl);
    query.exec();

    return true;
}

bool DBManager::deleteUser(QString username)
{
    QSqlQuery username_exists;

    username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();

    if (username_exists.first())
        return false;

    QSqlQuery query;

    query.prepare("DELETE FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();

    return true;
}

unsigned long long DBManager::getACL(QString moderator_name)
{
    if (moderator_name == "")
        return 0;

    QSqlQuery query("SELECT ACL FROM users WHERE USERNAME = ?");

    query.addBindValue(moderator_name);
    query.exec();

    if (!query.first())
        return 0;

    return query.value(0).toULongLong();
}

bool DBManager::authenticate(QString username, QString password)
{
    QSqlQuery query_salt("SELECT SALT FROM users WHERE USERNAME = ?");

    query_salt.addBindValue(username);
    query_salt.exec();

    if (!query_salt.first())
        return false;

    QString salt = query_salt.value(0).toString();

    QString salted_password;
    QMessageAuthenticationCode hmac(QCryptographicHash::Sha256);

    hmac.setKey(salt.toUtf8());
    hmac.addData(password.toUtf8());
    salted_password = hmac.result().toHex();

    QSqlQuery query_pass("SELECT PASSWORD FROM users WHERE SALT = ?");

    query_pass.addBindValue(salt);
    query_pass.exec();

    if (!query_pass.first())
        return false;

    QString stored_pass = query_pass.value(0).toString();

    return salted_password == stored_pass;
}

bool DBManager::updateACL(QString username, unsigned long long acl, bool mode)
{
    QSqlQuery username_exists;

    username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();

    if (!username_exists.first())
        return false;

    unsigned long long old_acl = username_exists.value(0).toULongLong();
    unsigned long long new_acl;

    if (mode) // adding perm
        new_acl = old_acl | acl;
    else // removing perm
        new_acl = old_acl & ~acl;

    if (acl == 0) // Allow clearing all perms via adding perm "NONE"
        new_acl = 0;

    QSqlQuery update_acl;

    update_acl.prepare("UPDATE users SET ACL = ? WHERE USERNAME = ?");
    update_acl.addBindValue(new_acl);
    update_acl.addBindValue(username);
    update_acl.exec();

    return true;
}

QStringList DBManager::getUsers()
{
    QStringList users;

    QSqlQuery query("SELECT USERNAME FROM users ORDER BY ID");

    while (query.next()) {
        users.append(query.value(0).toString());
    }

    return users;
}

QList<DBManager::BanInfo> DBManager::getBanInfo(QString lookup_type, QString id)
{
    QList<BanInfo> return_list;
    QSqlQuery query;
    QList<BanInfo> invalid;

    if (lookup_type == "banid") {
        query.prepare("SELECT * FROM BANS WHERE ID = ?");
    }
    else if (lookup_type == "hdid") {
        query.prepare("SELECT * FROM BANS WHERE HDID = ?");
    }
    else if (lookup_type == "ipid") {
        query.prepare("SELECT * FROM BANS WHERE IPID = ?");
    }
    else {
        qCritical("Invalid ban lookup type!");
        return invalid;
    }
    query.addBindValue(id);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        BanInfo ban;
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        return_list.append(ban);
    }

    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

int DBManager::getHazNum(QString ipid)
{
    QSqlQuery query;

    query.prepare("SELECT * FROM AUTOMOD WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        automod num;
        num.ipid = query.value(1).toString();
        num.date = query.value(2).toLongLong();
        num.action = query.value(3).toString();
        num.haznum = query.value(4).toInt();
        return num.haznum;
    }
    return 0;
}

long DBManager::getHazNumDate(QString ipid)
{
    QSqlQuery query;

    query.prepare("SELECT * FROM AUTOMOD WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        automod num;
        num.ipid = query.value(1).toString();
        num.date = query.value(2).toLongLong();
        num.action = query.value(3).toString();
        num.haznum = query.value(4).toInt();
        return num.date;
    }
    return 0;
}

void DBManager::addHazNum(automod num)
{
    QSqlQuery query;

    query.prepare("INSERT INTO AUTOMOD(IPID, DATE, ACTION, HAZNUM) VALUES(?, ?, ?, ?)");
    query.addBindValue(num.ipid);
    query.addBindValue(QString::number(num.date));
    query.addBindValue(num.action);
    query.addBindValue(num.haznum);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateHazNum(QString ipid, long date)
{
    QSqlQuery query;

    query.prepare("UPDATE automod SET DATE = ? WHERE IPID = ?");
    query.addBindValue(QString::number(date));
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateHazNum(QString ipid, int haznum)
{
    QSqlQuery query;

    query.prepare("UPDATE automod SET HAZNUM = ? WHERE IPID = ?");
    query.addBindValue(haznum);
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateHazNum(QString ipid, QString action)
{
    QSqlQuery query;

    query.prepare("UPDATE automod SET ACTION = ? WHERE IPID = ?");
    query.addBindValue(action);
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

bool DBManager::updateBan(int ban_id, QString field, QVariant updated_info)
{
    QSqlQuery query;

    if (field == "reason") {
        query.prepare("UPDATE bans SET REASON = ? WHERE ID = ?");
        query.addBindValue(updated_info.toString());
    }
    else if (field == "duration") {
        query.prepare("UPDATE bans SET DURATION = ? WHERE ID = ?");
        query.addBindValue(updated_info.toLongLong());
    }

    query.addBindValue(ban_id);

    if (!query.exec()) {
        qDebug() << query.lastError();
        return false;
    }
    else {
        return true;
    }
}

bool DBManager::updatePassword(QString username, QString password)
{
    QString salt;
    QSqlQuery salt_check;

    salt_check.prepare("SELECT SALT FROM users WHERE USERNAME = ?");
    salt_check.addBindValue(username);
    salt_check.exec();

    if (!salt_check.first())
        return false;
    else
        salt = salt_check.value(0).toString();

    QSqlQuery query;

    QString salted_password;
    QMessageAuthenticationCode hmac(QCryptographicHash::Sha256);

    hmac.setKey(salt.toUtf8());
    hmac.addData(password.toUtf8());
    salted_password = hmac.result().toHex();

    query.prepare("UPDATE users SET PASSWORD = ? WHERE USERNAME = ?");
    query.addBindValue(salted_password);
    query.addBindValue(username);
    query.exec();

    return true;
}

int DBManager::checkVersion()
{
    QSqlQuery query;

    query.prepare("PRAGMA user_version");
    query.exec();

    if (query.first()) {
        return query.value(0).toInt();
    }
    else {
        return 0;
    }
}

void DBManager::updateDB(int current_version)
{
    switch (current_version) {
    case 0:
        QSqlQuery("ALTER TABLE bans ADD COLUMN MODERATOR TEXT");
        Q_FALLTHROUGH();
    case 1:
        QSqlQuery ("PRAGMA user_version = " + QString::number(DB_VERSION));
        break;
    }
}

bool DBManager::ipidExist(QString ipid)
{
    QSqlQuery query;

    query.prepare("SELECT * FROM IPIDIP WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        idipinfo ipidip;
        ipidip.ipid = query.value(1).toString();
        ipidip.ip =   query.value(2).toString();
        ipidip.date = query.value(3).toString();

        if (!ipidip.ip.isEmpty())
            return false;
    }

    return true;
}


void DBManager::ipidip(QString ipid, QString ip, QString date)
{
    QSqlQuery query;

    bool exist = ipidExist(ipid);

    if (exist == false)
            return;

    query.prepare("INSERT INTO IPIDIP(IPID, IP, CREATED) VALUES(?, ?, ?)");
    query.addBindValue(ipid);
    query.addBindValue(ip);
    query.addBindValue(date);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

QList<DBManager::idipinfo> DBManager::getIpidInfo(QString ipid)
{
    QList<idipinfo> return_list;
    QSqlQuery query;

    query.prepare("SELECT * FROM IPIDIP WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        idipinfo ipidip;
        ipidip.ipid = query.value(1).toString();
        ipidip.ip =   query.value(2).toString();
        ipidip.date = query.value(3).toString();
        return_list.append(ipidip);
    }

    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}


DBManager::~DBManager()
{
    db.close();
}
