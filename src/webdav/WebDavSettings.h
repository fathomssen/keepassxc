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

#ifndef KEEPASSXC_WEBDAVSETTINGS_H
#define KEEPASSXC_WEBDAVSETTINGS_H

#include "WebDavParams.h"

#include <QHash>
#include <QObject>
#include <QSharedPointer>

class Database;

class WebDavSettings : public QObject
{
    Q_OBJECT
public:
    explicit WebDavSettings(const QSharedPointer<Database>& db, QObject* parent = nullptr);
    ~WebDavSettings() override;

    void setDatabase(const QSharedPointer<Database>& db);

    void addParams(WebDavParams* params);
    void removeParams(const QString& name);
    WebDavParams* getParams(const QString& name) const;
    QList<WebDavParams*> getAllParams() const;

    void loadSettings();
    void saveSettings() const;

private:
    void fromConfig(const QString& data);
    QString toConfig() const;

    QHash<QString, WebDavParams*> m_params;
    QSharedPointer<Database> m_db;
};

#endif // KEEPASSXC_WEBDAVSETTINGS_H
