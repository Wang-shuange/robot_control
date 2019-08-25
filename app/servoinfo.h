#ifndef SERVOINFO_H
#define SERVOINFO_H

#include <QObject>

class ServoInfo : public QObject
{
    Q_OBJECT
public:
    explicit ServoInfo(QObject *parent = nullptr);
 private:
   qint16 id;
   QString name;
   qint16 jdStart;
   qint16 jdMax;
   qint16 jdMin;
   qreal fScale;
   qint16 fOffSet;
   qint16 image_jd;
   QString src;

public:
   void set_id(qint16 id);
   qint16 get_id();
   void set_name(QString name);
   QString get_name();
   void set_jdStart(qint16 jdStart);
   quint16 get_jdStart();

   void set_jdMax(qint16 jdMax);
   qint16 get_jdMax();
   void set_jdMin(qint16 jdMin);
   qint16 get_jdMin();
   void set_fScale(qint16 fScale);
   qint16 get_fScale();
   void set_fOffSet(qint16 fOffSet);
   qint16 get_fOffSet();
   void set_image_jd(qint16 image_jd);
   qint16 get_image_jd();
   void set_src(QString src);
   QString  get_src();

signals:

public slots:
};

#endif // SERVOINFO_H
