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
#include "config_manager.h"

QSettings *ConfigManager::m_settings = new QSettings("config/config.ini", QSettings::IniFormat);
QSettings *ConfigManager::m_discord = new QSettings("config/discord.ini", QSettings::IniFormat);
ConfigManager::CommandSettings *ConfigManager::m_commands = new CommandSettings();
QSettings *ConfigManager::m_areas = new QSettings("config/areas.ini", QSettings::IniFormat);
QSettings *ConfigManager::m_hubs = new QSettings("config/hubs.ini", QSettings::IniFormat);
QSettings *ConfigManager::m_logtext = new QSettings("config/text/logtext.ini", QSettings::IniFormat);
QSettings *ConfigManager::m_ambience = new QSettings("config/ambience.ini", QSettings::IniFormat);
QElapsedTimer *ConfigManager::m_uptimeTimer = new QElapsedTimer;
QStringList *ConfigManager::m_musicList = new QStringList;
QStringList *ConfigManager::m_ordered_list = new QStringList;

bool ConfigManager::verifyServerConfig()
{
    // Verify directories
    QStringList l_directories{"config/", "config/text/"};
    for (const QString &l_directory : l_directories) {
        if (!dirExists(QFileInfo(l_directory))) {
            qCritical() << l_directory + " does not exist!";
            return false;
        }
    }

    // Verify config files
    QStringList l_config_files{"config/config.ini", "config/areas.ini", "config/backgrounds.txt", "config/characters.txt", "config/music.txt",
                               "config/discord.ini", "config/text/8ball.txt", "config/text/gimp.txt", "config/text/cdns.txt"};
    for (const QString &l_file : l_config_files) {
        if (!fileExists(QFileInfo(l_file))) {
            qCritical() << l_file + " does not exist!";
            return false;
        }
    }

    // Verify areas
    QSettings l_areas_ini("config/areas.ini", QSettings::IniFormat);
    if (l_areas_ini.childGroups().length() < 1) {
        qCritical() << "areas.ini is invalid!";
        return false;
    }

    // Verify config settings
    m_settings->beginGroup("Options");

    bool ok;
    m_settings->value("ms_port", 27016).toInt(&ok);
    if (!ok) {
        qCritical("ms_port is not a valid port!");
        return false;
    }

    m_settings->value("port", 27016).toInt(&ok);
    if (!ok) {
        qCritical("port is not a valid port!");
        return false;
    }

    m_settings->value("secure_port", -1).toInt(&ok);
    if (!ok) {
        qCritical("secure_port is not a valid port!");
        return false;
    }

    QString l_auth = m_settings->value("auth", "simple").toString().toLower();
    if (!(l_auth == "simple" || l_auth == "advanced")) {
        qCritical("auth is not a valid auth type!");
        return false;
    }

    m_settings->endGroup();

    m_commands->magic_8ball = (loadConfigFile("8ball"));
    m_commands->gimps = (loadConfigFile("gimp"));
    m_commands->filters = (loadConfigFile("filter"));
    m_commands->cdns = (loadConfigFile("cdns"));
    if (m_commands->cdns.isEmpty())
        m_commands->cdns = QStringList{"cdn.discord.com"};

    m_uptimeTimer->start();

    return true;
}

QString ConfigManager::bindIP() { return m_settings->value("Options/bind_ip", "all").toString(); }

QStringList ConfigManager::charlist()
{
    QStringList l_charlist;
    QFile l_file("config/characters.txt");
    l_file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!l_file.atEnd())
        l_charlist.append(l_file.readLine().trimmed());

    l_file.close();
    return l_charlist;
}

QStringList ConfigManager::backgrounds()
{
    QStringList l_backgrounds;
    QFile l_file("config/backgrounds.txt");
    l_file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!l_file.atEnd())
        l_backgrounds.append(l_file.readLine().trimmed());

    l_file.close();
    return l_backgrounds;
}

QStringList ConfigManager::musiclist()
{
    // Make sure the list is empty before appending new data.
    if (!m_ordered_list->empty())
        m_ordered_list->clear();

    QFile l_file("config/music.txt");
    l_file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!l_file.atEnd())
        m_ordered_list->append(l_file.readLine().trimmed());

    l_file.close();

    if (m_ordered_list->contains(".")) // Add a default category if none exists
        m_ordered_list->insert(0, "==Music==");

    return *m_musicList;
}

QStringList ConfigManager::ordered_songs() { return *m_ordered_list; }

QSettings *ConfigManager::areaData() { return m_areas; }

QSettings *ConfigManager::ambience() { return m_ambience; }

QSettings *ConfigManager::hubsData() { return m_hubs; }

QStringList ConfigManager::sanitizedAreaNames()
{
    QStringList l_area_names = m_areas->childGroups(); // invisibly does a lexicographical sort, because Qt is great like that
    std::sort(l_area_names.begin(), l_area_names.end(), [](const QString &a, const QString &b) { return a.split(":")[0].toInt() < b.split(":")[0].toInt(); });
    QStringList l_sanitized_area_names;

    for (const QString &areaName : std::as_const(l_area_names)) {
        QStringList l_nameSplit = areaName.split(":");
        l_nameSplit.removeFirst();
        l_nameSplit.removeFirst();
        QString l_area_name_sanitized = l_nameSplit.join(":");
        l_sanitized_area_names.append(l_area_name_sanitized);
    }

    return l_sanitized_area_names;
}

QStringList ConfigManager::rawAreaNames()
{
    QStringList l_area_names = m_areas->childGroups();
    std::sort(l_area_names.begin(), l_area_names.end(), [](const QString &a, const QString &b) { return a.split(":")[0].toInt() < b.split(":")[0].toInt(); }); // without sorting, the client gets the wrong list of areas if their number is greater than 10

    return l_area_names;
}

QStringList ConfigManager::sanitizedHubNames()
{
    QStringList l_hub_names = m_hubs->childGroups();
    std::sort(l_hub_names.begin(), l_hub_names.end(), [](const QString &a, const QString &b) { return a.split(":")[0].toInt() < b.split(":")[0].toInt(); });
    QStringList l_sanitized_hub_names;

    for (const QString &hubName : std::as_const(l_hub_names)) {
        QStringList l_nameSplit = hubName.split(":");
        l_nameSplit.removeFirst();
        QString l_hub_name_sanitized = l_nameSplit.join(":");
        l_sanitized_hub_names.append(l_hub_name_sanitized);
    }

    return l_sanitized_hub_names;
}

QStringList ConfigManager::rawHubNames()
{
    QStringList l_hub_names = m_hubs->childGroups();
    std::sort(l_hub_names.begin(), l_hub_names.end(), [](const QString &a, const QString &b) { return a.split(":")[0].toInt() < b.split(":")[0].toInt(); }); // Without sorting, the client gets the wrong list of areas.

    return l_hub_names;
}

QStringList ConfigManager::iprangeBans()
{
    QStringList l_iprange_bans;
    QFile l_file("config/iprange_bans.txt");
    l_file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!(l_file.atEnd()))
        l_iprange_bans.append(l_file.readLine().trimmed());

    l_file.close();
    return l_iprange_bans;
}

QStringList ConfigManager::ipignoreBans()
{
    QStringList l_iprange_ignore;
    QFile l_file("config/iprange_ignore.txt");
    l_file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!(l_file.atEnd()))
        l_iprange_ignore.append(l_file.readLine().trimmed());

    l_file.close();
    return l_iprange_ignore;
}

void ConfigManager::reloadSettings()
{
    m_settings->sync();
    m_discord->sync();
    m_logtext->sync();
}

QStringList ConfigManager::loadConfigFile(const QString filename)
{
    QStringList stringlist;
    QFile l_file("config/text/" + filename + ".txt");
    l_file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!(l_file.atEnd()))
        stringlist.append(l_file.readLine().trimmed());

    l_file.close();
    return stringlist;
}

int ConfigManager::maxPlayers()
{
    bool ok;
    int l_players = m_settings->value("Options/max_players", 100).toInt(&ok);
    if (!ok) {
        qWarning("max_players is not an int!");
        l_players = 100;
    }

    return l_players;
}

int ConfigManager::serverPort()
{
    if (m_settings->contains("Options/webao_port")) {
        qWarning("webao_port is deprecated, use port instead");
        return m_settings->value("Options/webao_port", 27016).toInt();
    }

    return m_settings->value("Options/port", 27016).toInt();
}

int ConfigManager::securePort() { return m_settings->value("Options/secure_port", -1).toInt(); }

QString ConfigManager::serverDescription() { return m_settings->value("Options/server_description", "This is my flashy new server!").toString(); }

QString ConfigManager::serverName() { return m_settings->value("Options/server_name", "An Unnamed Server").toString(); }

QString ConfigManager::serverTag() { return m_settings->value("Options/server_tag", serverName()).toString(); }

QString ConfigManager::motd() { return m_settings->value("Options/motd", "MOTD not set").toString(); }

bool ConfigManager::webaoEnabled() { return m_settings->value("Options/webao_enable", false).toBool(); }

DataTypes::AuthType ConfigManager::authType()
{
    QString l_auth = m_settings->value("Options/auth", "simple").toString().toUpper();
    return toDataType<DataTypes::AuthType>(l_auth);
}

QString ConfigManager::modpass() { return m_settings->value("Options/modpass", "changeme").toString(); }

int ConfigManager::logBuffer()
{
    bool ok;
    int l_buffer = m_settings->value("Options/logbuffer", 500).toInt(&ok);
    if (!ok) {
        qWarning("logbuffer is not an int!");
        l_buffer = 500;
    }

    return l_buffer;
}

DataTypes::LogType ConfigManager::loggingType()
{
    QString l_log = m_settings->value("Options/logging", "modcall").toString().toUpper();
    return toDataType<DataTypes::LogType>(l_log);
}

int ConfigManager::maxStatements()
{
    bool ok;
    int l_max = m_settings->value("Options/maximum_statements", 10).toInt(&ok);
    if (!ok) {
        qWarning("maximum_statements is not an int!");
        l_max = 10;
    }

    return l_max;
}
int ConfigManager::multiClientLimit()
{
    bool ok;
    int l_limit = m_settings->value("Options/multiclient_limit", 15).toInt(&ok);
    if (!ok) {
        qWarning("multiclient_limit is not an int!");
        l_limit = 15;
    }

    return l_limit;
}

int ConfigManager::maxCharacters()
{
    bool ok;
    int l_max = m_settings->value("Options/maximum_characters", 256).toInt(&ok);
    if (!ok) {
        qWarning("maximum_characters is not an int!");
        l_max = 256;
    }

    return l_max;
}

int ConfigManager::maxCharactersChillMod()
{
    bool ok;
    int l_max = m_settings->value("Options/maximum_characters_chillmod", 128).toInt(&ok);
    if (!ok) {
        qWarning("maximum_characters_chillmod is not an int!");
        l_max = 128;
    }

    return l_max;
}

int ConfigManager::messageFloodguard()
{
    bool ok;
    int l_flood = m_settings->value("Options/message_floodguard", 250).toInt(&ok);
    if (!ok) {
        qWarning("message_floodguard is not an int!");
        l_flood = 250;
    }

    return l_flood;
}

QUrl ConfigManager::assetUrl()
{
    QByteArray l_url = m_settings->value("Options/asset_url", "").toString().toUtf8();
    if (QUrl(l_url).isValid())
        return QUrl(l_url);
    else {
        qWarning("asset_url is not a valid url!");
        return QUrl(NULL);
    }
}

int ConfigManager::diceMaxValue()
{
    bool ok;
    int l_value = m_settings->value("Dice/max_value", 100).toInt(&ok);
    if (!ok) {
        qWarning("max_value is not an int!");
        l_value = 100;
    }

    return l_value;
}

int ConfigManager::diceMaxDice()
{
    bool ok;
    int l_dice = m_settings->value("Dice/max_dice", 100).toInt(&ok);
    if (!ok) {
        qWarning("max_dice is not an int!");
        l_dice = 100;
    }

    return l_dice;
}

bool ConfigManager::discordWebhookEnabled() { return m_discord->value("Discord/webhook_enabled", false).toBool(); }

bool ConfigManager::discordModcallWebhookEnabled() { return m_discord->value("Discord/webhook_modcall_enabled", false).toBool(); }

QString ConfigManager::discordModcallWebhookUrl() { return m_discord->value("Discord/webhook_modcall_url", "").toString(); }

QString ConfigManager::discordModcallWebhookContent() { return m_discord->value("Discord/webhook_modcall_content", "").toString(); }

bool ConfigManager::discordModcallWebhookSendFile() { return m_discord->value("Discord/webhook_modcall_sendfile", false).toBool(); }

bool ConfigManager::discordBanWebhookEnabled() { return m_discord->value("Discord/webhook_ban_enabled", false).toBool(); }

QString ConfigManager::discordBanWebhookUrl() { return m_discord->value("Discord/webhook_ban_url", "").toString(); }

bool ConfigManager::discordUptimeEnabled() { return m_discord->value("Discord/webhook_uptime_enabled", "false").toBool(); }

int ConfigManager::discordUptimeTime()
{
    bool ok;
    int l_aliveTime = m_discord->value("Discord/webhook_uptime_time", "60").toInt(&ok);
    if (!ok) {
        qWarning("alive_time is not an int");
        l_aliveTime = 60;
    }

    return l_aliveTime;
}

QString ConfigManager::discordUptimeWebhookUrl() { return m_discord->value("Discord/webhook_uptime_url", "").toString(); }

QString ConfigManager::discordWebhookColor()
{
    const QString l_default_color = "13312842";
    QString l_color = m_discord->value("Discord/webhook_color", l_default_color).toString();
    if (l_color.isEmpty())
        return l_default_color;
    else
        return l_color;
}

bool ConfigManager::passwordRequirements() { return m_settings->value("Password/password_requirements", true).toBool(); }

int ConfigManager::passwordMinLength()
{
    bool ok;
    int l_min = m_settings->value("Password/pass_min_length", 8).toInt(&ok);
    if (!ok) {
        qWarning("pass_min_length is not an int!");
        l_min = 8;
    }

    return l_min;
}

int ConfigManager::passwordMaxLength()
{
    bool ok;
    int l_max = m_settings->value("Password/pass_max_length", 0).toInt(&ok);
    if (!ok) {
        qWarning("pass_max_length is not an int!");
        l_max = 0;
    }

    return l_max;
}

bool ConfigManager::passwordRequireMixCase() { return m_settings->value("Password/pass_required_mix_case", true).toBool(); }

bool ConfigManager::passwordRequireNumbers() { return m_settings->value("Password/pass_required_numbers", true).toBool(); }

bool ConfigManager::passwordRequireSpecialCharacters() { return m_settings->value("Password/pass_required_special", true).toBool(); }

bool ConfigManager::passwordCanContainUsername() { return m_settings->value("Password/pass_can_contain_username", false).toBool(); }

QString ConfigManager::LogText(QString f_logtype) { return m_logtext->value("LogConfiguration/" + f_logtype, "").toString(); }

int ConfigManager::autoModTrigger() { return m_settings->value("Options/automodtrig", 300).toInt(); }

int ConfigManager::autoModOocTrigger() { return m_settings->value("Options/automodooctrig", 300).toInt(); }

int ConfigManager::autoModWarns() { return m_settings->value("Options/automodwarns", 3).toInt(); }

QString ConfigManager::autoModHaznumTerm() { return m_settings->value("Options/automodhaznumterm", "7d").toString(); }

QString ConfigManager::autoModBanDuration() { return m_settings->value("Options/automodbanduration", "7d").toString(); }

QString ConfigManager::autoModWarnTerm() { return m_settings->value("Options/automodwarnterm", "30m").toString(); }

bool ConfigManager::useYtdlp() { return m_settings->value("Options/ytdlp", true).toBool(); }

void ConfigManager::setAuthType(const DataTypes::AuthType f_auth) { m_settings->setValue("Options/auth", fromDataType<DataTypes::AuthType>(f_auth).toLower()); }

QStringList ConfigManager::magic8BallAnswers() { return m_commands->magic_8ball; }

QStringList ConfigManager::gimpList() { return m_commands->gimps; }

QStringList ConfigManager::filterList() { return m_commands->filters; }

QStringList ConfigManager::cdnList() { return m_commands->cdns; }

bool ConfigManager::publishServerEnabled() { return m_settings->value("Advertiser/advertise", "true").toBool(); }

QUrl ConfigManager::serverlistURL() { return m_settings->value("Advertiser/ms_ip", "").toUrl(); }

QString ConfigManager::serverDomainName() { return m_settings->value("Advertiser/hostname", "").toString(); }

bool ConfigManager::advertiseWSProxy() { return m_settings->value("Advertiser/cloudflare_enabled", "false").toBool(); }

qint64 ConfigManager::uptime() { return m_uptimeTimer->elapsed(); }

void ConfigManager::setMotd(const QString f_motd) { m_settings->setValue("Options/motd", f_motd); }

bool ConfigManager::webUsersSpectableOnly() { return m_settings->value("Options/wuso", "false").toBool(); }

void ConfigManager::webUsersSpectableOnlyToggle() { m_settings->setValue("Options/wuso", !webUsersSpectableOnly()); }

QStringList ConfigManager::getCustomStatuses() { return loadConfigFile("customstatuses"); }

int ConfigManager::getAreaCountLimit() { return m_settings->value("Options/arealimit", "25").toInt(); }

bool ConfigManager::fileExists(const QFileInfo &f_file) { return (f_file.exists() && f_file.isFile()); }

bool ConfigManager::dirExists(const QFileInfo &f_dir) { return (f_dir.exists() && f_dir.isDir()); }
