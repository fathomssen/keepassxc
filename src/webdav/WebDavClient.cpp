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

#include "WebDavClient.h"

#include "networking/NetworkManager.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

QNetworkAccessManager* WebDavClient::s_netMgr = nullptr;

WebDavClient::WebDavClient(QObject* parent)
    : QObject(parent)
{
}

void WebDavClient::setNetworkAccessManager(QNetworkAccessManager* mgr)
{
    s_netMgr = mgr;
}

void WebDavClient::setBasicAuth(QNetworkRequest& request, const QString& username, const QString& password)
{
    const QByteArray credentials = (username + QStringLiteral(":") + password).toUtf8().toBase64();
    request.setRawHeader("Authorization", QByteArrayLiteral("Basic ") + credentials);
}

WebDavClient::Reply WebDavClient::get(const QUrl& url,
                                      const QString& username,
                                      const QString& password,
                                      const QString& ifNoneMatchEtag,
                                      int timeoutMsec)
{
    QNetworkRequest request(url);
    setBasicAuth(request, username, password);
    if (!ifNoneMatchEtag.isEmpty()) {
        request.setRawHeader("If-None-Match", ifNoneMatchEtag.toUtf8());
    }
    return execRequest(request, "GET", {}, timeoutMsec);
}

WebDavClient::Reply WebDavClient::put(const QUrl& url,
                                      const QString& username,
                                      const QString& password,
                                      const QByteArray& data,
                                      int timeoutMsec)
{
    QNetworkRequest request(url);
    setBasicAuth(request, username, password);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
    return execRequest(request, "PUT", data, timeoutMsec);
}

WebDavClient::Reply WebDavClient::execRequest(QNetworkRequest& request,
                                              const QByteArray& verb,
                                              const QByteArray& sendData,
                                              int timeoutMsec)
{
    Reply result;
    QNetworkAccessManager* mgr = s_netMgr ? s_netMgr : getNetMgr();

    QNetworkReply* reply = nullptr;
    if (verb == "GET") {
        reply = mgr->get(request);
    } else if (verb == "PUT") {
        reply = mgr->put(request, sendData);
    } else {
        result.errorMessage = tr("Unsupported HTTP verb: %1").arg(QString::fromUtf8(verb));
        return result;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMsec);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        result.errorMessage = tr("Request timed out after %1 ms.").arg(timeoutMsec);
        return result;
    }

    result.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.etag = QString::fromUtf8(reply->rawHeader("ETag"));
    result.lastModified = QString::fromUtf8(reply->rawHeader("Last-Modified"));

    if (reply->error() != QNetworkReply::NoError && result.httpStatus != 304) {
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        return result;
    }

    result.body = reply->readAll();
    result.success = true;
    reply->deleteLater();
    return result;
}
