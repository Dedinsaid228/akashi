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

#include <QDir>

#include "include/logger.h"

void Logger::logIC(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r,
                   const QString& f_message_r, const QString& f_showname_r, const QString& f_oocname_r, const QString& f_uid_r)
{
    addEntry(f_charName_r, f_ipid_r, f_hwid_r, "IC", f_message_r, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::logOOC(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r,
                    const QString& f_message_r, const QString& f_showname_r, const QString& f_oocname_r, const QString &f_uid_r)
{
    if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "OOC", f_message_r, f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "OOC", f_message_r, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::logModcall(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r,
                        const QString& f_modcallReason_r, const QString& f_showname_r, const QString& f_oocname_r, const QString &f_uid_r)
{
    if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "MODCALL", f_modcallReason_r, f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "MODCALL", f_modcallReason_r, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::logCmd(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r, const QString& f_command_r,
                    const QStringList& f_cmdArgs_r, const QString& f_showname_r, const QString& f_oocname_r, const QString &f_uid_r)
{
    // Some commands contain sensitive data, like passwords
    // These must be filtered out
    if (f_command_r == "login") {
        if (f_charName_r.isEmpty())
            addEntry("Spectator", f_ipid_r, f_hwid_r, "LOGIN", "Attempted login", f_showname_r, f_oocname_r, f_uid_r);
        else
            addEntry(f_charName_r, f_ipid_r, f_hwid_r, "LOGIN", "Attempted login", f_showname_r, f_oocname_r, f_uid_r);
    }
        else return;
}

void Logger::logCmdAdvanced(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_type_r, const QString& f_message_r,
                            const QString& f_hwid_r, const QString& f_showname_r, const QString& f_oocname_r, const QString& f_uid_r)
{
    addEntry(f_charName_r, f_ipid_r, f_type_r, f_message_r,
             f_hwid_r, f_showname_r, f_oocname_r, f_uid_r);
}
void Logger::LogDisconnecting(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r,
                              const QString& f_showname_r, const QString& f_oocname_r, const QString &f_uid_r)
{
    if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "DISCONNECT", " ", f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "DISCONNECT", " ", f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::LogConnect(const QString& f_ipid_r)
{
        addEntry("Spectator", f_ipid_r, "null", "CONNECT", " ", " ", " ", "null");
}

void Logger::LogMusic(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r,
                      const QString& f_showname_r, const QString& f_oocname_r, const QString& f_music_r, const QString &f_uid_r)
{
    if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "PLAY MUSIC", f_music_r, f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "PLAY MUSIC", f_music_r, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::LogChangeChar(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r,
                           const QString& f_showname_r, const QString& f_oocname_r, const QString &f_uid_r, const QString &f_oldCharName_r)
{
    if (f_charName_r.isEmpty() && f_oldCharName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "CHANGE CHAR", "Spectator -> Spectator", f_showname_r, f_oocname_r, f_uid_r);
    else if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "CHANGE CHAR", f_oldCharName_r + " -> Spectator", f_showname_r, f_oocname_r, f_uid_r);
    else if (f_oldCharName_r.isEmpty())
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "CHANGE CHAR", "Spectator -> " + f_charName_r, f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "CHANGE CHAR", f_oldCharName_r + " -> " + f_charName_r, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::LogChangeArea(const QString &f_charName_r, const QString &f_ipid_r, const QString &f_hwid_r,
                           const QString &f_showname_r, const QString &f_oocname_r, const QString &f_area_r, const QString &f_uid_r)
{
    if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "CHANGE AREA", f_area_r, f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "CHANGE AREA", f_area_r, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::logLogin(const QString& f_charName_r, const QString& f_ipid_r, const QString& f_hwid_r, bool success, const QString& f_modname_r,
                      const QString& f_showname_r, const QString& f_oocname_r, const QString &f_uid_r)
{
    QString l_message = success ? "Logged in as " + f_modname_r : "Failed to log in as " + f_modname_r;

    if (f_charName_r.isEmpty())
        addEntry("Spectator", f_ipid_r, f_hwid_r, "LOGIN", l_message, f_showname_r, f_oocname_r, f_uid_r);
    else
        addEntry(f_charName_r, f_ipid_r, f_hwid_r, "LOGIN", l_message, f_showname_r, f_oocname_r, f_uid_r);
}

void Logger::addEntry(
        const QString& f_charName_r,
        const QString& f_ipid_r,
        const QString& f_type_r,
        const QString& f_message_r,
        const QString& f_hwid_r,
        const QString& f_showname_r,
        const QString& f_oocname_r,
        const QString& f_uid_r)
{
    QString l_time = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss") + " UTC";

    QString l_logEntry = QStringLiteral("[%1] [%2] %3/%8/%9 (UID: %10) (IPID: %4) (HDID: %6) [%5]: %7\n")
            .arg(l_time, m_areaName, f_charName_r, f_ipid_r, f_message_r, f_type_r, f_hwid_r, f_showname_r, f_oocname_r, f_uid_r);

    if (m_buffer.length() < m_maxLength) {
        m_buffer.enqueue(l_logEntry);

        if (m_logType == DataTypes::LogType::FULL) {
           flush();
        }
    }
    else {
        m_buffer.dequeue();
        m_buffer.enqueue(l_logEntry);
    }
}

void Logger::flush()
{
    QDir l_dir("logs/");
    if (!l_dir.exists()) {
        l_dir.mkpath(".");
    }

    QFile l_logfile;

    switch (m_logType) {
    case DataTypes::LogType::MODCALL:
        l_logfile.setFileName(QString("logs/report_%1_%2.log").arg(m_areaName, (QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss"))));
        break;
    case DataTypes::LogType::FULL:
        l_logfile.setFileName(QString("logs/server.log"));
        break;
    }

    if (l_logfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream file_stream(&l_logfile);
        file_stream.setCodec("UTF-8");

        while (!m_buffer.isEmpty())
            file_stream << m_buffer.dequeue();            
    }

    l_logfile.close();
}

QQueue<QString> Logger::buffer() const
{
    return m_buffer;
}
