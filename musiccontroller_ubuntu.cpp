/*
 *  libwatchfish - library with common functionality for SailfishOS smartwatch
 * connector programs. Copyright (C) 2015 Javier S. Pedro
 * <dev.git@javispedro.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Volume control for Ubuntu Touch via AccountsService DBus.
// (com.lomiri.AccountsService.Sound, Volume property is a double 0.0–1.0)
// Public API exposes int 0–100.

#include <unistd.h>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include "musiccontroller.h"
#include "musiccontroller_p.h"

namespace watchfish {

MusicControllerPrivate::~MusicControllerPrivate() { delete _accountsIface; }

void MusicControllerPrivate::connectAccountsBus() {
  if (_accountsIface && _accountsIface->isValid()) {
    return;
  }

  delete _accountsIface;

  // Build the per-user object path dynamically using the current UID
  QString userPath = QString("/org/freedesktop/Accounts/User%1").arg(getuid());

  _accountsIface = new QDBusInterface("org.freedesktop.Accounts", userPath,
                                      "org.freedesktop.DBus.Properties",
                                      QDBusConnection::systemBus());

  if (!_accountsIface->isValid()) {
    qWarning() << "Cannot connect to AccountsService DBus:"
               << _accountsIface->lastError().message();
  }
}

int MusicController::volume() const {
  if (!d_ptr->_accountsIface || !d_ptr->_accountsIface->isValid()) {
    qWarning() << "AccountsService DBus interface not available";
    return -1;
  }

  QDBusReply<QDBusVariant> answer = d_ptr->_accountsIface->call(
      "Get", "com.lomiri.AccountsService.Sound", "Volume");

  if (answer.isValid()) {
    double vol = answer.value().variant().toDouble();
    return qBound(0, static_cast<int>(vol * 100.0 + 0.5), 100);
  }

  qWarning() << "Failed to read volume:" << answer.error().message();
  return -1;
}

void MusicController::setVolume(const uint newVolume) {
  d_ptr->connectAccountsBus();

  if (!d_ptr->_accountsIface || !d_ptr->_accountsIface->isValid()) {
    qWarning() << "AccountsService DBus interface not available";
    return;
  }

  double vol = qBound(0u, newVolume, 100u) / 100.0;
  qDebug() << "Setting volume:" << newVolume << "(" << vol << ")";

  d_ptr->_accountsIface->call(
      "Set", "com.lomiri.AccountsService.Sound", "Volume",
      QVariant::fromValue(QDBusVariant(QVariant::fromValue(vol))));

  emit volumeChanged();
}

void MusicController::volumeUp() {
  d_ptr->connectAccountsBus();
  int curVolume = volume();
  if (curVolume < 0) {
    return;
  }
  if (curVolume >= 100) {
    qDebug() << "Cannot increase volume beyond maximum (100)";
    return;
  }
  setVolume(static_cast<uint>(qMin(curVolume + 5, 100)));
}

void MusicController::volumeDown() {
  d_ptr->connectAccountsBus();
  int curVolume = volume();
  if (curVolume <= 0) {
    qDebug() << "Cannot decrease volume beyond minimum (0)";
    return;
  }
  setVolume(static_cast<uint>(qMax(curVolume - 5, 0)));
}

} // namespace watchfish
