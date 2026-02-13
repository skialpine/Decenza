#include "flowcalibrationmodel.h"
#include "../history/shothistorystorage.h"
#include "../core/settings.h"
#include "../ble/de1device.h"

#include <QDateTime>
#include <QDebug>

FlowCalibrationModel::FlowCalibrationModel(QObject* parent)
    : QObject(parent)
{
}

void FlowCalibrationModel::setStorage(ShotHistoryStorage* storage) {
    m_storage = storage;
}

void FlowCalibrationModel::setSettings(Settings* settings) {
    m_settings = settings;
}

void FlowCalibrationModel::setDevice(DE1Device* device) {
    m_device = device;
}

void FlowCalibrationModel::setMultiplier(double m) {
    m = qBound(0.35, m, 2.0);
    if (qAbs(m_multiplier - m) > 0.001) {
        m_multiplier = m;
        recalculateFlow();
        emit multiplierChanged();
    }
}

void FlowCalibrationModel::loadRecentShots() {
    if (!m_storage) return;

    m_shotIds.clear();

    // Get recent shots and filter to those with weight flow rate data
    QVariantList shots = m_storage->getShots(0, 50);
    for (const auto& v : shots) {
        QVariantMap map = v.toMap();
        qint64 id = map["id"].toLongLong();
        ShotRecord record = m_storage->getShotRecord(id);
        if (!record.weightFlowRate.isEmpty()) {
            m_shotIds.append(id);
        }
        if (m_shotIds.size() >= 20) break;
    }

    if (m_shotIds.isEmpty()) {
        m_errorMessage = tr("No shots with scale data found. Run a shot with a Bluetooth scale connected.");
        m_currentIndex = -1;
        m_originalFlow.clear();
        m_recalculatedFlow.clear();
        m_weightFlowRate.clear();
        m_pressure.clear();
        m_shotInfo.clear();
        emit errorChanged();
        emit dataChanged();
    } else {
        m_errorMessage.clear();
        m_currentIndex = 0;
        m_multiplier = m_settings ? m_settings->flowCalibrationMultiplier() : 1.0;
        emit multiplierChanged();
        emit errorChanged();
        loadCurrentShot();
    }

    emit navigationChanged();
}

void FlowCalibrationModel::previousShot() {
    if (m_currentIndex > 0) {
        m_currentIndex--;
        loadCurrentShot();
        emit navigationChanged();
    }
}

void FlowCalibrationModel::nextShot() {
    if (m_currentIndex < m_shotIds.size() - 1) {
        m_currentIndex++;
        loadCurrentShot();
        emit navigationChanged();
    }
}

void FlowCalibrationModel::save() {
    if (m_settings) {
        m_settings->setFlowCalibrationMultiplier(m_multiplier);
        // Signal connection in MainController sends it to the machine
    }
}

void FlowCalibrationModel::resetToFactory() {
    setMultiplier(1.0);
}

void FlowCalibrationModel::loadCurrentShot() {
    if (m_currentIndex < 0 || m_currentIndex >= m_shotIds.size() || !m_storage) return;

    ShotRecord record = m_storage->getShotRecord(m_shotIds[m_currentIndex]);

    m_originalFlow = record.flow;
    m_weightFlowRate = record.weightFlowRate;
    m_pressure = record.pressure;

    // Assume shot was recorded with multiplier 1.0 (no per-shot storage yet)
    m_shotMultiplier = 1.0;

    // Calculate max time from pressure data (most reliable time source)
    m_maxTime = 60.0;
    if (!m_pressure.isEmpty()) {
        m_maxTime = m_pressure.last().x();
    } else if (!m_originalFlow.isEmpty()) {
        m_maxTime = m_originalFlow.last().x();
    }

    // Build shot info string
    QDateTime dt = QDateTime::fromSecsSinceEpoch(record.summary.timestamp);
    m_shotInfo = record.summary.profileName + " \u2014 " + dt.toString("MMM d, yyyy");

    recalculateFlow();
}

void FlowCalibrationModel::recalculateFlow() {
    m_recalculatedFlow.clear();
    m_recalculatedFlow.reserve(m_originalFlow.size());

    double shotMul = (m_shotMultiplier > 0.001) ? m_shotMultiplier : 1.0;
    for (const auto& pt : m_originalFlow) {
        double newY = m_multiplier * pt.y() / shotMul;
        m_recalculatedFlow.append(QPointF(pt.x(), newY));
    }

    emit dataChanged();
}

QVariantList FlowCalibrationModel::flowData() const {
    return pointsToVariant(m_recalculatedFlow);
}

QVariantList FlowCalibrationModel::weightFlowData() const {
    return pointsToVariant(m_weightFlowRate);
}

QVariantList FlowCalibrationModel::pressureData() const {
    return pointsToVariant(m_pressure);
}

QVariantList FlowCalibrationModel::pointsToVariant(const QVector<QPointF>& points) const {
    QVariantList result;
    for (const auto& pt : points) {
        QVariantMap p;
        p["x"] = pt.x();
        p["y"] = pt.y();
        result.append(p);
    }
    return result;
}
