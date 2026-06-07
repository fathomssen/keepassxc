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

#include "DatabaseSettingsWidgetWebDav.h"
#include "ui_DatabaseSettingsWidgetWebDav.h"

#include "gui/MessageBox.h"
#include "webdav/WebDavClient.h"
#include "webdav/WebDavCredentialStore.h"
#include "webdav/WebDavSettings.h"

DatabaseSettingsWidgetWebDav::DatabaseSettingsWidgetWebDav(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_webDavSettings(new WebDavSettings(nullptr, this))
    , m_ui(new Ui::DatabaseSettingsWidgetWebDav())
{
    m_ui->setupUi(this);
    m_ui->messageWidget->setHidden(true);

    connect(m_ui->saveSettingsButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetWebDav::saveCurrentSettings);
    connect(
        m_ui->removeSettingsButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetWebDav::removeCurrentSettings);
    connect(m_ui->settingsListWidget,
            &QListWidget::itemSelectionChanged,
            this,
            &DatabaseSettingsWidgetWebDav::editCurrentSettings);
    connect(m_ui->testConnectionButton, &QPushButton::clicked, this, &DatabaseSettingsWidgetWebDav::testConnection);

    auto setModified = [this]() { m_modified = true; };
    connect(m_ui->nameLineEdit, &QLineEdit::textChanged, setModified);
    connect(m_ui->urlLineEdit, &QLineEdit::textChanged, setModified);
    connect(m_ui->usernameLineEdit, &QLineEdit::textChanged, setModified);
    connect(m_ui->passwordLineEdit, &QLineEdit::textChanged, setModified);
    connect(m_ui->timeoutSec, QOverload<int>::of(&QSpinBox::valueChanged), setModified);
}

DatabaseSettingsWidgetWebDav::~DatabaseSettingsWidgetWebDav() = default;

void DatabaseSettingsWidgetWebDav::initialize()
{
    clearFields();
    m_webDavSettings->setDatabase(m_db);
    updateSettingsList();
    if (m_ui->settingsListWidget->count() > 0) {
        m_ui->settingsListWidget->setCurrentRow(0);
        m_ui->removeSettingsButton->setEnabled(true);
    } else {
        m_ui->removeSettingsButton->setDisabled(true);
    }
}

void DatabaseSettingsWidgetWebDav::uninitialize()
{
}

bool DatabaseSettingsWidgetWebDav::saveSettings()
{
    if (m_modified) {
        auto ans = MessageBox::question(this,
                                        tr("Save WebDAV Settings"),
                                        tr("You have unsaved changes. Do you want to save them?"),
                                        MessageBox::Save | MessageBox::Discard | MessageBox::Cancel,
                                        MessageBox::Save);
        if (ans == MessageBox::Save) {
            saveCurrentSettings();
        } else if (ans == MessageBox::Cancel) {
            return false;
        }
    }

    m_webDavSettings->saveSettings();
    return true;
}

void DatabaseSettingsWidgetWebDav::saveCurrentSettings()
{
    const QString name = m_ui->nameLineEdit->text();
    if (name.isEmpty()) {
        m_ui->messageWidget->showMessage(tr("Name cannot be empty."), MessageWidget::Warning);
        return;
    }
    const QString url = m_ui->urlLineEdit->text();
    if (url.isEmpty()) {
        m_ui->messageWidget->showMessage(tr("URL cannot be empty."), MessageWidget::Warning);
        return;
    }

    auto* params = new WebDavParams();
    params->name = name;
    params->url = url;
    params->username = m_ui->usernameLineEdit->text();
    params->timeoutMsec = m_ui->timeoutSec->value() * 1000;

    // Preserve existing cache metadata if this is an update to an existing entry
    if (const auto* existing = m_webDavSettings->getParams(name)) {
        params->cachedFilePath = existing->cachedFilePath;
        params->lastETag = existing->lastETag;
        params->lastModified = existing->lastModified;
    }

    m_webDavSettings->addParams(params);

    // Store password in session cache (not persisted to disk)
    const QString password = m_ui->passwordLineEdit->text();
    if (!password.isEmpty()) {
        WebDavCredentialStore::instance()->setPassword(name, password);
    }

    updateSettingsList();
    m_ui->settingsListWidget->setCurrentItem(findItemByName(name));
    m_ui->removeSettingsButton->setEnabled(true);
    m_modified = false;
}

QListWidgetItem* DatabaseSettingsWidgetWebDav::findItemByName(const QString& name)
{
    return m_ui->settingsListWidget->findItems(name, Qt::MatchExactly).first();
}

void DatabaseSettingsWidgetWebDav::removeCurrentSettings()
{
    const QString name = m_ui->nameLineEdit->text();
    WebDavCredentialStore::instance()->clearPassword(name);
    m_webDavSettings->removeParams(name);
    updateSettingsList();
    if (!m_webDavSettings->getAllParams().empty()) {
        m_ui->settingsListWidget->setCurrentRow(0);
        m_ui->removeSettingsButton->setEnabled(true);
    } else {
        clearFields();
        m_ui->removeSettingsButton->setDisabled(true);
    }
}

void DatabaseSettingsWidgetWebDav::editCurrentSettings()
{
    if (!m_ui->settingsListWidget->currentItem()) {
        return;
    }
    const QString name = m_ui->settingsListWidget->currentItem()->text();
    const auto* params = m_webDavSettings->getParams(name);
    if (!params) {
        return;
    }

    m_ui->nameLineEdit->setText(params->name);
    m_ui->urlLineEdit->setText(params->url);
    m_ui->usernameLineEdit->setText(params->username);
    m_ui->timeoutSec->setValue(params->timeoutMsec / 1000);

    // Restore cached password from session store (shown as placeholder if not cached)
    const QString cachedPw = WebDavCredentialStore::instance()->getPassword(name);
    m_ui->passwordLineEdit->setText(cachedPw);
    m_modified = false;
}

void DatabaseSettingsWidgetWebDav::updateSettingsList()
{
    m_ui->settingsListWidget->clear();
    for (const auto* params : m_webDavSettings->getAllParams()) {
        auto* item = new QListWidgetItem(m_ui->settingsListWidget);
        item->setText(params->name);
        m_ui->settingsListWidget->addItem(item);
    }
}

void DatabaseSettingsWidgetWebDav::clearFields()
{
    m_ui->nameLineEdit->clear();
    m_ui->urlLineEdit->clear();
    m_ui->usernameLineEdit->clear();
    m_ui->passwordLineEdit->clear();
    m_ui->timeoutSec->setValue(30);
    m_modified = false;
}

void DatabaseSettingsWidgetWebDav::testConnection()
{
    const QString url = m_ui->urlLineEdit->text();
    if (url.isEmpty()) {
        m_ui->messageWidget->showMessage(tr("URL cannot be empty."), MessageWidget::Warning);
        return;
    }

    const QString username = m_ui->usernameLineEdit->text();
    const QString password = m_ui->passwordLineEdit->text();

    WebDavClient client;
    // Use GET with If-None-Match "*" to check connectivity without fetching the full file
    const auto reply = client.get(QUrl(url), username, password, {}, m_ui->timeoutSec->value() * 1000);

    if (reply.success || reply.httpStatus == 304) {
        m_ui->messageWidget->showMessage(tr("Connection successful (HTTP %1).").arg(reply.httpStatus),
                                         MessageWidget::Positive);
    } else {
        m_ui->messageWidget->showMessage(
            tr("Connection failed (HTTP %1): %2").arg(reply.httpStatus).arg(reply.errorMessage),
            MessageWidget::Error);
    }
}
