/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WebDavSettings.h"

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Metadata.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

WebDavSettings::WebDavSettings(const QSharedPointer<Database>& db, QObject* parent)
    : QObject(parent)
{
    setDatabase(db);
}

WebDavSettings::~WebDavSettings() = default;

void WebDavSettings::setDatabase(const QSharedPointer<Database>& db)
{
    m_params.clear();
    m_db = db;
    loadSettings();
}

void WebDavSettings::addParams(WebDavParams* params)
{
    if (params->name.isEmpty()) {
        qWarning() << "WebDavSettings::addParams: connection name is empty";
        return;
    }
    m_params.insert(params->name, params);
}

void WebDavSettings::removeParams(const QString& name)
{
    m_params.remove(name);
}

WebDavParams* WebDavSettings::getParams(const QString& name) const
{
    return m_params.value(name, nullptr);
}

QList<WebDavParams*> WebDavSettings::getAllParams() const
{
    return m_params.values();
}

void WebDavSettings::loadSettings()
{
    if (m_db) {
        fromConfig(m_db->metadata()->customData()->value(CustomData::WebDavSettings));
    }
}

void WebDavSettings::saveSettings() const
{
    if (m_db) {
        m_db->metadata()->customData()->set(CustomData::WebDavSettings, toConfig());
    }
}

QString WebDavSettings::toConfig() const
{
    QJsonArray config;
    for (const auto* params : m_params.values()) {
        QJsonObject obj;
        obj["name"] = params->name;
        obj["url"] = params->url;
        obj["username"] = params->username;
        obj["timeoutMsec"] = params->timeoutMsec;
        obj["cachedFilePath"] = params->cachedFilePath;
        obj["lastETag"] = params->lastETag;
        obj["lastModified"] = params->lastModified;
        config << obj;
    }
    return QJsonDocument(config).toJson(QJsonDocument::Compact);
}

void WebDavSettings::fromConfig(const QString& data)
{
    m_params.clear();

    QJsonDocument json = QJsonDocument::fromJson(data.toUtf8());
    for (const auto& item : json.array().toVariantList()) {
        auto map = item.toMap();
        auto* params = new WebDavParams();
        params->name = map["name"].toString();
        params->url = map["url"].toString();
        params->username = map["username"].toString();
        params->timeoutMsec = map.value("timeoutMsec", 30000).toInt();
        params->cachedFilePath = map["cachedFilePath"].toString();
        params->lastETag = map["lastETag"].toString();
        params->lastModified = map["lastModified"].toString();
        m_params.insert(params->name, params);
    }
}
