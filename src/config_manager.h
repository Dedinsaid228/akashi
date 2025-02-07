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
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#define CONFIG_VERSION 1

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QMetaEnum>
#include <QSettings>
#include <QUrl>

#include "data_types.h"

/**
 * @brief The config file handler class.
 */
class ConfigManager
{

  public:
    /**
     * @brief Verifies the server configuration, confirming all required files/directories exist and are valid.
     *
     * @return True if the server configuration was verified, false otherwise.
     */
    static bool verifyServerConfig();

    /**
     * @brief Returns the IP the server binds to.
     *
     * @return See short description
     */
    static QString bindIP();

    /**
     * @brief Returns the character list of the server.
     *
     * @return See short description.
     */
    static QStringList charlist();

    /**
     * @brief Returns the a QStringList of the available backgrounds.
     *
     * @return See short description.
     */
    static QStringList backgrounds();

    /**
     * @brief Returns a QStringlist of the available songs.
     *
     * @return See short description.
     */
    static QStringList musiclist();

    /**
     * @brief Returns an ordered QList of all basesongs of this server.
     *
     * @return See short description.
     */
    static QStringList ordered_songs();

    /**
     * @brief Returns the content of
     * @return
     */
    static QSettings *areaData();

    /**
     * @brief Returns a pointer to the QSettings object which contains the ambience configuration.
     *
     * @return See short description.
     */
    static QSettings *ambience();

    /**
     * @brief Returns the content of
     * @return
     */
    static QSettings *hubsData(); // How long is this here? 0_0

    /**
     * @brief Returns a sanitized QStringList of the areas.
     *
     * @return See short description.
     */
    static QStringList sanitizedAreaNames();

    static QStringList areaNames();

    /**
     * @brief Returns the raw arealist
     *
     * @return See short description.
     */
    static QStringList rawAreaNames();

    static QStringList sanitizedHubNames();

    static QStringList rawHubNames();

    /**
     * @brief Returns the maximum number of players the server will allow.
     *
     * @return See short description.
     */
    static int maxPlayers();

    /**
     * @brief Returns the port to listen for connections on.
     *
     * @return See short description.
     */
    static int serverPort();

    /**
     * @brief Returns the SSL port to listen for connections on.
     *
     * @return See short description.
     */
    static int securePort();

    /**
     * @brief Returns the server description.
     *
     * @return See short description.
     */
    static QString serverDescription();

    /**
     * @brief Returns the server name.
     *
     * @return See short description.
     */
    static QString serverName();

    /**
     * @brief Returns the short "tag" version of the server.
     *
     * @return See short description.
     */
    static QString serverTag();

    /**
     * @brief Returns the server's Message of the Day.
     *
     * @return See short description.
     */
    static QString motd();

    /**
     * @brief Returns true if the server should accept webAO connections.
     *
     * @return See short description.
     */
    static bool webaoEnabled();

    /**
     * @brief Returns the server's authorization type.
     *
     * @return See short description.
     */
    static DataTypes::AuthType authType();

    /**
     * @brief Returns the server's moderator password.
     *
     * @return See short description.
     */
    static QString modpass();

    /**
     * @brief Returns the server's log buffer length.
     *
     * @return See short description.
     */
    static int logBuffer();

    /**
     * @brief Returns the server's logging type.
     *
     * @return See short description.
     */
    static DataTypes::LogType loggingType();

    /**
     * @brief Returns a list of the IPrange bans.
     *
     * @return See short description.
     */
    static QStringList iprangeBans();

    static QStringList ipignoreBans();

    /**
     * @brief Returns true if the server should advertise to the master server.
     *
     * @return See short description.
     */
    static int maxStatements();

    /**
     * @brief Returns the maximum number of permitted connections from the same IP.
     *
     * @return See short description.
     */
    static int multiClientLimit();

    /**
     * @brief Returns the maximum number of characters a message can contain.
     *
     * @return See short description.
     */
    static int maxCharacters();

    static int maxCharactersChillMod();

    /**
     * @brief Returns the duration of the message floodguard.
     *
     * @return See short description.
     */
    static int messageFloodguard();

    /**
     * @brief Returns the URL where the server should retrieve remote assets from.
     *
     * @return See short description.
     */
    static QUrl assetUrl();

    /**
     * @brief Returns the maximum number of sides dice can have.
     *
     * @return See short description.
     */
    static int diceMaxValue();

    /**
     * @brief Returns the maximum number of dice that can be rolled at once.
     *
     * @return See short description.
     */
    static int diceMaxDice();

    /**
     * @brief Returns true if the discord webhook integration is enabled.
     *
     * @return See short description.
     */
    static bool discordWebhookEnabled();

    /**
     * @brief Returns true if the discord modcall webhook is enabled.
     *
     * @return See short description.
     */
    static bool discordModcallWebhookEnabled();

    /**
     * @brief Returns the discord webhook URL.
     *
     * @return See short description.
     */
    static QString discordModcallWebhookUrl();

    /**
     * @brief Returns the discord webhook content.
     *
     * @return See short description.
     */
    static QString discordModcallWebhookContent();

    /**
     * @brief Returns true if the discord webhook should send log files.
     *
     * @return See short description.
     */
    static bool discordModcallWebhookSendFile();

    /**
     * @brief Returns true if the discord ban webhook is enabled.
     *
     * @return See short description.
     */
    static bool discordBanWebhookEnabled();

    /**
     * @brief Returns the Discord Ban Webhook URL.
     *
     * @return See short description.
     */
    static QString discordBanWebhookUrl();

    /**
     * @brief Returns if the Webhook sends an alive message.
     */
    static bool discordUptimeEnabled();

    /**
     * @brief Returns the time between posting.
     */
    static int discordUptimeTime();

    /**
     * @brief Returns the Discord Uptime Webhook URL.
     *
     * @return See short description.
     */
    static QString discordUptimeWebhookUrl();

    /**
     * @brief Returns a user configurable color code for the embeed object.s
     *
     * @return See short description.
     */
    static QString discordWebhookColor();

    /**
     * @brief Returns true if password requirements should be enforced.
     *
     * @return See short description.
     */
    static bool passwordRequirements();

    /**
     * @brief Returns the minimum length passwords must be.
     *
     * @return See short description.
     */
    static int passwordMinLength();

    /**
     * @brief Returns the maximum length passwords can be, or `0` for unlimited length.
     *
     * @return See short description.
     */
    static int passwordMaxLength();

    /**
     * @brief Returns true if passwords must be mixed case.
     *
     * @return See short description.
     */
    static bool passwordRequireMixCase();

    /**
     * @brief Returns true is passwords must contain one or more numbers.
     *
     * @return See short description.
     */
    static bool passwordRequireNumbers();

    /**
     * @brief Returns true if passwords must contain one or more special characters..
     *
     * @return See short description.
     */
    static bool passwordRequireSpecialCharacters();

    /**
     * @brief Returns true if passwords can contain the username.
     *
     * @return See short description.
     */
    static bool passwordCanContainUsername();

    /**
     * @brief Returns the logstring for the specified logtype.
     *
     * @param Name of the logstring we want.
     *
     * @return See short description.
     */
    static QString LogText(QString f_logtype);

    /**
     * @brief Returns a number indicating the number of seconds in which the automoderator is unresponsive to the client's behavior.
     *
     * @return See short description.
     */
    static int autoModTrigger();

    static int autoModOocTrigger();

    static int autoModWarns();

    static QString autoModHaznumTerm();

    /**
     * @brief Returns the duration of the ban given by the automoderator.
     *
     * @return See short description.
     */
    static QString autoModBanDuration();

    /**
     * @brief Returns the time after which one warn is removed.
     *
     * @return See short description.
     */
    static QString autoModWarnTerm();

    static bool useYtdlp();

    /**
     * @brief Returns a list of magic 8 ball answers.
     *
     * @return See short description.
     */
    static QStringList magic8BallAnswers();

    /**
     * @brief Returns the server gimp list.
     *
     * @return See short description.
     */
    static QStringList gimpList();

    /**
     * @brief Returns the server regex filter list
     *
     * @return See short description.
     */
    static QStringList filterList();

    /**
     * @brief Returns the server approved domain list.
     *
     * @return See short description.
     */
    static QStringList cdnList();

    /**
     * @brief Returns if the advertiser is enabled to advertise on ms3.
     */
    static bool publishServerEnabled();

    /**
     * @brief Returns the IP or URL of the masterserver.
     */
    static QUrl serverlistURL();

    /**
     * @brief Returns an optional hostname paramemter for the advertiser.
     * If used allows user to set a custom IP or domain name.
     */
    static QString serverDomainName();

    /**
     * @brief Returns a dummy port instead of the real port
     * @return
     */
    static bool advertiseWSProxy();

    /**
     * @brief Returns the uptime of the server in miliseconds.
     */
    static qint64 uptime();

    /**
     * @brief Sets the server's authorization type.
     *
     * @param f_auth The auth type to set.
     */
    static void setAuthType(const DataTypes::AuthType f_auth);

    /**
     * @brief Sets the server's Message of the Day.
     *
     * @param f_motd The MOTD to set.
     */
    static void setMotd(const QString f_motd);

    /**
     * @brief Returns true if WUSO is enabled.
     *
     * @return See short description.
     */
    static bool webUsersSpectableOnly();

    /**
     * @brief Enable/disable WUSO.
     *
     * @return See short description.
     */
    static void webUsersSpectableOnlyToggle();

    /**
     * @brief Reload the server configuration.
     */
    static void reloadSettings();

    static QStringList getCustomStatuses();

    static int getAreaCountLimit();

  private:
    /**
     * @brief Checks if a file exists and is valid.
     *
     * @param file The file to check.
     *
     * @return True if the file exists and is valid, false otherwise.
     */
    static bool fileExists(const QFileInfo &file);

    /**
     * @brief Checks if a directory exists and is valid.
     *
     * @param file The directory to check.
     *
     * @return True if the directory exists and is valid, false otherwise.
     */
    static bool dirExists(const QFileInfo &dir);

    /**
     * @brief A struct for storing QStringLists loaded from command configuration files.
     */
    struct CommandSettings
    {
        QStringList magic_8ball; //!< Contains answers for /8ball, found in config/text/8ball.txt
        QStringList gimps;       //!< Contains phrases for /gimp, found in config/text/gimp.txt
        QStringList filters;     //!< Contains filter regex, found in config/text/filter.txt
        QStringList cdns;        //!< Contains domains for custom song validation, found in config/text/cdns.txt
    };

    /**
     * @brief Contains the settings required for various commands.
     */
    static CommandSettings *m_commands;

    /**
     * @brief Stores all server configuration values.
     */
    static QSettings *m_settings;

    /**
     * @brief Stores all discord webhook configuration values.
     */
    static QSettings *m_discord;

    /**
     * @brief Stores all of the area valus.
     */
    static QSettings *m_areas;

    static QSettings *m_hubs;

    /**
     * @brief Stores all adjustable logstrings.
     */
    static QSettings *m_logtext;

    /**
     * @brief Stores all adjustable logstrings.
     */
    static QSettings *m_ambience;

    /**
     * @brief Contains the musiclist with time durations.
     */
    static QStringList *m_musicList;

    /**
     * @brief Contains an ordered list for the musiclist.
     */
    static QStringList *m_ordered_list;

    /**
     * @brief Pointer to QElapsedTimer to track the uptime of the server.
     */
    static QElapsedTimer *m_uptimeTimer;

    /**
     * @brief Returns a stringlist with the contents of a .txt file from config/text/.
     *
     * @param Name of the file to load.
     */
    static QStringList loadConfigFile(const QString filename);
};

#endif // CONFIG_MANAGER_H
