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

#include "WebDavHandler.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

std::function<QScopedPointer<WebDavClient>(QObject*)> WebDavHandler::m_createClient(
    [](QObject* parent) { return QScopedPointer<WebDavClient>(new WebDavClient(parent)); });

WebDavHandler::WebDavHandler(QObject* parent)
    : QObject(parent)
{
}

void WebDavHandler::setClientFactory(std::function<QScopedPointer<WebDavClient>(QObject*)> factory)
{
    m_createClient = std::move(factory);
}

QString WebDavHandler::tempFilePath()
{
    QString uuid = QUuid::createUuid().toString().remove(0, 1);
    uuid.chop(1);
    return QDir::toNativeSeparators(QDir::temp().absoluteFilePath("WebDavDatabase-" + uuid + ".kdbx"));
}

QString WebDavHandler::saveToCacheDir(const WebDavParams* params, const QByteArray& data)
{
    const QString cacheBase =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/webdav/");
    QDir().mkpath(cacheBase);

    static const QRegularExpression invalidChars(QStringLiteral("[^a-zA-Z0-9_\\-]"));
    const QString safeName = QString(params->name).replace(invalidChars, QStringLiteral("_"));
    const QString cachePath = cacheBase + safeName + QStringLiteral(".kdbx");

    QFile file(cachePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(data);
    }
    return cachePath;
}

void WebDavHandler::updateCacheMetadata(WebDavParams* params, const WebDavClient::Reply& reply)
{
    if (!reply.etag.isEmpty()) {
        params->lastETag = reply.etag;
    }
    if (!reply.lastModified.isEmpty()) {
        params->lastModified = reply.lastModified;
    }
}

WebDavHandler::WebDavResult WebDavHandler::download(const WebDavParams* params, const QString& password)
{
    WebDavResult result;
    if (!params) {
        result.errorMessage = tr("Invalid WebDAV parameters.");
        return result;
    }

    const QString destPath = tempFilePath();
    auto client = m_createClient(nullptr);
    const WebDavClient::Reply reply = client->get(
        QUrl(params->url), params->username, password, params->lastETag, params->timeoutMsec);

    if (reply.httpStatus == 304) {
        // Server confirmed nothing changed — use cache
        if (!params->cachedFilePath.isEmpty() && QFile::exists(params->cachedFilePath)) {
            QFile::copy(params->cachedFilePath, destPath);
            result.success = true;
            result.filePath = destPath;
            result.notModified = true;
            return result;
        }
        // Cache missing despite 304 — fall through to a full re-download without ETag
        const WebDavClient::Reply fullReply =
            client->get(QUrl(params->url), params->username, password, {}, params->timeoutMsec);
        if (fullReply.success) {
            QFile file(destPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fullReply.body);
            }
            result.success = true;
            result.filePath = destPath;
            return result;
        }
        result.errorMessage = fullReply.errorMessage;
        return result;
    }

    if (!reply.success) {
        // Network error — try offline cache
        if (!params->cachedFilePath.isEmpty() && QFile::exists(params->cachedFilePath)) {
            QFile::copy(params->cachedFilePath, destPath);
            result.success = true;
            result.filePath = destPath;
            result.notModified = true;
            result.errorMessage = tr("WebDAV server unreachable, using cached copy.");
            return result;
        }
        result.errorMessage = reply.errorMessage;
        return result;
    }

    // Successful 2xx response
    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.errorMessage = tr("Could not create temporary file: %1").arg(destPath);
        return result;
    }
    file.write(reply.body);
    file.close();

    if (QFileInfo(destPath).size() == 0) {
        result.errorMessage = tr("Downloaded file is empty.");
        return result;
    }

    result.success = true;
    result.filePath = destPath;
    result.etag = reply.etag;
    result.lastModified = reply.lastModified;
    return result;
}

WebDavHandler::WebDavResult WebDavHandler::upload(const QString& filePath,
                                                  const WebDavParams* params,
                                                  const QString& password)
{
    WebDavResult result;
    if (!params) {
        result.errorMessage = tr("Invalid WebDAV parameters.");
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = tr("Could not read file for upload: %1").arg(filePath);
        return result;
    }
    const QByteArray data = file.readAll();

    auto client = m_createClient(nullptr);
    const WebDavClient::Reply reply = client->put(QUrl(params->url), params->username, password, data, params->timeoutMsec);

    if (!reply.success) {
        result.errorMessage = tr("Upload failed (HTTP %1): %2").arg(reply.httpStatus).arg(reply.errorMessage);
        return result;
    }

    result.success = true;
    return result;
}
