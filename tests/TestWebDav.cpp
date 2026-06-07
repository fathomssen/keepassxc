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

#include "TestWebDav.h"
#include "mock/MockWebDavClient.h"

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "webdav/WebDavCredentialStore.h"
#include "webdav/WebDavHandler.h"
#include "webdav/WebDavSettings.h"

#include <QFile>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

QTEST_GUILESS_MAIN(TestWebDav)

static WebDavParams makeParams(const QString& name = QStringLiteral("test"))
{
    WebDavParams p;
    p.name = name;
    p.url = QStringLiteral("https://example.com/db.kdbx");
    p.username = QStringLiteral("user");
    p.timeoutMsec = 5000;
    return p;
}

void TestWebDav::initTestCase()
{
    QVERIFY(Crypto::init());
    // Install mock factory so no real network calls are made
    WebDavHandler::setClientFactory([](QObject* parent) {
        return QScopedPointer<WebDavClient>(new MockWebDavClient(parent));
    });
}

void TestWebDav::testDownloadSuccess()
{
    WebDavClient::Reply reply;
    reply.success = true;
    reply.httpStatus = 200;
    reply.body = QByteArrayLiteral("FAKE_KDBX_CONTENT");
    reply.etag = QStringLiteral("\"abc123\"");
    reply.lastModified = QStringLiteral("Sat, 07 Jun 2026 10:00:00 GMT");

    WebDavHandler::setClientFactory([reply](QObject* parent) {
        auto* mock = new MockWebDavClient(parent);
        mock->setNextGetReply(reply);
        return QScopedPointer<WebDavClient>(mock);
    });

    WebDavParams params = makeParams();
    WebDavHandler handler;
    auto result = handler.download(&params, QStringLiteral("secret"));

    QVERIFY(result.success);
    QVERIFY(!result.filePath.isEmpty());
    QVERIFY(QFile::exists(result.filePath));
    QCOMPARE(result.notModified, false);

    QFile f(result.filePath);
    f.open(QIODevice::ReadOnly);
    QCOMPARE(f.readAll(), reply.body);

    // Cleanup
    QFile::remove(result.filePath);
}

void TestWebDav::testDownloadNotModified304()
{
    // Create a fake cache file
    QTemporaryFile cacheFile;
    cacheFile.setAutoRemove(false);
    QVERIFY(cacheFile.open());
    cacheFile.write(QByteArrayLiteral("CACHED_KDBX"));
    cacheFile.close();

    WebDavClient::Reply reply;
    reply.success = true;
    reply.httpStatus = 304;
    reply.body = {};

    WebDavHandler::setClientFactory([reply](QObject* parent) {
        auto* mock = new MockWebDavClient(parent);
        mock->setNextGetReply(reply);
        return QScopedPointer<WebDavClient>(mock);
    });

    WebDavParams params = makeParams();
    params.lastETag = QStringLiteral("\"abc123\"");
    params.cachedFilePath = cacheFile.fileName();

    WebDavHandler handler;
    auto result = handler.download(&params, QStringLiteral("secret"));

    QVERIFY(result.success);
    QVERIFY(result.notModified);
    QVERIFY(QFile::exists(result.filePath));

    QFile f(result.filePath);
    f.open(QIODevice::ReadOnly);
    QCOMPARE(f.readAll(), QByteArrayLiteral("CACHED_KDBX"));

    QFile::remove(result.filePath);
    QFile::remove(cacheFile.fileName());
}

void TestWebDav::testDownloadOfflineFallback()
{
    QTemporaryFile cacheFile;
    cacheFile.setAutoRemove(false);
    QVERIFY(cacheFile.open());
    cacheFile.write(QByteArrayLiteral("CACHED_KDBX"));
    cacheFile.close();

    WebDavClient::Reply errorReply;
    errorReply.success = false;
    errorReply.errorMessage = QStringLiteral("Network unreachable");

    WebDavHandler::setClientFactory([errorReply](QObject* parent) {
        auto* mock = new MockWebDavClient(parent);
        mock->setNextGetReply(errorReply);
        return QScopedPointer<WebDavClient>(mock);
    });

    WebDavParams params = makeParams();
    params.cachedFilePath = cacheFile.fileName();

    WebDavHandler handler;
    auto result = handler.download(&params, QStringLiteral("secret"));

    QVERIFY(result.success);
    QVERIFY(result.notModified);
    QVERIFY(!result.errorMessage.isEmpty()); // Warning message set

    QFile f(result.filePath);
    f.open(QIODevice::ReadOnly);
    QCOMPARE(f.readAll(), QByteArrayLiteral("CACHED_KDBX"));

    QFile::remove(result.filePath);
    QFile::remove(cacheFile.fileName());
}

void TestWebDav::testDownloadOfflineNoCache()
{
    WebDavClient::Reply errorReply;
    errorReply.success = false;
    errorReply.errorMessage = QStringLiteral("Connection refused");

    WebDavHandler::setClientFactory([errorReply](QObject* parent) {
        auto* mock = new MockWebDavClient(parent);
        mock->setNextGetReply(errorReply);
        return QScopedPointer<WebDavClient>(mock);
    });

    WebDavParams params = makeParams();
    // No cachedFilePath set

    WebDavHandler handler;
    auto result = handler.download(&params, QStringLiteral("secret"));

    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
}

void TestWebDav::testUploadSuccess()
{
    WebDavClient::Reply putReply;
    putReply.success = true;
    putReply.httpStatus = 204;

    WebDavHandler::setClientFactory([putReply](QObject* parent) {
        auto* mock = new MockWebDavClient(parent);
        mock->setNextPutReply(putReply);
        return QScopedPointer<WebDavClient>(mock);
    });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write(QByteArrayLiteral("UPLOAD_CONTENT"));
    tempFile.close();

    WebDavParams params = makeParams();
    WebDavHandler handler;
    auto result = handler.upload(tempFile.fileName(), &params, QStringLiteral("secret"));

    QVERIFY(result.success);
}

void TestWebDav::testUploadFailure()
{
    WebDavClient::Reply putReply;
    putReply.success = false;
    putReply.httpStatus = 403;
    putReply.errorMessage = QStringLiteral("Forbidden");

    WebDavHandler::setClientFactory([putReply](QObject* parent) {
        auto* mock = new MockWebDavClient(parent);
        mock->setNextPutReply(putReply);
        return QScopedPointer<WebDavClient>(mock);
    });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write(QByteArrayLiteral("UPLOAD_CONTENT"));
    tempFile.close();

    WebDavParams params = makeParams();
    WebDavHandler handler;
    auto result = handler.upload(tempFile.fileName(), &params, QStringLiteral("secret"));

    QVERIFY(!result.success);
    QVERIFY(result.errorMessage.contains(QStringLiteral("403")));
}

void TestWebDav::testWebDavSettingsRoundTrip()
{
    auto db = QSharedPointer<Database>::create();

    auto* original = new WebDavParams();
    original->name = QStringLiteral("MyNextcloud");
    original->url = QStringLiteral("https://cloud.example.com/db.kdbx");
    original->username = QStringLiteral("alice");
    original->timeoutMsec = 15000;
    original->lastETag = QStringLiteral("\"etag42\"");
    original->lastModified = QStringLiteral("Sat, 07 Jun 2026 12:00:00 GMT");

    {
        WebDavSettings settings(db);
        settings.addParams(original);
        settings.saveSettings();
    }

    {
        WebDavSettings settings(db);
        settings.loadSettings();
        auto* loaded = settings.getParams(QStringLiteral("MyNextcloud"));
        QVERIFY(loaded != nullptr);
        QCOMPARE(loaded->name, original->name);
        QCOMPARE(loaded->url, original->url);
        QCOMPARE(loaded->username, original->username);
        QCOMPARE(loaded->timeoutMsec, original->timeoutMsec);
        QCOMPARE(loaded->lastETag, original->lastETag);
        QCOMPARE(loaded->lastModified, original->lastModified);
    }

    // Verify the key is protected in custom data
    QVERIFY(db->metadata()->customData()->isProtected(CustomData::WebDavSettings));
}

void TestWebDav::testCredentialStore()
{
    auto* store = WebDavCredentialStore::instance();
    store->clearAll();

    QVERIFY(store->getPassword(QStringLiteral("conn1")).isEmpty());

    store->setPassword(QStringLiteral("conn1"), QStringLiteral("s3cret"));
    QCOMPARE(store->getPassword(QStringLiteral("conn1")), QStringLiteral("s3cret"));

    store->clearPassword(QStringLiteral("conn1"));
    QVERIFY(store->getPassword(QStringLiteral("conn1")).isEmpty());
}
