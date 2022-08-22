//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
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
#include "include/aoclient.h"

#include "include/config_manager.h"
#include "include/crypto_helper.h"
#include "include/db_manager.h"
#include "include/server.h"

// This file is for commands under the authentication category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdLogin(int argc, QStringList argv)
{
    if (m_authenticated) {
        sendServerMessage("You are already logged in!");
        return;
    }

    switch (ConfigManager::authType()) {

    case DataTypes::AuthType::SIMPLE:
    {

        if (ConfigManager::modpass() == "") {
            sendServerMessage("No modpass is set! Please set a modpass before authenticating.");
        }
        else if (argv[0] == ConfigManager::modpass()) {
            sendPacket("AUTH", {"1"});                      // Client: "You were granted the Disable Modcalls button."
            sendServerMessage("Logged in as a moderator."); // pre-2.9.1 clients are hardcoded to display the mod UI when this string is sent in OOC
            m_authenticated = true;
            m_acl_role_id = ACLRolesHandler::SUPER_ID;
        }
        else {
            sendPacket("AUTH", {"0"}); // Client: "Login unsuccessful."
            sendServerMessage("Incorrect password.");
        }

        emit logLogin((m_current_char + " " + m_showname), m_ooc_name, "Moderator", m_ipid, server->getAreaName(m_current_area), m_authenticated, QString::number(m_id), m_hwid);
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "login", "Moderator", server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
        break;
    }
    case DataTypes::AuthType::ADVANCED:
    {

        if (argc < 2) {
            sendServerMessage("You must specify a username and a password");
            return;
        }

        QString l_username = argv[0];
        QString l_password = argv[1];

        if (server->getDatabaseManager()->authenticate(l_username, l_password)) {
            m_moderator_name = l_username;
            m_authenticated = true;
            m_acl_role_id = server->getDatabaseManager()->getACL(l_username);
            sendPacket("AUTH", {"1"}); // Client: "You were granted the Disable Modcalls button."

            if (m_version.release <= 2 && m_version.major <= 9 && m_version.minor <= 0)
                sendServerMessage("Logged in as a moderator."); // pre-2.9.1 clients are hardcoded to display the mod UI when this string is sent in OOC

            sendServerMessage("Welcome, " + l_username);
        }
        else {
            sendPacket("AUTH", {"0"}); // Client: "Login unsuccessful."
            sendServerMessage("Incorrect password.");
        }

        emit logLogin((m_current_char + " " + m_showname), m_ooc_name, l_username, m_ipid, server->getAreaName(m_current_area), m_authenticated, QString::number(m_id), m_hwid);
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "login", l_username, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
        break;
    }
    default:
    {
        qWarning() << "config.ini has an unrecognized auth_type!";
        sendServerMessage("Config.ini contains an invalid auth_type, please check your config.");
    }
    }
}

void AOClient::cmdChangeAuth(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (ConfigManager::authType() == DataTypes::AuthType::SIMPLE) {
        change_auth_started = true;
        sendServerMessage("WARNING!\nThis command will change how logging in as a moderator works.\nOnly proceed if you know what you are doing\nUse the command /rootpass to set the password for your root account.");
    }
}

void AOClient::cmdSetRootPass(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    if (!change_auth_started)
        return;

    if (!checkPasswordRequirements("root", argv[0])) {
        sendServerMessage("Password does not meet server requirements.");
        return;
    }

    sendServerMessage("Changing auth type and setting root password.\nLogin again with /login root [password]");
    m_authenticated = false;
    ConfigManager::setAuthType(DataTypes::AuthType::ADVANCED);

    QByteArray l_salt = CryptoHelper::randbytes(16);

    server->getDatabaseManager()->createUser("root", l_salt, argv[0], ACLRolesHandler::SUPER_ID);
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "rootpass", argv[0], server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdAddUser(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    if (!checkPasswordRequirements(argv[0], argv[1])) {
        sendServerMessage("Password does not meet server requirements.");
        return;
    }

    QByteArray l_salt = CryptoHelper::randbytes(16);

    if (server->getDatabaseManager()->createUser(argv[0], l_salt, argv[1], ACLRolesHandler::NONE_ID)) {
        sendServerMessage("Created user " + argv[0] + ".\nUse /addperm to modify their permissions.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "rootpass", argv[0], server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
    }
    else
        sendServerMessage("Unable to create user " + argv[0] + ".\nDoes a user with that name already exist?");
}

void AOClient::cmdRemoveUser(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    if (server->getDatabaseManager()->deleteUser(argv[0])) {
        sendServerMessage("Successfully removed user " + argv[0] + ".");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "REMOVEUSER", argv[0], server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
    }
    else
        sendServerMessage("Unable to remove user " + argv[0] + ".\nDoes it exist?");
}

void AOClient::cmdListPerms(int argc, QStringList argv)
{
    const ACLRole l_role = server->getACLRolesHandler()->getRoleById(m_acl_role_id);

    ACLRole l_target_role = l_role;
    qDebug() << m_acl_role_id;
    QStringList l_message;

    if (argc == 0) {
        l_message.append("You have been given the following permissions:");
    }
    else {
        if (!l_role.checkPermission(ACLRole::MODIFY_USERS)) {
            sendServerMessage("You do not have permission to view other users' permissions.");
            return;
        }

        l_message.append("User " + argv[0] + " has the following permissions:");
        l_target_role = server->getACLRolesHandler()->getRoleById(argv[0]);
    }

    if (l_target_role.getPermissions() == ACLRole::NONE) {
        l_message.append("NONE");
    }
    else if (l_target_role.checkPermission(ACLRole::SUPER)) {
        l_message.append("SUPER (Be careful! This grants the user all permissions.)");
    }
    else {
        const QList<ACLRole::Permission> l_permissions = ACLRole::PERMISSION_CAPTIONS.keys();
        for (const ACLRole::Permission i_permission : l_permissions) {
            if (l_target_role.checkPermission(i_permission)) {
                l_message.append(ACLRole::PERMISSION_CAPTIONS.value(i_permission));
            }
        }
    }

    sendServerMessage(l_message.join("\n"));
}

void AOClient::cmdSetPerms(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    const QString l_target_acl = argv[1];
    if (!server->getACLRolesHandler()->roleExists(l_target_acl)) {
        sendServerMessage("That role doesn't exist!");
        return;
    }

    if (l_target_acl == ACLRolesHandler::SUPER_ID && !checkPermission(ACLRole::SUPER)) {
        sendServerMessage("You aren't allowed to set that role!");
        return;
    }

    const QString l_target_username = argv[0];
    if (l_target_username == "root") {
        sendServerMessage("You can't change root's role!");
        return;
    }

    if (server->getDatabaseManager()->updateACL(l_target_username, l_target_acl)) {
        sendServerMessage("Successfully applied role " + l_target_acl + " to user " + l_target_username);
    }
    else {
        sendServerMessage(l_target_username + " wasn't found!");
    }
}

void AOClient::cmdRemovePerms(int argc, QStringList argv)
{
    argv.append(ACLRolesHandler::NONE_ID);
    cmdSetPerms(argc, argv);
}

void AOClient::cmdListUsers(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QStringList l_users = server->getDatabaseManager()->getUsers();

    sendServerMessage("All users:\n" + l_users.join("\n"));
}

void AOClient::cmdLogout(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (!m_authenticated) {
        sendServerMessage("You are not logged in!");
        return;
    }

    m_authenticated = false;
    m_acl_role_id = "";
    m_moderator_name = "";
    sendPacket("AUTH", {"-1"}); // Client: "You were logged out."
    emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "LOGOUT", "", server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
}

void AOClient::cmdChangePassword(int argc, QStringList argv)
{
    QString l_username;
    QString l_password = argv[0];

    if (argc == 1) {
        if (m_moderator_name.isEmpty()) {
            sendServerMessage("You are not logged in.");
            return;
        }

        l_username = m_moderator_name;
    }
    else if (argc == 2 && checkPermission(ACLRole::SUPER)) {
        l_username = argv[0];
        l_password = argv[1];
    }
    else {
        sendServerMessage("Invalid command syntax.");
        return;
    }

    if (!checkPasswordRequirements(l_username, l_password)) {
        sendServerMessage("Password does not meet server requirements.");
        return;
    }

    if (server->getDatabaseManager()->updatePassword(l_username, l_password)) {
        sendServerMessage("Successfully changed password.");
        emit logCMD((m_current_char + " " + m_showname), m_ipid, m_ooc_name, "CHANGEPASSWORD", "User: " + l_username + ". Password: " + l_password, server->getAreaName(m_current_area), QString::number(m_id), m_hwid);
    }
    else {
        sendServerMessage("There was an error changing the password.");
        return;
    }
}
