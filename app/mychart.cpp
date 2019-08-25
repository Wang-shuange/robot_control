#include "mychart.h"
#include <QGraphicsSceneMouseEvent>
MyChart::MyChart()
{

}

void MyChart::mousePressEvent(QGraphicsSceneMouseEvent *event)
{

    if ( event->button() != Qt::LeftButton )
    {
        return;
    }
        qDebug("[chart press:][x:%f,y:%f]\n",event->pos().x(),event->pos().y());
        QChart::mousePressEvent(event);
}

void MyChart::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if ( event->button() != Qt::LeftButton )
    {
        return;
    }
      qDebug("[chart release pos:x:%f,y:%f]",event->pos().x(),event->pos().y());
    QChart::mouseReleaseEvent(event);

}
