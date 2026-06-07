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

#ifndef KEEPASSX_DATABASESETTINGSWIDGETWEBDAV_H
#define KEEPASSX_DATABASESETTINGSWIDGETWEBDAV_H

#include "gui/dbsettings/DatabaseSettingsWidget.h"

#include <QListWidgetItem>
#include <QPointer>

class WebDavSettings;

namespace Ui
{
    class DatabaseSettingsWidgetWebDav;
}

class DatabaseSettingsWidgetWebDav : public DatabaseSettingsWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingsWidgetWebDav(QWidget* parent = nullptr);
    Q_DISABLE_COPY(DatabaseSettingsWidgetWebDav);
    ~DatabaseSettingsWidgetWebDav() override;

public slots:
    void initialize() override;
    void uninitialize() override;
    bool saveSettings() override;

private slots:
    void saveCurrentSettings();
    void removeCurrentSettings();
    void editCurrentSettings();
    void testConnection();

private:
    void updateSettingsList();
    QListWidgetItem* findItemByName(const QString& name);
    void clearFields();

    QScopedPointer<WebDavSettings> m_webDavSettings;
    const QScopedPointer<Ui::DatabaseSettingsWidgetWebDav> m_ui;
    bool m_modified = false;
};

#endif // KEEPASSX_DATABASESETTINGSWIDGETWEBDAV_H
