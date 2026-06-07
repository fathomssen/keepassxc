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

#include "WebDavOpenDialog.h"
#include "ui_WebDavOpenDialog.h"

#include <QDialogButtonBox>
#include <QPushButton>

WebDavOpenDialog::WebDavOpenDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::WebDavOpenDialog)
{
    m_ui->setupUi(this);

    auto* openButton = m_ui->buttonBox->button(QDialogButtonBox::Open);
    if (openButton) {
        openButton->setEnabled(false);
    }

    connect(m_ui->nameLineEdit, &QLineEdit::textChanged, this, &WebDavOpenDialog::updateOpenButton);
    connect(m_ui->urlLineEdit, &QLineEdit::textChanged, this, &WebDavOpenDialog::updateOpenButton);
}

WebDavOpenDialog::~WebDavOpenDialog() = default;

void WebDavOpenDialog::updateOpenButton()
{
    auto* openButton = m_ui->buttonBox->button(QDialogButtonBox::Open);
    if (openButton) {
        openButton->setEnabled(!m_ui->nameLineEdit->text().trimmed().isEmpty()
                               && !m_ui->urlLineEdit->text().trimmed().isEmpty());
    }
}

WebDavParams WebDavOpenDialog::params() const
{
    WebDavParams p;
    p.name = m_ui->nameLineEdit->text().trimmed();
    p.url = m_ui->urlLineEdit->text().trimmed();
    p.username = m_ui->usernameLineEdit->text().trimmed();
    p.timeoutMsec = m_ui->timeoutSec->value() * 1000;
    return p;
}

QString WebDavOpenDialog::password() const
{
    return m_ui->passwordLineEdit->text();
}
