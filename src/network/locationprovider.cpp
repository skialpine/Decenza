#include "locationprovider.h"

#include <QGeoPositionInfo>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDebug>
#include <cmath>

LocationProvider::LocationProvider(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Load saved manual city and coordinates
    QSettings settings;
    m_manualCity = settings.value("shotMap/manualCity", "").toString();
    m_manualLat = settings.value("shotMap/manualLat", 0.0).toDouble();
    m_manualLon = settings.value("shotMap/manualLon", 0.0).toDouble();
    m_manualGeocoded = settings.value("shotMap/manualGeocoded", false).toBool();

    // Try to create a position source
    m_source = QGeoPositionInfoSource::createDefaultSource(this);

    if (m_source) {
        connect(m_source, &QGeoPositionInfoSource::positionUpdated,
                this, &LocationProvider::onPositionUpdated);
        connect(m_source, &QGeoPositionInfoSource::errorOccurred,
                this, &LocationProvider::onPositionError);

        qDebug() << "LocationProvider: GPS source available:" << m_source->sourceName();
    } else {
        qDebug() << "LocationProvider: No GPS source available";
    }

    if (!m_manualCity.isEmpty()) {
        qDebug() << "LocationProvider: Manual city configured:" << m_manualCity
                 << "at" << m_manualLat << m_manualLon;
    }
}

LocationProvider::~LocationProvider()
{
    if (m_source) {
        m_source->stopUpdates();
    }
}

double LocationProvider::roundedLatitude() const
{
    // Use GPS if valid, otherwise use manual city coordinates
    double lat = m_currentLocation.valid ? m_currentLocation.latitude : m_manualLat;
    // Round to 1 decimal place (~11km precision)
    return std::round(lat * 10.0) / 10.0;
}

double LocationProvider::roundedLongitude() const
{
    // Use GPS if valid, otherwise use manual city coordinates
    double lon = m_currentLocation.valid ? m_currentLocation.longitude : m_manualLon;
    // Round to 1 decimal place (~11km precision)
    return std::round(lon * 10.0) / 10.0;
}

void LocationProvider::requestUpdate()
{
    if (!m_source) {
        emit locationError("No GPS source available");
        return;
    }

    qDebug() << "LocationProvider: Requesting position update...";
    m_source->requestUpdate(30000);  // 30 second timeout
}

void LocationProvider::onPositionUpdated(const QGeoPositionInfo& info)
{
    if (!info.isValid()) {
        qDebug() << "LocationProvider: Received invalid position";
        return;
    }

    QGeoCoordinate coord = info.coordinate();
    qDebug() << "LocationProvider: Position updated -"
             << "Lat:" << coord.latitude()
             << "Lon:" << coord.longitude();

    m_currentLocation.latitude = coord.latitude();
    m_currentLocation.longitude = coord.longitude();

    // Check if we need to reverse geocode (position changed significantly)
    double latDiff = std::abs(coord.latitude() - m_lastGeocodedLat);
    double lonDiff = std::abs(coord.longitude() - m_lastGeocodedLon);

    if (latDiff > GEOCODE_THRESHOLD_DEGREES || lonDiff > GEOCODE_THRESHOLD_DEGREES
        || m_currentLocation.city.isEmpty()) {
        reverseGeocode(coord.latitude(), coord.longitude());
    } else {
        // Position hasn't changed much, just update coordinates
        m_currentLocation.valid = true;
        emit locationChanged();
    }
}

void LocationProvider::onPositionError(QGeoPositionInfoSource::Error error)
{
    QString errorStr;
    switch (error) {
    case QGeoPositionInfoSource::AccessError:
        errorStr = "Location permission denied";
        break;
    case QGeoPositionInfoSource::ClosedError:
        errorStr = "Location source closed";
        break;
    case QGeoPositionInfoSource::NoError:
        return;
    default:
        errorStr = "Unknown location error";
        break;
    }

    qDebug() << "LocationProvider: Error -" << errorStr;
    emit locationError(errorStr);
}

void LocationProvider::reverseGeocode(double lat, double lon)
{
    // Use Nominatim for reverse geocoding (free, no API key needed)
    // Note: Must respect usage policy - max 1 request/second, include User-Agent
    QString url = QString("https://nominatim.openstreetmap.org/reverse?format=json&lat=%1&lon=%2&zoom=10")
                      .arg(lat, 0, 'f', 6)
                      .arg(lon, 0, 'f', 6);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Decenza_DE1/1.0 (espresso app)");

    qDebug() << "LocationProvider: Reverse geocoding...";

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReverseGeocodeFinished(reply);
    });

    // Remember this position to avoid re-geocoding
    m_lastGeocodedLat = lat;
    m_lastGeocodedLon = lon;
}

void LocationProvider::onReverseGeocodeFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "LocationProvider: Reverse geocode failed -" << reply->errorString();
        // Still mark as valid - we have coordinates, just no city name
        m_currentLocation.valid = true;
        emit locationChanged();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    // Extract city and country from address
    QJsonObject address = obj["address"].toObject();

    // Try different fields for city (Nominatim uses different names depending on location)
    QString city = address["city"].toString();
    if (city.isEmpty()) city = address["town"].toString();
    if (city.isEmpty()) city = address["village"].toString();
    if (city.isEmpty()) city = address["municipality"].toString();
    if (city.isEmpty()) city = address["county"].toString();
    if (city.isEmpty()) city = address["state"].toString();

    QString countryCode = address["country_code"].toString().toUpper();

    qDebug() << "LocationProvider: Geocoded to" << city << countryCode;

    m_currentLocation.city = city;
    m_currentLocation.countryCode = countryCode;
    m_currentLocation.valid = true;

    emit locationChanged();
}

QString LocationProvider::city() const
{
    // If GPS location is valid, use it; otherwise fall back to manual city
    if (m_currentLocation.valid) {
        return m_currentLocation.city;
    }
    return m_manualCity;
}

void LocationProvider::setManualCity(const QString& city)
{
    if (m_manualCity != city) {
        m_manualCity = city;
        m_manualGeocoded = false;
        m_manualLat = 0.0;
        m_manualLon = 0.0;

        // Save to settings
        QSettings settings;
        settings.setValue("shotMap/manualCity", city);
        settings.setValue("shotMap/manualGeocoded", false);
        settings.setValue("shotMap/manualLat", 0.0);
        settings.setValue("shotMap/manualLon", 0.0);

        qDebug() << "LocationProvider: Manual city set to:" << city;

        emit manualCityChanged();
        emit locationChanged();

        // Auto-geocode if city is not empty
        if (!city.isEmpty()) {
            geocodeManualCity();
        }
    }
}

void LocationProvider::geocodeManualCity()
{
    if (m_manualCity.isEmpty()) {
        qDebug() << "LocationProvider: No manual city to geocode";
        return;
    }

    forwardGeocode(m_manualCity);
}

void LocationProvider::forwardGeocode(const QString& city)
{
    // Use Nominatim for forward geocoding
    QString encodedCity = QUrl::toPercentEncoding(city);
    QString url = QString("https://nominatim.openstreetmap.org/search?format=json&q=%1&limit=1")
                      .arg(encodedCity);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Decenza_DE1/1.0 (espresso app)");

    qDebug() << "LocationProvider: Forward geocoding:" << city;

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onForwardGeocodeFinished(reply);
    });
}

void LocationProvider::onForwardGeocodeFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "LocationProvider: Forward geocode failed -" << reply->errorString();
        emit locationError("Failed to geocode city: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray results = doc.array();

    if (results.isEmpty()) {
        qDebug() << "LocationProvider: No geocoding results for" << m_manualCity;
        emit locationError("City not found: " + m_manualCity);
        return;
    }

    QJsonObject result = results.first().toObject();
    m_manualLat = result["lat"].toString().toDouble();
    m_manualLon = result["lon"].toString().toDouble();
    m_manualGeocoded = true;

    // Save to settings
    QSettings settings;
    settings.setValue("shotMap/manualLat", m_manualLat);
    settings.setValue("shotMap/manualLon", m_manualLon);
    settings.setValue("shotMap/manualGeocoded", true);

    QString displayName = result["display_name"].toString();
    qDebug() << "LocationProvider: Geocoded" << m_manualCity << "to"
             << m_manualLat << m_manualLon << "-" << displayName;

    emit locationChanged();
}
