#include "servoinfo.h"

ServoInfo::ServoInfo(QObject *parent) : QObject(parent)
{
    id=0;
    name=" ";
    jdStart=0;
    jdMax=0;
    jdMin=0;
    fScale=0.0;
    fOffSet=0;
    image_jd=0;
    src=" ";
}


void ServoInfo::set_id(qint16 id)
{
    this->id=id;
}

qint16 ServoInfo::get_id()
{

    return this->id;

}

void ServoInfo::set_name(QString name)
{
    this->name=name;
}

QString ServoInfo::get_name()
{
    return this->name;
}

void ServoInfo::set_jdStart(qint16 jdStart)
{
    this->jdStart= jdStart;
}

quint16 ServoInfo::get_jdStart()
{
    return this->jdStart;
}

void ServoInfo::set_jdMax(qint16 jdMax)
{
    this->jdMax=jdMax;
}

qint16 ServoInfo::get_jdMax()
{
    return this->jdMax;
}

void ServoInfo::set_jdMin(qint16 jdMin)
{
    this->jdMin=jdMin;
}

qint16 ServoInfo::get_jdMin()
{
    return this->jdMin;
}

void ServoInfo::set_fScale(qint16 fScale)
{
    this->fScale=fScale;
}

qint16 ServoInfo::get_fScale()
{
    return this->fScale;
}

void ServoInfo::set_fOffSet(qint16 fOffSet)
{
    this->fOffSet=fOffSet;
}

qint16 ServoInfo::get_fOffSet()
{
    return this->fOffSet;
}

void ServoInfo::set_image_jd(qint16 image_jd)
{
    this->image_jd=image_jd;
}

qint16 ServoInfo::get_image_jd()
{
    return this->image_jd;
}

void ServoInfo::set_src(QString src)
{
    this->src=src;

}

QString ServoInfo::get_src()
{
    return this->src;
}
