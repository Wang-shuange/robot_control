#include "drawscene.h"
#include<QGraphicsSceneMouseEvent>
#include<QGraphicsRectItem>
#include <QDebug>
#include <QKeyEvent>
#include "drawobj.h"
#include <vector>
#include <QPainter>
#include<QMenu>
#include<drawview.h>
#include<QGraphicsScene>
#include "xmltool.h"
#include "servoinfo.h"
bool isFill_bezier=false;
extern const double ANGLE_MAX;
extern const double  SCENE_WIDTH;
extern const double  SCENE_HEIGHT;


GridTool::GridTool(const QSize & grid , const QSize & space )
    :m_sizeGrid(grid)
    ,m_sizeGridSpace(20,20){
    Q_UNUSED(space)
}
void GridTool::paintGrid(QPainter *painter, const QRect &rect)
{
    QPen p(Qt::blue);
    p.setStyle(Qt::DashLine);
    p.setWidthF(0.2);
    painter->setPen(p);
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing,true);
    painter->fillRect(rect,Qt::transparent);
    for (int x=rect.left() ;x <rect.right()  ;x+=(int)(m_sizeGridSpace.width())) {
        painter->drawLine(x,rect.top(),x,rect.bottom());

    }
    for (int y=rect.top();y<rect.bottom() ;y+=(int)(m_sizeGridSpace.height()))
    {
        painter->drawLine(rect.left(),y,rect.right(),y);
    }
    p.setStyle(Qt::SolidLine);
    p.setColor(Qt::green);
    painter->drawLine(rect.right(),rect.top(),rect.right(),rect.bottom());
    painter->drawLine(rect.left(),rect.bottom(),rect.right(),rect.bottom());

    //draw shadow
    QColor c1(Qt::white);
    painter->fillRect(QRect(rect.right()+1,rect.top()+2,2,rect.height()),c1.dark(200));
    painter->fillRect(QRect(rect.left()+2,rect.bottom()+2,rect.width(),2),c1.dark(200));
    painter->restore();
}

class BBoxSort
{
public:
    BBoxSort( QGraphicsItem * item , const QRectF & rect , AlignType alignType )
        :item_(item),box(rect),align(alignType)
    {
        //topLeft
        min_ = alignType == HORZEVEN_ALIGN ? box.topLeft().x() : box.topLeft().y();
        //bottomRight
        max_ = alignType == HORZEVEN_ALIGN ? box.bottomRight().x() : box.bottomRight().y();
        //width or height
        extent_ = alignType == HORZEVEN_ALIGN ? box.width() : box.height();
        anchor =  min_*0.5 + max_ * 0.5;
    }
    qreal min() { return min_;}
    qreal max() { return max_;}
    qreal extent() { return extent_;}
    QGraphicsItem * item_;
    qreal anchor;
    qreal min_;
    qreal max_;
    qreal extent_;
    QRectF box;
    AlignType align ;
};

bool operator< (const BBoxSort &a, const BBoxSort &b)
{
    return (a.anchor < b.anchor);
}

DrawScene::DrawScene(QObject *parent)
    :QGraphicsScene(parent)
{
    connect(this,SIGNAL(itemAdded(QGraphicsItem *)),this,SLOT(slt_itemAdded(QGraphicsItem *)));
    m_load_ag=new ActionGroup(this);
    m_save_ag=new ActionGroup(this);
    m_load_ma=nullptr;
    m_save_ma=nullptr;
    m_view = nullptr;
    m_dx=m_dy=0;
//    m_grid=nullptr;
//    m_grid = new GridTool();//open grid;
    QGraphicsItem * item = addRect(QRectF(0,0,0,0));
    this->servo_origin.setX(20.00);
    this->servo_origin.setY(200.0);
    item->setAcceptHoverEvents(true);
    createActions();
    //    horizontalSlider=new QxtSpanSlider(Qt::Horizontal);
    //    horizontalSlider->setHandleMovementMode(QxtSpanSlider::NoOverlapping);
    //    horizontalSlider->setMaximum(200);
    //    horizontalSlider->setLowerValue(10);
    //    horizontalSlider->setUpperValue(200);
    //    horizontalSlider->installEventFilter(this);
    //    connect(horizontalSlider,SIGNAL(lowerValueChanged(int)),this,SLOT(lowerValueChangedSlot(int)));
    //    connect(horizontalSlider,SIGNAL(upperValueChanged(int)),this,SLOT(upperValueChangedSlot(int)));
    //    QGraphicsProxyWidget *pProxy = this->addWidget(horizontalSlider);
    //    QRectF rc(0,0,SCENE_WIDTH,10);
    //    pProxy->setGeometry(rc);
    //    pProxy->setZValue(1);
    if(m_scene_cur_point_list.size()>0)
        m_scene_cur_point_list.clear();
    get_servo_info();

}

void DrawScene::get_servo_info()
{
    XmlTool tool;
    if(!m_servo_name_list.isEmpty())
        m_servo_name_list.clear();
    m_cur_servoinfo=nullptr;
    tool.read_servoInfo("motoSet.xml",m_l_servoinfo);
    for(int i=0;i<m_l_servoinfo.size();++i)
    {
        m_servo_name_list.append(m_l_servoinfo[i]->get_name());
    }
//    qDebug("****");
    //    emit sig_received_servo_info(m_l_servoinfo);
}

void DrawScene::lowerValueChangedSlot(int value)
{

    qDebug("lower :%d\n",value);
}

void DrawScene::upperValueChangedSlot(int value)
{
    qDebug("upper :%d\n",value);
}

DrawScene::~DrawScene()
{
//    if(m_grid)
//        delete m_grid;
}

void DrawScene::align(AlignType alignType)
{
    AbstractShape * firstItem = qgraphicsitem_cast<AbstractShape*>(selectedItems().first());
    if ( !firstItem ) return;
    QRectF rectref = firstItem->mapRectToScene(firstItem->boundingRect());
    int nLeft, nRight, nTop, nBottom;
    qreal width = firstItem->width();
    qreal height = firstItem->height();
    nLeft=nRight=rectref.center().x();
    nTop=nBottom=rectref.center().y();
    QPointF pt = rectref.center();
    if ( alignType == HORZEVEN_ALIGN || alignType == VERTEVEN_ALIGN ){
        std::vector< BBoxSort  > sorted;
        foreach (QGraphicsItem *item , selectedItems()) {
            QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
            if ( g )
                continue;
            sorted.push_back(BBoxSort(item,item->mapRectToScene(item->boundingRect()),alignType));
        }
        //sort bbox by anchors
        std::sort(sorted.begin(), sorted.end());
        unsigned int len = sorted.size();
        bool changed = false;
        //overall bboxes span
        float dist = (sorted.back().max()-sorted.front().min());
        //space eaten by bboxes
        float span = 0;
        for (unsigned int i = 0; i < len; i++)
        {
            span += sorted[i].extent();
        }
        //new distance between each bbox
        float step = (dist - span) / (len - 1);
        float pos = sorted.front().min();
        for ( std::vector<BBoxSort> ::iterator it (sorted.begin());
              it < sorted.end();
              ++it )
        {
            {
                QPointF t;
                if ( alignType == HORZEVEN_ALIGN )
                    t.setX( pos - it->min() );
                else
                    t.setY(pos - it->min());
                it->item_->moveBy(t.x(),t.y());
                emit itemMoved(it->item_,t);
                changed = true;
            }
            pos += it->extent();
            pos += step;
        }

        return;
    }

    int i = 0;
    foreach (QGraphicsItem *item , selectedItems()) {
        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
        if ( g )
            continue;
        QRectF rectItem = item->mapRectToScene( item->boundingRect() );
        QPointF ptNew = rectItem.center();
        switch ( alignType ){
        case UP_ALIGN:
            ptNew.setY(nTop + (rectItem.height()-rectref.height())/2);
            break;
        case HORZ_ALIGN:
            ptNew.setY(pt.y());
            break;
        case VERT_ALIGN:
            ptNew.setX(pt.x());
            break;
        case DOWN_ALIGN:
            ptNew.setY(nBottom-(rectItem.height()-rectref.height())/2);
            break;
        case LEFT_ALIGN:
            ptNew.setX(nLeft-(rectref.width()-rectItem.width())/2);
            break;
        case RIGHT_ALIGN:
            ptNew.setX(nRight+(rectref.width()-rectItem.width())/2);
            break;
        case CENTER_ALIGN:
            ptNew=pt;
            break;
        case ALL_ALIGN:
        {
            AbstractShape * aitem = qgraphicsitem_cast<AbstractShape*>(item);
            if ( aitem ){
                qreal fx = width / aitem->width();
                qreal fy = height / aitem->height();
                if ( fx == 1.0 && fy == 1.0 ) break;
                aitem->stretch(RightBottom,fx,fy,aitem->opposite(RightBottom));
                aitem->updateCoordinate();
                emit itemResize(aitem,RightBottom,QPointF(fx,fy));
            }
        }
            break;
        case WIDTH_ALIGN:
        {
            AbstractShape * aitem = qgraphicsitem_cast<AbstractShape*>(item);
            if ( aitem ){
                qreal fx = width / aitem->width();
                if ( fx == 1.0 ) break;
                aitem->stretch(Right,fx,1,aitem->opposite(Right));
                aitem->updateCoordinate();
                emit itemResize(aitem,Right,QPointF(fx,1));
            }
        }
            break;

        case HEIGHT_ALIGN:
        {
            AbstractShape * aitem = qgraphicsitem_cast<AbstractShape*>(item);
            if ( aitem ){

                qreal fy = height / aitem->height();
                if (fy == 1.0 ) break ;
                aitem->stretch(Bottom,1,fy,aitem->opposite(Bottom));
                aitem->updateCoordinate();
                emit itemResize(aitem,Bottom,QPointF(1,fy));
            }
        }
            break;
        }
        QPointF ptLast= rectItem.center();
        QPointF ptMove = ptNew - ptLast;
        if ( !ptMove.isNull()){
            item->moveBy(ptMove.x(),ptMove.y());
            emit itemMoved(item,ptMove);
        }
        i++;
    }
}

void DrawScene::mouseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    switch( mouseEvent->type() ){
    case QEvent::GraphicsSceneMousePress:
    {
        QGraphicsScene::mousePressEvent(mouseEvent);
    }
        break;
    case QEvent::GraphicsSceneMouseMove:
        QGraphicsScene::mouseMoveEvent(mouseEvent);
        break;
    case QEvent::GraphicsSceneMouseRelease:
        QGraphicsScene::mouseReleaseEvent(mouseEvent);
        break;
    default:
        break;
    }
}

GraphicsItemGroup *DrawScene::createGroup(const QList<QGraphicsItem *> &items,bool isAdd)
{
    // Build a list of the first item's ancestors
    QList<QGraphicsItem *> ancestors;
    int n = 0;
    //    QPointF pt = items.first()->pos();
    if (!items.isEmpty()) {
        QGraphicsItem *parent = items.at(n++);
        while ((parent = parent->parentItem()))
            ancestors.append(parent);
    }
    // Find the common ancestor for all items
    QGraphicsItem *commonAncestor = nullptr;
    if (!ancestors.isEmpty()) {
        while (n < items.size()) {
            int commonIndex = -1;
            QGraphicsItem *parent = items.at(n++);
            do {
                int index = ancestors.indexOf(parent, qMax(0, commonIndex));
                if (index != -1) {
                    commonIndex = index;
                    break;
                }
            } while ((parent = parent->parentItem()));

            if (commonIndex == -1) {
                commonAncestor = nullptr;
                break;
            }

            commonAncestor = ancestors.at(commonIndex);
        }
    }

    // Create a new group at that level
    GraphicsItemGroup *group = new GraphicsItemGroup(commonAncestor);
    if (!commonAncestor && isAdd )
        addItem(group);
    foreach (QGraphicsItem *item, items){
        item->setSelected(false);
        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
        if ( !g )
            group->addToGroup(item);
    }
    group->updateCoordinate();
    return group;
}

void DrawScene::destroyGroup(QGraphicsItemGroup *group)
{
    group->setSelected(false);
    foreach (QGraphicsItem *item, group->childItems()){
        item->setSelected(true);
        group->removeFromGroup(item);
    }
    removeItem(group);
    delete group;
}

void DrawScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter,rect);
//    painter->fillRect(sceneRect(),Qt::white);
//    if( m_grid ){
//        m_grid->paintGrid(painter,sceneRect().toRect());
//    }
}
void DrawScene::slt_itemAdded(QGraphicsItem * item )//this function can set the current action group's servoid to itself for continue usage;
{
    QString name=((GraphicsPolygonItem*)(item))->displayName();//get item name
    if(name=="bezier")
    {
        GraphicsBezier* bez=(GraphicsBezier*)item; //get bezier object
        bez->m_servoid= QString::number(this->m_cur_servoinfo->get_id());//set servoid for bezier item;
    }else if(name=="line")
    {
        GraphicsLineItem*lin=(GraphicsLineItem*)item;
        lin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());

    }else if(name=="polyline")
    {
        GraphicsBezier* polylin=(GraphicsBezier*)item;
        polylin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
    }
}
//void DrawScene::when_action_created_over(QGraphicsItem * item,MotorAction* ma )
//{
//    //process
//    //1.get item's name for checking which trace algrithm should be used;
//    //2.filled items to current action group;
//    //3.called trace algrithm to show the trace;
//    QString name=((GraphicsPolygonItem*)(item))->displayName();//get item name
//    if(name=="bezier")
//    {
//        GraphicsBezier* bez=(GraphicsBezier*)item; //get bezier object
//        qDebug()<<"bezier points:"<< bez->m_points.size();
//        bez->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
//        qDebug("servo id:%d",bez->m_servoid.toInt());

//        ma->m_items.append(bez);
//        ma->handle_trace();

//    }else if(name=="line")
//    {
//        GraphicsLineItem*lin=(GraphicsLineItem*)item;
//        qDebug()<<"line points:"<<lin->m_points.size();
//        lin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
//        qDebug("servo id:%d",lin->m_servoid.toInt());
//        ma->m_items.append(lin);
//        ma->handle_trace();

//    }else if(name=="polyline")
//    {
//        GraphicsBezier* polylin=(GraphicsBezier*)item;
//        qDebug()<<"poly line points:"<<polylin->m_points.size();
//        polylin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
//        qDebug("servo id:%d",polylin->m_servoid.toInt());
//        ma->m_items.append(polylin);
//        ma->handle_trace();
//    }
//}


void DrawScene::when_action_created_over(QGraphicsItem * item )
{ //handle all the items one by one until the loop is end;
    //process
    //1.get item's name for checking which trace algrithm should be used;
    //2.filled items to current action group;
    //3.called trace algrithm to show the trace;
    //    if(m_save_ma!=nullptr)//maybe it's a bug ,it can cause the items which belong to before will be cleared;
    //        m_save_ma->m_items.clear(); //first ,clear items ,ensure  no prrior items in it;

    QString name=((GraphicsPolygonItem*)(item))->displayName();//get item name
    if(name=="bezier")//sort by type and then handle it ;
    {
        GraphicsBezier* bez=(GraphicsBezier*)item; //get bezier object
        //        qDebug()<<"bezier points:"<< bez->m_points.size();
        bez->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
        //        qDebug("servo id:%d",bez->m_servoid.toInt());
        if(m_save_ma!=nullptr)
        {
            m_save_ma->m_items.append(bez);
            m_save_ma->handle_trace();
        }
    }else if(name=="line")
    {
        GraphicsLineItem*lin=(GraphicsLineItem*)item;
        qDebug()<<"line points:"<<lin->m_points.size();
        lin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
        qDebug("servo id:%d",lin->m_servoid.toInt());
        if(m_save_ma!=nullptr)
        {
            m_save_ma->m_items.append(lin);
            m_save_ma->handle_trace();
        }
    }else if(name=="polyline")
    {
        GraphicsBezier* polylin=(GraphicsBezier*)item;
        qDebug()<<"poly line points:"<<polylin->m_points.size();
        polylin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
        qDebug("servo id:%d",polylin->m_servoid.toInt());
        if(m_save_ma!=nullptr)
        {
            m_save_ma->m_items.append(polylin);
            m_save_ma->handle_trace();
        }
    }
}


//void DrawScene::slt_itemAdded(QGraphicsItem * item )
//{
//    qDebug()<<"type:"<< ((GraphicsPolygonItem*)(item))->displayName()<<endl;
//    QString name=((GraphicsPolygonItem*)(item))->displayName();

//    if(name=="bezier")
//    {
//        GraphicsBezier* bez=(GraphicsBezier*)item;
//        qDebug()<<"bezier points:"<< bez->m_points.size();
//        bez->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
//        qDebug("servo id:%d",bez->m_servoid.toInt());
//        if(m_ma!=nullptr)
//        {
//            m_ma->m_items.append(bez);
//            m_ma->handle_trace();
//        }
//    }else if(name=="line")
//    {
//        GraphicsLineItem*lin=(GraphicsLineItem*)item;
//        qDebug()<<"line points:"<<lin->m_points.size();
//        lin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
//        qDebug("servo id:%d",lin->m_servoid.toInt());
//        if(m_ma!=nullptr)
//            m_ma->m_items.append(lin);
//        m_ma->handle_trace();

//    }else if(name=="polyline")
//    {
//        GraphicsBezier* polylin=(GraphicsBezier*)item;
//        qDebug()<<"poly line points:"<<polylin->m_points.size();
//        polylin->m_servoid= QString::number(this->m_cur_servoinfo->get_id());
//        qDebug("servo id:%d",polylin->m_servoid.toInt());
//        if(m_ma!=nullptr)
//            m_ma->m_items.append(polylin);
//        m_ma->handle_trace();
//    }
//}

void DrawScene::fill_bezier()
{
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        ((PolygonTool*)tool)->draw_bezier(this);
}
void DrawScene::simple_create_line(QVector<QPointF>& v)
{
    DrawTool::c_drawShape=line;
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        ((PolygonTool*)tool)->simple_draw_line(this,v);
}
void DrawScene::simple_create_polyline(QVector<QPointF>& v)
{
    DrawTool::c_drawShape=polyline;
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        ((PolygonTool*)tool)->simple_draw_polyline(this,v);
}
void DrawScene::fill_line()
{
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        ((PolygonTool*)tool)->draw_line(this);
}
void DrawScene::fill_polyline()
{

    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        ((PolygonTool*)tool)->draw_polyline(this);
}
void DrawScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        tool->mousePressEvent(mouseEvent,this);
}

void DrawScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        tool->mouseMoveEvent(mouseEvent,this);
}

void DrawScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    // qDebug("[scene release :][x:%f,y:%f]\n",mouseEvent->scenePos().x(),mouseEvent->scenePos().y());
    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        tool->mouseReleaseEvent(mouseEvent,this);
}

void DrawScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvet)
{
    this->m_scene_cur_point_list.append(mouseEvet->scenePos());

    DrawTool * tool = DrawTool::findTool( DrawTool::c_drawShape );
    if ( tool )
        tool->mouseDoubleClickEvent(mouseEvet,this);

}

void DrawScene::keyPressEvent(QKeyEvent *e)
{
    qreal dx=0,dy=0;
    m_moved = false;
    switch( e->key())
    {
    case Qt::Key_Up:
        dx = 0;
        dy = -1;
        m_moved = true;
        break;
    case Qt::Key_Down:
        dx = 0;
        dy = 1;
        m_moved = true;
        break;
    case Qt::Key_Left:
        dx = -1;
        dy = 0;
        m_moved = true;
        break;
    case Qt::Key_Right:
        dx = 1;
        dy = 0;
        m_moved = true;
        break;
    }
    m_dx += dx;
    m_dy += dy;
    if ( m_moved )
        foreach (QGraphicsItem *item, selectedItems()) {
            item->moveBy(dx,dy);
        }
    QGraphicsScene::keyPressEvent(e);
}

void DrawScene::keyReleaseEvent(QKeyEvent *e)
{
    if (m_moved && selectedItems().count()>0)
        emit itemMoved(nullptr,QPointF(m_dx,m_dy));
    m_dx=m_dy=0;
    QGraphicsScene::keyReleaseEvent(e);
}

void DrawScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    //清除原有菜单
    pop_menu->clear();
    QMenu* audio_menu=new QMenu();  //audio control
    audio_menu->addAction(load_audio_action);
    audio_menu->addSeparator();
    audio_menu->addAction(play_total_audio_action);
    audio_menu->addSeparator();
    audio_menu->addAction(pause_audio_action);
    audio_action->setMenu(audio_menu);

    QMenu*action_menu=new QMenu(); //action create
    action_menu->addAction(action_type_action);

    m_va.clear();
    for(int i=0;i<m_servo_name_list.size();++i)
    {
        m_va.append(new QAction(m_servo_name_list[i],this));
    }
    QMenu*servo_name_menu=new QMenu();
    if(m_servo_name_list.isEmpty())
    {
        qDebug("no name in this list");
        return ;
    }
    for(int i=0;i<m_servo_name_list.size();++i)
    {
        servo_name_menu->addSeparator();
        servo_name_menu->addAction(m_va.at(i));
        connect(m_va.at(i), &QAction::triggered, this, &DrawScene::select_servo_type_action);
    }
    action_type_action->setMenu(servo_name_menu);
    action_menu->addSeparator();
    action_menu->addAction(simple_mode_action);
    action_menu->addSeparator();
    action_menu->addAction(finish_action);
    action_group_action->setMenu(action_menu);

    QMenu*based_menu=new QMenu();
    based_menu->addAction(based_line_action);
    based_menu->addSeparator();
    based_menu->addAction(based_polyline_action);
    simple_mode_action->setMenu(based_menu);


    QMenu* st_menu=new QMenu(); //select start point for draw;
    st_menu->addAction(mouse_press_action);
    st_menu->addSeparator();
    st_menu->addAction(mouse_double_action);
    st_menu->addSeparator();
    st_menu->addAction(servo_origin_action);
    start_action->setMenu(st_menu);
    QMenu*ed_menu=new QMenu(); //edit item menu;
    ed_menu->addAction(start_action);
    ed_menu->addAction(fill_item_action);
    ed_menu->addSeparator();
//    ed_menu->addAction(del_item_action);
    item_edit_action->setMenu(ed_menu);
    QMenu* item_menu=new QMenu();
    item_menu->addAction(line_action);
    item_menu->addSeparator();
    item_menu->addAction(poyline_action);
    item_menu->addSeparator();
    item_menu->addAction(bezier_action);
    fill_item_action->setMenu(item_menu);



    pop_menu->addAction(audio_action);
    pop_menu->addSeparator();
    pop_menu->addAction(action_group_action);
    pop_menu->addSeparator();
    pop_menu->addAction(item_edit_action);
    //菜单出现的位置为当前鼠标的位置
    pop_menu->exec(QCursor::pos());
    event->accept();
}
void DrawScene::start_action_create(MyPushButton* btn)
{
    this->isServoOrigin=true;
    this->isAction_type_selected=true;
    m_select_name.clear();
    m_select_name=btn->text();
    qDebug()<<"select name:"<<m_select_name<<endl; //根据名字获取对应参数
    XmlTool tool;
    m_cur_servoinfo=new ServoInfo(this);
    tool.read_servoInfo_by_name("motoSet.xml",m_select_name,m_cur_servoinfo); //fill the servo info to a sevoinfo object;
    if(m_cur_servoinfo!=nullptr)
    {
        this->servo_origin.setX(0.0);
        qreal y=SCENE_HEIGHT-this->m_cur_servoinfo->get_jdStart()*SCENE_HEIGHT/ANGLE_MAX;
        this->servo_origin.setY(y);
        isSelected_type=true;
        emit sig_selected_actiongroup_type(isSelected_type);
    }
}
void DrawScene::select_servo_type_action()
{
    //process//
    //1.set the start point to servo  origin;
    //2.set action group's status is selected;
    //3.clear selected name and filled with new selected name;
    //4.get the servo info via servo name and filled with servoinfo object;
    //5.set action group origin location;
    this->isServoOrigin=true;
    this->isAction_type_selected=true;
    m_select_name.clear();
    m_select_name=((QAction*)sender())->text();
    qDebug()<<"select name:"<<m_select_name<<endl; //根据名字获取对应参数
    XmlTool tool;
    m_cur_servoinfo=new ServoInfo(this);
    tool.read_servoInfo_by_name("motoSet.xml",m_select_name,m_cur_servoinfo); //fill the servo info to a sevoinfo object;
    if(m_cur_servoinfo!=nullptr)
    {
        this->servo_origin.setX(0.0);
        qreal y=SCENE_HEIGHT-this->m_cur_servoinfo->get_jdStart()*SCENE_HEIGHT/ANGLE_MAX;
        this->servo_origin.setY(y);
        isSelected_type=true;
        emit sig_selected_actiongroup_type(isSelected_type);
    }
}
void DrawScene::select_start_and_end_point()
{
    dlg=new QDialog();
    dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    dlg->resize(200,100);
    dlg->setWindowTitle(tr("select start and end point"));
    dlg->setModal(true);
    dlg->move(this->view()->rect().width()/2,this->view()->rect().height()/2);
    QGridLayout *layout=new QGridLayout();
    QLabel *l_start_x=new QLabel(tr("start x:"));
    le_start_x=new QLineEdit();
    le_start_x->setMaximumWidth(100);
    QLabel *l_start_y=new QLabel(tr("start y:"));
    le_start_y=new QLineEdit();
    le_start_y->setMaximumWidth(100);

    QLabel *l_end_x=new QLabel(tr("end x:"));
    le_end_x=new QLineEdit();
    le_end_x->setMaximumWidth(100);
    QLabel *l_end_y=new QLabel(tr("end y:"));
    le_end_y=new QLineEdit();
    le_end_y->setMaximumWidth(100);

    QLabel *l_equidistant=new QLabel(tr("Equidistant nums:"));
    le_equidistant=new QLineEdit();
    le_equidistant->setMaximumWidth(100);

    QPushButton *btn_ok=new QPushButton(tr("确定"));
    btn_ok->setMaximumHeight(40);
    btn_ok->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    connect(btn_ok,SIGNAL(clicked(bool)),this,SLOT(slot_inputPoint(bool)));
    layout->addWidget(l_start_x,0,0);
    layout->addWidget(le_start_x,0,1);
    layout->addWidget(l_start_y,1,0);
    layout->addWidget(le_start_y,1,1);

    layout->addWidget(l_end_x,2,0);
    layout->addWidget(le_end_x,2,1);
    layout->addWidget(l_end_y,3,0);
    layout->addWidget(le_end_y,3,1);
    layout->addWidget(l_equidistant,4,0);
    layout->addWidget(le_equidistant,4,1);
    layout->addWidget(btn_ok,5,0,5,2);
    dlg->setLayout(layout);
    dlg->show();
}

void DrawScene::slot_inputPoint(bool)
{
    if(le_equidistant->text().isEmpty()||le_start_x->text().isEmpty()||le_start_y->text().isEmpty()||le_end_x->text().isEmpty()||le_end_y->text().isEmpty())
    {
        dlg->setWindowTitle(tr("please input point!"));
    }else
    {
        m_simple_start.setX(this->le_start_x->text().toDouble());
        m_simple_start.setY(this->le_start_y->text().toDouble());
        m_simple_end.setX(this->le_end_x->text().toDouble());
        m_simple_end.setY(this->le_end_y->text().toDouble());
        m_equidistant_num=le_equidistant->text().toInt();
        QVector<QPointF>v;
        v.append(m_simple_start);
        v.append(m_simple_end);
        if(isBased_line)
        {
            isBased_line=!isBased_line;
            this->simple_mode_create_line(v,m_equidistant_num);
        }
        else if(isBased_polyline)
        {
            isBased_polyline=!isBased_polyline;
            this->simple_mode_create_polyline(v,m_equidistant_num);
        }
        dlg->close();
        if(dlg!=nullptr)
            delete dlg;
    }
}
//void DrawScene::slot_inputPoint(bool)
//{
//    if(le_equidistant->text().isEmpty()||le_start_x->text().isEmpty()||le_start_y->text().isEmpty()||le_end_x->text().isEmpty()||le_end_y->text().isEmpty())
//    {
//        dlg->setWindowTitle(tr("please input point!"));
//    }else
//    {
//        m_simple_start.setX(this->le_start_x->text().toDouble());
//        m_simple_start.setY(this->le_start_y->text().toDouble());
//        m_simple_end.setX(this->le_end_x->text().toDouble());
//        m_simple_end.setY(this->le_end_y->text().toDouble());
//        m_equidistant_num=le_equidistant->text().toInt();
//        QVector<QPointF>v;
//        v.append(m_simple_start);
//        v.append(m_simple_end);
//        this->simple_mode_create_line(v,m_equidistant_num);
//        dlg->close();
//        if(dlg!=nullptr)
//            delete dlg;
//    }

//}

//bool DrawScene::eventFilter(QObject *watched, QEvent *event)
//{
//    //    if (watched == this->horizontalSlider) {
//    //        if (event->type() == QEvent::MouseButtonPress) {
//    //            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
//    //            qDebug("pos:####x:%f,y:%f\n",mouseEvent->posF(),mouseEvent->posF().y());
//    //            return true;
//    //        } else {
//    //            return false;
//    //        }
//    //    } else if(watched==this->item)
//    //    {
//    //        qDebug("@@@@@@@@@@@@@@@@@@@");
//    //        if(event->type()==QEvent::Paint){
//    //            qDebug("paint@@@@@@@@@@@@@@@@@@@");
//    //        }
//    //    }
//    //    else {

//    //        // pass the event on to the parent class
//    //        return QGraphicsScene::eventFilter(watched, event);
//    //    }
//}

void DrawScene::createActions()
{
    //创建菜单、菜单项
    pop_menu = new QMenu();//top level menu
    audio_action=new QAction(QIcon(":/icons/audio_control.png"),tr("音频控制"),this);
    load_audio_action=new QAction(QIcon(":/icons/load_audio.png"),tr("加载音频"),this);
    play_total_audio_action=new QAction(QIcon(":/icons/play_all.png"),tr("播放全部"),this);
    pause_audio_action=new QAction(QIcon(":/icons/pause.png"),tr("暂停播放"),this);
    action_group_action=new QAction(QIcon(":/icons/actiongroup.png"),"动作创建",this);
    simple_mode_action=new QAction(QIcon(":/icons/simple_mode.png"),"极简模式",this);
    based_line_action=new QAction(QIcon(":/icons/line.png"),"基于直线",this);
    based_polyline_action=new QAction(QIcon(":/icons/polyline.png"),"基于多线段",this);
    action_type_action=new QAction(QIcon(":/icons/type_select.png"),"类型选择",this);
    finish_action =new QAction(QIcon(":/icons/finish.png"),tr("结束创建"),this);
    start_action=new QAction(QIcon(":/icons/start_action.png"),tr("起点选择"),this);
    item_edit_action=new QAction(QIcon(":/icons/item_edit.png"),tr("图元编辑"),this);
    mouse_press_action = new QAction(QIcon(":/icons/mouse_press.png"),tr("点击位置"),this);
    servo_origin_action = new QAction(QIcon(":/icons/default_origin.png"),tr("默认起始位置"),this);
    mouse_double_action = new QAction(QIcon(":/icons/pre_node.png"),tr("前一个节点"),this);
    fill_item_action =new QAction(QIcon(":/icons/fill_item.png"),tr("填充图元"),this);
    line_action=new QAction(QIcon(":/icons/line_.png"),tr("直线"),this);
    poyline_action=new QAction(QIcon(":/icons/polyline_.png"),tr("多线段"),this);
    bezier_action=new QAction(QIcon(":/icons/bezier_.png"),tr("贝塞尔曲线"),this);
//    del_item_action =new QAction(QIcon(":/icons/del_item.png"),tr("删除图元"),this);

    //连接信号与槽
    connect(based_line_action,&QAction::triggered,this,&DrawScene::simple_mode_action_func);
    connect(based_polyline_action,&QAction::triggered,this,&DrawScene::simple_mode_action_polyline);
    connect(mouse_press_action, &QAction::triggered, this, &DrawScene::get_mouse_press);
    connect(servo_origin_action, &QAction::triggered, this, &DrawScene::get_servo_origin);
    connect(mouse_double_action, &QAction::triggered, this, &DrawScene::get_mouse_double);
    connect(fill_item_action, &QAction::triggered, this, &DrawScene::edit_graphics);
    connect(line_action, &QAction::triggered, this, &DrawScene::set_line);
    connect(poyline_action, &QAction::triggered, this, &DrawScene::set_poyline);
    connect(bezier_action, &QAction::triggered, this, &DrawScene::set_bezier);
    connect(load_audio_action,&QAction::triggered,this,&DrawScene::load_audio);
    connect(play_total_audio_action,&QAction::triggered,this,&DrawScene::play_audio);
    connect(pause_audio_action,&QAction::triggered,this,&DrawScene::pause_audio);
    connect(action_type_action,&QAction::triggered,this,&DrawScene::select_action_group_type);
    connect(finish_action,&QAction::triggered,this,&DrawScene::finish_action_group);

}
void DrawScene::simple_mode_action_func()
{
    isBased_line=true;
    qDebug("simple mode for create action!");
    this->select_start_and_end_point();
}
void DrawScene::simple_mode_action_polyline()
{
    isBased_polyline=true;
    qDebug("simple mode for based polyline!");
    this->select_start_and_end_point();
}
//void DrawScene::finish_action_group()
//{
//    this->isServoOrigin=true;
//    isSelected_type=false;
//    emit sig_selected_actiongroup_type(isSelected_type);
//    DrawView* view=dynamic_cast<DrawView*>(this->view());
//    if(view!=nullptr)
//        view->m_parent_window->statusBar()->showMessage(tr("结束当前动作组创建！"));
//    m_ma=new MotorAction(this);
//    m_ma->setId(m_cur_servoinfo->get_id());
//    GraphicsPolygonItem* tmp=nullptr;
//    int cnt=items().count();
//    for(int i=0;i<cnt;++i)//get all items which belong to this action group;
//    {
//        tmp=qgraphicsitem_cast<GraphicsPolygonItem*>(this->items()[i]);
//        if(tmp!=nullptr)
//            if(tmp->m_servoid.toInt()==this->m_cur_servoinfo->get_id())
//                when_action_created_over(tmp);
//    }

//    m_ag->m_actions.append(m_ma);
//}
//void DrawScene::finish_action_group()
//{
//    this->isServoOrigin=true;
//    isSelected_type=false;
//    emit sig_selected_actiongroup_type(isSelected_type);
//    DrawView* view=dynamic_cast<DrawView*>(this->view());
//    if(view!=nullptr)
//        view->m_parent_window->statusBar()->showMessage(tr("结束当前动作组创建！"));
//    m_ma=new MotorAction(this);
//    m_ma->setId(m_cur_servoinfo->get_id());
//    GraphicsPolygonItem* tmp=nullptr;
//    int cnt=items().count();
//    for(int i=0;i<cnt;++i)//get all items which belong to this action group;
//    {
//        tmp=qgraphicsitem_cast<GraphicsPolygonItem*>(this->items()[i]);
//        if(tmp!=nullptr)
//            if(tmp->m_servoid.toInt()==this->m_cur_servoinfo->get_id())
//                when_action_created_over(tmp);
//    }
//    for(int j=0;j<m_ag->m_actions.count();++j)
//    {
//        if(m_ag->m_actions[j]->getId()==m_ma->getId())
//        {
//            m_ag->m_actions.removeAt(j);

//        }
//    }
//    m_ag->m_actions.append(m_ma);
//}
//void DrawScene::finish_action_group()
//{
//    isSelected_type=false;
//    isFinished_action=true;
//    emit sig_selected_actiongroup_type(isSelected_type);
//    DrawView* view=dynamic_cast<DrawView*>(this->view());
//    if(view!=nullptr)
//        view->m_parent_window->statusBar()->showMessage(tr("结束当前动作组创建！"));
//    else
//        return;
//    m_save_ma=new MotorAction(this);
//    m_save_ma->setId(m_cur_servoinfo->get_id());//selected servo id
//    GraphicsPolygonItem* tmp=nullptr;
//    int cnt=items().count();
//    m_save_ma->m_items.clear();
//    m_save_ma->m_trace.clear();
//    for(int i=0;i<cnt;++i)//get all items which belong to this action group;
//    {
//        tmp=qgraphicsitem_cast<GraphicsPolygonItem*>(this->items()[i]);
//        if(tmp!=nullptr)
//            if(tmp->m_servoid.toInt()==this->m_cur_servoinfo->get_id()) //if the item's id equal current selected servo id
//                when_action_created_over(tmp);
//    }
//     int sz=m_save_ag->m_actions.count();
//     for(int j=0;j<sz;++j)
//     {
//         if(m_save_ma->getId()==m_save_ag->m_actions[j]->getId())
//         {
//             m_save_ag->m_actions.removeAt(j);
//         }
//     }
//    m_save_ag->m_actions.append(m_save_ma);
//    view->m_parent_window->save();
//}
void DrawScene::finish_action_group()
{
    isSelected_type=false;
    isFinished_action=true;
    emit sig_selected_actiongroup_type(isSelected_type);
    DrawView* view=dynamic_cast<DrawView*>(this->view());
    if(view!=nullptr)
        view->m_parent_window->statusBar()->showMessage(tr("结束当前动作组创建！"));
    else
        return;
    m_save_ma=new MotorAction(this);
    m_save_ma->setId(m_cur_servoinfo->get_id());//selected servo id
    GraphicsPolygonItem* tmp=nullptr;
    int cnt=items().count();
    if(cnt<=1)
    {
        ((DrawView*)this->view())->m_parent_window->statusBar()->showMessage(tr("动作组为空!"),2000);
        return;
    }
    m_save_ma->m_items.clear();
    m_save_ma->m_trace.clear();
    for(int i=0;i<cnt;++i)//get all items which belong to this action group;
    {
        tmp=qgraphicsitem_cast<GraphicsPolygonItem*>(this->items()[i]);
        if(tmp!=nullptr)
            if(tmp->m_servoid.toInt()==this->m_cur_servoinfo->get_id()) //if the item's id equal current selected servo id
                when_action_created_over(tmp);
    }
    int sz=m_save_ag->m_actions.count();
    for(int x=0;x<sz;x++)
    {
        if(m_save_ma->getId()==m_save_ag->m_actions.at(x)->getId())
        {
            m_save_ag->m_actions.erase(m_save_ag->m_actions.begin()+x);//删除元素的另一种方法
//           m_save_ag->m_actions.remove(x); //删除元素后，size会减小，所以当再次遍历时，要将SZ减1；
            sz-=1;
           qDebug("size:%d",m_save_ag->m_actions.size());
        }
    }
    m_save_ag->m_actions.append(m_save_ma);
    view->m_parent_window->save();
}

//void DrawScene::over_action_create(MyPushButton* btn)
//{
//    isSelected_type=false;
//    isFinished_action=true;
//    emit sig_selected_actiongroup_type(isSelected_type);
//    DrawView* view=dynamic_cast<DrawView*>(this->view());
//    if(view!=nullptr)
//        view->m_parent_window->statusBar()->showMessage(tr("结束当前动作组创建！"));
//    else
//        return;
//    m_save_ma=new MotorAction(this);
//    m_save_ma->setId(m_cur_servoinfo->get_id());//selected servo id
//    GraphicsPolygonItem* tmp=nullptr;
//    int cnt=items().count();
//    m_save_ma->m_items.clear();
//    m_save_ma->m_trace.clear();
//    for(int i=0;i<cnt;++i)//get all items which belong to this action group;
//    {
//        tmp=qgraphicsitem_cast<GraphicsPolygonItem*>(this->items()[i]);
//        if(tmp!=nullptr)
//            if(tmp->m_servoid.toInt()==this->m_cur_servoinfo->get_id()) //if the item's id equal current selected servo id
//                when_action_created_over(tmp);
//    }
//    m_save_ag->m_actions.append(m_save_ma);
//}
//void DrawScene::finish_action_group()
//{
//    this->isServoOrigin=true;
//    isSelected_type=false;
//    emit sig_selected_actiongroup_type(isSelected_type);
//    qDebug("finish action create");
//    //    if (!activeMdiChild()) return ;
//    //    DrawScene * scene = dynamic_cast<DrawScene*>(activeMdiChild()->scene());

//    //QGraphicsItemGroup

//    QList<QGraphicsItem *> selectedItems = this->selectedItems();
//    // Create a new group at that level
//    if ( selectedItems.count() < 1) return;
//    GraphicsItemGroup *group = this->createGroup(selectedItems);
//    ////////////////

//    //   int name= (group->childItems().first())->type();
//    //   qDebug()<<"name:"<<name;
//    //    group->setEnabled(false);
//    //    group->hide();
//    ///////////////

//}
void DrawScene::select_action_group_type()
{
    for(int i=0;i<m_servo_name_list.size();++i)
    {
        qDebug()<<"name["<<i<<"]"<<m_servo_name_list[i]<<endl;
    }


}
void DrawScene::load_audio()
{
    emit sig_load_audio();
}
void DrawScene::play_audio()
{
    emit sig_play_audio();
}
void DrawScene::pause_audio()
{
    static int cnt=0;

    if(cnt%2!=0)
        this->pause_audio_action->setText(tr("暂停播放"));
    else
        this->pause_audio_action->setText(tr("继续播放"));
    cnt++;
    qDebug("pause audio");
    emit sig_pause_audio();
}
void DrawScene::set_line()
{
    qDebug("set type with line\n");
    QString info;
    info="请用鼠标依次选中前后两个图元";
    emit sig_fill_graphics(info);
    this->isLine=true;
}
void DrawScene::set_poyline()
{

    qDebug("set type with poyline\n");
    QString info;
    info="请用鼠标依次选中前后两个图元";
    emit sig_fill_graphics(info);
    this->isPolyLine=true;

}
void DrawScene::set_bezier()
{
    qDebug("set type with bezier\n");
    QString info;
    info="请用鼠标依次选中前后两个图元";
    emit sig_fill_graphics(info);
    this->isBezier=true;
}

void DrawScene::slt_received_from_bezier(bool isPaint)
{
    Q_UNUSED(isPaint);
}
void DrawScene::finish_create_action()
{
    qDebug("finished action creation\n");
}
void DrawScene::add_item_with_fill(QVector<QPointF> v)
{
    if(v.isEmpty())
    {
        qDebug("no start and end point for use!\n");
        return;
    }
    qDebug(":-[start:x:%f,y:%f,end:x:%f,end:x:%f]\n",v.at(0).x(),v.at(0).y(),v.at(1).x(),v.at(1).y());

    if(isLine)
    {
        isLine=false;
        this->m_bezier_insert_start=v[0]; //v[0] start point
        this->m_bezier_insert_end=v[1];  // v[1] end point;
        fill_line();
    }
    if(isBezier)
    {
        isBezier=false;
        this->m_bezier_insert_start=v[0]; //v[0] start point
        this->m_bezier_insert_end=v[1];  // v[1] end point;
        QPointF c1(qAbs(v[1].x()-v[0].x())/3.+v[0].x(),(v[1].y()-v[0].y())/3.+v[0].y());
        QPointF c2(qAbs(v[1].x()-v[0].x())*2/3.+v[0].x(),(v[1].y()-v[0].y())*2/3.+v[0].y());
        this->m_c1=c1;
        this->m_c2=c2;
        fill_bezier();

    }
    if(isPolyLine)
    {
        isPolyLine=false;
        this->m_bezier_insert_start=v[0]; //v[0] start point
        this->m_bezier_insert_end=v[1];  // v[1] end point;
        QPointF c1(qAbs(v[1].x()-v[0].x())/3.+v[0].x(),(v[1].y()-v[0].y())/3.+v[0].y());
        QPointF c2(qAbs(v[1].x()-v[0].x())*2/3.+v[0].x(),(v[1].y()-v[0].y())*2/3.+v[0].y());
        this->m_c1=c1;
        this->m_c2=c2;
        fill_polyline();

    }
}
void DrawScene::simple_mode_create_polyline(QVector<QPointF>& v,int equidistant_num)
{
    if(v.isEmpty())
    {
        qDebug("no start and end point for use!\n");
        return;
    }
    qDebug(":-[start:x:%f,y:%f,end:x:%f,end:x:%f,equidistant num:%d]\n",v.at(0).x(),v.at(0).y(),v.at(1).x(),v.at(1).y(),equidistant_num);
    QVector<QPointF> v_equ_points;
    QPointF tp;
    v_equ_points.append(v[0]);
    for(int i=1;i<equidistant_num;++i)//create equidistant points for draw line
    {
        tp =QPointF(qAbs(v[1].x()-v[0].x())*i/equidistant_num+v[0].x(),(v[1].y()-v[0].y())*i/equidistant_num+v[0].y());
        v_equ_points.append(tp);
    }
    v_equ_points.append(v[1]);
    simple_create_polyline(v_equ_points);
}
void DrawScene::simple_mode_create_line(QVector<QPointF>& v,int equidistant_num)
{
    if(v.isEmpty())
    {
        qDebug("no start and end point for use!\n");
        return;
    }
    qDebug(":-[start:x:%f,y:%f,end:x:%f,end:x:%f,equidistant num:%d]\n",v.at(0).x(),v.at(0).y(),v.at(1).x(),v.at(1).y(),equidistant_num);
    //    this->m_simple_start=v[0]; //v[0] start point
    //    this->m_simple_end=v[1];  // v[1] end point;
    QVector<QPointF> v_equ_points;
    QPointF tp;
    v_equ_points.append(v[0]);
    for(int i=1;i<equidistant_num;++i)//create equidistant points for draw line
    {
        tp =QPointF(qAbs(v[1].x()-v[0].x())*i/equidistant_num+v[0].x(),(v[1].y()-v[0].y())*i/equidistant_num+v[0].y());
        v_equ_points.append(tp);
    }
    v_equ_points.append(v[1]);
    simple_create_line(v_equ_points);
}
void DrawScene::slt_graphics_item_selected(QString info)
{
    qDebug()<<"[info:]"<<info<<endl;
    graphics_item_received_info=info;
    qDebug()<<"graphics item:"<<graphics_item_received_info<<endl;
}

void DrawScene::edit_graphics()
{
    QString info;
    info="请用鼠标依次选中前后两个图元";
    emit sig_fill_graphics(info);

}
void DrawScene::get_mouse_press()
{
    qDebug("get mouse_press");
    isCurClick=true;
}
void DrawScene::get_servo_origin()
{
    qDebug("get servo_origin");
    isServoOrigin=true;
}
void DrawScene::get_mouse_double()
{
    qDebug("get mouse_double");
    isPreNode=true;
}


