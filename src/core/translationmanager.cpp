#include "translationmanager.h"
#include "settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>

TranslationManager::TranslationManager(Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Ensure translations directory exists
    QDir dir(translationsDir());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Load saved language from settings
    m_currentLanguage = m_settings->value("localization/language", "en").toString();

    // Load language metadata (list of available languages)
    loadLanguageMetadata();

    // Ensure English is always available
    if (!m_languageMetadata.contains("en")) {
        m_languageMetadata["en"] = QVariantMap{
            {"displayName", "English"},
            {"nativeName", "English"},
            {"isRtl", false}
        };
        saveLanguageMetadata();
    }

    // Update available languages list
    m_availableLanguages = m_languageMetadata.keys();
    if (!m_availableLanguages.contains("en")) {
        m_availableLanguages.prepend("en");
    }

    // Load string registry
    loadStringRegistry();

    // Load translations for current language
    loadTranslations();

    // Calculate initial untranslated count
    recalculateUntranslatedCount();

    // Timer to batch-save string registry
    QTimer* registrySaveTimer = new QTimer(this);
    registrySaveTimer->setInterval(5000);  // Save every 5 seconds if dirty
    connect(registrySaveTimer, &QTimer::timeout, this, [this]() {
        if (m_registryDirty) {
            saveStringRegistry();
            recalculateUntranslatedCount();
            m_registryDirty = false;
            emit totalStringCountChanged();
        }
    });
    registrySaveTimer->start();

    qDebug() << "TranslationManager initialized. Language:" << m_currentLanguage
             << "Strings:" << m_stringRegistry.size()
             << "Translations:" << m_translations.size();
}

QString TranslationManager::translationsDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/translations";
}

QString TranslationManager::languageFilePath(const QString& langCode) const
{
    return translationsDir() + "/" + langCode + ".json";
}

// --- Properties ---

QString TranslationManager::currentLanguage() const
{
    return m_currentLanguage;
}

void TranslationManager::setCurrentLanguage(const QString& lang)
{
    if (m_currentLanguage != lang) {
        m_currentLanguage = lang;
        m_settings->setValue("localization/language", lang);
        loadTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
        emit currentLanguageChanged();
    }
}

bool TranslationManager::editModeEnabled() const
{
    return m_editModeEnabled;
}

void TranslationManager::setEditModeEnabled(bool enabled)
{
    if (m_editModeEnabled != enabled) {
        m_editModeEnabled = enabled;
        emit editModeEnabledChanged();
    }
}

int TranslationManager::untranslatedCount() const
{
    return m_untranslatedCount;
}

int TranslationManager::totalStringCount() const
{
    return m_stringRegistry.size();
}

QStringList TranslationManager::availableLanguages() const
{
    return m_availableLanguages;
}

bool TranslationManager::isDownloading() const
{
    return m_downloading;
}

QString TranslationManager::lastError() const
{
    return m_lastError;
}

// --- Translation lookup ---

QString TranslationManager::translate(const QString& key, const QString& fallback)
{
    // Auto-register the string if not already registered
    if (!m_stringRegistry.contains(key)) {
        m_stringRegistry[key] = fallback;
        // Don't save on every call - batch save periodically
        m_registryDirty = true;
    }

    // English always returns fallback
    if (m_currentLanguage == "en") {
        return fallback;
    }

    // Return translation if exists, otherwise fallback
    return m_translations.value(key, fallback);
}

bool TranslationManager::hasTranslation(const QString& key) const
{
    // English is always "translated"
    if (m_currentLanguage == "en") {
        return true;
    }
    return m_translations.contains(key) && !m_translations.value(key).isEmpty();
}

// --- Translation editing ---

void TranslationManager::setTranslation(const QString& key, const QString& translation)
{
    if (m_currentLanguage == "en") {
        return;  // Can't edit English (it's the source)
    }

    m_translations[key] = translation;
    saveTranslations();
    recalculateUntranslatedCount();
    m_translationVersion++;
    emit translationsChanged();
    emit translationChanged(key);
}

void TranslationManager::deleteTranslation(const QString& key)
{
    if (m_translations.contains(key)) {
        m_translations.remove(key);
        saveTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
        emit translationChanged(key);
    }
}

// --- Language management ---

void TranslationManager::addLanguage(const QString& langCode, const QString& displayName, const QString& nativeName)
{
    if (langCode.isEmpty() || m_languageMetadata.contains(langCode)) {
        return;
    }

    m_languageMetadata[langCode] = QVariantMap{
        {"displayName", displayName},
        {"nativeName", nativeName.isEmpty() ? displayName : nativeName},
        {"isRtl", false}
    };

    saveLanguageMetadata();

    // Create empty translation file
    QFile file(languageFilePath(langCode));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        root["language"] = langCode;
        root["displayName"] = displayName;
        root["nativeName"] = nativeName.isEmpty() ? displayName : nativeName;
        root["translations"] = QJsonObject();
        file.write(QJsonDocument(root).toJson());
        file.close();
    }

    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();

    qDebug() << "Added language:" << langCode << displayName;
}

void TranslationManager::deleteLanguage(const QString& langCode)
{
    if (langCode == "en" || !m_languageMetadata.contains(langCode)) {
        return;  // Can't delete English
    }

    m_languageMetadata.remove(langCode);
    saveLanguageMetadata();

    // Delete translation file
    QFile::remove(languageFilePath(langCode));

    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();

    // Switch to English if current language was deleted
    if (m_currentLanguage == langCode) {
        setCurrentLanguage("en");
    }

    qDebug() << "Deleted language:" << langCode;
}

QString TranslationManager::getLanguageDisplayName(const QString& langCode) const
{
    if (m_languageMetadata.contains(langCode)) {
        return m_languageMetadata[langCode]["displayName"].toString();
    }
    return langCode;
}

QString TranslationManager::getLanguageNativeName(const QString& langCode) const
{
    if (m_languageMetadata.contains(langCode)) {
        return m_languageMetadata[langCode]["nativeName"].toString();
    }
    return langCode;
}

// --- String registry ---

void TranslationManager::registerString(const QString& key, const QString& fallback)
{
    if (!m_stringRegistry.contains(key)) {
        m_stringRegistry[key] = fallback;
        saveStringRegistry();
        recalculateUntranslatedCount();
        emit totalStringCountChanged();
    }
}

// --- Community translations ---

void TranslationManager::downloadLanguageList()
{
    if (m_downloading) {
        return;
    }

    m_downloading = true;
    emit downloadingChanged();

    QString url = QString("%1/languages.json").arg(GITHUB_RAW_BASE);
    qDebug() << "Fetching language list from:" << url;

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLanguageListFetched(reply);
    });
}

void TranslationManager::downloadLanguage(const QString& langCode)
{
    if (m_downloading || langCode == "en") {
        return;
    }

    m_downloading = true;
    m_downloadingLangCode = langCode;
    emit downloadingChanged();

    QString url = QString("%1/languages/%2.json").arg(GITHUB_RAW_BASE, langCode);
    qDebug() << "Fetching language file from:" << url;

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLanguageFileFetched(reply);
    });
}

void TranslationManager::onLanguageListFetched(QNetworkReply* reply)
{
    reply->deleteLater();
    m_downloading = false;
    emit downloadingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = QString("Failed to fetch language list: %1").arg(reply->errorString());
        emit lastErrorChanged();
        emit languageListDownloaded(false);
        qWarning() << m_lastError;
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        m_lastError = "Invalid language list format";
        emit lastErrorChanged();
        emit languageListDownloaded(false);
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray languages = root["languages"].toArray();

    for (const QJsonValue& val : languages) {
        QJsonObject lang = val.toObject();
        QString code = lang["code"].toString();
        QString displayName = lang["name"].toString();
        QString nativeName = lang["nativeName"].toString();
        bool isRtl = lang["isRtl"].toBool(false);

        if (!code.isEmpty() && !m_languageMetadata.contains(code)) {
            m_languageMetadata[code] = QVariantMap{
                {"displayName", displayName},
                {"nativeName", nativeName.isEmpty() ? displayName : nativeName},
                {"isRtl", isRtl},
                {"isRemote", true}  // Mark as available for download
            };
        }
    }

    saveLanguageMetadata();
    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();
    emit languageListDownloaded(true);

    qDebug() << "Language list updated. Available:" << m_availableLanguages;
}

void TranslationManager::onLanguageFileFetched(QNetworkReply* reply)
{
    reply->deleteLater();
    m_downloading = false;
    QString langCode = m_downloadingLangCode;
    m_downloadingLangCode.clear();
    emit downloadingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = QString("Failed to download %1: %2").arg(langCode, reply->errorString());
        emit lastErrorChanged();
        emit languageDownloaded(langCode, false, m_lastError);
        qWarning() << m_lastError;
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        m_lastError = "Invalid translation file format";
        emit lastErrorChanged();
        emit languageDownloaded(langCode, false, m_lastError);
        return;
    }

    // Save the downloaded file
    QFile file(languageFilePath(langCode));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }

    // Update metadata if provided
    QJsonObject root = doc.object();
    if (root.contains("displayName") || root.contains("nativeName")) {
        m_languageMetadata[langCode] = QVariantMap{
            {"displayName", root["displayName"].toString(langCode)},
            {"nativeName", root["nativeName"].toString(langCode)},
            {"isRtl", root["isRtl"].toBool(false)},
            {"isRemote", false}  // Now downloaded locally
        };
        saveLanguageMetadata();
    }

    // Reload if this is the current language
    if (langCode == m_currentLanguage) {
        loadTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
    }

    emit languageDownloaded(langCode, true, QString());
    qDebug() << "Downloaded language:" << langCode;
}

void TranslationManager::exportTranslation(const QString& filePath)
{
    if (m_currentLanguage == "en") {
        m_lastError = "Cannot export English (source language)";
        emit lastErrorChanged();
        return;
    }

    QJsonObject root;
    root["language"] = m_currentLanguage;
    root["displayName"] = getLanguageDisplayName(m_currentLanguage);
    root["nativeName"] = getLanguageNativeName(m_currentLanguage);
    root["isRtl"] = isRtlLanguage(m_currentLanguage);

    QJsonObject translations;
    for (auto it = m_translations.constBegin(); it != m_translations.constEnd(); ++it) {
        translations[it.key()] = it.value();
    }
    root["translations"] = translations;

    // Also include untranslated strings with empty values for reference
    QJsonObject untranslated;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        if (!m_translations.contains(it.key())) {
            untranslated[it.key()] = it.value();  // English fallback as hint
        }
    }
    root["untranslated"] = untranslated;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "Exported translation to:" << filePath;
    } else {
        m_lastError = QString("Failed to write file: %1").arg(filePath);
        emit lastErrorChanged();
    }
}

void TranslationManager::importTranslation(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Failed to open file: %1").arg(filePath);
        emit lastErrorChanged();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_lastError = "Invalid translation file format";
        emit lastErrorChanged();
        return;
    }

    QJsonObject root = doc.object();
    QString langCode = root["language"].toString();

    if (langCode.isEmpty()) {
        m_lastError = "Translation file missing language code";
        emit lastErrorChanged();
        return;
    }

    // Save the imported file
    QFile destFile(languageFilePath(langCode));
    if (destFile.open(QIODevice::WriteOnly)) {
        destFile.write(data);
        destFile.close();
    }

    // Update metadata
    m_languageMetadata[langCode] = QVariantMap{
        {"displayName", root["displayName"].toString(langCode)},
        {"nativeName", root["nativeName"].toString(langCode)},
        {"isRtl", root["isRtl"].toBool(false)}
    };
    saveLanguageMetadata();

    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();

    // If importing for current language, reload
    if (langCode == m_currentLanguage) {
        loadTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
    }

    qDebug() << "Imported translation for:" << langCode;
}

void TranslationManager::openGitHubSubmission()
{
    // Create a GitHub issue with the translation data pre-filled
    QString title = QString("Translation submission: %1").arg(getLanguageDisplayName(m_currentLanguage));

    QString body = QString("## Language: %1 (%2)\n\n"
                          "Translated %3 of %4 strings (%5%)\n\n"
                          "**Please attach your exported translation JSON file to this issue.**\n\n"
                          "You can export your translation from:\n"
                          "Settings -> Language -> Export Translation\n")
                       .arg(getLanguageDisplayName(m_currentLanguage))
                       .arg(m_currentLanguage)
                       .arg(m_translations.size())
                       .arg(m_stringRegistry.size())
                       .arg(m_stringRegistry.isEmpty() ? 0 : (m_translations.size() * 100 / m_stringRegistry.size()));

    QUrl url(GITHUB_ISSUES_URL);
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("body", body);
    query.addQueryItem("labels", "translation");
    url.setQuery(query);

    QDesktopServices::openUrl(url);
}

// --- Utility ---

QVariantList TranslationManager::getUntranslatedStrings() const
{
    QVariantList result;

    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        if (!m_translations.contains(it.key()) || m_translations.value(it.key()).isEmpty()) {
            result.append(QVariantMap{
                {"key", it.key()},
                {"fallback", it.value()}
            });
        }
    }

    return result;
}

bool TranslationManager::isRtlLanguage(const QString& langCode) const
{
    if (m_languageMetadata.contains(langCode)) {
        return m_languageMetadata[langCode]["isRtl"].toBool();
    }

    // Common RTL languages
    static const QStringList rtlLanguages = {"ar", "he", "fa", "ur"};
    return rtlLanguages.contains(langCode);
}

// --- Private helpers ---

void TranslationManager::loadTranslations()
{
    m_translations.clear();

    if (m_currentLanguage == "en") {
        return;  // English uses fallbacks
    }

    QFile file(languageFilePath(m_currentLanguage));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No translation file for:" << m_currentLanguage;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid translation file for:" << m_currentLanguage;
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject translations = root["translations"].toObject();

    for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
        m_translations[it.key()] = it.value().toString();
    }

    qDebug() << "Loaded" << m_translations.size() << "translations for:" << m_currentLanguage;
}

void TranslationManager::saveTranslations()
{
    if (m_currentLanguage == "en") {
        return;
    }

    QJsonObject root;
    root["language"] = m_currentLanguage;
    root["displayName"] = getLanguageDisplayName(m_currentLanguage);
    root["nativeName"] = getLanguageNativeName(m_currentLanguage);
    root["isRtl"] = isRtlLanguage(m_currentLanguage);

    QJsonObject translations;
    for (auto it = m_translations.constBegin(); it != m_translations.constEnd(); ++it) {
        translations[it.key()] = it.value();
    }
    root["translations"] = translations;

    QFile file(languageFilePath(m_currentLanguage));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void TranslationManager::loadLanguageMetadata()
{
    QString metaPath = translationsDir() + "/languages_meta.json";
    QFile file(metaPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        m_languageMetadata[it.key()] = it.value().toObject().toVariantMap();
    }
}

void TranslationManager::saveLanguageMetadata()
{
    QJsonObject root;
    for (auto it = m_languageMetadata.constBegin(); it != m_languageMetadata.constEnd(); ++it) {
        root[it.key()] = QJsonObject::fromVariantMap(it.value());
    }

    QString metaPath = translationsDir() + "/languages_meta.json";
    QFile file(metaPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void TranslationManager::loadStringRegistry()
{
    QString regPath = translationsDir() + "/string_registry.json";
    QFile file(regPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject strings = root["strings"].toObject();

    for (auto it = strings.constBegin(); it != strings.constEnd(); ++it) {
        m_stringRegistry[it.key()] = it.value().toString();
    }
}

void TranslationManager::saveStringRegistry()
{
    QJsonObject root;
    root["version"] = "1.0";

    QJsonObject strings;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        strings[it.key()] = it.value();
    }
    root["strings"] = strings;

    QString regPath = translationsDir() + "/string_registry.json";
    QFile file(regPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void TranslationManager::recalculateUntranslatedCount()
{
    if (m_currentLanguage == "en") {
        m_untranslatedCount = 0;
    } else {
        int count = 0;
        for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
            if (!m_translations.contains(it.key()) || m_translations.value(it.key()).isEmpty()) {
                count++;
            }
        }
        m_untranslatedCount = count;
    }
    emit untranslatedCountChanged();
}
