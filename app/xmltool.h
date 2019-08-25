#ifndef XMLTOOL_H
#define XMLTOOL_H

#include <QObject>
#include <QtXml> //也可以include <QDomDocument>
#include"servoinfo.h"
class XmlTool : public QObject
{
    Q_OBJECT
public:
    explicit XmlTool(QObject *parent = nullptr);

    void WriteXml(QString file_path);
    void ReadXml(QString file_path);
    void AddXml(QString file_path);
    void RemoveXml(QString file_path);
    void UpdateXml(QString file_path, QString find);
    void get_servos_name(QString file_path, QList<QString> &nameList);
    void read_servoInfo(QString file_path, QList<ServoInfo *> &servo_list);
    void read_servoInfo_by_name(QString file_path, QString name, ServoInfo *info);
signals:

public slots:
};

#endif // XMLTOOL_H
