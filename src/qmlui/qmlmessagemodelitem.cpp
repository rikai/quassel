/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#include "qmlchatline.h"
#include "qmlmessagemodel.h"
#include "qmlmessagemodelitem.h"
#include "uistyle.h"

QmlMessageModelItem::QmlMessageModelItem(const Message &msg)
  : MessageModelItem(),
    _styledMsg(msg)
{
  if(!msg.sender().contains('!'))
    _styledMsg.setFlags(msg.flags() |= Message::ServerMsg);
}

QVariant QmlMessageModelItem::data(int column, int role) const {
  QVariant variant;
  switch(role) {
  case QmlMessageModel::ChatLineDataRole: {
    QmlChatLine::Data data;
    data.timestamp.text = _styledMsg.decoratedTimestamp();
    data.timestamp.formats = UiStyle::FormatList() << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Timestamp);
    data.sender.text = _styledMsg.decoratedSender();
    data.sender.formats = UiStyle::FormatList() << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Sender);
    data.contents.text = _styledMsg.plainContents();
    data.contents.formats = _styledMsg.contentsFormatList();
    return QVariant::fromValue<QmlChatLine::Data>(data);
  }
  default:
    break;
  }
  if(!variant.isValid())
    return MessageModelItem::data(column, role);
  return variant;
}
