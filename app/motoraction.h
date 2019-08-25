#ifndef MOTORACTION_H
#define MOTORACTION_H

#include <QObject>
#include<QVector>
#include<QPointF>
#include "drawobj.h"
#include "actiongroup.h"
extern const double ANGLE_MAX;
extern const double  SCENE_WIDTH;
extern const double  SCENE_HEIGHT;
extern const double SERVO_DETAIL;
class ActionGroup;
class MotorAction : public QObject
{
    Q_OBJECT
public:
    explicit MotorAction(QObject *parent = nullptr);

    int getId() const;
    void setId(int value);
    void handle_trace();
signals:

public slots:

private:
    int id;
public:
    QVector<int> speed;
    int step4point;
    PointF tmp_points[150];
    PointF tmp_points_line[1000];
    double axisx_max_time_sec;
    double conversion_coefficient_angle;
    double time_div;
public:
    ActionGroup * m_action_group;
    QVector<GraphicsPolygonItem*> m_items;
    QVector<QPointF> m_trace;
    int getStep4point() const;
    void setStep4point(int value);
    void get_points(int step, QVector<QPointF> &in, QVector<QPointF> &out);
    PointF bezier_interpolation(double t, PointF *points, QVector<QPointF> &in);
    void draw_bezier_curves(PointF *points, PointF *out_points, int num, QVector<QPointF> &in);
    void get_points4bezier(GraphicsBezier *bez);
    double getTime_div() const;
    void setTime_div(double value);
    double getAxisx_max_time_sec() const;
    void setAxisx_max_time_sec(double value);
    double getConversion_coefficient_angle() const;
    void setConversion_coefficient_angle(double value);
    void sort_unique_trace();
    void handle_speed();
    QVector<int> getSpeed();
};
#endif // MOTORACTION_H
