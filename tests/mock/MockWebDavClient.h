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

#ifndef KEEPASSXC_MOCKWEBDAVCLIENT_H
#define KEEPASSXC_MOCKWEBDAVCLIENT_H

#include "webdav/WebDavClient.h"

class MockWebDavClient : public WebDavClient
{
public:
    explicit MockWebDavClient(QObject* parent = nullptr);

    void setNextGetReply(const WebDavClient::Reply& reply);
    void setNextPutReply(const WebDavClient::Reply& reply);

    Reply get(const QUrl& url,
              const QString& username,
              const QString& password,
              const QString& ifNoneMatchEtag = {},
              int timeoutMsec = 30000) override;

    Reply put(const QUrl& url,
              const QString& username,
              const QString& password,
              const QByteArray& data,
              int timeoutMsec = 30000) override;

private:
    Reply m_nextGet;
    Reply m_nextPut;
};

#endif // KEEPASSXC_MOCKWEBDAVCLIENT_H
