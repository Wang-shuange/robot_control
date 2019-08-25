#ifndef MYCHART_H
#define MYCHART_H
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtWidgets/QVBoxLayout>
#include <QtCharts/QValueAxis>
using namespace QtCharts;
class MyChart : public QChart
{
    Q_OBJECT
public:
    MyChart();

    // QGraphicsItem interface
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

#endif // MYCHART_H
