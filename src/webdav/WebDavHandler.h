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

#ifndef KEEPASSXC_WEBDAVHANDLER_H
#define KEEPASSXC_WEBDAVHANDLER_H

#include "WebDavClient.h"
#include "WebDavParams.h"

#include <QObject>
#include <QScopedPointer>
#include <functional>

class WebDavHandler : public QObject
{
    Q_OBJECT
public:
    struct WebDavResult
    {
        bool success = false;
        QString errorMessage;
        QString filePath;
        bool notModified = false; // true when served from local cache (304 or offline fallback)
        QString etag;             // ETag from a successful 200 GET
        QString lastModified;     // Last-Modified from a successful 200 GET
    };

    explicit WebDavHandler(QObject* parent = nullptr);

    // Downloads the database file. Falls back to local cache on network error.
    WebDavResult download(const WebDavParams* params, const QString& password);

    // Uploads the file at filePath via HTTP PUT.
    WebDavResult upload(const QString& filePath, const WebDavParams* params, const QString& password);

    // Updates ETag and Last-Modified in params from a successful GET reply.
    static void updateCacheMetadata(WebDavParams* params, const WebDavClient::Reply& reply);

    // Writes data to the persistent cache directory and returns the absolute path.
    static QString saveToCacheDir(const WebDavParams* params, const QByteArray& data);

    // Inject a custom client factory for testing.
    static void setClientFactory(std::function<QScopedPointer<WebDavClient>(QObject*)> factory);

private:
    static QString tempFilePath();

    static std::function<QScopedPointer<WebDavClient>(QObject*)> m_createClient;
};

#endif // KEEPASSXC_WEBDAVHANDLER_H
