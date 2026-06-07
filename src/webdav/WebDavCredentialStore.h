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

#ifndef KEEPASSXC_WEBDAVCREDENTIALSTORE_H
#define KEEPASSXC_WEBDAVCREDENTIALSTORE_H

#include <QHash>
#include <QString>

// In-process session password cache — passwords are never persisted to disk.
class WebDavCredentialStore
{
public:
    static WebDavCredentialStore* instance();

    QString getPassword(const QString& connectionName) const;
    void setPassword(const QString& connectionName, const QString& password);
    void clearPassword(const QString& connectionName);
    void clearAll();

private:
    WebDavCredentialStore() = default;

    QHash<QString, QString> m_passwords;
};

#endif // KEEPASSXC_WEBDAVCREDENTIALSTORE_H
