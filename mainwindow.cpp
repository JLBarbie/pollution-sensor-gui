#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chart.h"

#include <QDateTime>
#include <QDebug>
#include <QLabel>
#include <QThread>
#include <QTimer>

#include <QtCharts/QSplineSeries>
#include <QtCharts/QChartView>

using namespace QtCharts;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_ble_action(ACTION_UPDATE)
{
    ui->setupUi(this);

    createChart();

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent();

    m_discoveryAgent->setLowEnergyDiscoveryTimeout(5000);

    connect(ui->calibrateBtn, &QPushButton::clicked, [=]() {m_ble_action = ACTION_CALIBRATE;} );

    // Slot pour la recherche d'appareils BLE
    connect(m_discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(addDevice(QBluetoothDeviceInfo)));
    connect(m_discoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(onError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_discoveryAgent, SIGNAL(finished()), this, SLOT(onFinished()));

    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

MainWindow::~MainWindow()
{
    delete m_tempChart;
    delete ui;
}

void MainWindow::createChart()
{
    m_tempChart = new Chart(-10, 35, Qt::red);
    m_tempChart->getSeries()->setName("Temperature (C°)");
    QChartView *tempView = new QChartView(m_tempChart);
    tempView->setRenderHint(QPainter::Antialiasing);

    m_TVOCChart = new Chart(0,30, Qt::blue);
    m_TVOCChart->getSeries()->setName("TVOC");
    QChartView *tvocView = new QChartView(m_TVOCChart);
    tvocView->setRenderHint(QPainter::Antialiasing);

    m_HCHOChart = new Chart(0,10, Qt::darkGreen);
    m_HCHOChart->getSeries()->setName("HCHO");
    QChartView *hchoView = new QChartView(m_HCHOChart);
    hchoView->setRenderHint(QPainter::Antialiasing);

    m_CO2Chart  = new Chart(0,1000, Qt::darkGray);
    m_CO2Chart->getSeries()->setName("CO2");
    QChartView *co2View = new QChartView(m_CO2Chart);
    co2View->setRenderHint(QPainter::Antialiasing);

    ui->gridLayout->addWidget(tempView, 1, 0);
    ui->gridLayout->addWidget(tvocView, 1, 1);
    ui->gridLayout->addWidget(hchoView, 2, 0);
    ui->gridLayout->addWidget(co2View, 2, 1);
}

void MainWindow::onNotification(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    qDebug() << "Notification : " << characteristic.uuid();
    qDebug() << QTime::currentTime().toString("hh:mm:ss");
//    ui->dataLabel->setText(QByteArray::fromHex(newValue));

    QBluetoothUuid uuid_tx(QString("0000FFF1-0000-1000-8000-00805F9B34FB"));
    QBluetoothUuid uuid_rx(QString("0000FFF4-0000-1000-8000-00805F9B34FB"));

    if (characteristic.uuid() == uuid_rx)
    {
        const quint8 *data = reinterpret_cast<const quint8 *>(newValue.constData());

        if (data[0] == 170) // 0xaa
        {
            //qDebug() << "* DeviceWP6003 update:" << getAddress();
            //qDebug() << "- data?" << data[6];
        }
        else if (data[0] == 173) // 0xad
        {
            //qDebug() << "* DeviceWP6003 calibration started:" << getAddress();
        }
        else if (data[0] == 10) // 0x0a
        {
            QDate d(2000 + data[1], data[2], data[3]);
            QTime t(data[4], data[5]);

            int16_t temp = static_cast<int16_t>((data[6] << 8) + data[7]);
            uint16_t voc = static_cast<uint16_t>((data[10] << 8) + data[11]);
            uint16_t hcho = static_cast<uint16_t>((data[12] << 8) + data[13]);
            uint16_t co2 = static_cast<uint16_t>((data[16] << 8) + data[17]);

            // still preheating?
            //           if (voc < 16383 && hcho < 16383)
            //           {
            //               voc = voc;
            //               hcho = hcho;
            //           }
            //           co2 = co2;
            double temperature = temp / 10.f;

            m_tempChart->setLatestValue(temperature);
            m_TVOCChart->setLatestValue(voc);
            m_HCHOChart->setLatestValue(hcho);
            m_CO2Chart->setLatestValue(co2);

            m_lastUpdate = QDateTime::currentDateTime();


#ifndef QT_NO_DEBUG
            //qDebug() << "* DeviceWP6003 update:" << getAddress();
            qDebug() << "- Timecode:" << QDateTime(d, t);
            qDebug() << "- Temperature:" << temperature;
            qDebug() << "- TVOC:" << voc;
            qDebug() << "- HCHO:" << hcho;
            qDebug() << "- eCO2:" << co2;
#endif
        }
    }
}

void MainWindow::onDeviceConnect()
{
    log(INFO, "Connected to Sensor");
    connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &MainWindow::onServiceDiscovered);
    m_controller->discoverServices();
}

void MainWindow::addDevice(QBluetoothDeviceInfo info)
{
    // Bluetooth Low Energy ?
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
    {
        if(info.address() == QBluetoothAddress("60:03:03:93:AB:3D")) // Connect with name not address
        {
            m_controller = QLowEnergyController::createCentral(info, this);
            connect(m_controller, &QLowEnergyController::connected, this, &MainWindow::onDeviceConnect);
            m_controller->connectToDevice();
        }
    }
}

void MainWindow::onServiceDiscovered(const QBluetoothUuid &newService)
{
    log(INFO, "Service found : " + newService.toString());
    if (newService.toString() == "{0000fff0-0000-1000-8000-00805f9b34fb}")
    {
        m_service = m_controller->createServiceObject(QBluetoothUuid(newService), this);
        if (m_service)
        {
            connect(m_service, &QLowEnergyService::stateChanged, this, &MainWindow::serviceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged, this, &MainWindow::onNotification);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { m_service->discoverDetails(); });
        }
    }
}

void MainWindow::onError(QBluetoothDeviceDiscoveryAgent::Error err)
{
    // Need to be a lambda
    Q_UNUSED(err);
}

void MainWindow::onFinished()
{
}

void MainWindow::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceWP6003::serviceDetailsDiscovered_data(" << m_deviceAddress << ") > ServiceDiscovered";

        if (m_service)
        {
            QBluetoothUuid uuid_tx(QString("0000FFF1-0000-1000-8000-00805F9B34FB"));
            QBluetoothUuid uuid_rx(QString("0000FFF4-0000-1000-8000-00805F9B34FB"));

            // Characteristic "RX" // NOTIFY
            {
                QLowEnergyCharacteristic crx = m_service->characteristic(uuid_rx);
                QLowEnergyDescriptor notificationDesc = crx.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                m_service->writeDescriptor(notificationDesc, QByteArray::fromHex("0100"));
            }

            // Characteristic "TX" // WRITE
            {
                QLowEnergyCharacteristic ctx = m_service->characteristic(uuid_tx);

                if (m_ble_action == ACTION_CALIBRATE)
                {
                    // Send qualibration request "ad"
                    m_service->writeCharacteristic(ctx, QByteArray::fromHex("ad"), QLowEnergyService::WriteWithoutResponse);
                    m_ble_action = ACTION_UPDATE;
                    ui->calibrateBtn->setChecked(false);
                }
                else // if (m_ble_action == ACTION_UPDATE)
                {
                    // Send set datetime command
                    // "aa" + datetime
                    QDateTime cdt = QDateTime::currentDateTime();
                    QByteArray cmd(QByteArray::fromHex("aa"));
                    cmd.push_back(cdt.date().year()%100);
                    cmd.push_back(cdt.date().month());
                    cmd.push_back(cdt.date().day());
                    cmd.push_back(cdt.time().hour());
                    cmd.push_back(cdt.time().minute());
                    cmd.push_back(cdt.time().second());
                    log(HEX, QString(QByteArray::fromHex(cmd)));
                    m_service->writeCharacteristic(ctx, cmd, QLowEnergyService::WriteWithoutResponse);

                    // Set notify interval
                    // "ae" + interval
                    m_service->writeCharacteristic(ctx, QByteArray::fromHex("ae0101"), QLowEnergyService::WriteWithoutResponse);

                    // Send notify request : "ab"
                    m_service->writeCharacteristic(ctx, QByteArray::fromHex("ab"), QLowEnergyService::WriteWithoutResponse);
                }
            }
        }
    }
}

void MainWindow::log(LogType type, QString msg)
{
    switch (type)
    {
    case HEX:
        qDebug() << "[HEX] " << msg;
        break;
    case INFO:
        qDebug() << "[INFO]" << msg;
        break;
    case DEBUG:
        qDebug() << "[DBUG]" << msg;
        break;
    case NOTIF:
    default:
        qDebug() << "[NOTIF]" << msg;
        break;

    }
}

