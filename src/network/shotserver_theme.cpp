#include "shotserver.h"
#include "../core/settings.h"
#include "webtemplates/theme_page.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

QJsonObject ShotServer::buildThemeJson() const
{
    QJsonObject result;

    if (!m_settings) {
        return result;
    }

    // Active theme name
    result["activeThemeName"] = m_settings->activeThemeName();

    // All colors
    QJsonObject colors;
    QVariantMap themeColors = m_settings->customThemeColors();

    // Color definitions with defaults (matching Theme.qml)
    static const QMap<QString, QString> colorDefaults = {
        {"backgroundColor", "#1a1a2e"},
        {"surfaceColor", "#303048"},
        {"primaryColor", "#4e85f4"},
        {"secondaryColor", "#c0c5e3"},
        {"textColor", "#ffffff"},
        {"textSecondaryColor", "#a0a8b8"},
        {"accentColor", "#e94560"},
        {"successColor", "#00cc6d"},
        {"warningColor", "#ffaa00"},
        {"highlightColor", "#ffaa00"},
        {"errorColor", "#ff4444"},
        {"borderColor", "#3a3a4e"},
        {"pressureColor", "#18c37e"},
        {"pressureGoalColor", "#69fdb3"},
        {"flowColor", "#4e85f4"},
        {"flowGoalColor", "#7aaaff"},
        {"temperatureColor", "#e73249"},
        {"temperatureGoalColor", "#ffa5a6"},
        {"weightColor", "#a2693d"},
        {"weightFlowColor", "#d4a574"},
        {"dyeDoseColor", "#6F4E37"},
        {"dyeOutputColor", "#9C27B0"},
        {"dyeTdsColor", "#FF9800"},
        {"dyeEyColor", "#a2693d"},
        {"buttonDisabled", "#555555"},
        {"stopMarkerColor", "#FF6B6B"},
        {"frameMarkerColor", "#66ffffff"},
        {"modifiedIndicatorColor", "#FFCC00"},
        {"simulationIndicatorColor", "#E65100"},
        {"warningButtonColor", "#FFA500"},
        {"successButtonColor", "#2E7D32"},
        {"rowAlternateColor", "#1a1a1a"},
        {"rowAlternateLightColor", "#222222"},
        {"sourceBadgeBlueColor", "#4a90d9"},
        {"sourceBadgeGreenColor", "#4ad94a"},
        {"sourceBadgeOrangeColor", "#d9a04a"}
    };

    for (auto it = colorDefaults.constBegin(); it != colorDefaults.constEnd(); ++it) {
        QString val = themeColors.value(it.key()).toString();
        colors[it.key()] = val.isEmpty() ? it.value() : val;
    }
    result["colors"] = colors;

    // Font sizes
    QJsonObject fonts;
    QVariantMap fontSizes = m_settings->customFontSizes();
    static const QMap<QString, int> fontDefaults = {
        {"headingSize", 32},
        {"titleSize", 24},
        {"subtitleSize", 18},
        {"bodySize", 18},
        {"labelSize", 14},
        {"captionSize", 12},
        {"valueSize", 48},
        {"timerSize", 72}
    };

    for (auto it = fontDefaults.constBegin(); it != fontDefaults.constEnd(); ++it) {
        int val = fontSizes.value(it.key()).toInt();
        fonts[it.key()] = val > 0 ? val : it.value();
    }
    result["fonts"] = fonts;

    // Preset themes
    QJsonArray presets;
    QVariantList presetList = m_settings->getPresetThemes();
    for (const QVariant& v : presetList) {
        QVariantMap map = v.toMap();
        QJsonObject preset;
        preset["name"] = map["name"].toString();
        preset["primaryColor"] = map["primaryColor"].toString();
        preset["isBuiltIn"] = map["isBuiltIn"].toBool();
        presets.append(preset);
    }
    result["presets"] = presets;

    // Colors detected on the current page (set by QML tree walker)
    QJsonArray pageColors;
    for (const QString& colorName : m_settings->currentPageColors()) {
        pageColors.append(colorName);
    }
    result["pageColors"] = pageColors;

    return result;
}

void ShotServer::handleThemeApi(QTcpSocket* socket, const QString& method,
                                 const QString& path, const QByteArray& body)
{
    if (!m_settings) {
        sendResponse(socket, 500, "text/plain", "Settings not available");
        return;
    }

    // GET /api/theme - return full theme state
    if (path == "/api/theme" && method == "GET") {
        QJsonDocument doc(buildThemeJson());
        sendJson(socket, doc.toJson(QJsonDocument::Compact));
        return;
    }

    // POST /api/theme/flash - flash a color red/black on device to identify it
    if (path == "/api/theme/flash" && method == "POST") {
        QJsonObject obj = QJsonDocument::fromJson(body).object();
        QString name = obj["name"].toString();
        if (name.isEmpty()) {
            sendResponse(socket, 400, "text/plain", "Missing name");
            return;
        }
        m_settings->flashThemeColor(name);
        sendResponse(socket, 200, "application/json", "{\"ok\":true}");
        return;
    }

    // POST /api/theme/color - set a single color
    if (path == "/api/theme/color" && method == "POST") {
        QJsonObject obj = QJsonDocument::fromJson(body).object();
        QString name = obj["name"].toString();
        QString value = obj["value"].toString();
        if (name.isEmpty() || value.isEmpty()) {
            sendResponse(socket, 400, "text/plain", "Missing name or value");
            return;
        }
        m_settings->setThemeColor(name, value);
        sendResponse(socket, 200, "application/json", "{\"ok\":true}");
        return;
    }

    // POST /api/theme/font - set a single font size
    if (path == "/api/theme/font" && method == "POST") {
        QJsonObject obj = QJsonDocument::fromJson(body).object();
        QString name = obj["name"].toString();
        int value = obj["value"].toInt();
        if (name.isEmpty() || value <= 0) {
            sendResponse(socket, 400, "text/plain", "Missing name or invalid value");
            return;
        }
        m_settings->setFontSize(name, value);
        sendResponse(socket, 200, "application/json", "{\"ok\":true}");
        return;
    }

    // POST /api/theme/preset - apply a preset theme
    if (path == "/api/theme/preset" && method == "POST") {
        QJsonObject obj = QJsonDocument::fromJson(body).object();
        QString name = obj["name"].toString();
        if (name.isEmpty()) {
            sendResponse(socket, 400, "text/plain", "Missing name");
            return;
        }
        m_settings->applyPresetTheme(name);
        QJsonDocument doc(buildThemeJson());
        sendJson(socket, doc.toJson(QJsonDocument::Compact));
        return;
    }

    // POST /api/theme/palette - generate and apply random palette
    if (path == "/api/theme/palette" && method == "POST") {
        QJsonObject obj = QJsonDocument::fromJson(body).object();
        double hue = obj["hue"].toDouble();
        double saturation = obj["saturation"].toDouble();
        double lightness = obj["lightness"].toDouble();
        QVariantMap palette = m_settings->generatePalette(hue, saturation, lightness);
        m_settings->setCustomThemeColors(palette);
        m_settings->setActiveThemeName("Custom");
        QJsonDocument doc(buildThemeJson());
        sendJson(socket, doc.toJson(QJsonDocument::Compact));
        return;
    }

    // POST /api/theme/save - save current theme with name
    if (path == "/api/theme/save" && method == "POST") {
        QJsonObject obj = QJsonDocument::fromJson(body).object();
        QString name = obj["name"].toString();
        if (name.isEmpty()) {
            sendResponse(socket, 400, "text/plain", "Missing name");
            return;
        }
        m_settings->saveCurrentTheme(name);
        QJsonDocument doc(buildThemeJson());
        sendJson(socket, doc.toJson(QJsonDocument::Compact));
        return;
    }

    // POST /api/theme/reset - reset to defaults
    if (path == "/api/theme/reset" && method == "POST") {
        m_settings->resetThemeToDefault();
        m_settings->resetFontSizesToDefault();
        QJsonDocument doc(buildThemeJson());
        sendJson(socket, doc.toJson(QJsonDocument::Compact));
        return;
    }

    // DELETE /api/theme/preset/{name} - delete a user theme
    if (path.startsWith("/api/theme/preset/") && method == "DELETE") {
        QString name = QUrl::fromPercentEncoding(path.mid(18).toUtf8());
        if (name.isEmpty()) {
            sendResponse(socket, 400, "text/plain", "Missing theme name");
            return;
        }
        m_settings->deleteUserTheme(name);
        QJsonDocument doc(buildThemeJson());
        sendJson(socket, doc.toJson(QJsonDocument::Compact));
        return;
    }

    sendResponse(socket, 404, "text/plain", "Not Found");
}

QString ShotServer::generateThemePage() const
{
    return generateThemePageHtml();
}
