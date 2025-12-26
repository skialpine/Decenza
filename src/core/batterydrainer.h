#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <atomic>

class CpuWorker : public QThread {
    Q_OBJECT
public:
    explicit CpuWorker(QObject* parent = nullptr) : QThread(parent) {}
    void stop() { m_running = false; }

protected:
    void run() override;

private:
    std::atomic<bool> m_running{true};
};

class BatteryDrainer : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY gpuUsageChanged)
    Q_PROPERTY(int cpuCores READ cpuCores CONSTANT)

public:
    explicit BatteryDrainer(QObject* parent = nullptr);
    ~BatteryDrainer();

    bool running() const { return m_running; }
    double cpuUsage() const { return m_cpuUsage; }
    double gpuUsage() const { return m_gpuUsage; }
    int cpuCores() const { return QThread::idealThreadCount(); }

public slots:
    void start();
    void stop();
    void toggle();

signals:
    void runningChanged();
    void cpuUsageChanged();
    void gpuUsageChanged();

private slots:
    void updateUsageStats();

private:
    void startCpuWorkers();
    void stopCpuWorkers();
    void setMaxBrightness();
    void restoreBrightness();
    double readCpuUsage();
    double readGpuUsage();

    bool m_running = false;
    double m_cpuUsage = 0.0;
    double m_gpuUsage = 0.0;
    int m_savedBrightness = -1;
    QVector<CpuWorker*> m_workers;
    QTimer m_statsTimer;

    // For CPU usage calculation
    uint64_t m_prevIdleTime = 0;
    uint64_t m_prevTotalTime = 0;
};
