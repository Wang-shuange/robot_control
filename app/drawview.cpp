#include "drawview.h"
#include "drawscene.h"
#include <QSvgGenerator>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
extern double  TIME_MAX; //时间的最大刻度值
extern const double ANGLE_MAX;
extern const double  SCENE_WIDTH;
extern const double  SCENE_HEIGHT;
DrawView::DrawView(QGraphicsScene *scene)
    :QGraphicsView(scene)
{
    m_hruler = new QtRuleBar(Qt::Horizontal,this,this);
    m_vruler = new QtRuleBar(Qt::Vertical,this,this);
    box = new QtCornerBox(this);
    setViewport(new QWidget);
    setAttribute(Qt::WA_DeleteOnClose);
    isUntitled = true;
    modified = false;
    m_dur=0;
    this->m_parent_window=nullptr;
    //    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    centerOn(0,0);
    //    verticalScrollBar()->setSliderPosition(0);

    //    horizontalScrollBar()->setSliderPosition(0);
}

DrawView::~DrawView()
{
//    if(m_parent_window)
//    {
//        if(m_parent_window->play)
//        {
//            if(m_parent_window->play->isRunning())
//            {
//                m_parent_window->play->terminate();
//            }
//            delete m_parent_window->play;
//            m_parent_window->play=nullptr;
//        }
//    }
}
void DrawView::slt_receive_duration(quint64 dur)
{
    m_dur=dur;
}
void DrawView::zoomIn()
{
    scale(1.2,1.2);
    updateRuler();
}
//void DrawView::zoomIn()
//{
//    scale(1.2,1.2);
//    updateRuler();
//}

void DrawView::zoomOut()
{
    scale(1 / 1.2, 1 / 1.2);
    updateRuler();
}
//void DrawView::zoomOut()
//{
//    scale(1 / 1.2, 1 / 1.2);
//    updateRuler();
//}
void DrawView::newFile()
{
    isUntitled = true;
    curFile = tr(" scene%1.xml").arg(1);
    setWindowTitle(curFile + "[*]");

}

//void DrawView::newFile() //20190714
//{
//    static int sequenceNumber = 1;
//    isUntitled = true;
//    curFile = tr(" scene%1.xml").arg(sequenceNumber++);
//    setWindowTitle(curFile + "[*]");

//}

QStringList  DrawView::getAttribute(QXmlStreamReader& xml,QString element,QString attribute)
{
    QStringList temp;
    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;
        if(token == QXmlStreamReader::StartElement)
        {
            if(xml.name() == element)
            {

                QXmlStreamAttributes attributes = xml.attributes();
                if(attributes.hasAttribute(attribute))
                {
                    temp.append(attributes.value(attribute).toString());
                }
            }
        }
    }


    return  temp;
}

//// 重写closeEvent: 确认退出对话框
//void DrawView::closeEvent(QCloseEvent *event)
//{
//    QMessageBox::StandardButton button;
//    button=QMessageBox::question(this,tr("退出程序"),QString(tr("确认退出程序")),QMessageBox::Yes|QMessageBox::No);
//    if(button==QMessageBox::No)
//    {
//        event->ignore(); // 忽略退出信号，程序继续进行
//    }
//    else if(button==QMessageBox::Yes)
//    {
//        event->accept(); // 接受退出信号，程序退出
//    }
//}


QList<QStringList>  DrawView::getAttributes_ex(QXmlStreamReader& xml,QString element,QString actid,QString actid_value,QString attribute)
{
    QList<QStringList> t;
    QStringList actionids,servoids,angles,times;
    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;
        if(token == QXmlStreamReader::StartElement)
        {
            if(xml.name() == element)
            {
                QXmlStreamAttributes attributes = xml.attributes();

                if(attributes.value(actid)==actid_value)
                {
                    QString actionid=xml.attributes().value("actionGroupID").toString();
                    //                    qDebug()<<"[actionid]:"<<actionid<<endl;
                    actionids.append(actionid);
                    QString servoid=xml.attributes().value("servoID").toString();
                    //                    qDebug()<<"[servoid]:"<<servoid<<endl;
                    servoids.append(servoid);

                    QString angle=xml.attributes().value("angle").toString();
                    //                    qDebug()<<"[angle]:"<<angle<<endl;
                    angles.append(angle);
                    QString time=xml.attributes().value("time").toString();
                    //                    qDebug()<<"[time]:"<<time<<endl;
                    times.append(time);
                    t.clear();
                    t.push_back(actionids);
                    t.push_back(servoids);
                    t.push_back(times);
                    t.push_back(angles);
                }

            }
        }
    }
    return  t;
}

QList<QStringList> DrawView::getparameter(QString fileName,QString element,QString actid,QString actid_value,QString attr)
{
    QList<QStringList> t;
    QFile f(fileName);
    f.open(QFile::ReadOnly|QFile::Text);
    QXmlStreamReader xml(&f);
    t= getAttributes_ex(xml,element,actid,actid_value,attr);
    f.close();
    return t;
}


QStringList DrawView::sendInfo2Table(QString fileName,QString element,QString attr)
{
    QStringList temp;
    QFile f(fileName);
    f.open(QFile::ReadOnly|QFile::Text);
    QXmlStreamReader xml(&f);
    temp=getAttribute(xml,element,attr);
    f.close();
    return temp;
}

void DrawView::selectedByTable()
{

    //    qDebug()<<"hello select!"<<endl;
}

bool DrawView::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("action loader"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }
    QXmlStreamReader xml(&file);
    if (xml.readNextStartElement()) {
        if ( xml.name() == tr("canvas"))
        {
            int width = xml.attributes().value(tr("width")).toInt();
            int height = xml.attributes().value(tr("height")).toInt();
            scene()->setSceneRect(0,0,width,height);
            loadCanvas(&xml);
        }
    }
    setCurrentFile(fileName);
    qDebug()<<xml.errorString();
    return !xml.error();
}
bool DrawView::loadFile_ex(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("action loader"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }
    QXmlStreamReader xml(&file);
    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
    MotorAction* l_ma=nullptr;
    scene->m_save_ag->m_actions.clear();
    if (xml.readNextStartElement()) //root element
    {
        Q_ASSERT(xml.isStartElement() && xml.name() == "ActionGroup");
        if ( xml.name() == tr("ActionGroup"))
        {
            while(xml.readNextStartElement())
            {
                if(xml.name()==tr("FileMusic"))
                {
                    scene->m_save_ag->setMusic_src(xml.attributes().value(tr("src")).toString());
                    xml.skipCurrentElement();
                }else if(xml.name()==tr("motorAction"))
                {
                    l_ma=new MotorAction(this);
                    l_ma->m_items.clear();
                    l_ma->m_trace.clear();
                    l_ma->speed.clear();
                    int id= xml.attributes().value(tr("id")).toInt();
                    l_ma->setId(id);
                    QPointF temp;
                    double x,y;
                    while(xml.readNextStartElement())
                    {
                        if(xml.name()==tr("mPosition"))
                        {
                            x=xml.attributes().value(tr("X")).toDouble();
                            y=xml.attributes().value(tr("Y")).toDouble();
                            temp.setX(x);
                            temp.setY(y);
                            l_ma->m_trace.append(temp);
                            xml.skipCurrentElement();
                        }
                    }
                    l_ma->handle_speed();
                    int sz_ag=scene->m_save_ag->m_actions.count();
                    for(int n=0;n<sz_ag;++n)
                    {
                        if(scene->m_save_ag->m_actions[n]->getId()==l_ma->getId())
                            scene->m_save_ag->m_actions.removeAt(n);
                    }
                    scene->m_save_ag->m_actions.append((l_ma));
                    m_parent_window->slt_check_action_status(false);

                    for(int i=0;i<MAX_ACTION_COUNT;++i)
                    {
                        m_parent_window->btn_actions[i]->setFlat(true);
                        m_parent_window->btn_actions[i]->setStyleSheet("QPushButton{background-color:rgba(12,96,db,0); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                                                       "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
                        m_parent_window->btn_hides[i]->setFlat(true);
                        m_parent_window->btn_locks[i]->setFlat(true);
                        m_parent_window->btn_stretchs[i]->setFlat(true);
                    }
                    int i=0;
                    for(auto ma:scene->m_save_ag->m_actions)
                    {
                        for(auto btn:m_parent_window->btn_actions)
                        {
                            for(auto servo:m_parent_window->m_list_servoinfo)
                            {
                                if(servo->get_id()==ma->getId())
                                {
                                    if(servo->get_name()==btn->text())
                                    {
                                        btn->setEnabled(true);

                                        btn->setStyleSheet("QPushButton{background-color:rgba(0,255,0,1); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
                                                           "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
                                    }
                                }
                            }

                        }
                    }
                    for(int i=0;i<MAX_ACTION_COUNT;++i)
                    {
                        if(m_parent_window->btn_actions[i]->isEnabled())
                        {
                            m_parent_window->btn_hides[i]->setEnabled(true);
                            m_parent_window->btn_locks[i]->setEnabled(true);
                            m_parent_window->btn_stretchs[i]->setEnabled(true);
                        }
                    }
                }else if(xml.name()==tr("CarAction"))
                {
                    xml.skipCurrentElement();
                }else
                {
                    xml.skipCurrentElement();
                }
            }
        }
    }
    //    qDebug()<<xml.errorString();
    return !xml.error();
}
//bool DrawView::loadFile_ex(const QString &fileName)
//{
//    QFile file(fileName);
//    if (!file.open(QFile::ReadOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr("action loader"),
//                             tr("Cannot read file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamReader xml(&file);
//    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
//    MotorAction* l_ma=nullptr;
//    scene->m_load_ag->m_actions.clear();
//    if (xml.readNextStartElement()) //root element
//    {
//        Q_ASSERT(xml.isStartElement() && xml.name() == "ActionGroup");
//        if ( xml.name() == tr("ActionGroup"))
//        {
//            while(xml.readNextStartElement())
//            {
//                if(xml.name()==tr("FileMusic"))
//                {
//                    scene->m_load_ag->setMusic_src(xml.attributes().value(tr("src")).toString());
//                    xml.skipCurrentElement();
//                }else if(xml.name()==tr("motorAction"))
//                {
//                    l_ma=new MotorAction(this);
//                    l_ma->m_items.clear();
//                    l_ma->m_trace.clear();
//                    int id= xml.attributes().value(tr("id")).toInt();
//                    l_ma->setId(id);
//                    QPointF temp;
//                    double x,y;
//                    while(xml.readNextStartElement())
//                    {
//                        if(xml.name()==tr("mPosition"))
//                        {
//                            x=xml.attributes().value(tr("X")).toDouble();
//                            y=xml.attributes().value(tr("Y")).toDouble();
//                            temp.setX(x);
//                            temp.setY(y);
//                            l_ma->m_trace.append(temp);
//                            xml.skipCurrentElement();
//                        }
//                    }
//                    l_ma->handle_speed();
//                    scene->m_load_ag->m_actions.append((l_ma));
//                    //                    ///////////////////
//                    /// \brief i
//                        for(int i=0;i<MAX_ACTION_COUNT;++i)
//                        {

//                            m_parent_window->btn_actions[i]->setFlat(true);
//                              m_parent_window->btn_actions[i]->setStyleSheet("QPushButton{background-color:rgba(12,96,db,0); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
//                                                          "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
//                              m_parent_window->btn_hides[i]->setFlat(true);
//                              m_parent_window->btn_locks[i]->setFlat(true);
//                              m_parent_window->btn_actions[i]->setEnabled(false);
//                              m_parent_window->btn_hides[i]->setEnabled(false);
//                              m_parent_window->btn_locks[i]->setEnabled(false);
//                        }
//                    int i=0;
//                    for(auto ma:scene->m_load_ag->m_actions)
//                    {
//                        for(auto btn:m_parent_window->btn_actions)
//                        {
//                            for(auto servo:m_parent_window->m_list_servoinfo)
//                            {
//                                if(servo->get_id()==ma->getId())
//                                {
//                                    if(servo->get_name()==btn->text())
//                                    {
//                                        btn->setEnabled(true);

//                                        btn->setStyleSheet("QPushButton{background-color:rgba(0,255,0,1); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
//                                                           "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
//                                    }
//                                }
//                            }

//                        }
//                    }
//                    for(int i=0;i<MAX_ACTION_COUNT;++i)
//                    {
//                        if(m_parent_window->btn_actions[i]->isEnabled())
//                        {
//                            m_parent_window->btn_hides[i]->setEnabled(true);
//                            m_parent_window->btn_locks[i]->setEnabled(true);
//                        }
//                    }
//                    /////////////////////////
//                }else if(xml.name()==tr("CarAction"))
//                {
//                    xml.skipCurrentElement();
//                }else
//                {
//                    xml.skipCurrentElement();
//                }
//            }
//        }
//    }
//    qDebug()<<xml.errorString();
//    return !xml.error();
//}
void DrawView::set_actions_button()
{
    ///////////////////
    //    int i=0;
    //    for(auto ma:scene->m_load_ag->m_actions)
    //    {
    //        for(auto btn:m_parent_window->btn_actions)
    //        {
    //            for(auto servo:m_parent_window->m_list_servoinfo)
    //            {
    //                if(servo->get_id()==ma->getId())
    //                {
    //                   if(servo->get_name()==btn->text())
    //                   {
    //                       btn->setEnabled(true);

    //                       btn->setStyleSheet("QPushButton{background-color:rgba(0,255,0,1); color: gray; font:italic bold 10px/10px Arial; border-radius: 10px;  border: 2px groove gray; border-style: outset;}"
    //                                                 "QPushButton:hover{background-color:white; color: black;}" "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }" );
    //                   }
    //                }
    //            }

    //        }
    //    }
    //    for(int i=0;i<MAX_ACTION_COUNT;++i)
    //    {
    //        if(m_parent_window->btn_actions[i]->isEnabled())
    //        {
    //         m_parent_window->btn_hides[i]->setEnabled(true);
    //          m_parent_window->btn_locks[i]->setEnabled(true);
    //        }
    //    }
}
//bool DrawView::loadFile_ex(const QString &fileName)
//{
//    QFile file(fileName);
//    if (!file.open(QFile::ReadOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr("action loader"),
//                             tr("Cannot read file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamReader xml(&file);
//    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
//    MotorAction* l_ma=nullptr;
////    scene->m_ag->m_actions.clear();
//    if (xml.readNextStartElement()) //root element
//    {
//        Q_ASSERT(xml.isStartElement() && xml.name() == "ActionGroup");
//        if ( xml.name() == tr("ActionGroup"))
//        {
//            while(xml.readNextStartElement())
//            {
//                if(xml.name()==tr("FileMusic"))
//                {
//                    scene->m_ag->setMusic_src(xml.attributes().value(tr("src")).toString());
//                    xml.skipCurrentElement();
//                }else if(xml.name()==tr("motorAction"))
//                {
//                    l_ma=new MotorAction(this);
//                    l_ma->m_items.clear();
//                    l_ma->m_trace.clear();
//                    int id= xml.attributes().value(tr("id")).toInt();
//                    l_ma->setId(id);
//                    QPointF temp;
//                    double x,y;
//                    while(xml.readNextStartElement())
//                    {
//                        if(xml.name()==tr("mPosition"))
//                        {
//                            x=xml.attributes().value(tr("X")).toDouble();
//                            y=xml.attributes().value(tr("Y")).toDouble();
//                            temp.setX(x);
//                            temp.setY(y);
//                            l_ma->m_trace.append(temp);
//                            xml.skipCurrentElement();
//                        }
//                    }
//                    scene->m_ag->m_actions.append((l_ma));
//                }else if(xml.name()==tr("CarAction"))
//                {
//                    xml.skipCurrentElement();
//                }else
//                {
//                    xml.skipCurrentElement();
//                }
//            }
//        }
//    }
//    qDebug()<<xml.errorString();
//    return !xml.error();
//}
//bool DrawView::loadFile(const QString &fileName) //暂时注释
//{
//    QFile file(fileName);
//    if (!file.open(QFile::ReadOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr(" scene loader"),
//                             tr("Cannot read file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    /*---------get actiongroup info--------------------------*/
//    QStringList lActionID=sendInfo2Table(fileName,"bezier","actionGroupID");
//    QStringList lServoID=sendInfo2Table(fileName,"bezier","servoID");

//    ts.clear();
//    for(int i=0;i<lActionID.size();++i)
//    {
//        ts.append(getparameter(fileName,"parameter","actionGroupID",lActionID[i],"parameter"));

//    }
//    for(int m=0;m<ts.size();++m)
//    {
//        for(int n=0;n<ts[m].size();++n)
//        {
//            for(int k=0;k<ts[m][n].size();++k)
//            {
//                qDebug()<<"ts:["<<m<<"]["<<n<<"]["<<k<<"]"<<ts[m][n][k]<<endl;

//            }
//        }
//    }

//    emit actionInfoChanged(lActionID,lServoID);
//    emit receivedParameter(ts);
//    /*-------------------------------------------------------*/
//    QXmlStreamReader xml(&file);
//    if (xml.readNextStartElement()) {
//        if (xml.name() == tr("scene"))
//        {
//            int width = xml.attributes().value(tr("width")).toInt();
//            int height = xml.attributes().value(tr("height")).toInt();
//            scene()->setSceneRect(0,0,width,height);
//            loadCanvas(&xml);

//        }
//    }

//    setCurrentFile(fileName);
//    qDebug()<<xml.errorString();
//    return !xml.error();
//}
//bool DrawView::save() //20190714
//{
//    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
//    if(scene==nullptr)
//        return false;
//        qDebug("scene->isSelected_type=%d\n",scene->isSelected_type);
//    if(scene->isSelected_type)
//    {
//        QString s("请先完成动作组创建，然后再保存!");
//        this->m_parent_window-> statusBar()->showMessage(s);
//        return false;
//    }
//    if (isUntitled) {
//        return saveAs();
//    } else {
//        QString temp=curFile.left(curFile.length()-4);
//        QString extFileName=temp+tr("_exi.xml");
//        bool bSave_ex=saveFile_Ex(extFileName);
//        return  bSave_ex&&saveFile(curFile);
//    }
//}
bool DrawView::save_ex()
{
    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
    if(scene==nullptr)
        return false;
    qDebug("scene->isSelected_type=%d\n",scene->isSelected_type);
    if(scene->isSelected_type)
    {
        QString s("请先完成动作组创建，然后再保存!");
        this->m_parent_window-> statusBar()->showMessage(s);
        return false;
    }
    if (isUntitled) {
        return saveAs_ex();
    } else {
        QString temp=curFile.left(curFile.length()-4);
        QString extFileName=temp+tr("_exi.xml");
        bool bSave_ex=saveFile_Ex(extFileName);
        bool ret=  bSave_ex&&saveFile(curFile);
        m_parent_window->mdiArea->closeAllSubWindows();
        return  ret;
    }
}

bool DrawView::save()
{
    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
    if(scene==nullptr)
        return false;
    qDebug("scene->isSelected_type=%d\n",scene->isSelected_type);
    if(scene->isSelected_type)
    {
        QString s("请先完成动作组创建，然后再保存!");
        this->m_parent_window-> statusBar()->showMessage(s);
        return false;
    }
    if (isUntitled) {
        return saveAs();//this is a bug ,must be debug;
    } else {
        QString temp=curFile.left(curFile.length()-4);
        QString extFileName=temp+tr("_exi.xml");
        bool bSave_ex=saveFile_Ex(extFileName);
        bool ret=  bSave_ex&&saveFile(curFile);
        m_parent_window->main_save_ag=scene->m_save_ag;
        m_parent_window->mdiArea->closeAllSubWindows();
        ret= m_parent_window->openFile4save(curFile);
        return  ret;
    }
}

//bool DrawView::save()
//{
//    if (isUntitled) {
//        return saveAs();
//    } else {
//        return saveFile(curFile);
//    }
//}



//bool DrawView::saveAs() //the lastest saved on 2019-07-14
//{
//    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
//                                                    curFile);
//    if (fileName.isEmpty())
//        return false;
//    QString temp=fileName.left(fileName.length()-4);
//    QString extFileName=temp+tr("_exi.xml");
//    bool bSave_ex=saveFile_Ex(extFileName);
//    return  bSave_ex&&saveFile(fileName);

//}
//bool DrawView::saveAs() //for ensure hte scene is unique ,must close all the window first;
//{
//    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
//                                                    curFile);
//    if (fileName.isEmpty())
//        return false;
//    QString temp=fileName.left(fileName.length()-4);
//    QString extFileName=temp+tr("_exi.xml");
//    bool bSave_ex=saveFile_Ex(extFileName);
//    bool ret=bSave_ex&&saveFile(fileName);
//    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
//    if(scene==nullptr)
//        return false;
//    m_parent_window->main_save_ag=scene->m_save_ag; // save the
//    int sz=m_parent_window->main_save_ag->m_actions.count();
//    qDebug("###main_save_ag m_actions:%d",sz);

//   m_parent_window->mdiArea->closeAllSubWindows();
//    ret= m_parent_window->openFile4save(fileName);
//    /////////////////////
//   sz=scene->m_save_ag->m_actions.count();
//    qDebug("m_actions:%d",sz);
//    //////////////////
//    return  ret;

//}
bool DrawView::saveAs() //for ensure hte scene is unique ,must close all the window first;
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),curFile);
    if (fileName.isEmpty())  return false;
    QString temp=fileName.left(fileName.length()-4);
    QString extFileName=temp+tr("_exi.xml");
    bool bSave_ex=saveFile_Ex(extFileName);
    bool ret=bSave_ex&&saveFile(fileName);
    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
    if(scene==nullptr)
        return false;
    m_parent_window->mdiArea->closeAllSubWindows();
    ret= m_parent_window->openFile4save(fileName);
    return  ret;


}
//bool DrawView::saveAs() //for ensure hte scene is unique ,must close all the window first;
//{
////      QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),curFile);
//    QString fileName = this->currentFile();
//    if (fileName.isEmpty())  return false;
//    QString temp=fileName.left(fileName.length()-4);
//    QString extFileName=temp+tr("_exi.xml");
//    bool bSave_ex=saveFile_Ex(extFileName);
//    bool ret=bSave_ex&&saveFile(fileName);
////    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
////    if(scene==nullptr)
////        return false;
////     m_parent_window->mdiArea->closeAllSubWindows();
////    ret= m_parent_window->openFile4save(fileName);
////    return  ret;

//return true;
//}
//}

bool DrawView::saveAs_ex() //for ensure hte scene is unique ,must close all the window first;
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    curFile);
    if (fileName.isEmpty())
        return false;
    QString temp=fileName.left(fileName.length()-4);
    QString extFileName=temp+tr("_exi.xml");
    bool bSave_ex=saveFile_Ex(extFileName);
    bool ret=bSave_ex&&saveFile(fileName);
    m_parent_window->mdiArea->closeAllSubWindows();
    return  ret;

}
bool DrawView::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("action save"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE  robot_control>");
    xml.writeStartElement("canvas");
    xml.writeAttribute("width",QString("%1").arg(scene()->width()));
    xml.writeAttribute("height",QString("%1").arg(scene()->height()));

    foreach (QGraphicsItem *item , scene()->items()) {
        GraphicsItem * ab = qgraphicsitem_cast<GraphicsItem*>(item);
        GraphicsItemGroup *g = dynamic_cast<GraphicsItemGroup*>(item->parentItem());
        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g ){

            ab->saveToXml(&xml);
        }

    }
    xml.writeEndElement();
    xml.writeEndDocument();
#if 0
    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setSize(QSize(800, 600));
    generator.setTitle(tr("SVG Generator Example Drawing"));
    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
                                "Example provided with Qt."));
    //![configure SVG generator]
    //![begin painting]
    QPainter painter;
    painter.begin(&generator);
    //![begin painting]
    //!
    scene()->clearSelection();
    scene()->render(&painter);
    //![end painting]
    painter.end();
    //![end painting]
#endif
    setCurrentFile(fileName);
    return true;
}

//bool DrawView::saveFile(const QString &fileName)
//{
//    QFile file(fileName);
//    if (!file.open(QFile::WriteOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr("action save"),
//                             tr("Cannot write file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamWriter xml(&file);
//    xml.setAutoFormatting(true);
//    xml.writeStartDocument();
//    xml.writeDTD("<!DOCTYPE  robot_control>");
//    xml.writeStartElement("canvas");
//    xml.writeAttribute("width",QString("%1").arg(scene()->width()));
//    xml.writeAttribute("height",QString("%1").arg(scene()->height()));
//        foreach (QGraphicsItem *item , scene()->items()) {
//            AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(item);
//            QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
//            if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g ){

//                ab->saveToXml(&xml);
//            }
//        }

////    foreach (QGraphicsItem *item , scene()->items())
////    {
////        GraphicsBezier * ab = qgraphicsitem_cast<GraphicsBezier*>(item);
////        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
////        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g )
////        {
////            ab->saveToXml(&xml);
////        }
////    }
//    xml.writeEndElement();
//    xml.writeEndDocument();
//#if 0
//    QSvgGenerator generator;
//    generator.setFileName(fileName);
//    generator.setSize(QSize(800, 600));
//    generator.setTitle(tr("SVG Generator Example Drawing"));
//    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
//                                "Example provided with Qt."));
//    //![configure SVG generator]
//    //![begin painting]
//    QPainter painter;
//    painter.begin(&generator);
//    //![begin painting]
//    //!
//    scene()->clearSelection();
//    scene()->render(&painter);
//    //![end painting]
//    painter.end();
//    //![end painting]
//#endif
//    setCurrentFile(fileName);
//    return true;
//}
//bool DrawView::saveFile(const QString &fileName)
//{

//    QFile file(fileName);
//    if (!file.open(QFile::WriteOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr(" scene save"),
//                             tr("Cannot write file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamWriter xml(&file);
//    xml.setAutoFormatting(true);
//    xml.writeStartDocument();
//    xml.writeDTD("<!DOCTYPE robot_control>");
//    xml.writeStartElement("scene");
//    xml.writeAttribute("width",QString("%1").arg(scene()->width()));
//    xml.writeAttribute("height",QString("%1").arg(scene()->height()));
////    foreach (QGraphicsItem *item , scene()->items())
////    {
////        GraphicsBezier * ab = qgraphicsitem_cast<GraphicsBezier*>(item);
////        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
////        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g )
////        {
////            ab->saveToXml(&xml);
////        }
////    }

//    foreach (QGraphicsItem *item , scene()->items()) {
//        AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(item);
//        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
//        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g ){
//            ab->saveToXml(&xml);
//        }
//    }
//    xml.writeEndElement();
//    xml.writeEndDocument();
//#if 0
//    QSvgGenerator generator;
//    generator.setFileName(fileName);
//    generator.setSize(QSize(800, 600));
//    generator.setTitle(tr("SVG Generator Example Drawing"));
//    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
//                                "Example provided with Qt."));
//    //![configure SVG generator]
//    //![begin painting]
//    QPainter painter;
//    painter.begin(&generator);
//    //![begin painting]
//    //!
//    scene()->clearSelection();
//    scene()->render(&painter);
//    //![end painting]
//    painter.end();
//    //![end painting]
//#endif
//    setCurrentFile(fileName);
//    if(file.isOpen())
//        file.close();
//    return true;
//}

//bool DrawView::saveFile(const QString &fileName)
//{

//    QFile file(fileName);
//    if (!file.open(QFile::WriteOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr(" scene save"),
//                             tr("Cannot write file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamWriter xml(&file);
//    xml.setAutoFormatting(true);
//    xml.writeStartDocument();
//    xml.writeDTD("<!DOCTYPE robot_control>");
//    xml.writeStartElement("scene");
//    xml.writeAttribute("width",QString("%1").arg(scene()->width()));
//    xml.writeAttribute("height",QString("%1").arg(scene()->height()));
//    foreach (QGraphicsItem *item , scene()->items())
//    {
//        GraphicsBezier * ab = qgraphicsitem_cast<GraphicsBezier*>(item);
//        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
//        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g )
//        {
//            ab->saveToXml(&xml);
//        }
//    }
//    xml.writeEndElement();
//    xml.writeEndDocument();
//#if 0
//    QSvgGenerator generator;
//    generator.setFileName(fileName);
//    generator.setSize(QSize(800, 600));
//    generator.setTitle(tr("SVG Generator Example Drawing"));
//    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
//                                "Example provided with Qt."));
//    //![configure SVG generator]
//    //![begin painting]
//    QPainter painter;
//    painter.begin(&generator);
//    //![begin painting]
//    //!
//    scene()->clearSelection();
//    scene()->render(&painter);
//    //![end painting]
//    painter.end();
//    //![end painting]
//#endif
//    setCurrentFile(fileName);
//    if(file.isOpen())
//        file.close();
//    return true;
//}

//bool DrawView::saveFile_Ex(const QString &fileName)
//{
//    QFile file(fileName);
//    if (!file.open(QFile::WriteOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr(" scene save"),
//                             tr("Cannot write file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamWriter xml(&file);
//    xml.setAutoFormatting(true);
//    xml.writeStartDocument();
//    xml.writeDTD("<!DOCTYPE robot_control>");
//    xml.writeStartElement("ActionGroup");
//    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
//    if(scene==nullptr) return false;
//    ActionGroup* ag=dynamic_cast<ActionGroup*>(scene->m_ag);
//    if(ag==nullptr) return false;
//    xml.writeStartElement("FileMusic");
//    xml.writeAttribute(tr("src"),QString("%1").arg(ag->getMusic_src()));
//    xml.writeEndElement(); //FileMusic

//    MotorAction* mac=nullptr;
//    int cnt=ag->m_actions.count();
//    for(int i=0;i<cnt;++i) //motorActions
//    {
//        mac=ag->m_actions.at(i);
//        xml.writeStartElement("motorAction");
//        xml.writeAttribute(tr("id"),QString("%1").arg(mac->getId()));
//        double y;
//        int cnt=mac->m_trace.count();
//        for(int j=0;j<cnt;++j)
//        {
//            xml.writeStartElement("mPosition");
////            xml.writeAttribute(tr("X"),QString("%1").arg("1"));
//            xml.writeAttribute(tr("X"),QString("%1").arg(j));
//            y=ANGLE_MAX-mac->m_trace.at(j).y()*mac->getConversion_coefficient_angle();
//            xml.writeAttribute("Y",QString("%1").arg(y));
//            xml.writeEndElement(); //mPosition
//        }
//        xml.writeEndElement();//motorAction
//    }

//    if( !ag->m_car_actions.isEmpty())
//    {
//        for(int n=0;n<ag->m_car_actions.count();++n) //carActions
//        {
//            mac=ag->m_actions.at(n);
//            xml.writeStartElement("CarAction");
//            xml.writeEndElement(); //CarAction
//        }
//    }
//    xml.writeEndElement();
//    xml.writeEndDocument();
//#if 0
//    QSvgGenerator generator;
//    generator.setFileName(fileName);
//    generator.setSize(QSize(800, 600));
//    generator.setTitle(tr("SVG Generator Example Drawing"));
//    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
//                                "Example provided with Qt."));
//    //![configure SVG generator]
//    //![begin painting]
//    QPainter painter;
//    painter.begin(&generator);
//    //![begin painting]
//    //!
//    scene()->clearSelection();
//    scene()->render(&painter);
//    //![end painting]
//    painter.end();
//    //![end painting]
//#endif
//    if(file.isOpen())
//        file.close();
//    return true;
//}
bool DrawView::saveFile_Ex(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr(" scene save"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE robot_control>");
    xml.writeStartElement("ActionGroup");
    DrawScene* scene=dynamic_cast<DrawScene*>(this->scene());
    if(scene==nullptr) return false;
    xml.writeStartElement("FileMusic");
    scene->m_save_ag->setMusic_src(m_parent_window->m_audio_src);
    xml.writeAttribute(tr("src"),QString("%1").arg(scene->m_save_ag->getMusic_src()));
    xml.writeEndElement(); //FileMusic
    MotorAction* mac=nullptr;
    int sz=scene->m_save_ag->m_actions.count(); //zhe ge shihou haishi  zuobiao
    for(int i=0;i<sz;++i) //motorActions
    {
        mac=scene->m_save_ag->m_actions.at(i);
        xml.writeStartElement("motorAction");
        xml.writeAttribute(tr("id"),QString("%1").arg(mac->getId()));
        double x,y;
        int cnt=mac->m_trace.count();
        for(int j=0;j<cnt;++j)
        {
            xml.writeStartElement("mPosition");
            x=mac->m_trace[j].x();
            xml.writeAttribute(tr("X"),QString("%1").arg(x));
            y=mac->m_trace[j].y();
            xml.writeAttribute("Y",QString("%1").arg(y));
            xml.writeEndElement(); //mPosition
        }
        xml.writeEndElement();//motorAction
    }
    if( !scene->m_save_ag->m_car_actions.isEmpty())
    {
        for(int n=0;n<scene->m_save_ag->m_car_actions.count();++n) //carActions
        {
            xml.writeStartElement("CarAction");
            xml.writeEndElement(); //CarAction
        }
    }
    xml.writeEndElement();//ActionGroup
    xml.writeEndDocument();
#if 0
    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setSize(QSize(800, 600));
    generator.setTitle(tr("SVG Generator Example Drawing"));
    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
                                "Example provided with Qt."));
    //![configure SVG generator]
    //![begin painting]
    QPainter painter;
    painter.begin(&generator);
    //![begin painting]
    //!
    scene()->clearSelection();
    scene()->render(&painter);
    //![end painting]
    painter.end();
    //![end painting]
#endif
    if(file.isOpen())
        file.close();
    return true;
}
//bool DrawView::save_actiongroup_data(QXmlStreamWriter *xml)
//{
//    DrawScene* scene=qgraphicsitem_cast<DrawScene*>(this->scene());
//    if(scene==nullptr) return false;
//    ActionGroup* ag=dynamic_cast<ActionGroup*>(scene->m_ag);
//    if(ag==nullptr) return false;
//    xml->writeStartElement("FileMusic");
//    xml->writeAttribute(tr("src"),QString("%1").arg(ag->getMusic_src()));
//    xml->writeEndElement(); //FileMusic

//    MotorAction* mac=nullptr;
//    for(int i=0;i<ag->m_actions.count();++i) //motorActions
//    {
//        mac=ag->m_actions.at(i);
//        xml->writeStartElement("motorAction");
//        xml->writeAttribute(tr("id"),QString("%1").arg(mac->getId()));
//        double y;
//        for(int j=0;j<mac->m_trace.count();++j)
//        {
//            xml->writeStartElement("mPosition");
//            xml->writeAttribute(tr("X"),QString("%1").arg("1"));
//            y=mac->m_trace.at(i).y()*mac->getConversion_coefficient_angle();
//            xml->writeAttribute("Y",QString("%1").arg(y));
//            xml->writeEndElement(); //mPosition
//        }
//        xml->writeEndElement();//motorAction
//    }

//    if( !ag->m_car_actions.isEmpty())
//    {
//        for(int n=0;n<ag->m_car_actions.count();++n) //carActions
//        {
//            mac=ag->m_actions.at(i);
//            xml->writeStartElement("CarAction");
//            xml->writeEndElement(); //CarAction
//        }
//    }
//    return true;
//}

//bool DrawView::saveFile_Ex(const QString &fileName)
//{
//    QFile file(fileName);
//    if (!file.open(QFile::WriteOnly | QFile::Text)) {
//        QMessageBox::warning(this, tr(" scene save"),
//                             tr("Cannot write file %1:\n%2.")
//                             .arg(fileName)
//                             .arg(file.errorString()));
//        return false;
//    }
//    QXmlStreamWriter xml(&file);
//    xml.setAutoFormatting(true);
//    xml.writeStartDocument();
//    xml.writeDTD("<!DOCTYPE robot_control>");
//    xml.writeStartElement("ActionGroup");
//    foreach (QGraphicsItem *item , scene()->items()) {
//        GraphicsBezier * ab = qgraphicsitem_cast<GraphicsBezier*>(item);
//        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
//        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab) && !g ){
//            ab->saveToXml_Ex(&xml);
//        }

//    }
//    xml.writeEndElement();
//    xml.writeEndDocument();
//#if 0
//    QSvgGenerator generator;
//    generator.setFileName(fileName);
//    generator.setSize(QSize(800, 600));
//    generator.setTitle(tr("SVG Generator Example Drawing"));
//    generator.setDescription(tr("An SVG drawing created by the SVG Generator "
//                                "Example provided with Qt."));
//    //![configure SVG generator]
//    //![begin painting]
//    QPainter painter;
//    painter.begin(&generator);
//    //![begin painting]
//    //!
//    scene()->clearSelection();
//    scene()->render(&painter);
//    //![end painting]
//    painter.end();
//    //![end painting]
//#endif
//    if(file.isOpen())
//        file.close();
//    return true;
//}

//void DrawView::paintEvent(QPaintEvent *event)
//{
////Q_UNUSED(event);
////  QPainter painter(this->viewport());
////  QPen p(Qt::red);
////  p.setWidth(1);
////  painter.setPen(p);
////  m_vruler->drawPos(&painter);
////  m_hruler->drawPos(&painter);
//}
QString DrawView::userFriendlyCurrentFile()
{
    return strippedName(curFile);
}

void DrawView::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        emit closeDrawView();
        event->accept();
        this->m_parent_window->slt_check_action_status(true);
    } else {
        event->ignore();
    }

}

void DrawView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF pt =mapToScene(event->pos());
    m_hruler->updatePosition(event->pos());
    m_vruler->updatePosition(event->pos());
    emit positionChanged( pt.x() , pt.y() );
    QGraphicsView::mouseMoveEvent(event);
}

//void DrawView::resizeEvent(QResizeEvent *event)
//{
//    QGraphicsView::resizeEvent(event);

//    this->setViewportMargins(RULER_SIZE-1,RULER_SIZE-1,0,0);
//    m_hruler->resize(this->size().width()- RULER_SIZE - 1,RULER_SIZE);
//    m_hruler->move(RULER_SIZE,0);
//    m_vruler->resize(RULER_SIZE,this->size().height() - RULER_SIZE - 1);
//    m_vruler->move(0,RULER_SIZE);

//    box->resize(RULER_SIZE,RULER_SIZE);
//    box->move(0,0);
//    updateRuler();
//}
void DrawView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    this->setViewportMargins(RULER_SIZE-1,RULER_SIZE-1,0,0);
    m_hruler->resize(this->size().width()- RULER_SIZE - 1,RULER_SIZE);
    m_hruler->move(RULER_SIZE,0);
    m_vruler->resize(RULER_SIZE,this->size().height() - RULER_SIZE - 1);
    m_vruler->move(0,RULER_SIZE);

    box->resize(RULER_SIZE,RULER_SIZE);
    box->move(0,0);
    updateRuler();
}

void DrawView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx,dy);
    updateRuler();

}

//void DrawView::updateRuler()
//{
//    if ( scene() == nullptr) return;
//    QRectF viewbox = this->rect();
//    QPointF offset = mapFromScene(scene()->sceneRect().topLeft());
//    double factor =  1./transform().m11();
//    if(m_dur!=0)
//    {
//        double lower_x = factor * ( viewbox.left()  - offset.x() )*m_dur/1000000/SCENE_WIDTH;
//        double upper_x = factor * ( viewbox.right() -RULER_SIZE- offset.x()  )*m_dur/1000000/SCENE_WIDTH;
//        m_hruler->setRange(lower_x,upper_x,upper_x - lower_x );
//    }else
//    {
//        double lower_x = factor * ( viewbox.left()  - offset.x() )*TIME_MAX/SCENE_WIDTH;
//        double upper_x = factor * ( viewbox.right() -RULER_SIZE- offset.x()  )*TIME_MAX/SCENE_WIDTH;
//        m_hruler->setRange(lower_x,upper_x,upper_x - lower_x );

//    }
//       m_hruler->update();

//    double lower_y = (factor * ( viewbox.top() - offset.y()) * -1)*ANGLE_MAX/SCENE_HEIGHT;
//    double upper_y = (factor * ( viewbox.bottom() - RULER_SIZE - offset.y() ) * -1)*ANGLE_MAX/SCENE_HEIGHT;
//    m_vruler->setRange(lower_y,upper_y,upper_y - lower_y );
//    m_vruler->update();
//}

void DrawView::updateRuler()
{
    if ( scene() == nullptr) return;
    QRectF viewbox = this->rect();
    QPointF offset = mapFromScene(scene()->sceneRect().topLeft());
    double factor =  1./transform().m11();
    if(m_dur!=0)
    {
        double lower_x = factor * ( viewbox.left()  - offset.x() )*m_dur/1000000/SCENE_WIDTH;
        double upper_x = factor * ( viewbox.right() -RULER_SIZE- offset.x()  )*m_dur/1000000/SCENE_WIDTH;
        m_hruler->setRange(lower_x,upper_x,upper_x - lower_x );
    }else
    {
        double lower_x = factor * ( viewbox.left()  - offset.x() )*TIME_MAX/SCENE_WIDTH;
        double upper_x = factor * ( viewbox.right() -RULER_SIZE- offset.x()  )*TIME_MAX/SCENE_WIDTH;
        m_hruler->setRange(lower_x,upper_x,upper_x - lower_x );

    }
    m_hruler->update();
    double lower_y = (factor * ( viewbox.top() - offset.y()) * -1)*ANGLE_MAX/SCENE_HEIGHT;
    double upper_y = (factor * ( viewbox.bottom() - RULER_SIZE - offset.y() ) * -1)*ANGLE_MAX/SCENE_HEIGHT;
    m_vruler->setRange(lower_y+ANGLE_MAX,upper_y+ANGLE_MAX,upper_y - lower_y );
    m_vruler->update();
}

//void DrawView::updateRuler()
//{
//    if ( scene() == nullptr) return;
//    QRectF viewbox = this->rect();
//    QPointF offset = mapFromScene(scene()->sceneRect().topLeft());
//    double factor =  1./transform().m11();
//    double lower_x = factor * ( viewbox.left()  - offset.x() )*TIME_MAX/SCENE_WIDTH;
//    double upper_x = factor * ( viewbox.right() -RULER_SIZE- offset.x()  )*TIME_MAX/SCENE_WIDTH;
//    m_hruler->setRange(lower_x,upper_x,upper_x - lower_x );
//    m_hruler->update();
//    double lower_y = (factor * ( viewbox.top() - offset.y()) * -1)*ANGLE_MAX/SCENE_HEIGHT;
//    double upper_y = (factor * ( viewbox.bottom() - RULER_SIZE - offset.y() ) * -1)*ANGLE_MAX/SCENE_HEIGHT;
//    m_vruler->setRange(lower_y+ANGLE_MAX,upper_y+ANGLE_MAX,upper_y - lower_y );
//    m_vruler->update();
//}
//bool DrawView::maybeSave() //20190714
//{
//    if (isModified()) {
//        QMessageBox::StandardButton ret;
//        ret = QMessageBox::warning(this, tr("warning"),
//                                   tr("'%1' has been modified.\n"
//                                      "Do you want to save your changes?")
//                                   .arg(userFriendlyCurrentFile()),
//                                   QMessageBox::Save | QMessageBox::Discard
//                                   | QMessageBox::Cancel);
//        if (ret == QMessageBox::Save)
//            return save();
//        else if (ret == QMessageBox::Cancel)
//            return false;
//    }
//    return true;
//}
bool DrawView::maybeSave()
{
    if (isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("warning"),
                                   tr("'%1' has been modified.\n"
                                      "Do you want to save your changes?")
                                   .arg(userFriendlyCurrentFile()),
                                   QMessageBox::Save | QMessageBox::Discard
                                   | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save_ex();
        else if (ret == QMessageBox::Cancel)
        {

            return save_ex();
        }
    }
    return true;
}

void DrawView::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();
    isUntitled = false;
    setModified(false);
    setWindowModified(false);
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString DrawView::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();

}

void DrawView::loadCanvas( QXmlStreamReader *xml)
{
    Q_ASSERT(xml->isStartElement() && xml->name() == "canvas");
    while (xml->readNextStartElement()) {
        AbstractShape * item = nullptr;
        if (xml->name() == tr("rect")){
            item = new GraphicsRectItem(QRect(0,0,1,1));
        }else if (xml->name() == tr("roundrect")){
            item = new GraphicsRectItem(QRect(0,0,1,1),true);
        }else if (xml->name() == tr("ellipse"))
            item = new GraphicsEllipseItem(QRect(0,0,1,1));
        else if (xml->name()==tr("polygon"))
            item = new GraphicsPolygonItem();
        else if ( xml->name()==tr("bezier"))
            item = new GraphicsBezier();
        else if ( xml->name() == tr("polyline"))
            item = new GraphicsBezier(false);
        else if ( xml->name() == tr("line"))
            item = new GraphicsLineItem();
        else if ( xml->name() == tr("group"))
            item =qgraphicsitem_cast<AbstractShape*>(loadGroupFromXML(xml));
        else
            xml->skipCurrentElement();

        if (item && item->loadFromXml(xml))
            scene()->addItem(item);
        else if ( item )
            delete item;
    }
}

//void DrawView::loadCanvas( QXmlStreamReader *xml)
//{
//    Q_ASSERT(xml->isStartElement() && xml->name() == "scene");
//    while (xml->readNextStartElement()) {
//        if(xml->name()==tr("actionGroup"))
//        {
//            if (xml->readNextStartElement()) {
//                AbstractShape * item = nullptr;
//                if (xml->name() == tr("rect")){
//                    item = new GraphicsRectItem(QRect(0,0,1,1));
//                }else if (xml->name() == tr("roundrect")){
//                    item = new GraphicsRectItem(QRect(0,0,1,1),true);
//                }else if (xml->name() == tr("ellipse"))
//                    item = new GraphicsEllipseItem(QRect(0,0,1,1));
//                else if (xml->name()==tr("polygon"))
//                    item = new GraphicsPolygonItem();
//                else if ( xml->name()==tr("bezier"))
//                {
//                    item = new GraphicsBezier();
//                }
//                else if ( xml->name() == tr("polyline"))
//                    item = new GraphicsBezier(false);
//                else if ( xml->name() == tr("line"))
//                    item = new GraphicsLineItem();
//                else if ( xml->name() == tr("group"))
//                    item =qgraphicsitem_cast<AbstractShape*>(loadGroupFromXML(xml));
//                else
//                    xml->skipCurrentElement();
//                if (item && item->loadFromXml(xml))
//                {
//                    jump2Element(xml,"actionGroup");
//                    //                    jump2LowerElement(xml,"command");
//                    //                   QString str= xml->attributes().value("actionGroupID").toString();
//                    ////                    jump2Element(xml,"actionGroup");
//                    //                   qDebug()<<"actionid####:"<<str<<endl;
//                    scene()->addItem(item);

//                }
//                else if ( item )
//                    delete item;
//            }
//        }
//    }
//}

void DrawView::jump2Element(QXmlStreamReader* xml,QString desElement)
{
    while(xml->name()!=desElement)
    {
        if(xml->isEndElement())
        {
            xml->skipCurrentElement();
            qDebug()<<"name######:"<<xml->name();
        }

    }
}

void DrawView::jump2LowerElement(QXmlStreamReader* xml,QString desElement)
{
    while(xml->name()!=desElement)
    {
        if(xml->readNextStartElement())
        {
            qDebug()<<"name$$$$$$:"<<xml->name();
        }
    }
    qDebug()<<"name2222:"<<xml->name();
}


//void DrawView::loadCanvas( QXmlStreamReader *xml)
//{
//    Q_ASSERT(xml->isStartElement() && xml->name() == "scene");
//        if(xml->readNextStartElement()&&xml->name()==tr("actionGroup"))
//        {
//            if (xml->readNextStartElement()) {
//                AbstractShape * item = nullptr;
//                if (xml->name() == tr("rect")){
//                    item = new GraphicsRectItem(QRect(0,0,1,1));
//                }else if (xml->name() == tr("roundrect")){
//                    item = new GraphicsRectItem(QRect(0,0,1,1),true);
//                }else if (xml->name() == tr("ellipse"))
//                    item = new GraphicsEllipseItem(QRect(0,0,1,1));
//                else if (xml->name()==tr("polygon"))
//                    item = new GraphicsPolygonItem();
//                else if ( xml->name()==tr("bezier"))
//                    item = new GraphicsBezier();
//                else if ( xml->name() == tr("polyline"))
//                    item = new GraphicsBezier(false);
//                else if ( xml->name() == tr("line"))
//                    item = new GraphicsLineItem();
//                else if ( xml->name() == tr("group"))
//                    item =qgraphicsitem_cast<AbstractShape*>(loadGroupFromXML(xml));
//                else
//                    xml->skipCurrentElement();
//                if (item && item->loadFromXml(xml))
//                    scene()->addItem(item);
//                else if ( item )
//                    delete item;
//            }
//            QXmlStreamReader::TokenType token = xml->readNext();
//            if(token == QXmlStreamReader::StartElement)
//            {
//                if(xml->name()==tr("actionGroup"))
//                {
//                    continue;
//                }
//            }
//        }
//}

GraphicsItemGroup *DrawView::loadGroupFromXML(QXmlStreamReader *xml)
{
    QList<QGraphicsItem*> items;
    qreal angle = xml->attributes().value(tr("rotate")).toDouble();
    while (xml->readNextStartElement()) {
        AbstractShape * item = nullptr;
        if (xml->name() == tr("rect")){
            item = new GraphicsRectItem(QRect(0,0,1,1));
        }else if (xml->name() == tr("roundrect")){
            item = new GraphicsRectItem(QRect(0,0,1,1),true);
        }else if (xml->name() == tr("ellipse"))
            item = new GraphicsEllipseItem(QRect(0,0,1,1));
        else if (xml->name()==tr("polygon"))
            item = new GraphicsPolygonItem();
        else if ( xml->name()==tr("bezier"))
            item = new GraphicsBezier();
        else if ( xml->name() == tr("polyline"))
            item = new GraphicsBezier(false);
        else if ( xml->name() == tr("line"))
            item = new GraphicsLineItem();
        else if ( xml->name() == tr("group"))
            item =qgraphicsitem_cast<AbstractShape*>(loadGroupFromXML(xml));
        else
            xml->skipCurrentElement();
        if (item && item->loadFromXml(xml)){
            scene()->addItem(item);
            items.append(item);
        }else if ( item )
            delete item;
    }

    if ( items.count() > 0 ){
        DrawScene * s = dynamic_cast<DrawScene*>(scene());
        GraphicsItemGroup * group = s->createGroup(items,false);
        if (group){
            group->setRotation(angle);
            group->updateCoordinate();
            //qDebug()<<"angle:" <<angle;
        }
        return group;
    }
    return 0;
}

