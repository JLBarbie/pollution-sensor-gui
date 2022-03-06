#ifndef CHART_H
#define CHART_H

#include <QtCharts/QChart>
#include <QtCore/QTimer>

QT_CHARTS_BEGIN_NAMESPACE
class QSplineSeries;
class QValueAxis;
QT_CHARTS_END_NAMESPACE

QT_CHARTS_USE_NAMESPACE

//![1]
class Chart: public QChart
{
    Q_OBJECT
public:
//    Chart(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {});
    Chart(qreal minY, qreal maxY, Qt::GlobalColor color, QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {});
    virtual ~Chart();

    void setLatestValue(qreal value);
    QSplineSeries *getSeries();

public slots:
    void handleTimeout();

private:
    QTimer m_timer;
    QSplineSeries *m_series;
    QStringList m_titles;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    qreal m_step;
    qreal m_x;
    qreal m_y;

    // Chart options
    QPair<qreal, qreal> m_limitX;
    QPair<qreal, qreal> m_limitY;

};
//![1]

#endif /* CHART_H */
