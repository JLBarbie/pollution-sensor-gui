#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QMainWindow>
#include <QDateTime>

enum LogType {
    HEX,
    INFO,
    DEBUG,
    NOTIF
};

enum BLEAction {
    ACTION_CALIBRATE,
    ACTION_UPDATE
};

class Chart;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNotification(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onDeviceConnect();
    void onError(QBluetoothDeviceDiscoveryAgent::Error err);
    void onFinished();
    void addDevice(QBluetoothDeviceInfo devInfo);
    void onServiceDiscovered(const QBluetoothUuid &newService);
    void serviceStateChanged(QLowEnergyService::ServiceState newState);

private:
    void writeOnSensor();
    void createChart();
    void log(LogType type, QString msg);

    Ui::MainWindow *ui;

    BLEAction m_ble_action;

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller;
    QLowEnergyService *m_service;

    QLowEnergyCharacteristic m_sensor;
    QLowEnergyCharacteristic m_command;

    QDateTime m_lastUpdate;

    // Charts
    Chart *m_tempChart;
    Chart *m_TVOCChart;
    Chart *m_HCHOChart;
    Chart *m_CO2Chart;
};
#endif // MAINWINDOW_H
