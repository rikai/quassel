/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _NETWORKSSETTINGSPAGE_H_
#define _NETWORKSSETTINGSPAGE_H_

#include <QIcon>

#include "settingspage.h"
#include "ui_networkssettingspage.h"
#include "ui_networkeditdlg.h"
#include "ui_servereditdlg.h"
#include "ui_saveidentitiesdlg.h"

#include "network.h"
#include "types.h"

class NetworksSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    NetworksSettingsPage(QWidget *parent = 0);

    bool aboutToSave();

  public slots:
    void save();
    void load();

  private slots:
    void widgetHasChanged();
    void setWidgetStates();
    void coreConnectionStateChanged(bool);
    void networkConnectionStateChanged(Network::ConnectionState state);
    void networkConnectionError(const QString &msg);

    void displayNetwork(NetworkId);
    void setItemState(NetworkId, QListWidgetItem *item = 0);

    void clientNetworkAdded(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();

    void clientIdentityAdded(IdentityId);
    void clientIdentityRemoved(IdentityId);
    void clientIdentityUpdated();

    void on_networkList_itemSelectionChanged();
    void on_addNetwork_clicked();
    void on_deleteNetwork_clicked();
    void on_renameNetwork_clicked();

    void on_connectNow_clicked();

    void on_serverList_itemSelectionChanged();
    void on_addServer_clicked();
    void on_deleteServer_clicked();
    void on_editServer_clicked();
    void on_upServer_clicked();
    void on_downServer_clicked();

  private:
    Ui::NetworksSettingsPage ui;

    NetworkId currentId;
    QHash<NetworkId, NetworkInfo> networkInfos;
    bool _ignoreWidgetChanges;

    QIcon connectedIcon, connectingIcon, disconnectedIcon;

    void reset();
    bool testHasChanged();
    QListWidgetItem *insertNetwork(NetworkId);
    QListWidgetItem *insertNetwork(const NetworkInfo &info);
    QListWidgetItem *networkItem(NetworkId) const;
    void saveToNetworkInfo(NetworkInfo &);
};

class NetworkEditDlg : public QDialog {
  Q_OBJECT

  public:
    NetworkEditDlg(const QString &old, const QStringList &existing = QStringList(), QWidget *parent = 0);

    QString networkName() const;

  private slots:
    void on_networkEdit_textChanged(const QString &);

  private:
    Ui::NetworkEditDlg ui;

    QStringList existing;
};



class ServerEditDlg : public QDialog {
  Q_OBJECT

  public:
    ServerEditDlg(const QVariant &serverData = QVariant(), QWidget *parent = 0);

    QVariant serverData() const;

  private slots:
    void on_host_textChanged();

  private:
    Ui::ServerEditDlg ui;
};


class SaveNetworksDlg : public QDialog {
  Q_OBJECT

  public:
    SaveNetworksDlg(const QList<NetworkInfo> &toCreate, const QList<NetworkInfo> &toUpdate, const QList<NetworkId> &toRemove, QWidget *parent = 0);

  private slots:
    void clientEvent();

  private:
    Ui::SaveIdentitiesDlg ui;

    int numevents, rcvevents;
};

#endif
