#include "xmltool.h"
XmlTool::XmlTool(QObject *parent) : QObject(parent)
{

}

//写xml
void XmlTool::WriteXml(QString file_path)
{
    //打开或创建文件
    QFile file(file_path); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::WriteOnly|QFile::Truncate)) //可以用QIODevice，Truncate表示清空原来的内容
        return;
    QDomDocument doc;
    //写入xml头部
    QDomProcessingInstruction instruction; //添加处理命令
    instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    //添加根节点
    QDomElement root=doc.createElement("motoSystem");
    doc.appendChild(root);
    //添加第一个子节点及其子元素
    QDomElement moto=doc.createElement("motoSys");
    moto.setAttribute("id",31); //方式一：创建属性  其中键值对的值可以是各种类型
    moto.setAttribute("name","点头");
    moto.setAttribute("jdStart","90");
    moto.setAttribute("jdMax","140");
    moto.setAttribute("jdMin","40");
    moto.setAttribute("fScale","14.5");
    moto.setAttribute("fOffSet","0");
    QDomElement motoImg=doc.createElement("motoImg"); //创建子元素
    motoImg.setAttribute("jd","0");
    motoImg.setAttribute("src","img1.jpg");
    moto.appendChild(motoImg);
    root.appendChild(moto);
    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
}

void XmlTool::read_servoInfo_by_name(QString file_path,QString name,ServoInfo* info)
{
    QFile file(file_path); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return;
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();
    QDomElement root=doc.documentElement(); //返回根节点
    QDomNode node=root.firstChild(); //获得第一个子节点
    while(!node.isNull())  //如果节点不空
    {
        if(node.isElement()) //如果节点是元素
        {
            QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
             if(e.attribute("name")==name)
             {
              //   qDebug()<<e.tagName()<<" "<<e.attribute("id")<<" "<<e.attribute("name"); //打印键值对，tagName和nodeName是一个东西
                 info->set_id(e.attribute("id").toInt());
                 info->set_name(e.attribute("name"));
                 info->set_jdStart(e.attribute("jdStart").toInt());
                 info->set_jdMax(e.attribute("jdMax").toInt());
                 info->set_jdMin(e.attribute("jdMin").toInt());
                 info->set_fScale(e.attribute("fScale").toInt());
                 info->set_fOffSet(e.attribute("fOffSet").toInt());
                 QDomNodeList list=e.childNodes();
                 for(int i=0;i<list.count();i++) //遍历子元素，count和size都可以用,可用于标签数计数
                 {
                     QDomNode n=list.at(i);
                     if(node.isElement())
                     {
                         QDomElement c=n.toElement();
                         info->set_image_jd(c.attribute("jd").toInt());
                         info->set_src(c.attribute("src"));
                     }
                 }
                 return  ;
             }
        }
        node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
    }

}

void XmlTool::read_servoInfo(QString file_path,QList<ServoInfo*>& servo_list)
{
    QFile file(file_path); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return;
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();
    QDomElement root=doc.documentElement(); //返回根节点
//    qDebug()<<root.nodeName();
    QDomNode node=root.firstChild(); //获得第一个子节点
    while(!node.isNull())  //如果节点不空
    {
        if(node.isElement()) //如果节点是元素
        {
            QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
//            qDebug()<<e.tagName()<<" "<<e.attribute("id")<<" "<<e.attribute("name"); //打印键值对，tagName和nodeName是一个东西
            ServoInfo *info=new ServoInfo(this);
            info->set_id(e.attribute("id").toInt());
            info->set_name(e.attribute("name"));
            info->set_jdStart(e.attribute("jdStart").toInt());
            info->set_jdMax(e.attribute("jdStart").toInt());
            info->set_jdMin(e.attribute("jdMin").toInt());
            info->set_fScale(e.attribute("fScale").toInt());
            info->set_fOffSet(e.attribute("fOffSet").toInt());
            QDomNodeList list=e.childNodes();
            for(int i=0;i<list.count();i++) //遍历子元素，count和size都可以用,可用于标签数计数
            {
                QDomNode n=list.at(i);
                if(node.isElement())
                {
                    QDomElement c=n.toElement();
//                    qDebug()<<c.tagName()<<" "<<c.attribute("jd")<<" " <<c.attribute("src");
                    info->set_image_jd(c.attribute("jd").toInt());
                    info->set_src(c.attribute("src"));

                }

            }
            servo_list.append(info);
        }
        node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
    }

}

//读xml
void XmlTool::ReadXml(QString file_path)
{
    QFile file(file_path); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return;
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();
    QDomElement root=doc.documentElement(); //返回根节点
    qDebug()<<root.nodeName();
    QDomNode node=root.firstChild(); //获得第一个子节点
    while(!node.isNull())  //如果节点不空
    {
        if(node.isElement()) //如果节点是元素
        {
            QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
            qDebug()<<e.tagName()<<" "<<e.attribute("id")<<" "<<e.attribute("name"); //打印键值对，tagName和nodeName是一个东西
            QDomNodeList list=e.childNodes();
            for(int i=0;i<list.count();i++) //遍历子元素，count和size都可以用,可用于标签数计数
            {
                QDomNode n=list.at(i);
                if(node.isElement())
                    qDebug()<<n.nodeName()<<":"<<n.toElement().text();
            }
        }
        node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
    }

}

void XmlTool::get_servos_name(QString file_path,QList<QString>&nameList)
{
    QFile file(file_path); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return;
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }

    file.close();
    QDomElement root=doc.documentElement(); //返回根节点
    qDebug()<<root.nodeName();
    QDomNode node=root.firstChild(); //获得第一个子节点
    while(!node.isNull())  //如果节点不空
    {
        if(node.isElement()) //如果节点是元素
        {
            QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
            nameList.append(e.attribute("name"));
            qDebug()<<e.tagName()<<" "<<e.attribute("id")<<" "<<e.attribute("name"); //打印键值对，tagName和nodeName是一个东西
        }
        node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
    }
}

//增加xml内容
void XmlTool::AddXml(QString file_path)
{
    //打开文件
    QFile file(file_path); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::ReadOnly))
        return;

    //增加一个一级子节点以及元素
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();

    QDomElement root=doc.documentElement();
    QDomElement book=doc.createElement("book");
    book.setAttribute("id",3);
    book.setAttribute("time","1813/1/27");
    QDomElement title=doc.createElement("title");
    QDomText text;
    text=doc.createTextNode("Pride and Prejudice");
    title.appendChild(text);
    book.appendChild(title);
    QDomElement author=doc.createElement("author");
    text=doc.createTextNode("Jane Austen");
    author.appendChild(text);
    book.appendChild(author);
    root.appendChild(book);

    if(!file.open(QFile::WriteOnly|QFile::Truncate)) //先读进来，再重写，如果不用truncate就是在后面追加内容，就无效了
        return;
    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
}

//删减xml内容
void XmlTool::RemoveXml(QString file_path)
{
    QFile file(file_path); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::ReadOnly))
        return;
    //删除一个一级子节点及其元素，外层节点删除内层节点于此相同
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();  //一定要记得关掉啊，不然无法完成操作

    QDomElement root=doc.documentElement();
    QDomNodeList list=doc.elementsByTagName("book"); //由标签名定位
    for(int i=0;i<list.count();i++)
    {
        QDomElement e=list.at(i).toElement();
        if(e.attribute("time")=="2007/5/25")  //以属性名定位，类似于hash的方式，warning：这里仅仅删除一个节点，其实可以加个break
            root.removeChild(list.at(i));
    }

    if(!file.open(QFile::WriteOnly|QFile::Truncate))
        return;
    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
}

//更新xml内容
void XmlTool::UpdateXml(QString file_path,QString find)
{
    QFile file(file_path); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::ReadOnly))
        return;
    //更新一个标签项,如果知道xml的结构，直接定位到那个标签上定点更新
    //或者用遍历的方法去匹配tagname或者attribut，value来更新
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();
    QDomElement root=doc.documentElement();
    QDomNodeList list=root.elementsByTagName("motoSys");
    QDomNode node=list.at(list.size()-1).firstChild(); //定位到第三个一级子节点的子元素
    QDomNode oldnode=node.firstChild(); //标签之间的内容作为节点的子节点出现,当前是Pride and Projudice
    node.firstChild().setNodeValue("Emma");
    QDomNode newnode=node.firstChild();
    node.replaceChild(newnode,oldnode);
    if(!file.open(QFile::WriteOnly|QFile::Truncate))
        return;
    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
}


