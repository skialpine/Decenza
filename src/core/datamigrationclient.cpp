#include "datamigrationclient.h"
#include "settings.h"
#include "settingsserializer.h"
#include "profilestorage.h"
#include "../history/shothistorystorage.h"
#include "../screensaver/screensavervideomanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>

DataMigrationClient::DataMigrationClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

DataMigrationClient::~DataMigrationClient()
{
    cancel();
    delete m_tempDir;
}

void DataMigrationClient::connectToServer(const QString& serverUrl)
{
    if (m_connecting || m_importing) {
        return;
    }

    // Normalize URL
    m_serverUrl = serverUrl;
    if (!m_serverUrl.startsWith("http://") && !m_serverUrl.startsWith("https://")) {
        m_serverUrl = "http://" + m_serverUrl;
    }
    if (m_serverUrl.endsWith("/")) {
        m_serverUrl.chop(1);
    }

    m_connecting = true;
    m_errorMessage.clear();
    emit isConnectingChanged();
    emit serverUrlChanged();
    emit errorMessageChanged();

    setCurrentOperation(tr("Connecting..."));

    // Fetch manifest
    QUrl url(m_serverUrl + "/api/backup/manifest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Decenza-Migration/1.0");

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onManifestReply);
}

void DataMigrationClient::disconnect()
{
    cancel();
    m_serverUrl.clear();
    m_manifest.clear();
    emit serverUrlChanged();
    emit manifestChanged();
}

void DataMigrationClient::onManifestReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    m_connecting = false;
    emit isConnectingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        setError(tr("Connection failed: %1").arg(reply->errorString()));
        emit connectionFailed(m_errorMessage);
        reply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    m_currentReply = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        setError(tr("Invalid response from server"));
        emit connectionFailed(m_errorMessage);
        return;
    }

    m_manifest = doc.object().toVariantMap();
    emit manifestChanged();

    setCurrentOperation(tr("Connected"));
    emit connected();

    qDebug() << "DataMigrationClient: Connected to" << m_serverUrl
             << "- Device:" << m_manifest["deviceName"].toString()
             << "- Profiles:" << m_manifest["profileCount"].toInt()
             << "- Shots:" << m_manifest["shotCount"].toInt()
             << "- Media:" << m_manifest["mediaCount"].toInt();
}

void DataMigrationClient::importAll()
{
    if (m_importing || m_serverUrl.isEmpty()) {
        return;
    }

    m_importing = true;
    m_cancelled = false;
    m_settingsImported = 0;
    m_profilesImported = 0;
    m_shotsImported = 0;
    m_mediaImported = 0;
    m_progress = 0.0;
    m_errorMessage.clear();

    emit isImportingChanged();
    emit progressChanged();
    emit errorMessageChanged();

    // Calculate total bytes for progress
    m_totalBytes = m_manifest["settingsSize"].toLongLong()
                 + m_manifest["profilesSize"].toLongLong()
                 + m_manifest["shotsSize"].toLongLong()
                 + m_manifest["mediaSize"].toLongLong();
    m_receivedBytes = 0;

    // Queue imports
    m_importQueue.clear();
    if (m_manifest["hasSettings"].toBool()) {
        m_importQueue << "settings";
    }
    if (m_manifest["profileCount"].toInt() > 0) {
        m_importQueue << "profiles";
    }
    if (m_manifest["shotCount"].toInt() > 0) {
        m_importQueue << "shots";
    }
    if (m_manifest["mediaCount"].toInt() > 0) {
        m_importQueue << "media";
    }

    startNextImport();
}

void DataMigrationClient::startNextImport()
{
    if (m_cancelled) {
        m_importing = false;
        emit isImportingChanged();
        return;
    }

    if (m_importQueue.isEmpty()) {
        // All done
        m_importing = false;
        setProgress(1.0);
        setCurrentOperation(tr("Import complete"));
        emit isImportingChanged();
        emit importComplete(m_settingsImported, m_profilesImported, m_shotsImported, m_mediaImported);
        return;
    }

    QString next = m_importQueue.takeFirst();

    if (next == "settings") {
        importSettings();
    } else if (next == "profiles") {
        importProfiles();
    } else if (next == "shots") {
        importShots();
    } else if (next == "media") {
        importMedia();
    }
}

void DataMigrationClient::importSettings()
{
    setCurrentOperation(tr("Importing settings..."));

    QUrl url(m_serverUrl + "/api/backup/settings");
    QNetworkRequest request(url);

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onSettingsReply);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &DataMigrationClient::onDownloadProgress);
}

void DataMigrationClient::onSettingsReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "DataMigrationClient: Failed to import settings:" << reply->errorString();
        // Continue with next import
    } else {
        QByteArray data = reply->readAll();
        m_receivedBytes += data.size();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject() && m_settings) {
            SettingsSerializer::importFromJson(m_settings, doc.object());
            m_settingsImported = 1;
            qDebug() << "DataMigrationClient: Settings imported successfully";
        }
    }

    reply->deleteLater();
    m_currentReply = nullptr;
    startNextImport();
}

void DataMigrationClient::importProfiles()
{
    setCurrentOperation(tr("Fetching profile list..."));

    // First fetch the list of profiles
    QUrl url(m_serverUrl + "/api/backup/profiles");
    QNetworkRequest request(url);

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onProfileListReply);
}

void DataMigrationClient::onProfileListReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "DataMigrationClient: Failed to fetch profile list:" << reply->errorString();
        reply->deleteLater();
        m_currentReply = nullptr;
        startNextImport();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    m_currentReply = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "DataMigrationClient: Invalid profile list response";
        startNextImport();
        return;
    }

    // Build list of profiles to download
    m_pendingProfiles.clear();
    QJsonArray profiles = doc.array();
    for (const QJsonValue& val : profiles) {
        QJsonObject obj = val.toObject();
        ProfileDownload pd;
        pd.category = obj["category"].toString();
        pd.filename = obj["filename"].toString();
        pd.size = obj["size"].toInt();
        m_pendingProfiles.append(pd);
    }

    qDebug() << "DataMigrationClient: Found" << m_pendingProfiles.size() << "profiles to download";
    downloadNextProfile();
}

void DataMigrationClient::downloadNextProfile()
{
    if (m_cancelled) {
        m_pendingProfiles.clear();
        startNextImport();
        return;
    }

    if (m_pendingProfiles.isEmpty()) {
        qDebug() << "DataMigrationClient: Imported" << m_profilesImported << "profiles";
        startNextImport();
        return;
    }

    ProfileDownload pd = m_pendingProfiles.takeFirst();
    setCurrentOperation(tr("Importing profile: %1").arg(pd.filename));

    // URL encode the filename
    QString encodedFilename = QUrl::toPercentEncoding(pd.filename);
    QUrl url(m_serverUrl + "/api/backup/profile/" + pd.category + "/" + encodedFilename);
    QNetworkRequest request(url);

    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("category", pd.category);
    m_currentReply->setProperty("filename", pd.filename);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onProfileFileReply);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &DataMigrationClient::onDownloadProgress);
}

void DataMigrationClient::onProfileFileReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QString category = reply->property("category").toString();
    QString filename = reply->property("filename").toString();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "DataMigrationClient: Failed to download profile" << filename << ":" << reply->errorString();
    } else if (m_profileStorage) {
        QByteArray content = reply->readAll();
        m_receivedBytes += content.size();

        QString basePath;
        if (category == "user") {
            basePath = m_profileStorage->userProfilesPath();
        } else {
            basePath = m_profileStorage->downloadedProfilesPath();
        }

        QDir().mkpath(basePath);
        QString targetPath = basePath + "/" + filename;

        // Handle duplicate filenames
        if (QFile::exists(targetPath)) {
            QString baseName = QFileInfo(targetPath).completeBaseName();
            QString suffix = QFileInfo(targetPath).suffix();
            int counter = 1;
            do {
                targetPath = QString("%1/%2_imported%3.%4")
                    .arg(basePath, baseName)
                    .arg(counter > 1 ? QString::number(counter) : "")
                    .arg(suffix);
                counter++;
            } while (QFile::exists(targetPath));
        }

        QFile outFile(targetPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(content);
            outFile.close();
            m_profilesImported++;
        }
    }

    reply->deleteLater();
    m_currentReply = nullptr;
    downloadNextProfile();
}

void DataMigrationClient::importShots()
{
    setCurrentOperation(tr("Importing shot history..."));

    QUrl url(m_serverUrl + "/api/backup/shots");
    QNetworkRequest request(url);

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onShotsReply);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &DataMigrationClient::onDownloadProgress);
}

void DataMigrationClient::onShotsReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "DataMigrationClient: Failed to import shots:" << reply->errorString();
    } else if (m_shotHistory) {
        QByteArray dbData = reply->readAll();
        m_receivedBytes += dbData.size();

        // Save to temp file
        delete m_tempDir;
        m_tempDir = new QTemporaryDir();

        QString tempDbPath = m_tempDir->path() + "/shots.db";
        QFile tempFile(tempDbPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(dbData);
            tempFile.close();

            // Import using existing merge logic
            int beforeCount = m_shotHistory->totalShots();
            bool success = m_shotHistory->importDatabase(tempDbPath, true);  // merge=true
            if (success) {
                m_shotHistory->refreshTotalShots();
                m_shotsImported = m_shotHistory->totalShots() - beforeCount;
                qDebug() << "DataMigrationClient: Imported" << m_shotsImported << "new shots";
            }
        }
    }

    reply->deleteLater();
    m_currentReply = nullptr;
    startNextImport();
}

void DataMigrationClient::importMedia()
{
    setCurrentOperation(tr("Fetching media list..."));

    // First fetch the list of media files
    QUrl url(m_serverUrl + "/api/backup/media");
    QNetworkRequest request(url);

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onMediaListReply);
}

void DataMigrationClient::onMediaListReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "DataMigrationClient: Failed to fetch media list:" << reply->errorString();
        reply->deleteLater();
        m_currentReply = nullptr;
        startNextImport();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    m_currentReply = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "DataMigrationClient: Invalid media list response";
        startNextImport();
        return;
    }

    // Build list of media files to download
    m_pendingMedia.clear();
    QJsonArray mediaFiles = doc.array();
    for (const QJsonValue& val : mediaFiles) {
        QJsonObject obj = val.toObject();
        MediaDownload md;
        md.filename = obj["filename"].toString();
        md.size = obj["size"].toInt();
        m_pendingMedia.append(md);
    }

    qDebug() << "DataMigrationClient: Found" << m_pendingMedia.size() << "media files to download";
    downloadNextMedia();
}

void DataMigrationClient::downloadNextMedia()
{
    if (m_cancelled) {
        m_pendingMedia.clear();
        startNextImport();
        return;
    }

    if (m_pendingMedia.isEmpty()) {
        qDebug() << "DataMigrationClient: Imported" << m_mediaImported << "media files";
        startNextImport();
        return;
    }

    MediaDownload md = m_pendingMedia.takeFirst();
    setCurrentOperation(tr("Importing media: %1").arg(md.filename));

    // URL encode the filename
    QString encodedFilename = QUrl::toPercentEncoding(md.filename);
    QUrl url(m_serverUrl + "/api/backup/media/" + encodedFilename);
    QNetworkRequest request(url);

    m_currentReply = m_networkManager->get(request);
    m_currentReply->setProperty("filename", md.filename);
    connect(m_currentReply, &QNetworkReply::finished, this, &DataMigrationClient::onMediaFileReply);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &DataMigrationClient::onDownloadProgress);
}

void DataMigrationClient::onMediaFileReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QString filename = reply->property("filename").toString();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "DataMigrationClient: Failed to download media" << filename << ":" << reply->errorString();
    } else if (m_screensaver) {
        QByteArray content = reply->readAll();
        m_receivedBytes += content.size();

        // Save to temp file first, then add via manager
        delete m_tempDir;
        m_tempDir = new QTemporaryDir();
        QString tempPath = m_tempDir->path() + "/" + filename;

        QFile outFile(tempPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(content);
            outFile.close();

            // Add to personal media (handles duplicates internally)
            if (m_screensaver->addPersonalMedia(tempPath, filename)) {
                m_mediaImported++;
            }
        }
    }

    reply->deleteLater();
    m_currentReply = nullptr;
    downloadNextMedia();
}

void DataMigrationClient::cancel()
{
    m_cancelled = true;

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_connecting) {
        m_connecting = false;
        emit isConnectingChanged();
    }

    if (m_importing) {
        m_importing = false;
        emit isImportingChanged();
    }

    m_importQueue.clear();
    m_pendingProfiles.clear();
    m_pendingMedia.clear();
    setCurrentOperation(tr("Cancelled"));
}

void DataMigrationClient::onDownloadProgress(qint64 received, qint64 total)
{
    Q_UNUSED(total)

    if (m_totalBytes > 0) {
        double progress = static_cast<double>(m_receivedBytes + received) / m_totalBytes;
        setProgress(qMin(progress, 0.99));  // Cap at 99% until complete
    }
}

void DataMigrationClient::setProgress(double progress)
{
    if (qAbs(m_progress - progress) > 0.001) {
        m_progress = progress;
        emit progressChanged();
    }
}

void DataMigrationClient::setCurrentOperation(const QString& operation)
{
    if (m_currentOperation != operation) {
        m_currentOperation = operation;
        emit currentOperationChanged();
    }
}

void DataMigrationClient::setError(const QString& error)
{
    m_errorMessage = error;
    emit errorMessageChanged();
    qWarning() << "DataMigrationClient:" << error;
}
