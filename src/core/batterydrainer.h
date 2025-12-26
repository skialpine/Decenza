#pragma once

#include <QObject>
#include <QThread>
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
    Q_PROPERTY(double cpuLoad READ cpuLoad NOTIFY cpuLoadChanged)
    Q_PROPERTY(bool flashlightOn READ flashlightOn NOTIFY flashlightOnChanged)

public:
    explicit BatteryDrainer(QObject* parent = nullptr);
    ~BatteryDrainer();

    bool running() const { return m_running; }
    double cpuLoad() const { return m_cpuLoad; }
    bool flashlightOn() const { return m_flashlightOn; }

public slots:
    void start();
    void stop();
    void toggle();

signals:
    void runningChanged();
    void cpuLoadChanged();
    void flashlightOnChanged();

private:
    void startCpuWorkers();
    void stopCpuWorkers();
    void setMaxBrightness();
    void restoreBrightness();
    void enableFlashlight(bool on);

    bool m_running = false;
    double m_cpuLoad = 0.0;
    bool m_flashlightOn = false;
    int m_savedBrightness = -1;
    QVector<CpuWorker*> m_workers;
};
