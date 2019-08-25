#include "mainwindow.h"
#include <QtWidgets>
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include <QDockWidget>
#include <QGraphicsItem>
#include <QMdiSubWindow>
#include <QUndoStack>
#include "customproperty.h"
#include "drawobj.h"
#include "commands.h"
#include<QSerialPort>
#include<QSerialPortInfo>
#include "xmltool.h"
//#define __TEST_TIME__

double  TIME_MAX=20.; //时间的最大刻度值
const double SCENE_WIDTH=1920.;
const double SCENE_HEIGHT=1080.;
const double SERVO_DETAIL=4096.0;
const int TIME_SPAN_MS=20;
#define u8 unsigned char
#define u16 unsigned short
int  div_time=20;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->item=nullptr;
    main_save_ag=nullptr;
    main_load_ag=nullptr;
    this->addUndoStackDock();
    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setCentralWidget(mdiArea);
    this->setUnifiedTitleAndToolBarOnMac(true);
    connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(updateMenus()));
    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));
    prepare_audio();
    createActions();
    createMenus();
    createToolbars();
    createToolBox();
    createPropertyEditor();
    newFile();
    mdiArea->setWindowFlags(mdiArea->windowFlags() ^ Qt::FramelessWindowHint);
    mdiArea->tileSubWindows();
    mdiArea->setBackground(QBrush(Qt::gray));
    connect(QApplication::clipboard(),SIGNAL(dataChanged()),this,SLOT(dataChanged()));
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(updateActions()));
    m_timer.start(50);
    m_timer.setTimerType(Qt::PreciseTimer);
    theControlledObject = nullptr;
    memset(Info_st,0,MAX_ACTION_COUNT);
    memset(Info_ed,0,MAX_ACTION_COUNT);
    total_st=total_ed=0;
    Info_sz=0;
    this->m_curr_dur=20*1000*1000; //default is us;
    m_serialPort=new QSerialPort();

    //    m_check_action_status=new QTimer(this);
    //    connect(m_check_action_status,SIGNAL(timeout()),this,SLOT(slt_check_action_status()));
    //    m_check_action_status->start(50);
    //    m_check_action_status->setTimerType(Qt::PreciseTimer);
}
void MainWindow::slt_check_action_status(bool flag)
{
    if(flag)
    {
        for(int i=0;i<MAX_ACTION_COUNT;++i)
        {
            btn_actions[i]->setFlat(true);
            btn_actions[i]->setStyleSheet("QPushButton{background-color:rgba(12,96,db,0); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                          "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
            btn_actions[i]->setEnabled(false);
            btn_hides[i]->setEnabled(false);
            btn_locks[i]->setEnabled(false);
            btn_stretchs[i]->setEnabled(false);
            btn_hide_others[i]->setEnabled(false);
            ckb_actions[i]->setEnabled(false);
        }
    }else
    {
        for(int i=0;i<MAX_ACTION_COUNT;++i)
        {
            btn_actions[i]->setFlat(true);
            btn_actions[i]->setStyleSheet("QPushButton{background-color:rgba(12,96,db,0); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                          "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
            btn_actions[i]->setEnabled(true);
            btn_hides[i]->setEnabled(true);
            btn_locks[i]->setEnabled(true);
            btn_stretchs[i]->setEnabled(true);
            btn_hide_others[i]->setEnabled(true);
            ckb_actions[i]->setEnabled(true);
        }
    }

}
void MainWindow::slt_check_item_selected()
{
    if (!activeMdiChild()) return ;
    DrawScene* scene=this->get_scene();
    int sz=scene->items().count();
    QString name;
    GraphicsItem* item=nullptr;
    for(int i=0;i<sz;++i)
    {

        item=dynamic_cast<GraphicsItem*>(scene->items().at(i));
        if(item!=nullptr)
        {
            name=get_servo_name_by_id(item->m_servoid.toInt());
            if(item->isSelected())
            {
                for(auto btn:this->btn_actions) //check which btn will be setted;
                {
                    if( btn->text()==name)
                    {
                        btn->setEnabled(true);
                        btn->setStyleSheet("QPushButton{background-color:orange; color: white; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                           "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
                    }
                }
            }else
            {
                for(auto btn:this->btn_actions)
                {
                    if( name==btn->text())
                    {
                        btn->setEnabled(true);
                        btn->setStyleSheet("QPushButton{background-color:green; color: white; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                           "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
                    }
                }
            }
        }
    }
}
void MainWindow::prepare_audio()
{
    play=new MyPlay();
    connect(this,SIGNAL(sig_play(char*)),play,SLOT(slt_play(char*)));
    connect(this,SIGNAL(sig_load(char*,QString)),play,SLOT(slt_load(char*,QString)));
    qRegisterMetaType<QVector<qint16>>("QVector<qint16>");//遇到无法识别的信号类别，需要提前给系统注册后才能使用；
    connect(play,SIGNAL(sig_wave_vector(QVector<qint16>)),this,SLOT(slt_get_wave_vector(QVector<qint16>)));
    connect(play,SIGNAL(sig_duration(quint64)),this,SLOT(slt_duration(quint64)));
    connect(this,SIGNAL(sig_play_all(QString)),play,SLOT(slt_play_all(QString)));


}
void MainWindow::slt_duration(quint64 dur)
{
    this->m_curr_dur=dur;
}
void MainWindow::slt_get_wave_vector(QVector<qint16> val)
{
    this->writeData_vector(val);
}

qint64 MainWindow::writeData_vector(QVector<qint16>& v)
{
    m_chart = new MyChart();
    m_series = new QLineSeries;
    m_chart->addSeries(m_series);
    m_series->connect(m_series,&QLineSeries::clicked,[](const QPointF& point){
        qDebug() << "point clicked:" << point;
    });
    m_series->connect(m_series,&QLineSeries::pressed,[](const QPointF& point){
        qDebug() << "point pressed:" << point;
    });
    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0,v.count());
    axisX->setLabelFormat("%g");
    axisX->setMinorTickCount(10);
    axisX->hide();
    //    axisX->setTitleText("Samples");
    QValueAxis *axisY = new QValueAxis;
    axisY->setRange(-1.0, 1.0);
    axisY->setTitleText("Audio level");
    axisY->hide();
    axisY->setGridLineColor(Qt::red);
    m_chart->setAxisX(axisX, m_series);
    m_chart->setAxisY(axisY, m_series);
    m_chart->legend()->hide();
    m_chart->setTheme(QChart::ChartThemeBlueIcy);
    this->activeMdiChild()->scene()->addItem(m_chart);
    int resolution=1000;
    QVector<QPointF>points =m_series->pointsVector();
    points.clear();
    for(int i=0;i<v.size();++i)
    {
        if(i%resolution==0)
        {
            points.append(QPointF(static_cast<qreal>(i), static_cast<qreal>(v[i]/32768.0)));
        }
    }
    //    qDebug("[v.size:%d]",v.size());
    //    qDebug("[points.size:%d]",points.size());
    m_series->replace(points);
    m_series->setColor(Qt::green);
    m_series->setOpacity(0.5);
    m_chart->setEnabled(false);
    m_chart->setZValue(-5);
    DrawView* view=this->activeMdiChild();
    connect(this,SIGNAL(sig_send_duration(quint64)),this->activeMdiChild(),SLOT(slt_receive_duration(quint64)));
    qDebug()<<"###dur:"<<m_curr_dur<<endl;
    view->m_hruler->setRange(0,m_curr_dur/1000000,qAbs(m_curr_dur/1000000-0) );
    m_chart->setGeometry(-30,0, SCENE_WIDTH+100, SCENE_HEIGHT/5);
    emit sig_send_duration(m_curr_dur);//info the drawview to redraw the ruler;
    return 0;
}

void MainWindow::drawPalyHeader_ex(qreal pos)
{
    if (!activeMdiChild())
    {
        statusBar()->showMessage("[please create new scene file first!]",2000);
        return ;
    }
    DrawScene* sc=(DrawScene*)this->activeMdiChild()->scene();
    static quint64 cnt=0;
    QPen pen(Qt::red);
    pen.setWidth(1);
    QGraphicsItem* le=sc->addLine(pos,-1000,pos,1000,pen);
    lines.append(le);
    cnt++;
    if(cnt>=2)
        sc->removeItem(lines[cnt-2]);
}


void MainWindow::addUndoStackDock()
{
    undoStack = new QUndoStack();
    undoView = new QUndoView(undoStack);
    undoView->setAttribute(Qt::WA_QuitOnClose, false);
    QDockWidget *dock = new QDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setWidget(undoView);
}
void MainWindow::slt_fill_graphics(QString info)
{
    m_graphics_item_info=info;
    statusBar()->showMessage(info);
    m_isFill=true;
}

void MainWindow::getInfo()
{
}
//int 转 2字节 BYTE[],
void MainWindow::intToByte(qint16 i,char* out)
{
    out[0]=static_cast<char>(0xff&i);
    out[1]=static_cast<char>((0xff00&i)>>8);
}
//2字节 BYTE[] 转 int
qint16 MainWindow::bytesToInt(char* in)
{
    qint16 addr = in[1] & 0xFF;
    addr |= ((in[0] << 8) & 0xFF00);
    return addr;
}
void MainWindow::show_play_pos()
{
    static quint64 pos=0;
    quint64 spec=qCeil((qreal)(m_curr_dur/1000./20.));
    //    qDebug("[dur:%d\n]",spec);
    pos+=SCENE_WIDTH/spec;
    if(pos>=SCENE_WIDTH/2)
        m_pos->stop();
    drawPalyHeader_ex(pos);
}
DrawScene* MainWindow::get_scene()
{
    DrawScene*tmp=dynamic_cast<DrawScene*>(this->activeMdiChild()->scene());
    return tmp;
}

//void MainWindow::sendCommand()  //增加只播放选中动作组功能；
//{
//#ifdef __TEST_TIME__
//    QString current_time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz ");
//    qDebug()<<"[c_t]:"<<current_time<<endl;
//#endif
//    if(Info_sz>0) //the action nums
//    {
//        static int command_count=0;
//        quint8 servo_ids[Info_sz];//all the action servo id in the scene in action group;
//        quint16 angles[Info_sz];  //all angles
//        quint16 speeds[Info_sz];
//        memset(servo_ids,0,Info_sz);
//        memset(angles,0,Info_sz);
//        memset(speeds,0,Info_sz);
//        int st,ed;
//        for(int i=0;i<Info_sz;++i)
//        {
//            double s=m_selected_action[i]->m_trace.at(0).x();//得出起始点位置
//            double e=m_selected_action[i]->m_trace[m_selected_action[i]->m_trace.count()-1].x();//得出结束点位置
//            if(m_curr_dur!=0) //获取x轴刻度尺长度；
//            {
//                double x_tmp=m_curr_dur/1000./20;//转成毫秒
//                st=static_cast<int>(qCeil(s*x_tmp/SCENE_WIDTH));
//                ed=static_cast<int>(qCeil(e*x_tmp/SCENE_WIDTH));

//            }else
//            {
//                st=static_cast<int>(ceil(s*TIME_MAX/SCENE_WIDTH));
//                ed=static_cast<int>(ceil(e*TIME_MAX/SCENE_WIDTH));
//            }

//            Info_st[i]=st=0;
//            Info_ed[i]=ed;
//        }
//        int min_start_location=Info_st[0];//假设第一个动作组开始位置为最小，或者最大，这样方便比较；
//        int max_end_location=Info_ed[0];
//        for(int i=1;i<Info_sz;++i)
//        {
//            if (Info_st[i] < min_start_location)
//                min_start_location = Info_st[i];
//            if (Info_ed[i] > max_end_location)    // 最大值
//                max_end_location= Info_ed[i];
//        }
//        total_st=min_start_location;
//        total_ed=max_end_location;
//        qDebug()<<"total_st: "<<total_st;
//        qDebug()<<"total_ed: "<<total_ed;
//        int idx=command_count+total_st;
//        MotorAction* ma=nullptr;
//        for(int m=0;m<Info_sz;++m)
//        {
//            ma=m_selected_action[m];
//            if(ma==nullptr) return;
//            servo_ids[m]=m_selected_action[m]->getId();
//            if(idx<(m_selected_action[m]->m_trace.count()))
//            {
//                angles[m]=static_cast<quint16>((ANGLE_MAX-ma->m_trace.at(idx).y()*ma->getConversion_coefficient_angle())*11.4);
//                speeds[m]=ma->getSpeed()[idx];//speed

//            }else
//            {
//                angles[m]=static_cast<quint16>((ANGLE_MAX-ma->m_trace[ma->m_trace.count()-1].y()*ma->getConversion_coefficient_angle())*11.4);
//                speeds[m]=ma->getSpeed()[ma->getSpeed().count()-1];//speed
//            }
//            //            qDebug("angle[%d],[%d]=[%d]",m,idx,angles[m]);
//        }
//        command_count++;//命令发送的次数；
//        if(idx>=total_st&&idx<=total_ed)
//        {
//            for(int i=0;i<Info_sz;++i)//action nums
//            {
//                qDebug("@@id[%d]:%d,angle[%d]:%d,speed[%d]:%d\n ",i,servo_ids[i],i,angles[i],i,speeds[i]);
//            }
//            servo_handle(servo_ids,angles, speeds,Info_sz);
//            qDebug()<<"[send times]:"<<command_count<<endl;
//        }
//        if(idx>=total_ed)
//        {

//            total_st=total_ed=0;
//            memset(servo_ids,0,Info_sz);
//            memset(angles,0,Info_sz);
//            memset(speeds,0,Info_sz);
//            command_count=0;
//            qDebug()<<"[#stop]"<<endl;
//            m_t->stop();
//            btn_play->setEnabled(true);

//            if(m_t)
//                delete m_t;
//        }
//    }
//}

void MainWindow::sendCommand()
{
#ifdef __TEST_TIME__
    QString current_time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz ");
    qDebug()<<"[c_t]:"<<current_time<<endl;
#endif
    if(Info_sz>0) //the action nums
    {
        static int command_count=0;
        quint8 servo_ids[Info_sz];//all the action servo id in the scene in action group;
        quint16 angles[Info_sz];  //all angles
        quint16 speeds[Info_sz];
        memset(servo_ids,0,Info_sz);
        memset(angles,0,Info_sz);
        memset(speeds,0,Info_sz);
        int total=get_scene()->m_save_ag->m_actions.count();
        int st,ed;
        for(int i=0;i<total;++i)
        {
            double s=get_scene()->m_save_ag->m_actions[i]->m_trace.at(0).x();//得出起始点位置
            double e=get_scene()->m_save_ag->m_actions[i]->m_trace[get_scene()->m_save_ag->m_actions[i]->m_trace.count()-1].x();//得出结束点位置
            if(m_curr_dur!=0) //获取x轴刻度尺长度；
            {
                double x_tmp=m_curr_dur/1000./20;//转成毫秒
                st=static_cast<int>(qCeil(s*x_tmp/SCENE_WIDTH));
                ed=static_cast<int>(qCeil(e*x_tmp/SCENE_WIDTH));

            }else
            {
                st=static_cast<int>(ceil(s*TIME_MAX/SCENE_WIDTH));
                ed=static_cast<int>(ceil(e*TIME_MAX/SCENE_WIDTH));
            }

            Info_st[i]=st=0;
            Info_ed[i]=ed;
        }
        int min_start_location=Info_st[0];//假设第一个动作组开始位置为最小，或者最大，这样方便比较；
        int max_end_location=Info_ed[0];
        for(int i=1;i<Info_sz;++i)
        {
            if (Info_st[i] < min_start_location)
                min_start_location = Info_st[i];
            if (Info_ed[i] > max_end_location)    // 最大值
                max_end_location= Info_ed[i];
        }
        total_st=min_start_location;
        total_ed=max_end_location;
        //        qDebug()<<"total_st: "<<total_st;
        //        qDebug()<<"total_ed: "<<total_ed;
        int idx=command_count+total_st;
        MotorAction* ma=nullptr;
        for(int m=0;m<Info_sz;++m)
        {
            ma=get_scene()->m_save_ag->m_actions[m];
            if(ma==nullptr) return;
            servo_ids[m]=get_scene()->m_save_ag->m_actions[m]->getId();
            if(idx<(get_scene()->m_save_ag->m_actions[m]->m_trace.count()))
            {
                angles[m]=static_cast<quint16>((ANGLE_MAX-ma->m_trace.at(idx).y()*ma->getConversion_coefficient_angle())*11.4);
                speeds[m]=ma->getSpeed()[idx];//speed

            }else
            {
                angles[m]=static_cast<quint16>((ANGLE_MAX-ma->m_trace[ma->m_trace.count()-1].y()*ma->getConversion_coefficient_angle())*11.4);
                speeds[m]=ma->getSpeed()[ma->getSpeed().count()-1];//speed
            }
            //            qDebug("angle[%d],[%d]=[%d]",m,idx,angles[m]);
        }
        command_count++;//命令发送的次数；
        if(idx>=total_st&&idx<=total_ed)
        {
            //            for(int i=0;i<Info_sz;++i)//action nums
            //            {
            //                qDebug("#id[%d]:%d,angle[%d]:%d,speed[%d]:%d\n ",i,servo_ids[i],i,angles[i],i,speeds[i]);
            //            }
            servo_handle(servo_ids,angles, speeds,Info_sz);
            //            qDebug()<<"[send times]:"<<command_count<<endl;
        }
        if(idx>=total_ed)
        {

            total_st=total_ed=0;
            memset(servo_ids,0,Info_sz); //这些参数其实是单次发送命令时所对应的参数；
            memset(angles,0,Info_sz);
            memset(speeds,0,Info_sz);
            command_count=0;
            qDebug()<<"[#stop]"<<endl;
            m_t->stop();
            btn_play->setEnabled(true);

            if(m_t)
                delete m_t;
        }
    }
}

MainWindow::~MainWindow()
{
    if(m_t)
        delete m_t;
    if(m_serialPort->isOpen())
    {
        m_serialPort->clear();
        m_serialPort->close();
        delete m_serialPort;

    }
    if(this->combo_port)
        delete combo_port;

}

void MainWindow::item_control_hide(MyPushButton* btn_action,MyPushButton* btn_hide,bool& flag)
{
    auto servo_id=0;
    GraphicsItem* tmp=nullptr;
    for(auto servo:m_list_servoinfo)
    {
        if(btn_action->text()==servo->get_name())
        {
            servo_id=servo->get_id();
            break;
        }
    }
    int sz=get_scene()->items().count();
    if(flag)
    {

        flag=false;

        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                if(tmp->m_servoid.toInt()==servo_id)
                {
                    qDebug()<<tmp->displayName()<<endl;
                    tmp->hide();
                    btn_hide->setIcon(QIcon(":/icons/hide.png"));

                }
            }
        }
    }else
    {
        flag=true;
        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                if(tmp->m_servoid.toInt()==servo_id)
                {
                    tmp->show();
                    btn_hide->setIcon(QIcon(":/icons/show.png"));

                }

            }
        }
    }
}
//void MainWindow::item_control_hide(MyPushButton* btn_action,MyPushButton* btn_hide,bool& flag)
//{
//    auto servo_id=0;
//    GraphicsItem* tmp=nullptr;
//    for(auto servo:m_list_servoinfo)
//    {
//        if(btn_action->text()==servo->get_name())
//        {
//            servo_id=servo->get_id();
//            break;
//        }
//    }
//    int sz=get_scene()->items().count();
//    if(flag)
//    {

//        flag=false;

//        for(int i=0;i<sz;++i)
//        {
//            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
//            if(tmp!=nullptr)
//            {
//                if(tmp->m_servoid.toInt()==servo_id)
//                {
//                    qDebug()<<tmp->displayName()<<endl;
//                    tmp->hide();
//                    btn_hide->setIcon(QIcon(":/icons/hide.png"));

//                }
//            }
//        }
//    }else
//    {
//        flag=true;
//        for(int i=0;i<sz;++i)
//        {
//            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
//            if(tmp!=nullptr)
//            {
//                if(tmp->m_servoid.toInt()==servo_id)
//                {
//                    tmp->show();
//                    btn_hide->setIcon(QIcon(":/icons/show.png"));

//                }

//            }
//        }
//    }
//}
void MainWindow::actions_control_stretch(MyPushButton* btn_action,bool& flag)
{ 
    auto servo_id=0;
    GraphicsItem* tmp=nullptr;
    for(auto servo:m_list_servoinfo)
    {
        if(btn_action->text()==servo->get_name())
        {
            servo_id=servo->get_id();
            break;
        }
    }
    int sz=get_scene()->items().count();
    if(sz==0) return;
    if(flag)
    {
        flag=false;
        get_scene()->start_action_create(btn_action);
        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                if(tmp->m_servoid.toInt()==servo_id)
                {
                    tmp->setSelected(true);

                }
            }
        }
        on_group_triggered();
    }else
    {
        flag=true;
        on_unGroup_triggered();
    }

}
void MainWindow::actions_control_hide_others(MyPushButton* btn_action,MyPushButton* btn_hideothers,bool& flag)
{
    //1>get clicked button's name which means the selected action and mapped the action id;
    //2>get servo ids which need to be hide;

    QList<int> servo_ids;
    servo_ids.clear();
    auto servo_id_selected=0;

    for(auto servo:m_list_servoinfo)
    {
        if(btn_action->text()==servo->get_name())
        {
            servo_id_selected=servo->get_id();
            break;
        }
    }
    QVector<MotorAction*> v_ma=get_scene()->m_save_ag->m_actions;
    for(int i=0;i<v_ma.count();++i)
    {
        if(v_ma[i]->getId()!=servo_id_selected)
            servo_ids.append( v_ma[i]->getId());
    }


    GraphicsItem* tmp=nullptr;
    int sz=get_scene()->items().count();
    if(flag)
    {
        flag=false;

        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                for(int j=0;j<servo_ids.count();++j)
                {
                    if(tmp->m_servoid.toInt()==servo_ids[j])
                    {
                        tmp->hide();
                        btn_hideothers->setIcon(QIcon(":/icons/hide_other.png"));

                    }
                }

            }
        }
    }else
    {
        flag=true;
        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                for(int j=0;j<servo_ids.count();++j)
                {
                    if(tmp->m_servoid.toInt()==servo_ids[j])
                    {
                        tmp->show();
                        btn_hideothers->setIcon(QIcon(":/icons/show_other.png"));

                    }
                }

            }
        }
    }

}
void MainWindow::actions_control_select(MyPushButton* btn_action,bool& flag)
{
    auto servo_id=0;
    GraphicsItem* tmp=nullptr;
    for(auto servo:m_list_servoinfo)
    {
        if(btn_action->text()==servo->get_name())
        {
            servo_id=servo->get_id();
            break;
        }
    }
    int sz=get_scene()->items().count();
    if(flag)
    {
        flag=false;
        get_scene()->start_action_create(btn_action);

//     DrawView* view=this->activeMdiChild();
//     QRect abc=view->viewport()->geometry();
//     double dx = abc.width()/ get_scene()->width();
//     double dy = abc.height()/ get_scene()->height();
//     view->setTransform( QTransform());
//     view->translate(1000,100);
//     view->setSceneRect(1000,SCENE_HEIGHT,SCENE_WIDTH+1000,SCENE_HEIGHT);


//      view->scale(2.,2.);
//      view->updateRuler();

//        btn_action->setStyleSheet("QPushButton{background-color:purple; color: white; font: bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}");

                for(int i=0;i<sz;++i)
                {
                    tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
                    if(tmp!=nullptr)
                    {
                        if(tmp->m_servoid.toInt()==servo_id)
                        {
                            tmp->setSelected(true);

                        }
                    }
                }
    }else
    {
        flag=true;
        get_scene()->finish_action_group();
//        btn_action->setStyleSheet("QPushButton{background-color:blue; color: white; font: bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}");

        //        for(int i=0;i<sz;++i)
        //        {
        //            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
        //            if(tmp!=nullptr)
        //            {
        //                if(tmp->m_servoid.toInt()==servo_id)
        //                {
        //                    tmp->setSelected(false);

        //                }

        //            }
        //        }
    }

}
void MainWindow::item_control_lock(MyPushButton* btn_action,MyPushButton* btn_lock,bool& flag)
{
    auto servo_id=0;
    GraphicsItem* tmp=nullptr;
    for(auto servo:m_list_servoinfo)
    {
        if(btn_action->text()==servo->get_name())
        {
            servo_id=servo->get_id();
            break;
        }
    }
    int sz=get_scene()->items().count();
    if(flag)
    {
        flag=false;
        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                if(tmp->m_servoid.toInt()==servo_id)
                {
                    qDebug()<<tmp->displayName()<<endl;
                    tmp->setEnabled(false);
                    btn_lock->setIcon(QIcon(":/icons/lock.png"));

                }
            }
        }
    }else
    {
        flag=true;
        for(int i=0;i<sz;++i)
        {
            tmp=dynamic_cast<GraphicsItem*>(get_scene()->items()[i]);
            if(tmp!=nullptr)
            {
                if(tmp->m_servoid.toInt()==servo_id)
                {
                    tmp->setEnabled(true);
                    btn_lock->setIcon(QIcon(":/icons/unlock.png"));

                }

            }
        }
    }

}
void MainWindow::slt_btn_hides_clicked(bool b)
{
    static bool flag=true;
    Q_UNUSED(b);
    MyPushButton *t=(MyPushButton*)(sender());
    for(int i=0;i<MAX_ACTION_COUNT;++i)
    {
        if(t->index==i)
            item_control_hide(btn_actions[i],btn_hides[i],flag);

    }
}
void MainWindow::slt_btn_locks_clicked(bool b)
{
    static bool flag=true;
    Q_UNUSED(b);
    MyPushButton *t=(MyPushButton*)(sender());
    for(int i=0;i<MAX_ACTION_COUNT;++i)
    {
        if(t->index==i)
        {
            item_control_lock(btn_actions[i],btn_locks[i],flag);
            break;
        }
    }
}
void MainWindow::slt_btn_actions_clicked(bool b)
{
    static bool flag=true;
    Q_UNUSED(b);
    MyPushButton *t=(MyPushButton*)(sender());
    for(int i=0;i<MAX_ACTION_COUNT;++i)
    {
        if(t->index==i)
        {
            actions_control_select(btn_actions[i],flag);
            break;
        }
    }
}
void MainWindow::slt_btn_stretchs_clicked(bool b)
{
    static bool flag=true;
    Q_UNUSED(b);
    MyPushButton *t=(MyPushButton*)(sender());
    for(int i=0;i<MAX_ACTION_COUNT;++i)
    {
        if(t->index==i)
        {
            actions_control_stretch(btn_actions[i],flag);
            break;
        }
    }
}
void MainWindow::slt_btn_hide_others_clicked(bool b)
{
    static bool flag=true;
    Q_UNUSED(b);
    MyPushButton *t=(MyPushButton*)(sender());
    for(int i=0;i<MAX_ACTION_COUNT;++i)
    {
        if(t->index==i)
        {
            actions_control_hide_others(btn_actions[i],btn_hide_others[i],flag);
            break;
        }
    }
}
void MainWindow::create_action_list()
{
    QGroupBox* gbox=new QGroupBox();
    get_servo_info_main();
    QGridLayout*layout = new QGridLayout();
    for(int i=0;i<m_list_servoinfo.count();++i)
    {
        ckb_actions[i]=new MyCheckBox(i,this);
        ckb_actions[i]->setStyleSheet("QCheckBox{ spacing: 5px;}"

                                      "QCheckBox::indicator{   \
                                      width: 15px;   \
                height: 15px;   \
    } "  \
    "QCheckBox::indicator:unchecked{  \
image:url(:/icons/unchecked.png); \
} " \

"QCheckBox::indicator:checked{ \
image:url(:/icons/checked.png); \
} "\

"QCheckBox::indicator:checked:disabled{ \
image:url(:/icons/disabled.png);\
} ");

ckb_actions[i]->setChecked(true);
ckb_actions[i]->setToolTip(tr("选中动作组"));
ckb_actions[i]->setToolTipDuration(2000);
connect(ckb_actions[i],SIGNAL(stateChanged(int)),this,SLOT(slt_ckb_action_stateChanged(int)));

btn_stretchs[i]=new MyPushButton(i,this);
btn_stretchs[i]->setIcon(QIcon(":/icons/stretching.png"));
btn_stretchs[i]->setToolTip(tr("缩放图形"));
btn_stretchs[i]->setToolTipDuration(2000);
connect(btn_stretchs[i],SIGNAL(clicked(bool)),this,SLOT(slt_btn_stretchs_clicked(bool)));

btn_hide_others[i]=new MyPushButton(i,this);
btn_hide_others[i]->setIcon(QIcon(":/icons/show_other.png"));
btn_hide_others[i]->setToolTip(tr("隐藏/显示其它动作组"));
btn_hide_others[i]->setToolTipDuration(2000);
connect(btn_hide_others[i],SIGNAL(clicked(bool)),this,SLOT(slt_btn_hide_others_clicked(bool)));

btn_hides[i]=new MyPushButton(i,this);
btn_hides[i]->setIcon(QIcon(":/icons/show.png"));
btn_hides[i]->setToolTip(tr("隐藏/显示图形"));
btn_hides[i]->setToolTipDuration(2000);
connect(btn_hides[i],SIGNAL(clicked(bool)),this,SLOT(slt_btn_hides_clicked(bool)));
btn_locks[i]=new MyPushButton(i,this);
btn_locks[i]->setIcon(QIcon(":/icons/unlock.png"));
btn_locks[i]->setToolTip(tr("锁定/解锁图形"));
btn_locks[i]->setToolTipDuration(2000);
btn_actions[i]=new MyPushButton(i,this);
btn_actions[i]->setText(m_list_servoinfo[i]->get_name());
connect(btn_actions[i],SIGNAL(clicked(bool)),this,SLOT(slt_btn_actions_clicked(bool)));
connect(btn_locks[i],SIGNAL(clicked(bool)),this,SLOT(slt_btn_locks_clicked(bool)));
btn_hides[i]->setMaximumWidth(20);
ckb_actions[i]->setMaximumWidth(20);
btn_locks[i]->setMaximumWidth(20);
btn_stretchs[i]->setMaximumWidth(20);
btn_hide_others[i]->setMaximumWidth(20);
btn_actions[i]->setMaximumHeight(20);

btn_actions[i]->setToolTip(tr("动作组控制"));
btn_actions[i]->setToolTipDuration(2000);
btn_actions[i]->setStyleSheet("QPushButton{background-color:rgba(12,96,db,1); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                              "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
btn_hide_others[i]->setFlat(true);
btn_actions[i]->setFlat(true);
btn_hides[i]->setFlat(true);
btn_locks[i]->setFlat(true);
btn_stretchs[i]->setFlat(true);
btn_hide_others[i]->setEnabled(false);
btn_actions[i]->setEnabled(false);
btn_hides[i]->setEnabled(false);
btn_locks[i]->setEnabled(false);
btn_stretchs[i]->setEnabled(false);
btn_actions[i]->setMinimumWidth(100);
layout->addWidget(btn_hides[i],i,2);
layout->addWidget(btn_locks[i],i,3);
layout->addWidget(btn_stretchs[i],i,4);
layout->addWidget(btn_hide_others[i],i,5);
layout->addWidget(btn_actions[i],i,1);
layout->addWidget(ckb_actions[i],i,0);
}
gbox->setLayout(layout);
gbox->setFlat(true);
QDockWidget *dock = new QDockWidget(tr("动作组清单"));
addDockWidget(Qt::LeftDockWidgetArea, dock);
dock->setMaximumHeight(650);
//            dock->setMinimumHeight(300);
dock->setWidget(gbox);
}
void MainWindow::slt_ckb_action_stateChanged(int index)
{
        MyCheckBox *t=(MyCheckBox*)(sender());
    for(int i=0;i<MAX_ACTION_COUNT;++i)
    {
        if(t->index==i)
        {
                qDebug()<<"status:"<<i<<t->isChecked()<<endl;
//            item_control_lock(btn_actions[i],btn_locks[i],flag);
            break;
        }
    }



}
void MainWindow::createToolBox()
{
    this->create_action_list();
    QGroupBox *groupBox = new QGroupBox();
    QLabel * label_port=new QLabel(tr("Port Name:"));

    label_port->setStyleSheet("QLabel{ color: grap;}" "QLabel:hover{ color: red; }" );
    label_port->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    QStringList m_serialPortName;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        m_serialPortName << info.portName();
        //        qDebug()<<"serialPortName:"<<info.portName();
    }
    combo_port=new MyComboBox();
    connect(combo_port,SIGNAL(clicked()),this,SLOT(slot_loadCom()));
    combo_port->addItems(m_serialPortName);
    combo_port->setStyleSheet("QComboBox {  border: 1px solid gray;  border-radius: 3px; padding: 1px 18px 1px 3px;min-width: 6em;  }"
                              "QComboBox:!editable, QComboBox::drop-down:editable {   \
                              background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,   \
                                                          stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,   \
                                                          stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);  \
}"

"QComboBox:on {  \
padding-top: 3px;  \
padding-left: 4px;  \
}"

"QComboBox::drop-down {   \
subcontrol-origin: padding;  \
subcontrol-position: top right;   \
width: 15px;   \
border-left-width: 1px;   \
border-left-color: darkgray;   \
border-left-style: solid;   \
border-top-right-radius: 3px;    \
border-bottom-right-radius: 3px;  \
}"

"QComboBox::down-arrow {  \
image: url(:/icons/dropdown.png);  \
} "   \
"QComboBox::down-arrow:on {    \
top: 1px;   \
left: 1px;  \
}  ");

combo_port->setMaximumWidth(20);
connect(combo_port,SIGNAL(currentIndexChanged(QString)),this,SLOT(slot_getPort(QString)));
QLabel* label_baudrate=new QLabel(tr("Baud Rate:"));
label_baudrate->setStyleSheet("QLabel{ color: grap;}" "QLabel:hover{ color: red; }" );
label_baudrate->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
MyComboBox*com_baudrate=new MyComboBox();

com_baudrate->setStyleSheet("QComboBox {  border: 1px solid gray;  border-radius: 3px; padding: 1px 18px 1px 3px;min-width: 6em;  }"
                            "QComboBox:!editable, QComboBox::drop-down:editable {   \
                            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,   \
                                                        stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,   \
                                                        stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);  \
}"

"QComboBox:on {  \
padding-top: 3px;  \
padding-left: 4px;  \
}"

"QComboBox::drop-down {   \
subcontrol-origin: padding;  \
subcontrol-position: top right;   \
width: 15px;   \
border-left-width: 1px;   \
border-left-color: darkgray;   \
border-left-style: solid;   \
border-top-right-radius: 3px;    \
border-bottom-right-radius: 3px;  \
}"

"QComboBox::down-arrow {  \
image: url(:/icons/dropdown.png);  \
} "   \
"QComboBox::down-arrow:on {    \
top: 1px;   \
left: 1px;  \
}  ");


QStringList baudrate;
baudrate<<"115200"<<"9600"<<"57600";
com_baudrate->addItems(baudrate);
com_baudrate->setMaximumWidth(20);

QPushButton* btn_conn=new QPushButton(QIcon(":/icons/com_conn.png"),tr("connect"));
btn_conn->setStyleSheet("QPushButton{background-color:orange; color: white;font:italic bold 16px/16px Arial;   border-radius: 10px;  border: 2px groove gray; border-style: outset;}"

                        "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
//        btn_conn->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
btn_conn->setMinimumHeight(30);
connect(btn_conn,SIGNAL(clicked(bool)),this,SLOT(slot_com_connect()));
QPushButton* btn_disconnect=new QPushButton(QIcon(":/icons/com_disconnect.png"),tr("disconn"));
btn_disconnect->setStyleSheet("QPushButton{background-color:orange; color: white; font:italic bold 16px/16px Arial;    border-radius: 10px;  border: 2px groove gray; border-style: outset;}"

                              "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
//        btn_disconnect->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
btn_disconnect->setMinimumHeight(30);
connect(btn_disconnect,SIGNAL(clicked(bool)),this,SLOT(slot_com_disconnect()));


btn_play=new QPushButton(QIcon(":/icons/play_white.png"),tr("play"));

btn_play->setStyleSheet("QPushButton{background-color:purple; color: white; font:italic bold 16px/16px Arial;    border-radius: 10px;  border: 2px groove gray; border-style: outset;}"

                        "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
btn_play->setMinimumHeight(30);
btn_play->setMinimumWidth(120);
connect(btn_play,SIGNAL(clicked()),this,SLOT(slot_play()));
btn_stop=new QPushButton(QIcon(":/icons/stop_white.png"),tr("stop"));
btn_stop->setStyleSheet("QPushButton{background-color:purple; color: white; font:italic bold 16px/16px Arial;    border-radius: 10px;  border: 2px groove gray; border-style: outset;}"

                        "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
btn_stop->setMinimumHeight(30);
btn_stop->setMinimumWidth(120);
connect(btn_stop,SIGNAL(clicked()),this,SLOT(slot_stop()));


QGridLayout*layout = new QGridLayout();
layout->addWidget(label_port,0,0);
layout->addWidget(combo_port,0,1);
layout->addWidget(label_baudrate,1,0);
layout->addWidget(com_baudrate,1,1);
layout->addWidget(btn_conn,2,0);
layout->addWidget(btn_disconnect,2,1);
layout->addWidget(btn_play,3,0);
layout->addWidget(btn_stop,3,1);

groupBox->setLayout(layout);
groupBox->setFlat(true);
QDockWidget *dock1 = new QDockWidget(tr("操作面板"));
addDockWidget(Qt::RightDockWidgetArea, dock1);
dock1->setMaximumHeight(200);
dock1->setMinimumHeight(200);
dock1->setWidget(groupBox);
}

void MainWindow::get_servo_info_main()
{
    XmlTool *tool=new XmlTool(this);
    m_list_servoinfo.clear();
    tool->read_servoInfo("motoSet.xml",m_list_servoinfo);
    for(int i=0;i<m_list_servoinfo.size();++i)
    {
        qDebug()<<"name:"<< m_list_servoinfo[i]->get_name()<<"id:"<<m_list_servoinfo[i]->get_id()<<endl;
    }
}
void MainWindow::slot_loadCom()
{
    int cnt=combo_port->count();
    combo_port->clear();
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        bool isExist=false;
        for(int i=0;i<cnt;++i)
        {
            QString combo_text=combo_port->itemText(i);
            if(combo_text!=info.portName()) //如果combo中没有这个串口，则添加；
                isExist=false;
            else
                isExist=true;
        }
        if(!isExist)
            combo_port->addItem(info.portName());
    }
}
void MainWindow::setLineEditValue(int value)
{
    int pos = slider->value();
    QString str = QString("%1").arg(pos);
    lineEdit->setText(str);
    div_time=pos;
}
void MainWindow::servo_handle(quint8*id,quint16*angle,quint16* speed,int len)
{
    quint8 id_temp[MAX_ACTION_COUNT];
    quint16 al_temp[MAX_ACTION_COUNT];
    quint16 sp_temp[MAX_ACTION_COUNT];
    memset(id_temp,0,MAX_ACTION_COUNT);
    memset(al_temp,0,MAX_ACTION_COUNT);
    memset(sp_temp,0,MAX_ACTION_COUNT);
    quint8 *pId=id_temp;
    quint16 *pAl,*pSp;
    pAl=al_temp;
    pSp=sp_temp;
    int len_temp=0;
    for(int s=0;s<len;++s)
    {
        //        qDebug("id[%d]:%d,angle[%d]:%d,speed[%d]:%d\n ",s,id[s],s,angle[s],s,speed[s]);
        //        if(id[s]==0&&angle[s]==0)
        //        {
        //            ;
        //            //            ;qDebug()<<"###param is invalid!"<<endl;
        //        }else
        if(id[s]!=0)
        {
            len_temp++;
            *pId++=id[s];
            *pAl++=angle[s];
            *pSp++=speed[s];
        }
    }
    //    qDebug()<<"len_temp:"<<len_temp<<endl;
    //  for(int x=0;x<len_temp;++x)
    //        qDebug("id[%d]:%d,al[%d]:%d,sp[%d]:%d\n",x,id_temp[x],x,al_temp[x],x,sp_temp[x]);

    if(len_temp>0)
    {
        QByteArray buf;
        int data_len=len_temp*7+4;
        buf[0]=0xff;
        buf[1]=0xff;// 命令头，两个FF
        buf[2]=0xfe;//广播
        buf[3]=data_len;//數據長度
        buf[4]=0x83;//功能碼
        buf[5]=0x2a;//首地址
        buf[6]=0x06;
        for(quint8 i=0 ;i<len_temp;++i) //write parameter
        {
            buf[7*(i+1)]=id_temp[i]; //servo id
            char o[2];

            intToByte(al_temp[i],o);
            buf[8*(i+1)-1*i]=o[0]; //angle low
            buf[9*(i+1)-2*i]=o[1]; //angle high

            buf[10*(i+1)-3*i]=0x00;//time low
            buf[11*(i+1)-4*i]=0x00;//time high
            char s[2];

            if(sp_temp[i]==0)
            {
                buf[12*(i+1)-5*i]=0x01;//speed lo 1
                buf[13*(i+1)-6*i]=0x00;//speed high 0
            }else
            {
                intToByte(sp_temp[i],s); //speed
                buf[12*(i+1)-5*i]=s[0];//speed lo
                buf[13*(i+1)-6*i]=s[1];//speed high
            }
        }
        quint8 sum=0;
        for(int k=2;k<=(6+len_temp*7);++k)
            sum+=buf[k];
        sum= ~sum;
        buf[buf.size()]=sum;//checksum 放在最后位置；
        m_serialPort->write(buf,buf.size());

        QString str1 = buf.toHex().toUpper();
        QString str;
        for(int i = 0;i<str1.length ();i+=2)//填加空格
        {
            QString str2 = str1.mid(i,2);
            str += str2;
            str += " ";
        }
        qDebug()<<str<<endl;
    }
}
//void MainWindow::servo_handle(quint8*id,quint16*angle,quint16* speed,int len)
//{
//    quint8 id_temp[12];
//    quint16 al_temp[12];
//    quint16 sp_temp[12];
//    memset(id_temp,0,12);
//    memset(al_temp,0,12);
//    memset(sp_temp,0,12);
//    quint8 *pId=id_temp;
//    quint16 *pAl,*pSp;
//    pAl=al_temp;
//    pSp=sp_temp;
//    int len_temp=0;
//    for(int s=0;s<len;++s)
//    {
//        //        qDebug("id[%d]:%d,angle[%d]:%d,speed[%d]:%d\n ",s,id[s],s,angle[s],s,speed[s]);
//        //        if(id[s]==0&&angle[s]==0)
//        //        {
//        //            ;
//        //            //            ;qDebug()<<"###param is invalid!"<<endl;
//        //        }else
//        if(id[s]!=0)
//        {
//            len_temp++;
//            *pId++=id[s];
//            *pAl++=angle[s];
//            *pSp++=speed[s];
//        }
//    }
//    //    qDebug()<<"len_temp:"<<len_temp<<endl;
//    //  for(int x=0;x<len_temp;++x)
//    //        qDebug("id[%d]:%d,al[%d]:%d,sp[%d]:%d\n",x,id_temp[x],x,al_temp[x],x,sp_temp[x]);

//    if(len_temp>0)
//    {
//        QByteArray buf;
//        int data_len=len_temp*7+4;
//        buf[0]=0xff;
//        buf[1]=0xff;// 命令头，两个FF
//        buf[2]=0xfe;//广播
//        buf[3]=data_len;//數據長度
//        buf[4]=0x83;//功能碼
//        buf[5]=0x2a;//首地址
//        buf[6]=0x06;
//        for(quint8 i=0 ;i<len_temp;++i) //write parameter
//        {
//            buf[7*(i+1)]=id_temp[i]; //servo id
//            char o[2];

//            intToByte(al_temp[i],o);
//            buf[8*(i+1)-1*i]=o[0]; //angle low
//            buf[9*(i+1)-2*i]=o[1]; //angle high

//            buf[10*(i+1)-3*i]=0x00;//time low
//            buf[11*(i+1)-4*i]=0x00;//time high
//            char s[2];

//            if(sp_temp[i]==0)
//            {
//                buf[12*(i+1)-5*i]=0x01;//speed lo 1
//                buf[13*(i+1)-6*i]=0x00;//speed high 0
//            }else
//            {
//                intToByte(sp_temp[i],s); //speed
//                buf[12*(i+1)-5*i]=s[0];//speed lo
//                buf[13*(i+1)-6*i]=s[1];//speed high
//            }
//        }
//        quint8 sum=0;
//        for(int k=2;k<=(6+len_temp*7);++k)
//            sum+=buf[k];
//        sum= ~sum;
//        buf[buf.size()]=sum;//checksum 放在最后位置；
//        m_serialPort->write(buf,buf.size());

//        QString str1 = buf.toHex().toUpper();
//        QString str;
//        for(int i = 0;i<str1.length ();i+=2)//填加空格
//        {
//            QString str2 = str1.mid(i,2);
//            str += str2;
//            str += " ";
//        }
//        qDebug()<<str<<endl;
//    }
//}


quint8 MainWindow::CheckSum(quint8 *buff,int Size)
{
    unsigned long cksum = 0;
    //将数据以字为单位累加到cksum中
    while (Size > 1)
    {
        cksum += *buff;
        Size -= sizeof(quint8);
    }
    //如果为奇数，将最后一个字节扩展到双字，在累加到cksum中
    if (Size)
    {
        cksum += *(quint8*)buff;
    }
    //将cksum的高16位和低16位相加，取反后得到效验和
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (quint8)(~cksum);//位运算符号：位取反!比如有int a=3;则先把十进制数3,转化为二进制数，即00000011.那么~a=11111100
}

//void MainWindow::slot_play()
//{
//    if(!m_serialPort->isOpen())
//    {
//        statusBar()->showMessage(tr(":-,亲，请先打开串口，然后再播放,谢谢!"), 2000);
//        return ;

//    }
//    QList<QMdiSubWindow*> sw=mdiArea->subWindowList();
//    int  sz=sw.size();
//    if(sz==0)
//    {
//        Info_sz=0;
//    }
//    if(Info_sz==0)
//    {
//        qDebug()<<":- 亲，请先加载场景文件，然后再播放,谢谢!"<<endl;
//        statusBar()->showMessage(tr(":-,亲，请先加载场景文件，然后再播放,谢谢!"), 2000);
//        return;
//    }
//    btn_play->setEnabled(false);
//    m_t=new QTimer(this);
//    m_t->setTimerType(Qt::PreciseTimer);
//    connect(m_t,SIGNAL(timeout()),this,SLOT(sendCommand()));
//    m_t->start(20);
//    qDebug("[play]");
//}

void  MainWindow::get_selected_actions(QVector<MotorAction*>& willsends)
{
    //1.get total actions which has been created;
    //2.get ckb status which is checked or not;
    //based below action ,to cofirm which will be send;
    Info_sz= get_scene()->m_save_ag->m_actions.count();
    if(Info_sz==0) return;
    QList<int> servo_ids;
    servo_ids.clear();
    for(int i=0;i<Info_sz;++i) //原有已保存的动作组
    {
        for(int j=0;j<MAX_ACTION_COUNT;++j)
        {
            if(ckb_actions[j]->isCheckable())
            {
                for(auto servo:m_list_servoinfo)
                {
                    if(btn_actions[j]->text()==servo->get_name())
                    {
                        servo_ids.append(servo->get_id());
                    }
                }
            }
        }
    }
    servo_ids;
    for(int s=0;s<Info_sz;++s)
    {
        for(int m=0;m<servo_ids.count();++m)
        {
            if(get_scene()->m_save_ag->m_actions[s]->getId()==servo_ids[m])
            {
                willsends.append(get_scene()->m_save_ag->m_actions[s]);
            }
        }
    }
    willsends;
}
void MainWindow::slot_play() //增加播放选中动作组的功能；
{
//    if(!m_serialPort->isOpen())
//    {
//        statusBar()->showMessage(tr(":-,亲，请先打开串口，然后再播放,谢谢!"), 2000);
//        return ;
//    }
    QList<QMdiSubWindow*> sw=mdiArea->subWindowList();
    int  subwindow_sz=sw.size();
    m_selected_action.clear();
    this->get_selected_actions(m_selected_action);
    Info_sz=m_selected_action.count();
      if(Info_sz==0||subwindow_sz==0)
    {
        qDebug()<<":- 亲，请先加载场景文件，然后再播放,谢谢!"<<endl;
        statusBar()->showMessage(tr(":-,亲，请先加载场景文件，然后再播放,谢谢!"), 2000);
        return;
    }
    btn_play->setEnabled(false);
    m_t=new QTimer(this);
    m_t->setTimerType(Qt::PreciseTimer);
    connect(m_t,SIGNAL(timeout()),this,SLOT(sendCommand()));
    m_t->start(20);
    qDebug("[play]");
}
//void MainWindow::slot_play()
//{
//    if(!m_serialPort->isOpen())
//    {
//        statusBar()->showMessage(tr(":-,亲，请先打开串口，然后再播放,谢谢!"), 2000);
//        return ;
//    }
//    QList<QMdiSubWindow*> sw=mdiArea->subWindowList();
//    int  subwindow_sz=sw.size();

//    Info_sz= get_scene()->m_save_ag->m_actions.count(); //number of actions in scene;
//    if(Info_sz==0||subwindow_sz==0)
//    {
//        qDebug()<<":- 亲，请先加载场景文件，然后再播放,谢谢!"<<endl;
//        statusBar()->showMessage(tr(":-,亲，请先加载场景文件，然后再播放,谢谢!"), 2000);
//        return;
//    }
//    btn_play->setEnabled(false);
//    m_t=new QTimer(this);
//    m_t->setTimerType(Qt::PreciseTimer);
//    connect(m_t,SIGNAL(timeout()),this,SLOT(sendCommand()));
//    m_t->start(20);
//    qDebug("[play]");
//}

void MainWindow::slot_stop()
{
    if(!btn_play->isEnabled())
    {
        m_t->stop();
        qDebug("[stop]");
        statusBar()->showMessage(tr(":-，亲，停止播放任务,谢谢!]"), 2000);
        btn_play->setEnabled(true);
    }else
    {
        qDebug("[can not execute stop]");
        statusBar()->showMessage(tr(":- 亲，没有播放任务正在执行哦!"), 2000);
    }
}
DrawView *MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<DrawView *>(activeSubWindow->widget());
    return 0;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
        DrawView *mdiChild = qobject_cast<DrawView *>(window->widget());
        if (mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return 0;
}
void MainWindow::slot_com_connect()
{
    m_serialPort->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections);//设置波特率和读写方向
    m_serialPort->setDataBits(QSerialPort::Data8);      //数据位为8位
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);//无流控制
    m_serialPort->setParity(QSerialPort::NoParity); //无校验位
    m_serialPort->setStopBits(QSerialPort::OneStop); //一位停止位
    if(m_serialPort->isOpen())
    {
        m_serialPort->clear();
        m_serialPort->close();
    }
    m_serialPort->setPortName(combo_port->currentText());
    qDebug()<<"[default port:"<<m_serialPort->portName()<<endl;
    if(!m_serialPort->open(QIODevice::ReadWrite))//用ReadWrite 的模式尝试打开串口
    {
        qDebug()<<m_serialPortName<<"打开失败!";
        statusBar()->showMessage(tr(" 打开串口失败!"), 2000);

        return;
    }else
    {
        qDebug()<<m_serialPortName<<"打开成功!";
        statusBar()->showMessage(tr(" 打开串口成功!"), 2000);
    }
    //    //打开成功
    //连接信号槽 当下位机发送数据QSerialPortInfo 会发送个 readyRead 信号,我们定义个槽void receiveInfo()解析数据
    connect(m_serialPort,SIGNAL(readyRead()),this,SLOT(receiveInfo()));
}
//接收单片机的数据
void MainWindow::receiveInfo()
{
    QByteArray info = m_serialPort->readAll();
    QByteArray hexData = info.toHex();
    qDebug()<<"###"<<hexData<<endl;
}
//向单片机发送数据

//基本和单片机交互 数据 都是16进制的 我们这里自己写一个 Qstring 转为 16进制的函数
void MainWindow::convertStringToHex(const QString &str, QByteArray &byteData)
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    byteData.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = convertCharToHex(hstr);
        lowhexdata = convertCharToHex(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        byteData[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    byteData.resize(hexdatalen);
}

//另一个 函数 char 转为 16进制
char MainWindow::convertCharToHex(char ch)
{
    /*
        0x30等于十进制的48，48也是0的ASCII值，，
        1-9的ASCII值是49-57，，所以某一个值－0x30，，
        就是将字符0-9转换为0-9

        */
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

//写两个函数 向单片机发送数据
void MainWindow::sendInfo(char* info,int len){

    //    for(int i=0; i<len; ++i)
    //    {
    //        printf("0x%x\n", info[i]);
    //    }
    m_serialPort->write(info,len);
}

void MainWindow::sendInfo(const QString &info){
    QByteArray sendBuf;
    info.trimmed();
    qDebug()<<"Write to serial: "<<info;
    convertStringToHex(info, sendBuf); //把QString 转换为hex
    m_serialPort->write(sendBuf);
}
void MainWindow::slot_getPort(QString port)
{
    qDebug()<<"[port]:"<<port<<endl;
    this->m_serialPortName=port;
}
void MainWindow::slot_com_conf()
{

}
void MainWindow::slot_com_disconnect()
{
    if(m_serialPort->isOpen())
    {
        m_serialPort->clear();
        m_serialPort->close();
    }


}

void MainWindow::slot_play_all_audio()
{

    // qDebug()<<"[FILE:"<<__FILE__<<",LINE"<<__LINE__<<",FUNC"<<__FUNCTION__<<endl;
    qDebug("[play all audio]");
    if (!activeMdiChild())
    {
        statusBar()->showMessage("[please create new scene file first!]",2000);
        return ;
    }
    slt_request_play_all();
}
void MainWindow::slot_load_audio()
{
    if (!activeMdiChild())
    {
        statusBar()->showMessage("[please create new scene file first!]",2000);
        return ;
    }
    qDebug("[load audio!]");
    slt_request_load();

}
void MainWindow::slot_play_partial_audio()
{
    qDebug("[play partial audio!]");
}

void MainWindow::slot_net_conf()
{
    qDebug()<<"config net for play!"<<endl;
}
void MainWindow::createActions()
{
    loadAct = new QAction(tr("&loadAudio"), this);
    loadAct->setShortcut(QKeySequence("ctrl + 9"));
    loadAct->setStatusTip(tr("load audio"));
    connect(loadAct, SIGNAL(triggered()), this, SLOT(slot_load_audio()));
    playAllAct = new QAction(tr("&playAllAudio"), this);
    playAllAct->setShortcut(QKeySequence("ctrl + 2"));
    playAllAct->setStatusTip(tr("play total audio"));
    connect(playAllAct, SIGNAL(triggered()), this, SLOT(slot_play_all_audio()));
    playPartialAct = new QAction(tr("&playPartialAudio"), this);
    playPartialAct->setShortcut(QKeySequence("ctrl + 7"));
    playPartialAct->setStatusTip(tr("play partial audio"));
    connect( playPartialAct, SIGNAL(triggered()), this, SLOT(slot_play_partial_audio()));



    comAct = new QAction(tr("&com config"), this);
    comAct->setShortcut(QKeySequence("ctrl + 3"));
    comAct->setStatusTip(tr("connect robot"));
    connect(comAct, SIGNAL(triggered()), this, SLOT(slot_com_conf()));

    netAct = new QAction(tr("&net config"), this);
    netAct->setShortcut(QKeySequence("ctrl + 4"));
    netAct->setStatusTip(tr("connect robot"));
    connect(netAct, SIGNAL(triggered()), this, SLOT(slot_net_conf()));



    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeActiveSubWindow()));

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeAllSubWindows()));

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()),
            mdiArea, SLOT(activateNextSubWindow()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                 "window"));
    connect(previousAct, SIGNAL(triggered()),
            mdiArea, SLOT(activatePreviousSubWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's copyright"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    // create align actions
    rightAct   = new QAction(QIcon(":/icons/align_right.png"),tr("right"),this);
    leftAct    = new QAction(QIcon(":/icons/align_left.png"),tr("left"),this);
    vCenterAct = new QAction(QIcon(":/icons/align_vcenter.png"),tr("vcenter"),this);
    hCenterAct = new QAction(QIcon(":/icons/align_hcenter.png"),tr("hcenter"),this);
    upAct      = new QAction(QIcon(":/icons/align_top.png"),tr("top"),this);
    downAct    = new QAction(QIcon(":/icons/align_bottom.png"),tr("bottom"),this);
    horzAct    = new QAction(QIcon(":/icons/align_horzeven.png"),tr("Horizontal"),this);
    vertAct    = new QAction(QIcon(":/icons/align_verteven.png"),tr("vertical"),this);
    heightAct  = new QAction(QIcon(":/icons/align_height.png"),tr("height"),this);
    widthAct   = new QAction(QIcon(":/icons/align_width.png"),tr("width"),this);
    allAct     = new QAction(QIcon(":/icons/align_all.png"),tr("width and height"),this);

    bringToFrontAct = new QAction(QIcon(":/icons/bringtofront.png"),tr("bring to front"),this);
    sendToBackAct   = new QAction(QIcon(":/icons/sendtoback.png"),tr("send to back"),this);
    groupAct        = new QAction(QIcon(":/icons/group.png"),tr("group"),this);
    unGroupAct        = new QAction(QIcon(":/icons/ungroup.png"),tr("ungroup"),this);

    connect(bringToFrontAct,SIGNAL(triggered()),this,SLOT(on_actionBringToFront_triggered()));
    connect(sendToBackAct,SIGNAL(triggered()),this,SLOT(on_actionSendToBack_triggered()));
    connect(rightAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(leftAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(vCenterAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(hCenterAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(upAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(downAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));

    connect(horzAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(vertAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(heightAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(widthAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(allAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));

    connect(groupAct,SIGNAL(triggered()),this,SLOT(on_group_triggered()));
    connect(unGroupAct,SIGNAL(triggered()),this,SLOT(on_unGroup_triggered()));


    //create draw actions
    selectAct = new QAction(QIcon(":/icons/arrow.png"),tr("select tool"),this);

    selectAct->setCheckable(true);
    lineAct = new QAction(QIcon(":/icons/line.png"),tr("line tool"),this);
    lineAct->setCheckable(true);
    //    rectAct = new QAction(QIcon(":/icons/rectangle.png"),tr("rect tool"),this);
    //    rectAct->setCheckable(true);

    //    roundRectAct =  new QAction(QIcon(":/icons/roundrect.png"),tr("roundrect tool"),this);
    //    roundRectAct->setCheckable(true);
    //    ellipseAct = new QAction(QIcon(":/icons/ellipse.png"),tr("ellipse tool"),this);
    //    ellipseAct->setCheckable(true);
    //    polygonAct = new QAction(QIcon(":/icons/polygon.png"),tr("polygon tool"),this);
    //    polygonAct->setCheckable(true);
    polylineAct = new QAction(QIcon(":/icons/polyline.png"),tr("polyline tool"),this);
    polylineAct->setCheckable(true);
    bezierAct= new QAction(QIcon(":/icons/bezier.png"),tr("bezier tool"),this);
    bezierAct->setCheckable(true);

    rotateAct = new QAction(QIcon(":/icons/rotate.png"),tr("rotate tool"),this);
    rotateAct->setCheckable(true);

    drawActionGroup = new QActionGroup(this);
    drawActionGroup->addAction(selectAct);
    drawActionGroup->addAction(lineAct);
    //    drawActionGroup->addAction(rectAct);
    //    drawActionGroup->addAction(roundRectAct);
    //    drawActionGroup->addAction(ellipseAct);
    //    drawActionGroup->addAction(polygonAct);
    drawActionGroup->addAction(polylineAct);
    drawActionGroup->addAction(bezierAct);
    drawActionGroup->addAction(rotateAct);
    selectAct->setChecked(true);


    connect(selectAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(lineAct,SIGNAL(triggered()),this,SLOT(addShape()));
    //    connect(rectAct,SIGNAL(triggered()),this,SLOT(addShape()));
    //    connect(roundRectAct,SIGNAL(triggered()),this,SLOT(addShape()));
    //    connect(ellipseAct,SIGNAL(triggered()),this,SLOT(addShape()));
    //    connect(polygonAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(polylineAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(bezierAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(rotateAct,SIGNAL(triggered()),this,SLOT(addShape()));

    deleteAct = new QAction(tr("&Delete"), this);
    deleteAct->setShortcut(QKeySequence::Delete);

    undoAct = undoStack->createUndoAction(this,tr("undo"));
    undoAct->setIcon(QIcon(":/icons/undo.png"));
    undoAct->setShortcuts(QKeySequence::Undo);

    redoAct = undoStack->createRedoAction(this,tr("redo"));
    redoAct->setIcon(QIcon(":/icons/redo.png"));
    redoAct->setShortcuts(QKeySequence::Redo);

    zoomInAct = new QAction(QIcon(":/icons/zoomin.png"),tr("zoomIn"),this);
    zoomOutAct = new QAction(QIcon(":/icons/zoomout.png"),tr("zoomOut"),this);

    copyAct = new QAction(QIcon(":/icons/copy.png"),tr("copy"),this);
    copyAct->setShortcut(QKeySequence::Copy);

    pasteAct = new QAction(QIcon(":/icons/paste.png"),tr("paste"),this);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(false);
    cutAct = new QAction(QIcon(":/icons/cut.png"),tr("cut"),this);
    cutAct->setShortcut(QKeySequence::Cut);

    connect(copyAct,SIGNAL(triggered()),this,SLOT(on_copy()));
    connect(pasteAct,SIGNAL(triggered()),this,SLOT(on_paste()));
    connect(cutAct,SIGNAL(triggered()),this,SLOT(on_cut()));

    connect(zoomInAct , SIGNAL(triggered()),this,SLOT(zoomIn()));
    connect(zoomOutAct , SIGNAL(triggered()),this,SLOT(zoomOut()));
    connect(deleteAct, SIGNAL(triggered()), this, SLOT(deleteItem()));

    funcAct = new QAction(tr("func test"),this);
    connect(funcAct,SIGNAL(triggered()),this,SLOT(on_func_test_triggered()));

}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);


    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(deleteAct);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);


    QMenu *connMenu = menuBar()->addMenu(tr("&Connect"));
    connMenu->addAction(comAct);
    connMenu->addAction(netAct);
    connMenu->addSeparator();

    QMenu *audioMenu = menuBar()->addMenu(tr("Aud&io"));
    audioMenu->addAction(loadAct);
    audioMenu->addAction(playAllAct);
    audioMenu->addSeparator();
    audioMenu->addAction(playPartialAct);



    QMenu *toolMenu = menuBar()->addMenu(tr("&Tools"));
    QMenu *shapeTool = new QMenu("&Shape");
    shapeTool->addAction(selectAct);
    //    shapeTool->addAction(rectAct);
    //    shapeTool->addAction(roundRectAct);
    //    shapeTool->addAction(ellipseAct);
    //    shapeTool->addAction(polygonAct);
    shapeTool->addAction(polylineAct);
    shapeTool->addAction(bezierAct);
    shapeTool->addAction(rotateAct);
    toolMenu->addMenu(shapeTool);
    QMenu *alignMenu = new QMenu("Align");
    alignMenu->addAction(rightAct);
    alignMenu->addAction(leftAct);
    alignMenu->addAction(hCenterAct);
    alignMenu->addAction(vCenterAct);
    alignMenu->addAction(upAct);
    alignMenu->addAction(downAct);
    alignMenu->addAction(horzAct);
    alignMenu->addAction(vertAct);
    alignMenu->addAction(heightAct);
    alignMenu->addAction(widthAct);
    alignMenu->addAction(allAct);
    toolMenu->addMenu(alignMenu);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    updateWindowMenu();
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    menuBar()->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    //    helpMenu->addAction(aboutQtAct);
    helpMenu->addAction(funcAct);

}

void MainWindow::createToolbars()
{
    // create edit toolbar
    editToolBar = addToolBar(tr("edit"));
    editToolBar->setIconSize(QSize(24,24));
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    editToolBar->addAction(cutAct);

    editToolBar->addAction(undoAct);
    editToolBar->addAction(redoAct);

    editToolBar->addAction(zoomInAct);
    editToolBar->addAction(zoomOutAct);

    // create draw toolbar
    drawToolBar = addToolBar(tr(""));
    drawToolBar->setIconSize(QSize(24,24));
    drawToolBar->addAction(selectAct);
    drawToolBar->addAction(lineAct);
    //    drawToolBar->addAction(rectAct);
    //    drawToolBar->addAction(roundRectAct);
    //    drawToolBar->addAction(ellipseAct);
    //    drawToolBar->addAction(polygonAct);
    drawToolBar->addAction(polylineAct);
    drawToolBar->addAction(bezierAct);
    drawToolBar->addAction(rotateAct);

    // create align toolbar
    alignToolBar = addToolBar(tr("align"));
    alignToolBar->setIconSize(QSize(24,24));
    alignToolBar->addAction(upAct);
    alignToolBar->addAction(downAct);
    alignToolBar->addAction(rightAct);
    alignToolBar->addAction(leftAct);
    alignToolBar->addAction(vCenterAct);
    alignToolBar->addAction(hCenterAct);

    alignToolBar->addAction(horzAct);
    alignToolBar->addAction(vertAct);
    alignToolBar->addAction(heightAct);
    alignToolBar->addAction(widthAct);
    alignToolBar->addAction(allAct);

    alignToolBar->addAction(bringToFrontAct);
    alignToolBar->addAction(sendToBackAct);
    alignToolBar->addAction(groupAct);
    alignToolBar->addAction(unGroupAct);
}

void MainWindow::createPropertyEditor()
{
    dockProperty = new QDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, dockProperty);

    propertyEditor = new ObjectController(this);
    dockProperty->setWidget(propertyEditor);
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeMdiChild() != 0);
    saveAct->setEnabled(hasMdiChild);
    if (!hasMdiChild){
        undoStack->clear();
    }
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    separatorAct->setVisible(hasMdiChild);

    bool hasSelection = (activeMdiChild() &&
                         activeMdiChild()->scene()->selectedItems().count()>0);
    cutAct->setEnabled(hasSelection);
    copyAct->setEnabled(hasSelection);
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        DrawView *child = qobject_cast<DrawView *>(windows.at(i)->widget());

        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1)
                    .arg(child->userFriendlyCurrentFile());
        } else {
            text = tr("%1 %2").arg(i + 1)
                    .arg(child->userFriendlyCurrentFile());
        }
        QAction *action  = windowMenu->addAction(text);
        action->setCheckable(true);
        action ->setChecked(child == activeMdiChild());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i));
    }
}
//void MainWindow::clearTableWidget()
//{
//    int rowNum =  tableWidget->rowCount();
//    for(int i = 0;i < rowNum;i++)
//    {
//        tableWidget->removeRow(0);
//    }
//    tableWidget->clearContents();
//}
void addWidget_ex()
{
    ///////////// 创建 widget
    QLabel *pPixmapLabel = new QLabel();
    QLineEdit *pAccountLineEdit = new QLineEdit();
    QLineEdit *pPasswdLineEdit = new QLineEdit();
    QCheckBox *pRememberCheckBox = new QCheckBox();
    QCheckBox *pAutoLoginCheckBox = new QCheckBox();
    QPushButton *pLoginButton = new QPushButton();
    QPushButton *pRegisterButton = new QPushButton();
    QPushButton *pForgotButton = new QPushButton();

    pPixmapLabel->setStyleSheet("border-image: url(:/Images/logo); min-width:90px; min-height:90px; border-radius:45px; background:transparent;");
    pAccountLineEdit->setPlaceholderText(QStringLiteral("QQ号码/手机/邮箱"));
    pPasswdLineEdit->setPlaceholderText(QStringLiteral("密码"));
    pPasswdLineEdit->setEchoMode(QLineEdit::Password);
    pRememberCheckBox->setText(QStringLiteral("记住密码"));
    pAutoLoginCheckBox->setText(QStringLiteral("自动登录"));
    pLoginButton->setText(QStringLiteral("登录"));
    pRegisterButton->setText(QStringLiteral("注册账号"));
    pForgotButton->setText(QStringLiteral("找回密码"));

    pLoginButton->setFixedHeight(30);
    pAccountLineEdit->setFixedWidth(180);

    // 添加 widget
    QGraphicsScene *pScene = new QGraphicsScene();
    QGraphicsProxyWidget *pPixmapWidget = pScene->addWidget(pPixmapLabel);
    QGraphicsProxyWidget *pAccountWidget = pScene->addWidget(pAccountLineEdit);
    QGraphicsProxyWidget *pPasswdWidget = pScene->addWidget(pPasswdLineEdit);
    QGraphicsProxyWidget *pRememberWidget = pScene->addWidget(pRememberCheckBox);
    QGraphicsProxyWidget *pAutoLoginWidget = pScene->addWidget(pAutoLoginCheckBox);
    QGraphicsProxyWidget *pLoginWidget = pScene->addWidget(pLoginButton);
    QGraphicsProxyWidget *pRegisterWidget = pScene->addWidget(pRegisterButton);
    QGraphicsProxyWidget *pForgotWidget = pScene->addWidget(pForgotButton);

    // 添加至网格布局中
    QGraphicsGridLayout *pLayout = new QGraphicsGridLayout();
    pLayout->addItem(pPixmapWidget, 0, 0, 3, 1);
    pLayout->addItem(pAccountWidget, 0, 1, 1, 2);
    pLayout->addItem(pRegisterWidget, 0, 4);
    pLayout->addItem(pPasswdWidget, 1, 1, 1, 2);
    pLayout->addItem(pForgotWidget, 1, 4);
    pLayout->addItem(pRememberWidget, 2, 1, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    pLayout->addItem(pAutoLoginWidget, 2, 2, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    pLayout->addItem(pLoginWidget, 3, 1, 1, 2);
    pLayout->setHorizontalSpacing(10);
    pLayout->setVerticalSpacing(10);
    pLayout->setContentsMargins(10, 10, 10, 10);

    QGraphicsWidget *pWidget = new QGraphicsWidget();
    pWidget->setLayout(pLayout);

    // 将 item 添加至场景中
    pScene->addItem(pWidget);

    // 为视图设置场景
    QGraphicsView *pView = new QGraphicsView();
    pView->setScene(pScene);
    pView->show();
}
//void MainWindow::newFile() //20190714
//{
//    DrawView *child = createMdiChild();
//    child->newFile();
//    child->show();
//}
void MainWindow::newFile()
{
    if(mdiArea==nullptr) return;
    if(mdiArea!=nullptr&&mdiArea->subWindowList().size()>0)
        this->mdiArea->closeAllSubWindows();
    DrawView *child = createMdiChild();
    child->newFile();
    child->m_parent_window->slt_check_action_status(false);
    child->show();
}
void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
        QMdiSubWindow *existing = findMdiChild(fileName);
        if (existing) {
            mdiArea->setActiveSubWindow(existing);  //check if the file want to open  is whether openned or not;
            return;
        }

        if (openFile(fileName))
            statusBar()->showMessage(tr("File loaded"), 2000);
    }
}
//bool MainWindow::openFile(const QString &fileName)
//{
//    DrawView *child = createMdiChild();
//    const bool succeeded = child->loadFile(fileName);
//    if (succeeded)
//        child->show();
//    else
//        child->close();
//    return succeeded;
//}
//bool MainWindow::openFile(const QString &fileName) //20190714
//{
//    DrawView *child = createMdiChild();
//    bool succeeded=child->loadFile(fileName);
//    QString temp=fileName.left(fileName.length()-4);
//    QString extFileName=temp+tr("_exi.xml");
//    succeeded = child->loadFile_ex(extFileName);
//    if (succeeded)
//        child->show();
//    else
//        child->close();
//    return succeeded;
//}
bool MainWindow::openFile4save(const QString &fileName) //20190715
{
    DrawView *child = createMdiChild();
    DrawScene* scene=dynamic_cast<DrawScene*>(get_scene());
    if(scene==nullptr) return false;
    bool succeeded=child->loadFile(fileName);
    QString temp=fileName.left(fileName.length()-4);
    QString extFileName=temp+tr("_exi.xml");
    slt_reload_audio();
    succeeded = child->loadFile_ex(extFileName);
    if (succeeded)
        child->show();
    else
        child->close();
    return succeeded;
}
//bool MainWindow::openFile4save(const QString &fileName) //20190715
//{
//    DrawView *child = createMdiChild();
//    DrawScene* scene=dynamic_cast<DrawScene*>(get_scene());
//    if(scene==nullptr) return false;
//    scene->m_save_ag=main_save_ag;
//    int sz=scene->m_save_ag->m_actions.count();
//    qDebug("***###m_actions:%d",sz);
//    bool succeeded=child->loadFile(fileName);
//    QString temp=fileName.left(fileName.length()-4);
//    QString extFileName=temp+tr("_exi.xml");
//    succeeded = child->loadFile_ex(extFileName);
//    if (succeeded)
//        child->show();
//    else
//        child->close();
//    return succeeded;
//}
//bool MainWindow::openFile(const QString &fileName)//20190715
//{
//    if(mdiArea!=nullptr&&mdiArea->subWindowList().size()>0)
//        this->mdiArea->closeAllSubWindows();
//    DrawView *child = createMdiChild();
//    bool succeeded=child->loadFile(fileName);
//    QString temp=fileName.left(fileName.length()-4);
//    QString extFileName=temp+tr("_exi.xml");
//    succeeded = child->loadFile_ex(extFileName);
//    if (succeeded)
//        child->show();
//    else
//        child->close();
//    return succeeded;
//}
bool MainWindow::openFile(const QString &fileName)
{
    if(mdiArea!=nullptr&&mdiArea->subWindowList().size()>0)
        this->mdiArea->closeAllSubWindows();//before open file ,muset close all current files;
    DrawView *child = createMdiChild();
    bool succeeded=child->loadFile(fileName);
    QString temp=fileName.left(fileName.length()-4);
    QString extFileName=temp+tr("_exi.xml");
    succeeded = child->loadFile_ex(extFileName);
    if (succeeded)
    {
        DrawScene* scene=dynamic_cast<DrawScene*>(child->scene());
        child->show();
    }
    else
        child->close();
    return succeeded;
}
//bool MainWindow::openFile(const QString &fileName)  //之前可用，暂时注释，测试用
//{
//    DrawView *child = createMdiChild();
//    const bool succeeded = child->loadFile(fileName);

//    if(!child->ts.isEmpty())
//    {
//        total_parameter.clear();
//        total_parameter=child->ts;
//        child->ts.clear();
//        for(int m=0;m<MAX_ACTION_COUNT;++m)
//        {
//            for(int n=0;n<PARAM_COUNT;++n)
//            {
//                for(int k=0;k<MAX_POINT_COUNT;++k)
//                {
//                    param[m][n][k]=" ";
//                }
//            }
//        }
//        getInfo();
//    }

//    if (succeeded)
//        child->show();
//    else
//        child->close();
//    return succeeded;
//}

//void MainWindow::save()
//{
//    DrawView *v=activeMdiChild();
//    if (v && v->save())
//    {
//        this->openFile(v->currentFile());
//        statusBar()->showMessage(tr("File saved"), 2000);
//        if(v)
//            v->parentWidget()->close();
//    }
//}

void MainWindow::save()
{
    if (activeMdiChild() && activeMdiChild()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        event->accept();
    }
}
//void MainWindow::removeView(QCloseEvent *event)
//{
//    mdiArea->closeAllSubWindows();
//    if (mdiArea->currentSubWindow()) {
//        event->ignore();
//    } else {
//        event->accept();
//    }
//}
void MainWindow::slot_tableItemSelect()
{
    //    QTableWidgetItem *actitem=nullptr;
    //    QTableWidgetItem *servitem=nullptr;
    //    actitem=this->tableWidget->selectedItems()[0];
    //    QString actid=actitem->data(0).toString();
    //    servitem=this->tableWidget->selectedItems()[1];
    //    QString servid=actitem->data(0).toString();
    //    qDebug()<<"[actid:]"<<actitem->data(0).toString();
    //    qDebug()<<"[servid:]"<<servitem->data(0).toString();
    //    if(this->item!=nullptr)
    //    {
    //        this->item->m_actionid=actid;
    //        this->item->m_actionid=servid;
    //    }
}
//DrawView *MainWindow::createMdiChild()
//{
//    DrawScene *scene = new DrawScene(this);
//    QRectF rc = QRectF(0 , 0 , SCENE_WIDTH, SCENE_HEIGHT);
//    scene->setSceneRect(rc);

//    connect(scene, SIGNAL(selectionChanged()),
//            this, SLOT(itemSelected()));

//    connect(scene,SIGNAL(itemAdded(QGraphicsItem*)),
//            this, SLOT(itemAdded(QGraphicsItem*)));
//    connect(scene,SIGNAL(itemMoved(QGraphicsItem*,QPointF)),
//            this,SLOT(itemMoved(QGraphicsItem*,QPointF)));
//    connect(scene,SIGNAL(itemRotate(QGraphicsItem*,qreal)),
//            this,SLOT(itemRotate(QGraphicsItem*,qreal)));

//    connect(scene,SIGNAL(itemResize(QGraphicsItem* , int , const QPointF&)),
//            this,SLOT(itemResize(QGraphicsItem*,int,QPointF)));

//    connect(scene,SIGNAL(itemControl(QGraphicsItem* , int , const QPointF&,const QPointF&)),
//            this,SLOT(itemControl(QGraphicsItem*,int,QPointF,QPointF)));

//    view = new DrawView(scene);
//    view->setStyleSheet("QWidget{background-image: url(:/icons/hot.png);} ");
//    connect(tableWidget,SIGNAL(itemSelectionChanged()),this,SLOT(slot_tableItemSelect()));
//    connect(tableWidget, SIGNAL(itemSelectionChanged()),view, SLOT(selectedByTable()));
//    view->setWindowFlags(Qt::CustomizeWindowHint);

//    view->setWindowFlags(Qt::FramelessWindowHint);
//    scene->setView(view);
//    connect(view,SIGNAL(positionChanged(int,int)),this,SLOT(positionChanged(int,int)));
//    connect(view,SIGNAL(closeDrawView()),this,SLOT(slot_closeDrawView()));
//    //////////////////////////////////////////////////////////////////
//    connect(view,SIGNAL(actionInfoChanged(QStringList,QStringList)),this,SLOT(receiveActionChange(QStringList,QStringList)));
//    ////////////////////
//    view->setRenderHint(QPainter::Antialiasing);
//    view->setCacheMode(QGraphicsView::CacheBackground);
//    view->setOptimizationFlags(QGraphicsView::DontSavePainterState);
//    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
//    //view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
//    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

//    // move orign point to leftbottom
//    view->setTransform(view->transform().scale(1,-1));
//    scene->setBackgroundBrush(Qt::black);
//    mdiArea->addSubWindow(view);
//    view->showMaximized();
//    return view;
//}
void MainWindow::slt_selected_actiongroup_type(bool isSelected)
{
    m_isSelected_servo_type=isSelected;
}
DrawView *MainWindow::createMdiChild()
{
    DrawScene *scene = new DrawScene(this);
    QRectF rc = QRectF(0 , 0 , SCENE_WIDTH, SCENE_HEIGHT);
    //    connect(scene,SIGNAL(sig_received_servo_info()),this,SLOT(slt_received_servo_info()));
    connect(scene,SIGNAL( sig_selected_actiongroup_type(bool)),this,SLOT(slt_selected_actiongroup_type(bool)));
    connect(scene,SIGNAL(sig_pause_audio()),play,SLOT(slt_pause()));
    connect(scene,SIGNAL(sig_load_audio()),this,SLOT( slot_load_audio()));
    connect(scene,SIGNAL(sig_play_audio()),this,SLOT(slot_play_all_audio()));
    connect(this,SIGNAL(sig_fill_data_to_scene(QVector<QPointF>)),scene,SLOT(add_item_with_fill(QVector<QPointF>)));
    connect(this,SIGNAL(sig_graphics_item_selected(QString)),scene,SLOT(slt_graphics_item_selected(QString)));
    connect(scene,SIGNAL(sig_fill_graphics(QString)),this,SLOT(slt_fill_graphics(QString)));
    scene->setSceneRect(rc);
    connect(scene, SIGNAL(selectionChanged()),
            this, SLOT(itemSelected()));
    connect(scene,SIGNAL(itemAdded(QGraphicsItem*)),
            this, SLOT(itemAdded(QGraphicsItem*)));
    connect(scene,SIGNAL(itemMoved(QGraphicsItem*,QPointF)),
            this,SLOT(itemMoved(QGraphicsItem*,QPointF)));
    connect(scene,SIGNAL(itemRotate(QGraphicsItem*,qreal)),
            this,SLOT(itemRotate(QGraphicsItem*,qreal)));

    connect(scene,SIGNAL(itemResize(QGraphicsItem* , int , const QPointF&)),
            this,SLOT(itemResize(QGraphicsItem*,int,QPointF)));

    connect(scene,SIGNAL(itemControl(QGraphicsItem* , int , const QPointF&,const QPointF&)),
            this,SLOT(itemControl(QGraphicsItem*,int,QPointF,QPointF)));

    view = new DrawView(scene);
    //    QRectF rcf=view->rect();
    //    view->setStyleSheet("QWidget{background-image: url(:/icons/hot.png);} ");
    //    connect(tableWidget,SIGNAL(itemSelectionChanged()),this,SLOT(slot_tableItemSelect()));
    //    connect(tableWidget, SIGNAL(itemSelectionChanged()),view, SLOT(selectedByTable()));
    view->setWindowFlags(Qt::CustomizeWindowHint);
    //    view->setWindowFlags(Qt::FramelessWindowHint);
    view->setWindowFlags(view->windowFlags()^Qt::FramelessWindowHint);
    scene->setView(view);
    connect(view,SIGNAL(positionChanged(int,int)),this,SLOT(positionChanged(int,int)));
    connect(view,SIGNAL(closeDrawView()),this,SLOT(slot_closeDrawView()));
    //////////////////////////////////////////////////////////////////
    connect(view,SIGNAL(actionInfoChanged(QStringList,QStringList)),this,SLOT(receiveActionChange(QStringList,QStringList)));
    ////////////////////
    view->setRenderHint(QPainter::Antialiasing);
    view->setCacheMode(QGraphicsView::CacheBackground);
    view->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    //view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // view->setWindowFlags(child->windowFlags()^Qt::FramelessWindowHint);
    // move orign point to leftbottom
    //    view->setTransform(view->transform().scale(1,-1));
    scene->setBackgroundBrush(Qt::transparent);
    mdiArea->addSubWindow(view);
    view->m_parent_window=this;
    /////////////////////////
    m_check_item_bselected=new QTimer(this);
    connect(m_check_item_bselected,SIGNAL(timeout()),this,SLOT(slt_check_item_selected()));
    m_check_item_bselected->start(50);
    m_check_item_bselected->setTimerType(Qt::PreciseTimer);
    //////////////////////////////
    view->showMaximized();
    return view;
}
void MainWindow::slot_closeDrawView()
{
    //    clearTableWidget();
}
void MainWindow::receiveActionChange(QStringList la,QStringList ls)
{
    //    int rowNum =  tableWidget->rowCount();
    //    for(int i = 0;i < rowNum;i++)
    //    {
    //        tableWidget->removeRow(0);
    //    }
    //    tableWidget->clearContents();
    //    for(int i=0;i<la.size();++i)
    //    {
    //        tableWidget->insertRow(0);
    //        tableWidget->setItem(0,0,new QTableWidgetItem(la[i]));
    //        tableWidget->item(0,0)->setTextAlignment(Qt::AlignCenter);
    //        tableWidget->setItem(0,1,new QTableWidgetItem(ls[i]));
    //        tableWidget->item(0,1)->setTextAlignment(Qt::AlignCenter);
    //    }

}

void MainWindow::addShape()
{
    if ( sender() == selectAct )
        DrawTool::c_drawShape = selection;
    else if (sender() == lineAct )
        DrawTool::c_drawShape = line;
    //    else if ( sender() == rectAct )
    //        DrawTool::c_drawShape = rectangle;
    //    else if ( sender() == roundRectAct )
    //        DrawTool::c_drawShape = roundrect;
    //    else if ( sender() == ellipseAct )
    //        DrawTool::c_drawShape = ellipse ;
    //    else if ( sender() == polygonAct )
    //        DrawTool::c_drawShape = polygon;
    else if ( sender() == bezierAct )
        DrawTool::c_drawShape = bezier ;
    else if (sender() == rotateAct )
        DrawTool::c_drawShape = rotation;
    else if (sender() == polylineAct )
    {
        DrawTool::c_drawShape = polyline;
    }

    if ( sender() != selectAct && sender() != rotateAct ){
        activeMdiChild()->scene()->clearSelection();
    }
}


//void MainWindow::updateActions()
//{
//    QGraphicsScene * scene =nullptr;
//    if (activeMdiChild())
//        scene = activeMdiChild()->scene();

//    selectAct->setEnabled(scene);
//    lineAct->setEnabled(scene);
////    lineAct->setEnabled(false);
//    rectAct->setEnabled(false);

//    roundRectAct->setEnabled(false);
//    ellipseAct->setEnabled(false);
//    bezierAct->setEnabled(scene);
//    rotateAct->setEnabled(scene);
//    polygonAct->setEnabled(false);
//    polylineAct->setEnabled(false);

//    zoomInAct->setEnabled(scene);
//    zoomOutAct->setEnabled(scene);

//    selectAct->setChecked(DrawTool::c_drawShape == selection);
//    lineAct->setChecked(DrawTool::c_drawShape == line);
//    rectAct->setChecked(DrawTool::c_drawShape == rectangle);
//    roundRectAct->setChecked(DrawTool::c_drawShape == roundrect);
//    ellipseAct->setChecked(DrawTool::c_drawShape == ellipse);
//    bezierAct->setChecked(DrawTool::c_drawShape == bezier);
//    rotateAct->setChecked(DrawTool::c_drawShape == rotation);
//    polygonAct->setChecked(DrawTool::c_drawShape == polygon);
//    polylineAct->setChecked(DrawTool::c_drawShape == polyline );
//    undoAct->setEnabled(undoStack->canUndo());
//    redoAct->setEnabled(undoStack->canRedo());


//    bringToFrontAct->setEnabled(scene && scene->selectedItems().count() > 0);
//    sendToBackAct->setEnabled(scene && scene->selectedItems().count() > 0);
//    groupAct->setEnabled( scene && scene->selectedItems().count() > 0);
//    unGroupAct->setEnabled(scene &&scene->selectedItems().count() > 0 &&
//                           dynamic_cast<GraphicsItemGroup*>( scene->selectedItems().first()));

//    leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    rightAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    vCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    hCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    upAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    downAct->setEnabled(scene && scene->selectedItems().count() > 1);

//    heightAct->setEnabled(scene && scene->selectedItems().count() > 1);
//    widthAct->setEnabled(scene &&scene->selectedItems().count() > 1);
//    allAct->setEnabled(scene &&scene->selectedItems().count()>1);
//    horzAct->setEnabled(scene &&scene->selectedItems().count() > 2);
//    vertAct->setEnabled(scene &&scene->selectedItems().count() > 2);

//    copyAct->setEnabled(scene &&scene->selectedItems().count() > 0);
//    cutAct->setEnabled(scene &&scene->selectedItems().count() > 0);
//}

void MainWindow::updateActions()
{
    QGraphicsScene * scene =nullptr;
    if (activeMdiChild())
        scene =activeMdiChild()->scene();
    if(m_isSelected_servo_type)
    {
        //    rectAct->setEnabled(false);
        //    roundRectAct->setEnabled(false);
        //    ellipseAct->setEnabled(false);
        //    polygonAct->setEnabled(false);
        selectAct->setEnabled(scene);
        lineAct->setEnabled(scene);
        bezierAct->setEnabled(scene);
        rotateAct->setEnabled(scene);
        polylineAct->setEnabled(scene);
        zoomInAct->setEnabled(scene);
        zoomOutAct->setEnabled(scene);

        selectAct->setChecked(DrawTool::c_drawShape == selection);
        lineAct->setChecked(DrawTool::c_drawShape == line);
        //    rectAct->setChecked(DrawTool::c_drawShape == rectangle);
        //    roundRectAct->setChecked(DrawTool::c_drawShape == roundrect);
        //    ellipseAct->setChecked(DrawTool::c_drawShape == ellipse);
        bezierAct->setChecked(DrawTool::c_drawShape == bezier);
        rotateAct->setChecked(DrawTool::c_drawShape == rotation);
        //    polygonAct->setChecked(DrawTool::c_drawShape == polygon);
        polylineAct->setChecked(DrawTool::c_drawShape == polyline );
        undoAct->setEnabled(undoStack->canUndo());
        redoAct->setEnabled(undoStack->canRedo());


        bringToFrontAct->setEnabled(scene && scene->selectedItems().count() > 0);
        sendToBackAct->setEnabled(scene && scene->selectedItems().count() > 0);
        groupAct->setEnabled( scene && scene->selectedItems().count() > 0);
        unGroupAct->setEnabled(scene &&scene->selectedItems().count() > 0 &&
                               dynamic_cast<GraphicsItemGroup*>( scene->selectedItems().first()));

        leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
        rightAct->setEnabled(scene && scene->selectedItems().count() > 1);
        leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
        vCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
        hCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
        upAct->setEnabled(scene && scene->selectedItems().count() > 1);
        downAct->setEnabled(scene && scene->selectedItems().count() > 1);

        heightAct->setEnabled(scene && scene->selectedItems().count() > 1);
        widthAct->setEnabled(scene &&scene->selectedItems().count() > 1);
        allAct->setEnabled(scene &&scene->selectedItems().count()>1);
        horzAct->setEnabled(scene &&scene->selectedItems().count() > 2);
        vertAct->setEnabled(scene &&scene->selectedItems().count() > 2);

        copyAct->setEnabled(scene &&scene->selectedItems().count() > 0);
        cutAct->setEnabled(scene &&scene->selectedItems().count() > 0);
    }else
    {
        selectAct->setEnabled(false);
        lineAct->setEnabled(false);
        bezierAct->setEnabled(false);
        rotateAct->setEnabled(false);
        polylineAct->setEnabled(false);
        zoomInAct->setEnabled(false);
        zoomOutAct->setEnabled(false);

        selectAct->setChecked(DrawTool::c_drawShape == selection);
        lineAct->setChecked(DrawTool::c_drawShape == line);
        //    rectAct->setChecked(DrawTool::c_drawShape == rectangle);
        //    roundRectAct->setChecked(DrawTool::c_drawShape == roundrect);
        //    ellipseAct->setChecked(DrawTool::c_drawShape == ellipse);
        bezierAct->setChecked(DrawTool::c_drawShape == bezier);
        rotateAct->setChecked(DrawTool::c_drawShape == rotation);
        //    polygonAct->setChecked(DrawTool::c_drawShape == polygon);
        polylineAct->setChecked(DrawTool::c_drawShape == polyline );
        undoAct->setEnabled(undoStack->canUndo());
        redoAct->setEnabled(undoStack->canRedo());


        bringToFrontAct->setEnabled(scene && scene->selectedItems().count() > 0);
        sendToBackAct->setEnabled(scene && scene->selectedItems().count() > 0);
        groupAct->setEnabled( scene && scene->selectedItems().count() > 0);
        unGroupAct->setEnabled(scene &&scene->selectedItems().count() > 0 &&
                               dynamic_cast<GraphicsItemGroup*>( scene->selectedItems().first()));

        leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
        rightAct->setEnabled(scene && scene->selectedItems().count() > 1);
        leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
        vCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
        hCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
        upAct->setEnabled(scene && scene->selectedItems().count() > 1);
        downAct->setEnabled(scene && scene->selectedItems().count() > 1);

        heightAct->setEnabled(scene && scene->selectedItems().count() > 1);
        widthAct->setEnabled(scene &&scene->selectedItems().count() > 1);
        allAct->setEnabled(scene &&scene->selectedItems().count()>1);
        horzAct->setEnabled(scene &&scene->selectedItems().count() > 2);
        vertAct->setEnabled(scene &&scene->selectedItems().count() > 2);
        copyAct->setEnabled(scene &&scene->selectedItems().count() > 0);
        cutAct->setEnabled(scene &&scene->selectedItems().count() > 0);

    }
}

//void MainWindow::itemSelected()
//{
//    if (!activeMdiChild()) return ;
//    QGraphicsScene * scene = activeMdiChild()->scene();
//    if ( scene->selectedItems().count() > 0
//         && scene->selectedItems().first()->isSelected())//为什么要选择第一个?难道只是为了设置按照z序列设置第一个对象的属性吗?
//    {
//        item=(GraphicsItem*)scene->selectedItems().first();
//        theControlledObject = dynamic_cast<QObject*>(item);
//        propertyEditor->setObject(theControlledObject);
//        qDebug()<<"[ACTION_ID]:"<<item->m_actionid<<endl;
//        int row_num=tableWidget->rowCount();

//        for(int i=0;i<row_num;++i)
//        {
//            for(int j=0;j<scene->selectedItems().size();++j)
//            {
//                QTableWidgetItem *it= tableWidget->item(i,0);
//                if(it->data(0).toString().isEmpty()!=true&&it->data(0).toString()==((GraphicsItem*)scene->selectedItems().at(j))->m_actionid)
//                {
//                    qDebug()<<"ok:"<<endl;
//                    tableWidget->selectRow(i);
//                }
//            }
//        }
//    }
//    return ;
//}
void MainWindow::get_item_start_end(GraphicsItem* item)
{
    static int cnt=0;
    QPointF start,end;
    GraphicsPolygonItem* it = qgraphicsitem_cast<GraphicsPolygonItem*>(item);
    if(it->m_points.size()<2)
        return;
    start=it->mapToScene(it->m_points[0]);
    end=it->mapToScene(it->m_points[it->m_points.size()-1]);
    qDebug("start pos on scene:[X:%f,Y:%f]\n",start.x(),start.y());
    qDebug("end pos on scene:[X:%f,Y:%f]\n",end.x(),end.y());
    cnt++;
    if(cnt==1)//pre item ,get end point
    {
        m_fill_start=end;//if first item has been selectd ,save it's endpoint as the new item's start point;
    }
    if(cnt==2)
    {
        m_fill_end=start;//if second item,save it's start point as the new item's endpoint;
        qDebug("### 2 times");
        QVector<QPointF> temp;
        temp.clear();
        temp.append(m_fill_start);
        temp.append(m_fill_end);
        emit sig_fill_data_to_scene(temp);
        temp.clear();
        m_isFill=false;
        cnt=0;
    }
}
//void MainWindow::itemSelected()
//{
//    if (!activeMdiChild()) return ;
//    QGraphicsScene * scene = activeMdiChild()->scene();
//    if ( scene->selectedItems().count() > 0
//         && scene->selectedItems().first()->isSelected())//为什么要选择第一个?难道只是为了设置按照z序列设置第一个对象的属性吗?
//    {
//        item=(GraphicsItem*)scene->selectedItems().first();
//        //        QPen pen(Qt::blue,Qt::DotLine);
//        //        item->setPen(pen);
//        if(m_isFill)
//            get_item_start_end(item);//get item's limits;
//        theControlledObject = dynamic_cast<QObject*>(item);
//        propertyEditor->setObject(theControlledObject);
//        qDebug()<<"[ACTION_ID]:"<<item->m_actionid<<endl;
//        int row_num=tableWidget->rowCount();
//        for(int i=0;i<row_num;++i)
//        {
//            for(int j=0;j<scene->selectedItems().size();++j)
//            {
//                QTableWidgetItem *it= tableWidget->item(i,0);
//                if(it->data(0).toString().isEmpty()!=true&&it->data(0).toString()==((GraphicsItem*)scene->selectedItems().at(j))->m_actionid)
//                {
//                    qDebug()<<"ok:"<<endl;
//                    tableWidget->selectRow(i);
//                }
//            }
//        }
//    }
//    return ;
//}

//void MainWindow::itemSelected()
//{
//    if (!activeMdiChild()) return ;
//    QGraphicsScene * scene = activeMdiChild()->scene();

//    if ( scene->selectedItems().count() > 0
//         && scene->selectedItems().first()->isSelected())
//    {
//        QGraphicsItem *item = scene->selectedItems().first();

//        theControlledObject = dynamic_cast<QObject*>(item);
//        propertyEditor->setObject(theControlledObject);


//    }
//    return ;
//    if ( theControlledObject )
//    {
//        propertyEditor->setObject(theControlledObject);
//    }
//}


//void MainWindow::itemSelected() //20190714
//{
//    if (!activeMdiChild()) return ;
//    QGraphicsScene * scene = activeMdiChild()->scene();
//    if ( scene->selectedItems().count() > 0
//         && scene->selectedItems().first()->isSelected())//为什么要选择第一个?难道只是为了设置按照z序列设置第一个对象的属性吗?
//    {
//        item=(GraphicsItem*)scene->selectedItems().first();
//        //        QPen pen(Qt::blue,Qt::DotLine);
//        //        item->setPen(pen);
//        if(m_isFill)
//            get_item_start_end(item);//get item's limits;
//        theControlledObject = dynamic_cast<QObject*>(item);
//        propertyEditor->setObject(theControlledObject);//增加属性编辑功能

//        qDebug()<<"[ACTION_ID]:"<<item->m_servoid<<endl;


//    }
//    return ;
//}
void MainWindow::itemSelected()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    if ( scene->selectedItems().count() > 0
         && scene->selectedItems().first()->isSelected())//为什么要选择第一个?难道只是为了设置按照z序列设置第一个对象的属性吗?
    {
        item=(GraphicsItem*)scene->selectedItems().first();
        //        QPen pen(Qt::blue,Qt::DotLine);
        //        item->setPen(pen);
        if(m_isFill)
            get_item_start_end(item);//get item's limits;
        theControlledObject = dynamic_cast<QObject*>(item);
        propertyEditor->setObject(theControlledObject);//增加属性编辑功能
        qDebug()<<"[ACTION_ID]:"<<item->m_servoid<<endl;
        QString name=get_servo_name_by_id(item->m_servoid.toInt());
        for(auto btn:this->btn_actions)
        {

            if( btn->text()==name)
            {
                btn->setEnabled(true);
                btn->setStyleSheet("QPushButton{background-color:orange; color: white; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                   "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
            }/*else
            {
                QString st;
                for(auto ma:get_scene()->m_save_ag->m_actions)
                {
                    if(ma!=nullptr)
                    {
                        st= get_servo_name_by_id(ma->getId());
                        if(btn->text()==st)
                        {
                            btn->setStyleSheet("QPushButton{background-color:green; color: white; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                               "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
                        }
                    }
                }
            }*/
        }
    }
    return ;
}
QString  MainWindow::get_servo_name_by_id(int id)
{
    if(id==0) return QString(tr(" "));
    for(int i=0;i<m_list_servoinfo.count();++i)
    {
        if(id==m_list_servoinfo[i]->get_id())
        {
            return m_list_servoinfo[i]->get_name();
        }
    }
    return QString(tr(" "));
}
//void MainWindow::itemMoved(QGraphicsItem *item, const QPointF &oldPosition)
//{
//    Q_UNUSED(item);
//    if (!activeMdiChild()) return ;
//        activeMdiChild()->setModified(true);

//    if ( item ){
//        QUndoCommand *moveCommand = new MoveShapeCommand(item, oldPosition);
//        undoStack->push(moveCommand);
//    }else{
//        QUndoCommand *moveCommand = new MoveShapeCommand(activeMdiChild()->scene(), oldPosition);
//        undoStack->push(moveCommand);
//    }
//}
void MainWindow::itemMoved(QGraphicsItem *item, const QPointF &oldPosition)
{
    //    Q_UNUSED(item);
    GraphicsBezier* itemx=qgraphicsitem_cast<GraphicsBezier*>(item);
    for(int i=0;i<itemx->m_points.count();++i)
    {
        qDebug(" bef:m_point[%d],x=%.2f,y=%.2f\n",i,itemx->m_points[i].x(),itemx->m_points[i].y());
    }
    if (!activeMdiChild()) return ;
    activeMdiChild()->setModified(true);

    if ( item ){
        QUndoCommand *moveCommand = new MoveShapeCommand(item, oldPosition);
        undoStack->push(moveCommand);
    }else{
        QUndoCommand *moveCommand = new MoveShapeCommand(activeMdiChild()->scene(), oldPosition);
        undoStack->push(moveCommand);
    }
    for(int i=0;i<itemx->m_points.count();++i)
    {
        qDebug(" after:m_point[%d],x=%.2f,y=%.2f\n",i,itemx->m_points[i].x(),itemx->m_points[i].y());
    }
}


//void MainWindow::itemMoved(QGraphicsItem *item, const QPointF &oldPosition)
//{
//    Q_UNUSED(item);
//    if (!activeMdiChild()) return ;
//    activeMdiChild()->setModified(true);

//    if ( item ){
//        QUndoCommand *moveCommand = new MoveShapeCommand(item, oldPosition);
//        undoStack->push(moveCommand);
//        ( (DrawScene*) ( activeMdiChild()->scene()))->update();
//    }else{
//        QUndoCommand *moveCommand = new MoveShapeCommand(activeMdiChild()->scene(), oldPosition);
//        undoStack->push(moveCommand);
//        ( (DrawScene*) ( activeMdiChild()->scene()))->update();
//    }
//}
void MainWindow::addActionID()
{
    dlg=new QDialog();
    dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    dlg->resize(200,100);
    dlg->setWindowTitle(tr("set parameter"));
    dlg->setModal(true);
    dlg->move(rect().width()/2,rect().height()/2);
    QGridLayout *layout=new QGridLayout();
    QLabel *l_actionID=new QLabel(tr("动作组ID:"));
    le_actionID=new QLineEdit();
    le_actionID->setMaximumWidth(100);
    QLabel*l_servoID=new QLabel(tr("舵    机ID:"));
    le_servoID=new QLineEdit();
    le_servoID->setMaximumWidth(100);
    QPushButton *btn_ok=new QPushButton(tr("确定"));
    btn_ok->setMaximumHeight(40);
    btn_ok->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    connect(btn_ok,SIGNAL(clicked(bool)),this,SLOT(slot_inputParameter()));
    layout->addWidget(l_actionID,0,0);
    layout->addWidget(le_actionID,0,1);
    layout->addWidget(l_servoID,1,0);
    layout->addWidget(le_servoID,1,1);
    layout->addWidget(btn_ok,2,0,1,2);
    dlg->setLayout(layout);
    dlg->show();
}

void MainWindow::itemAdded(QGraphicsItem *item)
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->setModified(true);
    QUndoCommand *addCommand = new AddShapeCommand(item, item->scene());
    undoStack->push(addCommand);

}
void MainWindow::slot_inputParameter()
{
    if(le_actionID->text().isEmpty()||le_servoID->text().isEmpty())
    {
        dlg->setWindowTitle(tr("请指定参数!"));
    }else
    {
        if(this->item!=nullptr)
        {
            this->item->m_actionid=this->le_actionID->text();//current
            this->item->m_servoid=this->le_servoID->text();
            qDebug()<<"[writed actid:]"<<item->m_actionid;
            qDebug()<<"[writed servoid:]"<<item->m_servoid;
        }
        dlg->close();
        if(dlg!=nullptr)
            delete dlg;
    }

}
void MainWindow::itemRotate(QGraphicsItem *item, const qreal oldAngle)
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->setModified(true);

    QUndoCommand *rotateCommand = new RotateShapeCommand(item , oldAngle);
    undoStack->push(rotateCommand);
}

void MainWindow::itemResize(QGraphicsItem *item, int handle, const QPointF& scale)
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->setModified(true);

    QUndoCommand *resizeCommand = new ResizeShapeCommand(item ,handle, scale );
    undoStack->push(resizeCommand);
}

void MainWindow::itemControl(QGraphicsItem *item, int handle, const QPointF & newPos ,const QPointF &lastPos_)
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->setModified(true);

    QUndoCommand *controlCommand = new ControlShapeCommand(item ,handle, newPos, lastPos_ );
    undoStack->push(controlCommand);
}

void MainWindow::deleteItem()
{
    qDebug()<<"deleteItem";
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    activeMdiChild()->setModified(true);

    if (scene->selectedItems().isEmpty())
        return;
    QUndoCommand *deleteCommand = new RemoveShapeCommand(scene);
    undoStack->push(deleteCommand);
    scene->update();

}

void MainWindow::on_actionBringToFront_triggered()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    if (scene->selectedItems().isEmpty())
        return;
    activeMdiChild()->setModified(true);

    QGraphicsItem *selectedItem = scene->selectedItems().first();

    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();
    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() >= zValue && item->type() == GraphicsItem::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);


}
void MainWindow::on_actionSendToBack_triggered()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    if (scene->selectedItems().isEmpty())
        return;

    activeMdiChild()->setModified(true);

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() <= zValue && item->type() == GraphicsItem::Type)
            zValue = item->zValue() - 0.1;

    }
    selectedItem->setZValue(zValue);
}

void MainWindow::on_aglin_triggered()
{
    if (!activeMdiChild()) return ;
    DrawScene * scene =dynamic_cast<DrawScene*>(activeMdiChild()->scene());

    activeMdiChild()->setModified(true);

    if ( sender() == rightAct ){
        scene->align(RIGHT_ALIGN);
    }else if ( sender() == leftAct){
        scene->align(LEFT_ALIGN);
    }else if ( sender() == upAct ){
        scene->align(UP_ALIGN);
    }else if ( sender() == downAct ){
        scene->align(DOWN_ALIGN);
    }else if ( sender() == vCenterAct ){
        scene->align(VERT_ALIGN);
    }else if ( sender() == hCenterAct){
        scene->align(HORZ_ALIGN);
    }else if ( sender() == heightAct )
        scene->align(HEIGHT_ALIGN);
    else if ( sender()==widthAct )
        scene->align(WIDTH_ALIGN);
    else if ( sender() == horzAct )
        scene->align(HORZEVEN_ALIGN);
    else if ( sender() == vertAct )
        scene->align(VERTEVEN_ALIGN);
    else if ( sender () == allAct )
        scene->align(ALL_ALIGN);
}

void MainWindow::zoomIn()
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->zoomIn();
}

void MainWindow::zoomOut()
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->zoomOut();
}

void MainWindow::on_group_triggered()
{
    if (!activeMdiChild()) return ;
    DrawScene * scene = dynamic_cast<DrawScene*>(activeMdiChild()->scene());

    //QGraphicsItemGroup
    QList<QGraphicsItem *> selectedItems = scene->selectedItems();
    // Create a new group at that level
    if ( selectedItems.count() < 1) return;
    GraphicsItemGroup *group = scene->createGroup(selectedItems);
    ////////////////

    //   int name= (group->childItems().first())->type();
    //   qDebug()<<"name:"<<name;
    //    group->setEnabled(false);
    //    group->hide();
    ///////////////
    QUndoCommand *groupCommand = new GroupShapeCommand(group,scene);
    undoStack->push(groupCommand);
}

void MainWindow::on_unGroup_triggered()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    GraphicsItemGroup * group = dynamic_cast<GraphicsItemGroup*>(selectedItem);
    if ( group ){
        QUndoCommand *unGroupCommand = new UnGroupShapeCommand(group,scene);
        undoStack->push(unGroupCommand);
    }
}

void MainWindow::on_func_test_triggered()
{
    //    QtPenPropertyManager *penPropertyManager = new QtPenPropertyManager();
    //    QtProperty * property = penPropertyManager->addProperty("pen");
    //    QtTreePropertyBrowser *editor = new QtTreePropertyBrowser();
    //    editor->setFactoryForManager(penPropertyManager->subIntPropertyManager(),new QtSpinBoxFactory());
    //    editor->setFactoryForManager(penPropertyManager->subEnumPropertyManager(),new QtEnumEditorFactory());
    //    editor->addProperty(property);

    //    QPen pen;
    //    pen.setWidth(10);
    //    pen.setCapStyle(Qt::RoundCap);
    //    pen.setJoinStyle(Qt::SvgMiterJoin);
    //    penPropertyManager->setValue(property,pen);
    //    editor->show();
    //    QtGradientEditor * editor1 = new QtGradientEditor(nullptr);
    //    editor1->show();

    QtTreePropertyBrowser* tree=new QtTreePropertyBrowser();
    QtVariantPropertyManager *m_pVarManager=new QtVariantPropertyManager(tree);
    QtVariantEditorFactory *m_pVarFactory=new QtVariantEditorFactory(tree);
    QtProperty *groupItem = m_pVarManager->addProperty(QtVariantPropertyManager::groupTypeId(),QStringLiteral("Group1"));

    QtVariantProperty *item = m_pVarManager->addProperty(QVariant::Int, QStringLiteral("Int: "));
    item->setValue(100);
    groupItem->addSubProperty(item);

    item = m_pVarManager->addProperty(QVariant::Bool,QStringLiteral("Bool: "));
    item->setValue(true);
    groupItem->addSubProperty(item);

    item = m_pVarManager->addProperty(QVariant::Double,QStringLiteral("Double: "));
    item->setValue(3.14);
    groupItem->addSubProperty(item);

    item = m_pVarManager->addProperty(QVariant::String,QStringLiteral("String: "));
    item->setValue(QStringLiteral("hello world"));
    groupItem->addSubProperty(item);

    tree->addProperty(groupItem);
    tree->setFactoryForManager(m_pVarManager,m_pVarFactory);
    tree->show();
    tree->move(rect().width()/2,rect().height()/2);
    dockProperty->showNormal();
}

void MainWindow::on_copy()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    ShapeMimeData * data = new ShapeMimeData( scene->selectedItems() );
    QApplication::clipboard()->setMimeData(data);
}

void MainWindow::on_paste()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    addActionID();
    QMimeData * mp = const_cast<QMimeData *>(QApplication::clipboard()->mimeData()) ;
    ShapeMimeData * data = dynamic_cast< ShapeMimeData*>( mp );
    if ( data ){
        scene->clearSelection();
        foreach (QGraphicsItem * item , data->items() ) {
            AbstractShape *sp = qgraphicsitem_cast<AbstractShape*>(item);
            QGraphicsItem * copy = sp->duplicate();
            if ( copy ){
                copy->setSelected(true);
                copy->moveBy(100,100);
                QUndoCommand *addCommand = new AddShapeCommand(copy, scene);
                undoStack->push(addCommand);
            }
        }
    }
}

void MainWindow::on_cut()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    QList<QGraphicsItem *> copylist ;
    foreach (QGraphicsItem *item , scene->selectedItems()) {
        AbstractShape *sp = qgraphicsitem_cast<AbstractShape*>(item);
        QGraphicsItem * copy = sp->duplicate();
        if ( copy )
            copylist.append(copy);
    }
    QUndoCommand *deleteCommand = new RemoveShapeCommand(scene);
    undoStack->push(deleteCommand);
    if ( copylist.count() > 0 ){
        ShapeMimeData * data = new ShapeMimeData( copylist );
        QApplication::clipboard()->setMimeData(data);
    }
}

void MainWindow::dataChanged()
{
    pasteAct->setEnabled(true);
}

void MainWindow::positionChanged(int x, int y)
{
    char buf[255];
    sprintf(buf,"%d,%d",x,y);
    statusBar()->showMessage(buf);
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
    //      window->setWindowFlags(window->windowFlags() ^ Qt::FramelessWindowHint);
}
void MainWindow::slt_request_play_all()
{
    //    if(play->isRunning())
    //    {
    //        qDebug("play thread is runniing!");
    //        play->terminate();
    //    }
    play->start();
    qDebug("[will play all!]");
    QString cmd="play_all";
    emit sig_play_all(cmd);
    ////////////
    m_pos=new QTimer(this);
    m_pos->setTimerType(Qt::PreciseTimer);
    connect(m_pos,SIGNAL(timeout()),this,SLOT(show_play_pos()));
    m_pos->start(20);
}
void MainWindow::slt_request_play()
{
    //    play->start();
    //    qDebug("[will play!]");
    //    QString filename= QFileDialog::getOpenFileName();
    //    if(filename.isEmpty())
    //    {
    //        printf("[file is empty!]");
    //        return;
    //    }
    //    qDebug("[send url]:%s\n",filename.toLatin1().data());
    //    emit sig_play(filename.toLatin1().data());
    //    play->terminate();
}
void MainWindow::slt_request_load()
{
    play->start();
    QString filename= QFileDialog::getOpenFileName();
    if(filename.isEmpty())
    {
        printf("[file is empty!]");
        return;
    }
    qDebug("[send url]:%s\n",filename.toLatin1().data());
    m_audio_src=filename;
    emit sig_load(filename.toLatin1().data(),tr("load"));
}
void MainWindow::slt_reload_audio()
{
    if(m_audio_src.isEmpty()) return;
    emit sig_load(m_audio_src.toLatin1().data(),tr("load"));
}
void MainWindow::about()
{
    QtTreePropertyBrowser* tree=new QtTreePropertyBrowser();
    QtVariantPropertyManager *m_pVarManager=new QtVariantPropertyManager(tree);
    QtVariantEditorFactory *m_pVarFactory=new QtVariantEditorFactory(tree);
    QtVariantProperty *item = m_pVarManager->addProperty(QVariant::String, QStringLiteral("程序名称: "));
    item->setValue("高仿真机器人控制台");
    tree->addProperty(item);
    item = m_pVarManager->addProperty(QVariant::String,QStringLiteral("版本: "));
    item->setValue("1.0.0.1");
    tree->addProperty(item);
    item = m_pVarManager->addProperty(QVariant::String,QStringLiteral("作者: "));
    item->setValue("王拴阁");
    tree->addProperty(item);
    item = m_pVarManager->addProperty(QVariant::String,QStringLiteral("所有权: "));
    item->setValue(QStringLiteral("上海惊鸿机器人有限公司"));
    tree->addProperty(item);
    tree->setFactoryForManager(m_pVarManager,m_pVarFactory);
    tree->show();
    tree->move(rect().width()/2,rect().height()/2);
    dockProperty->showNormal();
}
