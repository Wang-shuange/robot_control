#include "motoraction.h"
#include <math.h>
#include "drawscene.h"
#include<vector>
#include<algorithm>
using namespace std;
extern const double  SCENE_WIDTH;

MotorAction::MotorAction(QObject *parent) : QObject(parent)
{
    id=0;
    this->step4point=0;
    this->m_trace.clear();
    this->m_items.clear();
    axisx_max_time_sec=20.;//default ,it must followed by audioo duration;
    time_div=20.;
    m_action_group=nullptr;
    speed.clear();
    this->setConversion_coefficient_angle(ANGLE_MAX/SCENE_HEIGHT);
}

int MotorAction::getId() const
{
    return id;
}

void MotorAction::setId(int value)
{
    id = value;
}
void MotorAction::draw_bezier_curves(PointF *points, PointF *out_points, int num, QVector<QPointF> &in)
{
    double step = 1.0/ num;
    double t = 0.;
    for (int i = 0; i < num; i++)
    {
        PointF temp_point = bezier_interpolation(t, points,in);
        t += step;
        out_points[i] = temp_point;
    }
}
PointF  MotorAction::bezier_interpolation(double t, PointF* points,QVector<QPointF>& in)
{
    int siz=in.count();
    for (int i = 1; i < siz; ++i)
    {
        for (int j = 0; j < siz- i; ++j)
        {
            if (i == 1)
            {
                tmp_points[j].X = (double)(points[j].X * (1 - t) + points[j + 1].X * t);
                tmp_points[j].Y = (double)(points[j].Y * (1 - t) + points[j + 1].Y * t);
                continue;
            }
            tmp_points[j].X = (double)(tmp_points[j].X * (1 - t) + tmp_points[j + 1].X * t);
            tmp_points[j].Y = (double)(tmp_points[j].Y * (1 - t) + tmp_points[j + 1].Y * t);
        }
    }
    return tmp_points[0];
}
void  MotorAction::get_points4bezier(GraphicsBezier* bez)
{
    double coefficient_axis=this->axisx_max_time_sec/SCENE_WIDTH;
    int time_span= static_cast<int>(ceil(qAbs( bez->m_points.last().x()-bez->m_points.first().x())*coefficient_axis*1000./this->time_div));//data num
    int cnt=bez->m_points.count();
    QPointF f;
    PointF *in = (PointF *)malloc(cnt* sizeof(PointF));
    for (int i = 0; i <cnt; i++)
    {
        f= bez->mapToScene(bez->m_points[i]);
        in[i].X = f.x();
        in[i].Y = f.y();
    }
    PointF out[time_span];
    draw_bezier_curves(in, out,time_span,bez->m_points);
    QPointF temp;
    for(int s=0;s<time_span;++s)
    {
        temp.setX(out[s].X);
        temp.setY(out[s].Y);
        this->m_trace.append(temp);
    }
    free(in);
}
void MotorAction::get_points(int step,QVector<QPointF>&in,QVector<QPointF>&out)
{
    double coefficient_axis=this->axisx_max_time_sec/SCENE_WIDTH;
    if(in.count()<2)  return;
    QVector<int>sz;//area
    QVector<double>k;//index
    QVector<double>b;

    for(int i=0;i<in.count()-1;++i)
    {
        int num_points=static_cast<int>(ceil(qAbs(in[i+1].x()-in[i].x())*coefficient_axis*1000./time_div));
        sz.append(num_points);
        k.append((in[i].y()-in[i+1].y())/(in[i].x()-in[i+1].x()));
        b.append(in[i].y()-k[i]*in[i].x());
        QVector<double> x;
        QPointF temp;
        for(int s=0;s<sz[i]; ++s)
        {
            x.append(in[i].x()+s/(coefficient_axis*1000./time_div));
            temp=QPointF(x[s],k[i]*x[s]+b[i]);
            out.append(temp);
        }
    }
}void MotorAction::handle_trace()
{

    this->step4point=1;
    if(m_items.size()==0)
        return;
    if(m_items.last()->displayName()=="line")
    {
        GraphicsLineItem* line=(GraphicsLineItem*)m_items.last();
        if(line!=nullptr)
        {
            QVector<QPointF> total;
            for(int z=0;z<line->m_points.count();++z)
            {
                QPointF temp= line->mapToScene(line->m_points[z]);
                total.append(temp);
            }
            get_points(this->step4point,total,m_trace);

            DrawScene* scene=dynamic_cast<DrawScene*>(line->scene());
            if(scene!=nullptr)
            {
                this->m_action_group=scene->m_save_ag;
            }

        }
    }else if(m_items.last()->displayName()=="bezier")
    {
        GraphicsBezier*bezier=(GraphicsBezier*)m_items.last();
        if(bezier!=nullptr)
        {
            get_points4bezier(bezier);

            DrawScene* scene=dynamic_cast<DrawScene*>(bezier->scene());
            if(scene!=nullptr)
            {
                this->m_action_group=scene->m_save_ag;
            }

        }
    }else if(m_items.last()->displayName()=="polyline")
    {
        GraphicsBezier* polyline=(GraphicsBezier*)m_items.last();
        if(polyline!=nullptr)
        {
            QVector<QPointF> total;
            for(int z=0;z<polyline->m_points.count();++z)
            {
                QPointF temp=polyline->mapToScene(polyline->m_points[z]);
                total.append(temp);
            }
            get_points(this->step4point,total,m_trace);
            DrawScene* scene=dynamic_cast<DrawScene*>(polyline->scene());
            if(scene!=nullptr)
            {
                this->m_action_group=scene->m_save_ag;

            }
        }
    }
#ifdef __DEBUG__
    qDebug("m_trace size:%d",m_trace.size());

    for(int x=0;x<m_trace.count();++x)
        qDebug("m_trace[%d]:x:%.2f,y:%.2f\n",x,m_trace[x].x(),m_trace[x].y());
#endif
    sort_unique_trace();
}


//void MotorAction::handle_trace()
//{

//    this->step4point=1;
//    if(m_items.size()==0)
//        return;
//    if(m_items.last()->displayName()=="line")
//    {
//        GraphicsLineItem* line=(GraphicsLineItem*)m_items.last();
//        if(line!=nullptr)
//        {
//            QVector<QPointF> total;
//            for(int z=0;z<line->m_points.count();++z)
//            {
//                QPointF temp= line->mapToScene(line->m_points[z]);
//                total.append(temp);
//            }
//            get_points(this->step4point,total,m_trace);

//            DrawScene* scene=dynamic_cast<DrawScene*>(line->scene());
//            if(scene!=nullptr)
//            {
//                this->m_action_group=scene->m_ag;
//            }

//        }
//    }else if(m_items.last()->displayName()=="bezier")
//    {
//        GraphicsBezier*bezier=(GraphicsBezier*)m_items.last();
//        if(bezier!=nullptr)
//        {
//            get_points4bezier(bezier);

//            DrawScene* scene=dynamic_cast<DrawScene*>(bezier->scene());
//            if(scene!=nullptr)
//            {
//                this->m_action_group=scene->m_ag;
//            }

//        }
//    }else if(m_items.last()->displayName()=="polyline")
//    {
//        GraphicsBezier* polyline=(GraphicsBezier*)m_items.last();
//        if(polyline!=nullptr)
//        {
//            QVector<QPointF> total;
//            for(int z=0;z<polyline->m_points.count();++z)
//            {
//                QPointF temp=polyline->mapToScene(polyline->m_points[z]);
//                total.append(temp);
//            }
//            get_points(this->step4point,total,m_trace);
//            DrawScene* scene=dynamic_cast<DrawScene*>(polyline->scene());
//            if(scene!=nullptr)
//            {
//                this->m_action_group=scene->m_ag;

//            }
//        }
//    }
//    qDebug("m_trace size:%d",m_trace.size());
//    for(int x=0;x<m_trace.count();++x)
//        qDebug("m_trace[%d]:x:%.2f,y:%.2f\n",x,m_trace[x].x(),m_trace[x].y());
//    sort_unique_trace();
//}

QVector<int> MotorAction::getSpeed()
{
    return speed;
}
bool positive_by_x_coordinate(QPointF p1,QPointF p2)
{
    return p1.x()<p2.x();
}

bool unique_by_x_coordinate(QPointF p1,QPointF p2)
{
    return p1.x()==p2.x();
}
void MotorAction::handle_speed()
{
    if(this->m_trace.count()<2)
        return ;
    int delta_angle;
    double delta_t=time_div/1000. ;//将毫秒转成秒；
    double s;
    int tmp=0;
    for(int i=0;i<m_trace.count();++i)
    {
        if(i>0) //目标点的值为目标角度减去当前角度再除以时间；
        {
            delta_angle=(qAbs(m_trace[i].y()-m_trace[i-1].y())/360.)*4096.;
            s=ceil(delta_angle/delta_t);

        }else  //第一个目标点速度值可以用下一个点的速度值,前提是点的个数要大于等于两个；
        {
            delta_angle=(qAbs(m_trace[i].y()-m_trace[i+1].y())/360.)*4096.;
            s=ceil(delta_angle/delta_t);
        }
        if(s>4094) s=4096;
        if(s==0.) s=1.;
        tmp=static_cast<int>(s);
        speed.append(tmp);
    }
}

void MotorAction::sort_unique_trace()
{
    vector<QPointF> stdvp= this->m_trace.toStdVector();
    sort(stdvp.begin(),stdvp.end(),positive_by_x_coordinate);
    vector<QPointF>::iterator itp=unique(stdvp.begin(),stdvp.end(),unique_by_x_coordinate);
    stdvp.erase(itp,stdvp.end());
    m_trace.clear();
    for(int k=0;k<stdvp.size();++k)
    {
        m_trace.append(stdvp[k]);
    }
}
double MotorAction::getConversion_coefficient_angle() const
{
    return conversion_coefficient_angle;
}

void MotorAction::setConversion_coefficient_angle(double value)
{
    conversion_coefficient_angle = value;
}

double MotorAction::getAxisx_max_time_sec() const
{
    return axisx_max_time_sec;
}

void MotorAction::setAxisx_max_time_sec(double value)
{
    axisx_max_time_sec = value;
}

double MotorAction::getTime_div() const
{
    return time_div;
}

void MotorAction::setTime_div(double value)
{
    time_div = value;
}
int MotorAction::getStep4point() const
{
    return step4point;
}

void MotorAction::setStep4point(int value)
{
    step4point = value;
}


