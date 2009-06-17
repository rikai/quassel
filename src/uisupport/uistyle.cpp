/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include <QApplication>

#include "qssparser.h"
#include "quassel.h"
#include "uistyle.h"
#include "uisettings.h"
#include "util.h"

UiStyle::UiStyle() {
  // register FormatList if that hasn't happened yet
  // FIXME I don't think this actually avoids double registration... then again... does it hurt?
  if(QVariant::nameToType("UiStyle::FormatList") == QVariant::Invalid) {
    qRegisterMetaType<FormatList>("UiStyle::FormatList");
    qRegisterMetaTypeStreamOperators<FormatList>("UiStyle::FormatList");
    Q_ASSERT(QVariant::nameToType("UiStyle::FormatList") != QVariant::Invalid);
  }

  // Now initialize the mapping between FormatCodes and FormatTypes...
  _formatCodes["%O"] = None;
  _formatCodes["%B"] = Bold;
  _formatCodes["%S"] = Italic;
  _formatCodes["%U"] = Underline;
  _formatCodes["%R"] = Reverse;

  _formatCodes["%DN"] = Nick;
  _formatCodes["%DH"] = Hostmask;
  _formatCodes["%DC"] = ChannelName;
  _formatCodes["%DM"] = ModeFlags;
  _formatCodes["%DU"] = Url;

  loadStyleSheet();
}

UiStyle::~ UiStyle() {
  qDeleteAll(_metricsCache);
}

QTextCharFormat UiStyle::cachedFormat(quint64 key) const {
  return _formatCache.value(key, QTextCharFormat());
}

QTextCharFormat UiStyle::cachedFormat(quint32 formatType, quint32 messageLabel) const {
  return cachedFormat(formatType | ((quint64)messageLabel << 32));
}

void UiStyle::setCachedFormat(const QTextCharFormat &format, quint32 formatType, quint32 messageLabel) {
  _formatCache[formatType | ((quint64)messageLabel << 32)] = format;
}

// Merge a subelement format into an existing message format
void UiStyle::mergeSubElementFormat(QTextCharFormat& fmt, quint32 ftype, quint32 label) {
  quint64 key = ftype | ((quint64)label << 32);

  // start with the most general format and then specialize
  fmt.merge(cachedFormat(key & 0x00000000fffffff0));  // basic subelement format
  fmt.merge(cachedFormat(key & 0x00000000ffffffff));  // subelement + msgtype
  fmt.merge(cachedFormat(key & 0xffff0000fffffff0));  // subelement + nickhash
  fmt.merge(cachedFormat(key & 0xffff0000ffffffff));  // subelement + nickhash + msgtype
  fmt.merge(cachedFormat(key & 0x0000fffffffffff0));  // label + subelement
  fmt.merge(cachedFormat(key & 0x0000ffffffffffff));  // label + subelement + msgtype
  fmt.merge(cachedFormat(key & 0xfffffffffffffff0));  // label + subelement + nickhash
  fmt.merge(cachedFormat(key & 0xffffffffffffffff));  // label + subelement + nickhash + msgtype
}

// NOTE: This function is intimately tied to the values in FormatType. Don't change this
//       until you _really_ know what you do!
QTextCharFormat UiStyle::format(quint32 ftype, quint32 label) {
  if(ftype == Invalid)
    return QTextCharFormat();

  quint64 key = ftype | ((quint64)label << 32);

  // check if we have exactly this format readily cached already
  QTextCharFormat fmt = cachedFormat(key);
  if(fmt.properties().count())
    return fmt;

  fmt.merge(cachedFormat(key & 0x0000000000000000));  // basic
  fmt.merge(cachedFormat(key & 0x000000000000000f));  // msgtype
  fmt.merge(cachedFormat(key & 0x0000ffff00000000));  // label
  fmt.merge(cachedFormat(key & 0x0000ffff0000000f));  // label + msgtype

  // TODO: allow combinations for mirc formats and colors (each), e.g. setting a special format for "bold and italic"
  //       or "foreground 01 and background 03"
  if((ftype & 0xfff0)) { // element format
    for(quint32 mask = 0x10; mask <= 0x2000; mask <<= 1) {
      if(ftype & mask) {
        mergeSubElementFormat(fmt, ftype & 0xffff, label);
      }
    }
  }

  // Now we handle color codes
  if(ftype & 0x00400000)
    mergeSubElementFormat(fmt, ftype & 0x0f400000, label); // foreground
  if(ftype & 0x00800000)
    mergeSubElementFormat(fmt, ftype & 0xf0800000, label); // background
  if((ftype & 0x00c00000) == 0x00c00000)
    mergeSubElementFormat(fmt, ftype & 0xffc00000, label); // combination

  // URL
  if(ftype & Url)
    mergeSubElementFormat(fmt, ftype & Url, label);

  setCachedFormat(fmt, ftype, label);
  return fmt;
}

QFontMetricsF *UiStyle::fontMetrics(quint32 ftype, quint32 label) {
  // QFontMetricsF is not assignable, so we need to store pointers :/
  quint64 key = ftype | ((quint64)label << 32);

  if(_metricsCache.contains(key))
    return _metricsCache.value(key);

  return (_metricsCache[key] = new QFontMetricsF(format(ftype, label).font()));
}

UiStyle::FormatType UiStyle::formatType(Message::Type msgType) const {
  switch(msgType) {
    case Message::Plain:
      return PlainMsg;
    case Message::Notice:
      return NoticeMsg;
    case Message::Action:
      return ActionMsg;
    case Message::Nick:
      return NickMsg;
    case Message::Mode:
      return ModeMsg;
    case Message::Join:
      return JoinMsg;
    case Message::Part:
      return PartMsg;
    case Message::Quit:
      return QuitMsg;
    case Message::Kick:
      return KickMsg;
    case Message::Kill:
      return KillMsg;
    case Message::Server:
      return ServerMsg;
    case Message::Info:
      return InfoMsg;
    case Message::Error:
      return ErrorMsg;
    case Message::DayChange:
      return DayChangeMsg;
  }
  Q_ASSERT(false); // we need to handle all message types
  return ErrorMsg;
}

UiStyle::FormatType UiStyle::formatType(const QString & code) const {
  if(_formatCodes.contains(code)) return _formatCodes.value(code);
  return Invalid;
}

QString UiStyle::formatCode(FormatType ftype) const {
  return _formatCodes.key(ftype);
}

QList<QTextLayout::FormatRange> UiStyle::toTextLayoutList(const FormatList &formatList, int textLength) {
  QList<QTextLayout::FormatRange> formatRanges;
  QTextLayout::FormatRange range;
  int i = 0;
  for(i = 0; i < formatList.count(); i++) {
    range.format = format(formatList.at(i).second);
    range.start = formatList.at(i).first;
    if(i > 0) formatRanges.last().length = range.start - formatRanges.last().start;
    formatRanges.append(range);
  }
  if(i > 0) formatRanges.last().length = textLength - formatRanges.last().start;
  return formatRanges;
}

// This method expects a well-formatted string, there is no error checking!
// Since we create those ourselves, we should be pretty safe that nobody does something crappy here.
UiStyle::StyledString UiStyle::styleString(const QString &s_, quint32 baseFormat) {
  QString s = s_;
  if(s.length() > 65535) {
    qWarning() << QString("String too long to be styled: %1").arg(s);
    return StyledString();
  }
  StyledString result;
  result.formatList.append(qMakePair((quint16)0, (quint32)None));
  quint32 curfmt = baseFormat;
  int pos = 0; quint16 length = 0;
  for(;;) {
    pos = s.indexOf('%', pos);
    if(pos < 0) break;
    if(s[pos+1] == '%') { // escaped %, we just remove one and continue
      s.remove(pos, 1);
      pos++;
      continue;
    }
    if(s[pos+1] == 'D' && s[pos+2] == 'c') { // color code
      if(s[pos+3] == '-') {  // color off
        curfmt &= 0x003fffff;
        length = 4;
      } else {
        int color = 10 * s[pos+4].digitValue() + s[pos+5].digitValue();
        //TODO: use 99 as transparent color (re mirc color "standard")
        color &= 0x0f;
        if(s[pos+3] == 'f') {
          curfmt &= 0xf0ffffff;
          curfmt |= (color << 24) | 0x00400000;
        } else {
          curfmt &= 0x0fffffff;
          curfmt |= (color << 28) | 0x00800000;
        }
        length = 6;
      }
    } else if(s[pos+1] == 'O') { // reset formatting
      curfmt &= 0x0000000f; // we keep message type-specific formatting
      length = 2;
    } else if(s[pos+1] == 'R') { // reverse
      // TODO: implement reverse formatting

      length = 2;
    } else { // all others are toggles
      QString code = QString("%") + s[pos+1];
      if(s[pos+1] == 'D') code += s[pos+2];
      FormatType ftype = formatType(code);
      if(ftype == Invalid) {
        qWarning() << (QString("Invalid format code in string: %1").arg(s));
        continue;
      }
      curfmt ^= ftype;
      length = code.length();
    }
    s.remove(pos, length);
    if(pos == result.formatList.last().first)
      result.formatList.last().second = curfmt;
    else
      result.formatList.append(qMakePair((quint16)pos, curfmt));
  }
  result.plainText = s;
  return result;
}

QString UiStyle::mircToInternal(const QString &mirc_) const {
  QString mirc = mirc_;
  mirc.replace('%', "%%");      // escape % just to be sure
  mirc.replace('\x02', "%B");
  mirc.replace('\x0f', "%O");
  mirc.replace('\x12', "%R");
  mirc.replace('\x16', "%R");
  mirc.replace('\x1d', "%S");
  mirc.replace('\x1f', "%U");

  // Now we bring the color codes (\x03) in a sane format that can be parsed more easily later.
  // %Dcfxx is foreground, %Dcbxx is background color, where xx is a 2 digit dec number denoting the color code.
  // %Dc- turns color off.
  // Note: We use the "mirc standard" as described in <http://www.mirc.co.uk/help/color.txt>.
  //       This means that we don't accept something like \x03,5 (even though others, like WeeChat, do).
  int pos = 0;
  for(;;) {
    pos = mirc.indexOf('\x03', pos);
    if(pos < 0) break; // no more mirc color codes
    QString ins, num;
    int l = mirc.length();
    int i = pos + 1;
    // check for fg color
    if(i < l && mirc[i].isDigit()) {
      num = mirc[i++];
      if(i < l && mirc[i].isDigit()) num.append(mirc[i++]);
      else num.prepend('0');
      ins = QString("%Dcf%1").arg(num);

      if(i+1 < l && mirc[i] == ',' && mirc[i+1].isDigit()) {
        i++;
        num = mirc[i++];
        if(i < l && mirc[i].isDigit()) num.append(mirc[i++]);
        else num.prepend('0');
        ins += QString("%Dcb%1").arg(num);
      }
    } else {
      ins = "%Dc-";
    }
    mirc.replace(pos, i-pos, ins);
  }
  return mirc;
}

/***********************************************************************************/
UiStyle::StyledMessage::StyledMessage(const Message &msg)
  : Message(msg)
{
}

void UiStyle::StyledMessage::style(UiStyle *style) const {
  QString user = userFromMask(sender());
  QString host = hostFromMask(sender());
  QString nick = nickFromMask(sender());
  QString txt = style->mircToInternal(contents());
  QString bufferName = bufferInfo().bufferName();
  bufferName.replace('%', "%%"); // well, you _can_ have a % in a buffername apparently... -_-

  QString t;
  switch(type()) {
    case Message::Plain:
      //: Plain Message
      t = tr("%1").arg(txt); break;
    case Message::Notice:
      //: Notice Message
      t = tr("%1").arg(txt); break;
    case Message::Action:
      //: Action Message
      t = tr("%DN%1%DN %2").arg(nick).arg(txt);
      break;
    case Message::Nick:
      //: Nick Message
      if(nick == contents()) t = tr("You are now known as %DN%1%DN").arg(txt);
      else t = tr("%DN%1%DN is now known as %DN%2%DN").arg(nick, txt);
      break;
    case Message::Mode:
      //: Mode Message
      if(nick.isEmpty()) t = tr("User mode: %DM%1%DM").arg(txt);
      else t = tr("Mode %DM%1%DM by %DN%2%DN").arg(txt, nick);
      break;
    case Message::Join:
      //: Join Message
      t = tr("%DN%1%DN %DH(%2@%3)%DH has joined %DC%4%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      //: Part Message
      t = tr("%DN%1%DN %DH(%2@%3)%DH has left %DC%4%DC").arg(nick, user, host, bufferName);
      if(!txt.isEmpty()) t = QString("%1 (%2)").arg(t).arg(txt);
      break;
    case Message::Quit:
      //: Quit Message
      t = tr("%DN%1%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
      if(!txt.isEmpty()) t = QString("%1 (%2)").arg(t).arg(txt);
      break;
    case Message::Kick: {
        QString victim = txt.section(" ", 0, 0);
        QString kickmsg = txt.section(" ", 1);
        //: Kick Message
        t = tr("%DN%1%DN has kicked %DN%2%DN from %DC%3%DC").arg(nick).arg(victim).arg(bufferName);
        if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
      }
      break;
    //case Message::Kill: FIXME

    case Message::Server:
      //: Server Message
      t = tr("%1").arg(txt); break;
    case Message::Info:
      //: Info Message
      t = tr("%1").arg(txt); break;
    case Message::Error:
      //: Error Message
      t = tr("%1").arg(txt); break;
    case Message::DayChange:
      //: Day Change Message
      t = tr("{Day changed to %1}").arg(timestamp().toString());
      break;
    default:
      t = tr("[%1]").arg(txt);
  }
  _contents = style->styleString(t, style->formatType(type()));
}

QString UiStyle::StyledMessage::decoratedTimestamp() const {
  return QString("[%1]").arg(timestamp().toLocalTime().toString("hh:mm:ss"));
}

QString UiStyle::StyledMessage::plainSender() const {
  switch(type()) {
    case Message::Plain:
    case Message::Notice:
      return nickFromMask(sender());
    default:
      return QString();
  }
}

QString UiStyle::StyledMessage::decoratedSender() const {
  switch(type()) {
    case Message::Plain:
      return tr("<%1>").arg(plainSender()); break;
    case Message::Notice:
      return tr("[%1]").arg(plainSender()); break;
<<<<<<< HEAD
    case Message::Topic:
    case Message::Server:
      return tr("*"); break;
    case Message::Error:
      return tr("*"); break;
=======
    case Message::Action:
      return tr("-*-"); break;
    case Message::Nick:
      return tr("<->"); break;
    case Message::Mode:
      return tr("***"); break;
>>>>>>> Handle all message types properly in UiStyle; eliminate msgtype format codes
    case Message::Join:
      return tr("-->"); break;
    case Message::Part:
      return tr("<--"); break;
    case Message::Quit:
      return tr("<--"); break;
    case Message::Kick:
      return tr("<-*"); break;
    case Message::Kill:
      return tr("<-x"); break;
    case Message::Server:
      return tr("*"); break;
    case Message::Info:
      return tr("*"); break;
    case Message::Error:
      return tr("*"); break;
    case Message::DayChange:
      return tr("-"); break;
    default:
      return tr("%1").arg(plainSender());
  }
}

UiStyle::FormatType UiStyle::StyledMessage::senderFormat() const {
  switch(type()) {
    case Message::Plain:
      // To produce random like but stable nick colorings some sort of hashing should work best.
      // In this case we just use the qt function qChecksum which produces a
      // CRC16 hash. This should be fast and 16 bits are more than enough.
      {
        QString nick = nickFromMask(sender()).toLower();
        if(!nick.isEmpty()) {
          int chopCount = 0;
          while(nick[nick.count() - 1 - chopCount] == '_') {
            chopCount++;
          }
          nick.chop(chopCount);
        }
        quint16 hash = qChecksum(nick.toAscii().data(), nick.toAscii().size());
        return (UiStyle::FormatType)((((hash % 12) + 1) << 24) + 0x200); // FIXME: amount of sender colors hardwired
      }
    case Message::Notice:
      return UiStyle::NoticeMsg; break;
    case Message::Topic:
    case Message::Server:
      return UiStyle::ServerMsg; break;
    case Message::Error:
      return UiStyle::ErrorMsg; break;
    case Message::Join:
      return UiStyle::JoinMsg; break;
    case Message::Part:
      return UiStyle::PartMsg; break;
    case Message::Quit:
      return UiStyle::QuitMsg; break;
    case Message::Kick:
      return UiStyle::KickMsg; break;
    case Message::Nick:
      return UiStyle::NickMsg; break;
    case Message::Mode:
      return UiStyle::ModeMsg; break;
    case Message::Action:
      return UiStyle::ActionMsg; break;
    default:
      return UiStyle::ErrorMsg;
  }
}

/***********************************************************************************/

QDataStream &operator<<(QDataStream &out, const UiStyle::FormatList &formatList) {
  out << formatList.count();
  UiStyle::FormatList::const_iterator it = formatList.begin();
  while(it != formatList.end()) {
    out << (*it).first << (*it).second;
    ++it;
  }
  return out;
}

QDataStream &operator>>(QDataStream &in, UiStyle::FormatList &formatList) {
  quint16 cnt;
  in >> cnt;
  for(quint16 i = 0; i < cnt; i++) {
    quint16 pos; quint32 ftype;
    in >> pos >> ftype;
    formatList.append(qMakePair((quint16)pos, ftype));
  }
  return in;
}

/***********************************************************************************/
// Stylesheet handling
/***********************************************************************************/

void UiStyle::loadStyleSheet() {
  QssParser parser;
  parser.loadStyleSheet(qApp->styleSheet());

  // TODO handle results
  QApplication::setPalette(parser.palette());

  qDeleteAll(_metricsCache);
  _metricsCache.clear();
  _formatCache = parser.formats();
}
