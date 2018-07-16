/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2018 DigitalGlobe (http://www.digitalglobe.com/)
 */

#include "OsmApiNetworkRequest.h"

//  Hootenanny
#include <hoot/core/util/HootException.h>

//  Qt
#include <QEventLoop>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>

namespace hoot
{

OsmApiNetworkRequest::OsmApiNetworkRequest()
{
}

bool OsmApiNetworkRequest::networkRequest(QUrl url, QNetworkAccessManager::Operation http_op, const QByteArray& data)
{
  //  Reset status
  _status = 0;
  _content.clear();
  //  Do HTTP request
  boost::shared_ptr<QNetworkAccessManager> pNAM(new QNetworkAccessManager());
  QNetworkRequest request(url);

  if (url.scheme().toLower() == "https")
  {
    //  Setup the SSL configuration
    QSslConfiguration config(QSslConfiguration::defaultConfiguration());
    config.setProtocol(QSsl::SslProtocol::AnyProtocol);
    config.setPeerVerifyMode(QSslSocket::VerifyPeer);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(config);
  }

  if (url.userInfo() != "")
  {
    //  Using basic authentication, replace with OAuth when necessary
    QString base64 = url.userInfo().toUtf8().toBase64();
    request.setRawHeader("Authorization", QString("Basic %1").arg(base64).toUtf8());
    url.setUserInfo("");
  }
  //  Call the correct function on the network access manager
  QNetworkReply* reply = NULL;
  switch (http_op)
  {
  case QNetworkAccessManager::Operation::GetOperation:
    reply = pNAM->get(request);
    break;
  case QNetworkAccessManager::Operation::PutOperation:
    reply = pNAM->put(request, data);
    break;
  case QNetworkAccessManager::Operation::PostOperation:
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "text/xml");
    reply = pNAM->post(request, data);
    break;
  default:
    return false;
    break;
  }
  //  Wait for finished signal from reply object
  QEventLoop loop;
  QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
  loop.exec();
  //  Get the status and content of the reply if available
  _status = _getHttpResponseCode(reply);
  if (reply != NULL)
    _content = reply->readAll();
  //  Check error status on our reply
  if (QNetworkReply::NoError != reply->error())
  {
    QString errMsg = reply->errorString();
    throw HootException(QString("Network error for request (%1): %2\n%3")
      .arg(url.toString())
      .arg(errMsg).arg(QString(_content)));
  }
  //  return successfully
  return true;
}

int OsmApiNetworkRequest::_getHttpResponseCode(QNetworkReply* reply)
{
  if (reply != NULL)
  {
    //  Get the status code
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    //  Convert and return it if it is valid
    if (status.isValid())
      return status.toInt();
  }
  return -1;
}

}