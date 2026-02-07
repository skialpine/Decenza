#include "librarysharing.h"
#include "version.h"
#include "../core/settings.h"
#include "../core/widgetlibrary.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QBuffer>
#include <QUuid>
#include <QDebug>

static const char* API_BASE = "https://api.decenza.coffee/v1/library";

LibrarySharing::LibrarySharing(Settings* settings, WidgetLibrary* library, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_library(library)
{
}

// ---------------------------------------------------------------------------
// Request builder
// ---------------------------------------------------------------------------

QNetworkRequest LibrarySharing::buildRequest(const QString& path) const
{
    QNetworkRequest request{QUrl(QString("%1%2").arg(API_BASE, path))};
    request.setRawHeader("User-Agent",
        QString("Decenza-DE1/%1").arg(VERSION_STRING).toUtf8());
    request.setRawHeader("X-Device-Id", m_settings->deviceId().toUtf8());
    return request;
}

// ---------------------------------------------------------------------------
// Upload
// ---------------------------------------------------------------------------

void LibrarySharing::uploadEntry(const QString& entryId)
{
    uploadEntryWithThumbnail(entryId, QImage());
}

void LibrarySharing::uploadEntryWithThumbnail(const QString& entryId, const QImage& thumbnail)
{
    if (m_uploading) {
        qWarning() << "LibrarySharing: Already uploading";
        return;
    }

    QByteArray entryJson = m_library->exportEntry(entryId);
    if (entryJson.isEmpty()) {
        setLastError("Entry not found: " + entryId);
        emit uploadFailed(m_lastError);
        return;
    }

    setUploading(true);
    setLastError(QString());

    // Convert thumbnail to PNG bytes if provided
    QByteArray thumbnailPng;
    QImage img = thumbnail;
    if (img.isNull()) {
        // Try loading from the library's stored thumbnail
        QString thumbPath = m_library->thumbnailPath(entryId);
        if (!thumbPath.isEmpty()) {
            img.load(thumbPath);
        }
    }
    if (!img.isNull()) {
        QBuffer buf(&thumbnailPng);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
    }

    // Build multipart request
    QString boundary = "----DecenzaBoundary" + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QNetworkRequest request = buildRequest("/entries");
    request.setHeader(QNetworkRequest::ContentTypeHeader,
        QString("multipart/form-data; boundary=%1").arg(boundary));

    QByteArray body = buildMultipart(entryJson, thumbnailPng, boundary);

    qDebug() << "LibrarySharing: Uploading entry" << entryId
             << "(" << body.size() << "bytes)";
    qDebug() << "LibrarySharing: Content-Type:" << request.header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << "LibrarySharing: Entry JSON (" << entryJson.size() << "bytes):" << entryJson.left(500);
    if (!thumbnailPng.isEmpty())
        qDebug() << "LibrarySharing: Thumbnail:" << thumbnailPng.size() << "bytes";
    else
        qDebug() << "LibrarySharing: No thumbnail";

    QNetworkReply* reply = m_networkManager.post(request, body);
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onUploadFinished);
}

QByteArray LibrarySharing::buildMultipart(const QByteArray& entryJson,
                                            const QByteArray& thumbnailPng,
                                            const QString& boundary) const
{
    QByteArray body;
    QByteArray sep = ("--" + boundary + "\r\n").toUtf8();
    QByteArray end = ("--" + boundary + "--\r\n").toUtf8();

    // JSON part
    body += sep;
    body += "Content-Disposition: form-data; name=\"entry\"; filename=\"entry.json\"\r\n";
    body += "Content-Type: application/json\r\n\r\n";
    body += entryJson;
    body += "\r\n";

    // Thumbnail part (optional)
    if (!thumbnailPng.isEmpty()) {
        body += sep;
        body += "Content-Disposition: form-data; name=\"thumbnail\"; filename=\"thumbnail.png\"\r\n";
        body += "Content-Type: image/png\r\n\r\n";
        body += thumbnailPng;
        body += "\r\n";
    }

    body += end;
    return body;
}

void LibrarySharing::onUploadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    setUploading(false);

    QByteArray responseBody = reply->readAll();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "LibrarySharing: Upload response status:" << statusCode;
    qDebug() << "LibrarySharing: Upload response body:" << responseBody.left(1000);

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qWarning() << "LibrarySharing: Upload failed -" << error;
        setLastError(error);
        emit uploadFailed(error);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseBody);
    QJsonObject obj = doc.object();

    if (obj.contains("id")) {
        QString serverId = obj["id"].toString();
        qDebug() << "LibrarySharing: Upload successful, server ID:" << serverId;
        emit uploadSuccess(serverId);
    } else {
        QString error = obj["error"].toString("Upload failed");
        qWarning() << "LibrarySharing: Server error -" << error;
        setLastError(error);
        emit uploadFailed(error);
    }
}

// ---------------------------------------------------------------------------
// Browse community
// ---------------------------------------------------------------------------

void LibrarySharing::browseCommunity(const QString& type,
                                       const QString& variable,
                                       const QString& action,
                                       const QString& search,
                                       const QString& sort,
                                       int page)
{
    if (m_browsing) {
        qWarning() << "LibrarySharing: Already browsing";
        return;
    }

    setBrowsing(true);
    setLastError(QString());

    QUrlQuery query;
    if (!type.isEmpty())     query.addQueryItem("type", type);
    if (!variable.isEmpty()) query.addQueryItem("variable", variable);
    if (!action.isEmpty())   query.addQueryItem("action", action);
    if (!search.isEmpty())   query.addQueryItem("search", search);
    if (!sort.isEmpty())     query.addQueryItem("sort", sort);
    query.addQueryItem("page", QString::number(page));

    QUrl url(QString("%1/entries").arg(API_BASE));
    url.setQuery(query);

    QNetworkRequest request{url};
    request.setRawHeader("User-Agent",
        QString("Decenza-DE1/%1").arg(VERSION_STRING).toUtf8());
    request.setRawHeader("X-Device-Id", m_settings->deviceId().toUtf8());

    qDebug() << "LibrarySharing: Browsing community -" << url.toString();

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onBrowseFinished);
}

void LibrarySharing::browseMyUploads(int page)
{
    if (m_browsing) {
        qWarning() << "LibrarySharing: Already browsing";
        return;
    }

    setBrowsing(true);
    setLastError(QString());

    QUrlQuery query;
    query.addQueryItem("device_id", "mine");
    query.addQueryItem("page", QString::number(page));

    QUrl url(QString("%1/entries").arg(API_BASE));
    url.setQuery(query);

    QNetworkRequest request{url};
    request.setRawHeader("User-Agent",
        QString("Decenza-DE1/%1").arg(VERSION_STRING).toUtf8());
    request.setRawHeader("X-Device-Id", m_settings->deviceId().toUtf8());

    qDebug() << "LibrarySharing: Browsing my uploads";

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onBrowseFinished);
}

void LibrarySharing::onBrowseFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    setBrowsing(false);

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qWarning() << "LibrarySharing: Browse failed -" << error;
        setLastError(error);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    QVariantList entries;
    QJsonArray arr = obj["entries"].toArray();
    for (const auto& val : arr) {
        entries.append(val.toObject().toVariantMap());
    }

    m_communityEntries = entries;
    emit communityEntriesChanged();

    int total = obj["total"].toInt(entries.size());
    setTotalCommunityResults(total);

    qDebug() << "LibrarySharing: Browse returned" << entries.size()
             << "entries (total:" << total << ")";
}

// ---------------------------------------------------------------------------
// Featured
// ---------------------------------------------------------------------------

void LibrarySharing::loadFeatured()
{
    setLastError(QString());

    QUrlQuery query;
    query.addQueryItem("sort", "popular");
    query.addQueryItem("page", "1");
    query.addQueryItem("per_page", "10");

    QUrl url(QString("%1/entries").arg(API_BASE));
    url.setQuery(query);

    QNetworkRequest request{url};
    request.setRawHeader("User-Agent",
        QString("Decenza-DE1/%1").arg(VERSION_STRING).toUtf8());
    request.setRawHeader("X-Device-Id", m_settings->deviceId().toUtf8());

    qDebug() << "LibrarySharing: Loading featured entries -" << url.toString();

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onFeaturedFinished);
}

void LibrarySharing::onFeaturedFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "LibrarySharing: Featured load failed -" << reply->errorString();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    QVariantList entries;
    QJsonArray arr = obj["entries"].toArray();
    for (const auto& val : arr) {
        entries.append(val.toObject().toVariantMap());
    }

    m_featuredEntries = entries;
    emit featuredEntriesChanged();

    qDebug() << "LibrarySharing: Loaded" << entries.size() << "featured entries";
}

// ---------------------------------------------------------------------------
// Download (two-step: metadata, then full data on confirm)
// ---------------------------------------------------------------------------

void LibrarySharing::downloadEntry(const QString& serverId)
{
    if (m_downloading) {
        qWarning() << "LibrarySharing: Already downloading";
        return;
    }

    setDownloading(true);
    setLastError(QString());
    m_pendingDownloadId = serverId;

    // Fetch full entry data
    QNetworkRequest request = buildRequest("/entries/" + serverId);

    qDebug() << "LibrarySharing: Downloading entry" << serverId;

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onDownloadDataFinished);
}

void LibrarySharing::onDownloadMetaFinished()
{
    // Reserved for future two-step preview-then-download flow
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) reply->deleteLater();
}

void LibrarySharing::onDownloadDataFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    setDownloading(false);

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qWarning() << "LibrarySharing: Download failed -" << error;
        setLastError(error);
        emit downloadFailed(error);
        return;
    }

    QByteArray data = reply->readAll();

    // Import into local library
    QString localId = m_library->importEntry(data);
    if (localId.isEmpty()) {
        QString error = "Failed to import downloaded entry";
        setLastError(error);
        emit downloadFailed(error);
        return;
    }

    qDebug() << "LibrarySharing: Downloaded and imported as" << localId;
    emit downloadComplete(localId);

    // Record download on server (fire-and-forget)
    QNetworkRequest req = buildRequest("/entries/" + m_pendingDownloadId + "/download");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* recordReply = m_networkManager.post(req, "{}");
    connect(recordReply, &QNetworkReply::finished, this, &LibrarySharing::onRecordDownloadFinished);
}

void LibrarySharing::onRecordDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "LibrarySharing: Failed to record download (non-critical) -"
                 << reply->errorString();
    }
}

// ---------------------------------------------------------------------------
// Delete from server
// ---------------------------------------------------------------------------

void LibrarySharing::deleteFromServer(const QString& serverId)
{
    setLastError(QString());

    QNetworkRequest request = buildRequest("/entries/" + serverId);

    qDebug() << "LibrarySharing: Deleting server entry" << serverId;

    QNetworkReply* reply = m_networkManager.deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onDeleteFinished);
}

void LibrarySharing::onDeleteFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qWarning() << "LibrarySharing: Delete failed -" << error;
        setLastError(error);
        return;
    }

    qDebug() << "LibrarySharing: Server entry deleted";
}

// ---------------------------------------------------------------------------
// Flag / report
// ---------------------------------------------------------------------------

void LibrarySharing::flagEntry(const QString& serverId, const QString& reason)
{
    setLastError(QString());

    QNetworkRequest request = buildRequest("/entries/" + serverId + "/flag");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["reason"] = reason;

    QNetworkReply* reply = m_networkManager.post(request,
        QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, &LibrarySharing::onFlagFinished);
}

void LibrarySharing::onFlagFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "LibrarySharing: Flag failed -" << reply->errorString();
    } else {
        qDebug() << "LibrarySharing: Entry flagged";
    }
}

// ---------------------------------------------------------------------------
// Property setters
// ---------------------------------------------------------------------------

void LibrarySharing::setUploading(bool v)
{
    if (m_uploading != v) {
        m_uploading = v;
        emit uploadingChanged();
    }
}

void LibrarySharing::setBrowsing(bool v)
{
    if (m_browsing != v) {
        m_browsing = v;
        emit browsingChanged();
    }
}

void LibrarySharing::setDownloading(bool v)
{
    if (m_downloading != v) {
        m_downloading = v;
        emit downloadingChanged();
    }
}

void LibrarySharing::setLastError(const QString& error)
{
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

void LibrarySharing::setTotalCommunityResults(int count)
{
    if (m_totalCommunityResults != count) {
        m_totalCommunityResults = count;
        emit totalCommunityResultsChanged();
    }
}
