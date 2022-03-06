#include "chart.h"
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QtCore/QRandomGenerator>
#include <QtCore/QDebug>

Chart::Chart(qreal minY, qreal maxY, Qt::GlobalColor color, QGraphicsItem *parent, Qt::WindowFlags wFlags):
    QChart(QChart::ChartTypeCartesian, parent, wFlags),
    m_series(0),
    m_axisX(new QValueAxis()),
    m_axisY(new QValueAxis()),
    m_step(0),
    m_x(9),
    m_y(0)
{
    m_series = new QSplineSeries(this);
    QPen linePen(color);
    linePen.setWidth(3);
    m_series->setPen(linePen);

    m_series->append(m_x, minY);

    addSeries(m_series);

    addAxis(m_axisX,Qt::AlignBottom);
    addAxis(m_axisY,Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    m_axisX->setTickCount(10);
    m_axisX->setRange(0, 10);
    m_axisY->setRange(minY, maxY);
}

Chart::~Chart()
{
    delete m_series;
}

QSplineSeries* Chart::getSeries()
{
    return m_series;
}

void Chart::handleTimeout()
{
    qreal x = plotArea().width() / m_axisX->tickCount();
    qreal y = (m_axisX->max() - m_axisX->min()) / m_axisX->tickCount();
    m_x += y;
    m_y = QRandomGenerator::global()->bounded(5) - 2.5;
    m_series->append(m_x, m_y);
    scroll(x, 0);
    if (m_x == 100)
        m_timer.stop();
}

void Chart::setLatestValue(qreal value)
{
    qreal x = plotArea().width() / m_axisX->tickCount();
    qreal y = (m_axisX->max() - m_axisX->min()) / m_axisX->tickCount();
    m_x += y;
    m_y = value;
    m_series->append(m_x, m_y);
    qDebug() << "Create point at " << m_x << " : " << m_y;
    scroll(x, 0);
}
