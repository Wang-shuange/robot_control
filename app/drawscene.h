#ifndef DRAWSCENE
#define DRAWSCENE

#include <QGraphicsScene>
#include "drawtool.h"
#include "drawobj.h"
#include "customslider.h"
#include "QxtSpanSlider.h"
#include "servoinfo.h"
#include "actiongroup.h"
#include<QLineEdit>

QT_BEGIN_NAMESPACE
class MyPushButton;
class QGraphicsSceneMouseEvent;
class QMenu;
class QPointF;
class QGraphicsLineItem;
class QFont;
class QGraphicsTextItem;
class QColor;
class QKeyEvent;
class QPainter;
QT_END_NAMESPACE

enum AlignType
{
    UP_ALIGN=0,
    HORZ_ALIGN,
    VERT_ALIGN,
    DOWN_ALIGN,
    LEFT_ALIGN,
    RIGHT_ALIGN,
    CENTER_ALIGN,
    HORZEVEN_ALIGN,
    VERTEVEN_ALIGN,
    WIDTH_ALIGN,
    HEIGHT_ALIGN,
    ALL_ALIGN
};

class GridTool
{
public:
    GridTool(const QSize &grid = QSize(3200,2400) , const QSize & space = QSize(20,20) );
    void paintGrid(QPainter *painter,const QRect & rect );
protected:
    QSize m_sizeGrid;
    QSize m_sizeGridSpace;
};

class GraphicsItemGroup;

class DrawScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit DrawScene(QObject *parent = 0);
    ~DrawScene() override;
    void setView(QGraphicsView * view ) { m_view = view ; }
    QGraphicsView * view() { return m_view; }
    void align(AlignType alignType );
    void mouseEvent(QGraphicsSceneMouseEvent *mouseEvent );
    GraphicsItemGroup * createGroup(const QList<QGraphicsItem *> &items ,bool isAdd = true);
    void destroyGroup(QGraphicsItemGroup *group);
signals:
    void sig_received_servo_info();
    void sig_selected_actiongroup_type(bool isSelected);
    void sig_fill_bezier_item(QVector<QPointF>v);
    void sig_update();
    void sig_load_audio();
    void sig_play_audio();
    void sig_pause_audio();
    void sig_fill_graphics(QString info);
    void itemMoved( QGraphicsItem * item , const QPointF & oldPosition );
    void itemRotate(QGraphicsItem * item , const qreal oldAngle );
    void itemAdded(QGraphicsItem * item );
    void itemResize(QGraphicsItem * item , int handle , const QPointF& scale );
    void itemControl(QGraphicsItem * item , int handle , const QPointF & newPos , const QPointF& lastPos_ );

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvet) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    QGraphicsView * m_view;

    qreal m_dx;
    qreal m_dy;
    bool  m_moved;
    GridTool *m_grid;
public:
    QString m_itemID;
    QList<QStringList> m_param;
    QMenu *pop_menu;
    QAction* start_action;
    QAction* item_edit_action;
    QAction *mouse_press_action;
    QAction *servo_origin_action;
    QAction *mouse_double_action;
    QAction *fill_item_action;
    QAction *line_action,*poyline_action,*bezier_action;
    QAction *del_item_action;
    QAction *finish_action;
    QAction *audio_action;
    QAction *load_audio_action;
    QAction *play_total_audio_action;
    QAction *play_part_audio_action;
    QAction *pause_audio_action;
    QAction *stop_audio_action;
    QAction *action_group_action;
    QAction *action_type_action;
    QAction *simple_mode_action;
    QAction *based_polyline_action,*based_line_action;

    QVector<QAction*> m_va;
    QVector<QPointF> m_scene_cur_point_list;
    bool isCurClick=false;
    bool isServoOrigin=true;
    bool isPreNode=false;
    QPointF servo_origin;
    QString graphics_item_received_info;
    QPointF fill_start,fill_end;
    bool isFill_item=false;
    void createActions();
    bool isPolyLine=false;
    bool isBezier=false;
    bool isLine=false;

    bool isCreateLine=false;
    bool isCreatePolyLine=false;

    CustomSlider *m_slider;
      QxtSpanSlider *horizontalSlider;
      GraphicsBezier* item;
      QPointF m_bezier_insert_start,m_bezier_insert_end;
      QPointF m_c1,m_c2;
        QList<QString> m_servo_name_list;
        QString m_select_name;
        QList<ServoInfo*> m_l_servoinfo;
        ServoInfo *m_cur_servoinfo;
        bool isAction_type_selected=false;
        ActionGroup * m_load_ag,*m_save_ag;
        MotorAction * m_save_ma,*m_load_ma;

        bool isSelected_type=false;
        bool isFinished_action=false;
       QDialog *dlg;
        QLineEdit * le_start_x,*le_start_y,*le_end_x,*le_end_y,*le_equidistant;
        QPointF m_simple_start,m_simple_end;
        int m_equidistant_num;
        bool isBased_line=false;
        bool isBased_polyline=false;


public slots:
    void get_mouse_press();
    void get_servo_origin();
    void get_mouse_double();
    void edit_graphics();
    void slt_graphics_item_selected(QString info);
    void add_item_with_fill(QVector<QPointF> v);
    void finish_create_action();
    void set_poyline();
    void set_bezier();
    void load_audio();
    void play_audio();
    void pause_audio();
    void lowerValueChangedSlot(int value);
    void upperValueChangedSlot(int value);
    void set_line();
    void slt_received_from_bezier(bool isPaint);
    void fill_bezier();
    void fill_polyline();
    void fill_line();
    void select_action_group_type();
    void finish_action_group();
    void select_servo_type_action();
    void slt_itemAdded(QGraphicsItem *item);
    void simple_mode_action_func();
    void slot_inputPoint(bool);
    void simple_mode_create_line(QVector<QPointF>& v, int equidistant_num);
    void simple_create_line(QVector<QPointF> &v);
    void simple_mode_action_polyline();
    void simple_mode_create_polyline(QVector<QPointF> &v, int equidistant_num);
    void simple_create_polyline(QVector<QPointF> &v);
//    void when_action_created_over(QGraphicsItem *item, MotorAction *ma);
    void get_servo_info();
    void start_action_create(MyPushButton *btn);
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

    // QObject interface
public:
//    bool eventFilter(QObject *watched, QEvent *event) override;
    void when_action_created_over(QGraphicsItem *item);
    void select_start_and_end_point();
};

#endif // DRAWSCENE

