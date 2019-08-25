#include "drawtool.h"
#include "drawobj.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QtMath>
#include "drawobj.h"
#define PI 3.1416
extern const double ANGLE_MAX=180.;
extern const double  SCENE_WIDTH;
extern const double  SCENE_HEIGHT;

QList<DrawTool*> DrawTool::c_tools;
QPointF DrawTool::c_down;
QPointF DrawTool::c_last;
quint32 DrawTool::c_nDownFlags;

DrawShape DrawTool::c_drawShape = selection;

static SelectTool selectTool;
static RectTool   rectTool(rectangle);
static RectTool   roundRectTool(roundrect);
static RectTool   ellipseTool(ellipse);

static PolygonTool lineTool(line);
static PolygonTool polygonTool(polygon);
static PolygonTool bezierTool(bezier);
static PolygonTool polylineTool(polyline);

static RotationTool rotationTool;

enum SelectMode
{
    none,
    netSelect,
    move,
    size,
    rotate,
    editor,
};

SelectMode selectMode = none;

int nDragHandle = Handle_None;

static void setCursor(DrawScene * scene , const QCursor & cursor )
{
    QGraphicsView * view = scene->view();
    if (view)
        view->setCursor(cursor);
}

DrawTool::DrawTool(DrawShape shape)//DrawShape is a emun that saved the shape type;
{
    m_drawShape = shape ;
    m_hoverSizer = false;
    c_tools.push_back(this);

}
void DrawTool::constraint4servo(DrawScene* scene)
{
    if(scene->m_cur_servoinfo)
    {
        double max_limit=SCENE_HEIGHT-(static_cast<double>(scene->m_cur_servoinfo->get_jdMax()))*SCENE_HEIGHT/ANGLE_MAX;
        double min_limit=SCENE_HEIGHT-(static_cast<double>(scene->m_cur_servoinfo->get_jdMin()))*SCENE_HEIGHT/ANGLE_MAX;
        //        qDebug()<<"max_limit:"<<max_limit<<endl;
        //        qDebug()<<"min_limit:"<<min_limit<<endl;
        //        qDebug()<<"c_down.y:"<<c_down.y()<<endl;

        if(c_down.y()<max_limit||c_last.y()<max_limit)
        {
            c_down.setY(max_limit);
            c_last.setY(max_limit);
        }

        if(c_down.y()>min_limit||c_last.y()>min_limit)
        {
            c_down.setY(min_limit);
            c_last.setY(min_limit);
        }
    }
}
void DrawTool::mousePressEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    c_down = event->scenePos();
    c_last = event->scenePos();
    constraint4servo(scene);

}

void DrawTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    c_last = event->scenePos();
    constraint4servo(scene);
}

void DrawTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    if (event->scenePos() == c_down )
        c_drawShape = selection;
    setCursor(scene,Qt::ArrowCursor);
}

void DrawTool::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    //    this->m_last_doubleclick=event->scenePos();
    //    qDebug("[drawtool last point:][x:%f,y:%f]\n",event->scenePos().x(),event->scenePos().y());
}

DrawTool *DrawTool::findTool(DrawShape drawShape)
{
    QList<DrawTool*>::const_iterator iter = c_tools.constBegin();
    for ( ; iter != c_tools.constEnd() ; ++iter ){
        if ((*iter)->m_drawShape == drawShape )
            return (*iter);
    }
    return nullptr;
}

SelectTool::SelectTool()
    :DrawTool(selection)
{
    dashRect = nullptr;
    selLayer = nullptr;
    opposite_ = QPointF();
}

void SelectTool::mousePressEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mousePressEvent(event,scene);

    if ( event->button() != Qt::LeftButton ) return;

    if (!m_hoverSizer)
        scene->mouseEvent(event);

    nDragHandle = Handle_None;
    selectMode = none;
    QList<QGraphicsItem *> items = scene->selectedItems();
    AbstractShape *item = nullptr;

    if ( items.count() == 1 )
        item = qgraphicsitem_cast<AbstractShape*>(items.first());

    if ( item != nullptr ){

        nDragHandle = item->collidesWithHandle(event->scenePos());
        if ( nDragHandle != Handle_None && nDragHandle <= Left )
            selectMode = size;
        else if ( nDragHandle > Left )
            selectMode = editor;
        else
            selectMode =  move;

        if ( nDragHandle!= Handle_None && nDragHandle <= Left ){
            opposite_ = item->opposite(nDragHandle);
            if( opposite_.x() == 0 )
                opposite_.setX(1);
            if (opposite_.y() == 0 )
                opposite_.setY(1);
        }

        setCursor(scene,Qt::CrossCursor);

    }else if ( items.count() > 1 )
        selectMode =  move;

    if( selectMode == none ){
        selectMode = netSelect;
        if ( scene->view() ){
            QGraphicsView * view = scene->view();
            view->setDragMode(QGraphicsView::RubberBandDrag);
        }
#if 0
        if ( selLayer ){
            scene->destroyGroup(selLayer);
            selLayer = 0;
        }
#endif
    }

    if ( selectMode == move && items.count() == 1 ){

        if (dashRect ){
            scene->removeItem(dashRect);
            delete dashRect;
            dashRect = 0;
        }

        dashRect = new QGraphicsPathItem(item->shape());
        dashRect->setPen(Qt::DashLine);
        dashRect->setPos(item->pos());
        dashRect->setTransformOriginPoint(item->transformOriginPoint());
        dashRect->setTransform(item->transform());
        dashRect->setRotation(item->rotation());
        dashRect->setScale(item->scale());
        dashRect->setZValue(item->zValue());
        scene->addItem(dashRect);

        initialPositions = item->pos();
    }
}

void SelectTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mouseMoveEvent(event,scene);
    QList<QGraphicsItem *> items = scene->selectedItems();
    AbstractShape * item = nullptr;
    if ( items.count() == 1 ){
        item = qgraphicsitem_cast<AbstractShape*>(items.first());
        if ( item != 0 ){
            if ( nDragHandle != Handle_None && selectMode == size ){
                if (opposite_.isNull()){
                    opposite_ = item->opposite(nDragHandle);
                    if( opposite_.x() == 0 )
                        opposite_.setX(1);
                    if (opposite_.y() == 0 )
                        opposite_.setY(1);
                }

                QPointF new_delta = item->mapFromScene(c_last) - opposite_;
                QPointF initial_delta = item->mapFromScene(c_down) - opposite_;

                double sx = new_delta.x() / initial_delta.x();
                double sy = new_delta.y() / initial_delta.y();

                item->stretch(nDragHandle, sx , sy , opposite_);

                emit scene->itemResize(item,nDragHandle,QPointF(sx,sy));

                //  qDebug()<<"scale:"<<nDragHandle<< item->mapToScene(opposite_)<< sx << " ，" << sy
                //         << new_delta << item->mapFromScene(c_last)
                //         << initial_delta << item->mapFromScene(c_down) << item->boundingRect();

            } else if ( nDragHandle > Left  && selectMode == editor ){
                item->control(nDragHandle,c_last);
                emit scene->itemControl(item,nDragHandle,c_last,c_down);
            }
            else if(nDragHandle == Handle_None ){
                int handle = item->collidesWithHandle(event->scenePos());
                if ( handle != Handle_None){
                    setCursor(scene,Qt::OpenHandCursor);
                    m_hoverSizer = true;
                }else{
                    setCursor(scene,Qt::ArrowCursor);
                    m_hoverSizer = false;
                }
            }
        }
    }

    if ( selectMode == move ){
        setCursor(scene,Qt::CrossCursor);
        if ( dashRect ){
            dashRect->setPos(initialPositions + c_last - c_down);
        }
    }

    if ( selectMode != size  && items.count() > 1)
    {
        scene->mouseEvent(event);

    }

}

void SelectTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{

    DrawTool::mouseReleaseEvent(event,scene);

    if ( event->button() != Qt::LeftButton ) return;

    QList<QGraphicsItem *> items = scene->selectedItems();
    if ( items.count() == 1 ){
        AbstractShape * item = qgraphicsitem_cast<AbstractShape*>(items.first());
        if ( item != 0  && selectMode == move && c_last != c_down ){
            item->setPos(initialPositions + c_last - c_down);
            emit scene->itemMoved(item , c_last - c_down );
        }else if ( item !=0 && (selectMode == size || selectMode ==editor) && c_last != c_down ){

            item->updateCoordinate();
        }
    }else if ( items.count() > 1 && selectMode == move && c_last != c_down ){
        emit scene->itemMoved(nullptr , c_last - c_down );
    }

    if (selectMode == netSelect ){

        if ( scene->view() ){
            QGraphicsView * view = scene->view();
            view->setDragMode(QGraphicsView::NoDrag);
        }
#if 0
        if ( scene->selectedItems().count() > 1 ){
            selLayer = scene->createGroup(scene->selectedItems());
            selLayer->setSelected(true);
        }
#endif
    }
    if (dashRect ){
        scene->removeItem(dashRect);
        delete dashRect;
        dashRect = nullptr;
    }
    selectMode = none;
    nDragHandle = Handle_None;
    m_hoverSizer = false;
    opposite_ = QPointF();
    scene->mouseEvent(event);
}

RotationTool::RotationTool()
    :DrawTool(rotation)
{
    lastAngle = 0.0;
    dashRect = 0;
}

void RotationTool::mousePressEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mousePressEvent(event,scene);
    if ( event->button() != Qt::LeftButton ) return;

    if (!m_hoverSizer)
        scene->mouseEvent(event);

    QList<QGraphicsItem *> items = scene->selectedItems();
    if ( items.count() == 1 ){
        AbstractShape * item = qgraphicsitem_cast<AbstractShape*>(items.first());
        if ( item != 0 ){
            nDragHandle = item->collidesWithHandle(event->scenePos());
            if ( nDragHandle !=Handle_None)
            {
                QPointF origin = item->mapToScene(item->boundingRect().center());

                qreal len_y = c_last.y() - origin.y();
                qreal len_x = c_last.x() - origin.x();

                qreal angle = atan2(len_y,len_x)*180/PI;

                lastAngle = angle;
                selectMode = rotate;

                if (dashRect ){
                    scene->removeItem(dashRect);
                    delete dashRect;
                    dashRect = 0;
                }

                dashRect = new QGraphicsPathItem(item->shape());
                dashRect->setPen(Qt::DashLine);
                dashRect->setPos(item->pos());
                dashRect->setTransformOriginPoint(item->transformOriginPoint());
                dashRect->setTransform(item->transform());
                dashRect->setRotation(item->rotation());
                dashRect->setScale(item->scale());
                dashRect->setZValue(item->zValue());
                scene->addItem(dashRect);
                setCursor(scene,QCursor((QPixmap(":/icons/rotate.png"))));
            }
            else{
                c_drawShape = selection;
                selectTool.mousePressEvent(event,scene);
            }
        }
    }
}

void RotationTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mouseMoveEvent(event,scene);
    QList<QGraphicsItem *> items = scene->selectedItems();
    if ( items.count() == 1 ){
        AbstractShape * item = qgraphicsitem_cast<AbstractShape*>(items.first());
        if ( item != 0  && nDragHandle !=Handle_None && selectMode == rotate ){

            QPointF origin = item->mapToScene(item->boundingRect().center());

            qreal len_y = c_last.y() - origin.y();
            qreal len_x = c_last.x() - origin.x();
            qreal angle = atan2(len_y,len_x)*180/PI;

            angle = item->rotation() + int(angle - lastAngle) ;

            if ( angle > 360 )
                angle -= 360;
            if ( angle < -360 )
                angle+=360;

            if ( dashRect ){
                //dashRect->setTransform(QTransform::fromTranslate(15,15),true);
                //dashRect->setTransform(QTransform().rotate(angle));
                //dashRect->setTransform(QTransform::fromTranslate(-15,-15),true);
                dashRect->setRotation( angle );
            }

            setCursor(scene,QCursor((QPixmap(":/icons/rotate.png"))));
        }
        else if ( item )
        {
            int handle = item->collidesWithHandle(event->scenePos());
            if ( handle != Handle_None){
                setCursor(scene,QCursor((QPixmap(":/icons/rotate.png"))));
                m_hoverSizer = true;
            }else{
                setCursor(scene,Qt::ArrowCursor);
                m_hoverSizer = false;
            }
        }
    }
    scene->mouseEvent(event);
}

void RotationTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mouseReleaseEvent(event,scene);
    if ( event->button() != Qt::LeftButton ) return;

    QList<QGraphicsItem *> items = scene->selectedItems();
    if ( items.count() == 1 ){
        AbstractShape * item = qgraphicsitem_cast<AbstractShape*>(items.first());
        if ( item != 0  && nDragHandle !=Handle_None && selectMode == rotate ){

            QPointF origin = item->mapToScene(item->boundingRect().center());
            QPointF delta = c_last - origin ;
            qreal len_y = c_last.y() - origin.y();
            qreal len_x = c_last.x() - origin.x();
            qreal angle = atan2(len_y,len_x)*180/PI,oldAngle = item->rotation();
            angle = item->rotation() + int(angle - lastAngle) ;

            if ( angle > 360 )
                angle -= 360;
            if ( angle < -360 )
                angle+=360;

            item->setRotation( angle );
            emit scene->itemRotate(item , oldAngle);
            qDebug()<<"rotate:"<<angle<<item->boundingRect();
        }
    }

    setCursor(scene,Qt::ArrowCursor);
    selectMode = none;
    nDragHandle = Handle_None;
    lastAngle = 0;
    m_hoverSizer = false;
    if (dashRect ){
        scene->removeItem(dashRect);
        delete dashRect;
        dashRect = 0;
    }
    scene->mouseEvent(event);
}

RectTool::RectTool(DrawShape drawShape)
    :DrawTool(drawShape)
{
    item = 0;
}

void RectTool::mousePressEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{

    if ( event->button() != Qt::LeftButton ) return;

    scene->clearSelection();
    DrawTool::mousePressEvent(event,scene);
    switch ( c_drawShape ){
    case rectangle:
        qDebug() << "press rect";
        item = new GraphicsRectItem(QRect(1,1,1,1));
        break;
    case roundrect:
        item = new GraphicsRectItem(QRect(1,1,1,1),true);
        break;
    case ellipse:
        item = new GraphicsEllipseItem(QRect(1,1,1,1));
        break;
    }
    if ( item == 0) return;
    c_down+=QPoint(2,2);
    item->setPos(event->scenePos());
    scene->addItem(item);
    item->setSelected(true);

    selectMode = size;
    nDragHandle = RightBottom;

}

void RectTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    setCursor(scene,Qt::CrossCursor);

    selectTool.mouseMoveEvent(event,scene);
}

void RectTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    selectTool.mouseReleaseEvent(event,scene);

    if ( event->scenePos() == (c_down-QPoint(2,2))){

        if ( item != 0){
            item->setSelected(false);
            scene->removeItem(item);
            delete item ;
            item = 0;
        }
        qDebug()<<"RectTool removeItem:";
    }else if( item ){
        emit scene->itemAdded( item );

    }
    c_drawShape = selection;
}


PolygonTool::PolygonTool(DrawShape shape)
    :DrawTool(shape)
{
    item = nullptr;
    m_nPoints = 0;
}
void PolygonTool::simple_draw_polyline(DrawScene *scene,QVector<QPointF>& v)
{
    QVector<GraphicsBezier*>vPolyLines;
    GraphicsBezier* temp;
    for(int i=0;i<v.size()-1;++i) //create line items;
    {
        temp=new GraphicsBezier(false);
        vPolyLines.append(temp);
    }
    QPointF  c1,c2;
    for(int j=0;j<vPolyLines.size()-1;++j)
    {
        vPolyLines[j]->setPos(v[j]);
        scene->addItem(vPolyLines[j]);
        initialPositions =v[j];
        vPolyLines[j]->addPoint(v[j]);
        vPolyLines[j]->setSelected(true);
        c1=QPointF(qAbs(v[j+1].x()-v[j].x())/3.+v[j].x(),(v[j+1].y()-v[j].y())/3.+v[j].y());
        c2=QPointF(qAbs(v[j+1].x()-v[j].x())*2/3.+v[j].x(),(v[j+1].y()-v[j].y())*2/3.+v[j].y());
        vPolyLines[j]->addPoint(c1);
        vPolyLines[j]->addPoint(c2);
        vPolyLines[j]->addPoint(v[j+1]);
        vPolyLines[j]->endPoint(v[j+1]);
        selectMode = size ;
        nDragHandle = vPolyLines[j]->handleCount();
        setCursor(scene,Qt::CrossCursor);
        //        if ( vPolyLines[j] != nullptr ){
        //            if ( nDragHandle != Handle_None && selectMode == size ){
        //                item->control(nDragHandle,v[j+1]);
        //            }
        //        }
        vPolyLines[j]->updateCoordinate();
        emit scene->itemAdded( vPolyLines[j] );
        vPolyLines[j] = nullptr;
        selectMode = none;
        c_drawShape = selection;
    }
}
void PolygonTool::simple_draw_line(DrawScene *scene,QVector<QPointF>& v)
{
    QVector<GraphicsLineItem*>vLines;
    GraphicsLineItem* temp;
    for(int i=0;i<v.size()-1;++i) //create line items;
    {
        temp=new GraphicsLineItem(nullptr);
        vLines.append(temp);
    }
    for(int j=0;j<vLines.size()-1;++j)
    {
        vLines[j]->setPos(v[j]);
        scene->addItem(vLines[j]);
        initialPositions =v[j];
        vLines[j]->addPoint(v[j]);
        vLines[j]->setSelected(true);
        vLines[j]->addPoint(v[j+1]);
        vLines[j]->endPoint(v[j+1]);
        selectMode = size ;
        nDragHandle = vLines[j]->handleCount();
        setCursor(scene,Qt::CrossCursor);
        //        if ( vLines[j] != nullptr ){
        //            if ( nDragHandle != Handle_None && selectMode == size ){
        //                item->control(nDragHandle,v[j+1]);
        //            }
        //        }
        vLines[j]->updateCoordinate();
        emit scene->itemAdded( vLines[j] );
        vLines[j] = nullptr;
        selectMode = none;
        c_drawShape = selection;
    }
}

void PolygonTool::draw_line(DrawScene *scene)
{
    item = new GraphicsLineItem(0);
    if(item==nullptr) return;
    if((!scene->m_bezier_insert_start.isNull())&&(!scene->m_bezier_insert_end.isNull()))
    {
        item->setPos(scene->m_bezier_insert_start);
        scene->addItem(item);
        initialPositions = scene->m_bezier_insert_start;
        item->addPoint(scene->m_bezier_insert_start);
    }else
    {
        return;
    }
    item->setSelected(true);
    item->addPoint(scene->m_bezier_insert_end);
    item->endPoint(scene->m_bezier_insert_end);
    //     scene->m_scene_cur_point_list.append(scene->m_bezier_insert_end);
    selectMode = size ;
    nDragHandle = item->handleCount();
    setCursor(scene,Qt::CrossCursor);
    if ( item != 0 ){
        if ( nDragHandle != Handle_None && selectMode == size ){
            item->control(nDragHandle,scene->m_bezier_insert_end);
        }
    }
    item->updateCoordinate();
    emit scene->itemAdded( item );
    item = nullptr;
    selectMode = none;
    c_drawShape = selection;
}

void PolygonTool::draw_polyline(DrawScene *scene)
{
    item = new GraphicsBezier(false);
    if(item==nullptr) return;
    if((!scene->m_bezier_insert_start.isNull())&&(!scene->m_bezier_insert_end.isNull()))
    {
        item->setPos(scene->m_bezier_insert_start);
        scene->addItem(item);
        initialPositions = scene->m_bezier_insert_start;
        item->addPoint(scene->m_bezier_insert_start);
    }else
    {
        return;
    }
    item->setSelected(true);
    item->addPoint(scene->m_c1);
    item->addPoint(scene->m_c2);
    item->addPoint(scene->m_bezier_insert_end);
    item->endPoint(scene->m_bezier_insert_end);
    //     scene->m_scene_cur_point_list.append(scene->m_bezier_insert_end);
    selectMode = size ;
    nDragHandle = item->handleCount();
    setCursor(scene,Qt::CrossCursor);
    if ( item != 0 ){
        if ( nDragHandle != Handle_None && selectMode == size ){
            item->control(nDragHandle,scene->m_bezier_insert_end);
        }
    }
    item->updateCoordinate();
    emit scene->itemAdded( item );
    item = nullptr;
    selectMode = none;
    c_drawShape = selection;
}
void PolygonTool::draw_bezier(DrawScene *scene)
{
    item = new GraphicsBezier();
    if(item==nullptr)  return;
    if((!scene->m_bezier_insert_start.isNull())&&(!scene->m_bezier_insert_end.isNull()))
    {
        item->setPos(scene->m_bezier_insert_start);
        scene->addItem(item);
        initialPositions = scene->m_bezier_insert_start;
        item->addPoint(scene->m_bezier_insert_start);
    }else
    {
        return;
    }
    item->setSelected(true);
    item->addPoint(scene->m_c1);
    item->addPoint(scene->m_c2);
    item->addPoint(scene->m_bezier_insert_end);
    item->endPoint(scene->m_bezier_insert_end);
    //     scene->m_scene_cur_point_list.append(scene->m_bezier_insert_end);
    selectMode = size ;
    nDragHandle = item->handleCount();
    setCursor(scene,Qt::CrossCursor);
    if ( item != 0 ){
        if ( nDragHandle != Handle_None && selectMode == size ){
            item->control(nDragHandle,scene->m_bezier_insert_end);
        }
    }
    item->updateCoordinate();
    emit scene->itemAdded( item );
    item = nullptr;
    selectMode = none;
    c_drawShape = selection;
}

void PolygonTool::mousePressEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mousePressEvent(event,scene); //record c_down and c_last;

    if ( event->button() != Qt::LeftButton ||scene->isFill_item )
    {
        return;
    }
    if ( item == nullptr ) //create new item
    {
        if ( c_drawShape == polygon ){
            item = new GraphicsPolygonItem(nullptr);
        }else if (c_drawShape == bezier ){
            item = new GraphicsBezier();
        }else if ( c_drawShape == polyline ){
            item = new GraphicsBezier(false);
        }else if ( c_drawShape == line ){
            item = new GraphicsLineItem(nullptr);
        }
        if(scene->isServoOrigin) //默认为此选项，画第一个点的时候，从这个点开始
        {
            if(!scene->servo_origin.isNull())
            {
                item->setPos(scene->servo_origin);
                scene->addItem(item);
                initialPositions = scene->servo_origin;
                item->addPoint(scene->servo_origin);
                scene->isServoOrigin=false;
            }
        }
        else if(scene->isPreNode)
        {
            if(scene->m_scene_cur_point_list.size()>0) //判断上一个节点是否存在
            {
                if(scene->isServoOrigin) //它的优先级要高于上一个节点模式
                {
                    item->setPos(scene->servo_origin);
                    scene->addItem(item);
                    initialPositions = scene->servo_origin;
                    item->addPoint(scene->servo_origin);
                    scene->isServoOrigin=false;
                    scene->isPreNode=true;
                }
                else if(scene->isCurClick)
                {
                    //                    qDebug("[started from current click!]\n");
                    item->setPos(event->scenePos());
                    scene->addItem(item);
                    initialPositions = c_down;
                    item->addPoint(c_down);
                    scene->isCurClick=false;

                }else //start from pre node;
                {
                    item->setPos(scene->m_scene_cur_point_list.last());
                    scene->addItem(item);
                    initialPositions = scene->m_scene_cur_point_list.last();
                    item->addPoint(scene->m_scene_cur_point_list.last());
                }

            }else
                /* qDebug("[pre node is empty!")*/;
        }else   // started from current click!
        {
            item->setPos(event->scenePos());
            scene->addItem(item);
            initialPositions = c_down;
            item->addPoint(c_down);
            scene->isCurClick=false;
        }
        item->setSelected(true);
        m_nPoints++;

    }else if ( c_down == c_last ) //do nothing;
    {
    }
    //item->addPoint(c_down+QPoint(1,0));//why the point x +1?
    //  qDebug("c_down:x:%f,y:%f\n",c_down.x(),c_down.y());
    item->addPoint(c_down);
    m_nPoints++;
    //    qDebug("m_nPoints:%d",m_nPoints);
    selectMode = size ;
    nDragHandle = item->handleCount();
}
//void PolygonTool::mousePressEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
//{
//    DrawTool::mousePressEvent(event,scene); //record c_down and c_last;

//    if ( event->button() != Qt::LeftButton ||scene->isFill_item )
//    {
//        return;
//    }
//    if ( item == nullptr ) //create new item
//    {
//        if ( c_drawShape == polygon ){
//            item = new GraphicsPolygonItem(nullptr);
//        }else if (c_drawShape == bezier ){
//            item = new GraphicsBezier();
//        }else if ( c_drawShape == polyline ){
//            item = new GraphicsBezier(false);
//        }else if ( c_drawShape == line ){
//            item = new GraphicsLineItem(0);
//        }
//        if(scene->isServoOrigin) //默认为此选项，画第一个点的时候，从这个点开始
//        {
//            if(!scene->servo_origin.isNull())
//            {
//                item->setPos(scene->servo_origin);
//                scene->addItem(item);
//                initialPositions = scene->servo_origin;
//                item->addPoint(scene->servo_origin);
//                scene->isServoOrigin=false;
//            }
//        }
//        else if(scene->isPreNode)
//        {
//            if(!scene->m_scene_cur_point.isNull()) //判断上一个节点是否存在
//            {
//                if(scene->isServoOrigin) //它的优先级要高于上一个节点模式
//                {
//                    item->setPos(scene->servo_origin);
//                    scene->addItem(item);
//                    initialPositions = scene->servo_origin;
//                    item->addPoint(scene->servo_origin);
//                    scene->isServoOrigin=false;
//                    scene->isPreNode=true;
//                }
//                else if(scene->isCurClick)
//                {
////                    qDebug("[started from current click!]\n");
//                    item->setPos(event->scenePos());
//                    scene->addItem(item);
//                    initialPositions = c_down;
//                    item->addPoint(c_down);
//                    scene->isCurClick=false;

//                }else //start from pre node;
//                {
//                    item->setPos(scene->m_scene_cur_point);
//                    scene->addItem(item);
//                    initialPositions = scene->m_scene_cur_point;
//                    item->addPoint(scene->m_scene_cur_point);
//                }

//            }else
//               /* qDebug("[pre node is empty!")*/;
//        }else   // started from current click!
//        {
//            item->setPos(event->scenePos());
//            scene->addItem(item);
//            initialPositions = c_down;
//            item->addPoint(c_down);
//            scene->isCurClick=false;
//        }
//        item->setSelected(true);
//        m_nPoints++;

//    }else if ( c_down == c_last ) //do nothing;
//    {
//    }
//    //item->addPoint(c_down+QPoint(1,0));//why the point x +1?
//    //  qDebug("c_down:x:%f,y:%f\n",c_down.x(),c_down.y());
//    item->addPoint(c_down);
//    m_nPoints++;
////    qDebug("m_nPoints:%d",m_nPoints);
//    selectMode = size ;
//    nDragHandle = item->handleCount();
//}

void PolygonTool::mouseMoveEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mouseMoveEvent(event,scene);
    //  qDebug("[polygon mouse move x:%f,y:%f\n",event->scenePos().x(),event->scenePos().y());
    setCursor(scene,Qt::CrossCursor);
    if ( item != 0 ){
        if ( nDragHandle != Handle_None && selectMode == size ){
            //qDebug("[c_last x:%f,y:%f\n",c_last.x(),c_last.y());
            item->control(nDragHandle,c_last);
        }
    }
}

void PolygonTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mousePressEvent(event,scene);
    //    if ( c_drawShape == line ){
    //        item->endPoint(event->scenePos());
    //        item->updateCoordinate();
    //        emit scene->itemAdded( item );
    //        item = nullptr;
    //        selectMode = none;
    //        c_drawShape = selection;
    //        m_nPoints = 0;
    //    }
}

void PolygonTool::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event, DrawScene *scene)
{
    DrawTool::mouseDoubleClickEvent(event,scene);
    item->endPoint(event->scenePos());
    item->updateCoordinate();
    emit scene->itemAdded( item );
    item = nullptr;
    selectMode = none;
    c_drawShape = selection;
    m_nPoints = 0;
}
