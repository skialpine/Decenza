#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QVariantMap>
#include <QStringList>

class Settings;

class TranslationManager : public QObject {
    Q_OBJECT

    // Current language settings
    Q_PROPERTY(QString currentLanguage READ currentLanguage WRITE setCurrentLanguage NOTIFY currentLanguageChanged)
    Q_PROPERTY(bool editModeEnabled READ editModeEnabled WRITE setEditModeEnabled NOTIFY editModeEnabledChanged)

    // Translation status
    Q_PROPERTY(int untranslatedCount READ untranslatedCount NOTIFY untranslatedCountChanged)
    Q_PROPERTY(int totalStringCount READ totalStringCount NOTIFY totalStringCountChanged)
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages NOTIFY availableLanguagesChanged)

    // Network status
    Q_PROPERTY(bool downloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

    // Version counter - increments when translations change, used for QML reactivity
    Q_PROPERTY(int translationVersion READ translationVersion NOTIFY translationsChanged)

public:
    explicit TranslationManager(Settings* settings, QObject* parent = nullptr);

    // Properties
    QString currentLanguage() const;
    void setCurrentLanguage(const QString& lang);
    bool editModeEnabled() const;
    void setEditModeEnabled(bool enabled);
    int untranslatedCount() const;
    int totalStringCount() const;
    QStringList availableLanguages() const;
    bool isDownloading() const;
    QString lastError() const;
    int translationVersion() const { return m_translationVersion; }

    // Translation lookup (auto-registers strings)
    Q_INVOKABLE QString translate(const QString& key, const QString& fallback);
    Q_INVOKABLE bool hasTranslation(const QString& key) const;

    // Translation editing
    Q_INVOKABLE void setTranslation(const QString& key, const QString& translation);
    Q_INVOKABLE void deleteTranslation(const QString& key);

    // Language management
    Q_INVOKABLE void addLanguage(const QString& langCode, const QString& displayName, const QString& nativeName = QString());
    Q_INVOKABLE void deleteLanguage(const QString& langCode);
    Q_INVOKABLE QString getLanguageDisplayName(const QString& langCode) const;
    Q_INVOKABLE QString getLanguageNativeName(const QString& langCode) const;

    // String registry - tracks all known keys in the app
    Q_INVOKABLE void registerString(const QString& key, const QString& fallback);

    // Community translations
    Q_INVOKABLE void downloadLanguageList();
    Q_INVOKABLE void downloadLanguage(const QString& langCode);
    Q_INVOKABLE void exportTranslation(const QString& filePath);
    Q_INVOKABLE void importTranslation(const QString& filePath);
    Q_INVOKABLE void openGitHubSubmission();

    // Utility
    Q_INVOKABLE QVariantList getUntranslatedStrings() const;
    Q_INVOKABLE bool isRtlLanguage(const QString& langCode) const;

signals:
    void currentLanguageChanged();
    void editModeEnabledChanged();
    void untranslatedCountChanged();
    void totalStringCountChanged();
    void availableLanguagesChanged();
    void downloadingChanged();
    void lastErrorChanged();

    void translationsChanged();
    void translationChanged(const QString& key);
    void languageDownloaded(const QString& langCode, bool success, const QString& error);
    void languageListDownloaded(bool success);

private slots:
    void onLanguageListFetched(QNetworkReply* reply);
    void onLanguageFileFetched(QNetworkReply* reply);

private:
    void loadTranslations();
    void saveTranslations();
    void loadLanguageMetadata();
    void saveLanguageMetadata();
    void loadStringRegistry();
    void saveStringRegistry();
    void recalculateUntranslatedCount();
    QString translationsDir() const;
    QString languageFilePath(const QString& langCode) const;

    Settings* m_settings;
    QNetworkAccessManager* m_networkManager;

    QString m_currentLanguage;
    bool m_editModeEnabled = false;
    bool m_downloading = false;
    QString m_lastError;

    // translations[key] = translated_text
    QMap<QString, QString> m_translations;

    // Registry of all known string keys and their English fallbacks
    // registry[key] = english_fallback
    QMap<QString, QString> m_stringRegistry;

    // Language metadata: {langCode: {displayName, nativeName, isRtl}}
    QMap<QString, QVariantMap> m_languageMetadata;

    // List of available language codes (local + community)
    QStringList m_availableLanguages;

    int m_untranslatedCount = 0;
    int m_translationVersion = 0;

    // Track which language is being downloaded
    QString m_downloadingLangCode;

    // Dirty flag for batch saving string registry
    bool m_registryDirty = false;

    static constexpr const char* GITHUB_RAW_BASE = "https://raw.githubusercontent.com/Kulitorum/de1-qt-translations/main";
    static constexpr const char* GITHUB_ISSUES_URL = "https://github.com/Kulitorum/de1-qt-translations/issues/new";
};
