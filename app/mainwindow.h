#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMap>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include "drawscene.h"
#include "objectcontroller.h"
#include "drawview.h"
#include<QSerialPort>
#include "mycombobox.h"
#include "mychart.h"
#include "QxtSpanSlider.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtWidgets/QVBoxLayout>
#include <QtCharts/QValueAxis>
#include <QMetaType>
#include "myplay.h"
#include <QMetaType>


QT_BEGIN_NAMESPACE
const int MAX_POINT_COUNT=1000;
const int MAX_ACTION_COUNT=22;
const int PARAM_COUNT=4;
class QAction;
class QActionGroup;
class QToolBox;
class QSpinBox;
class QComboBox;
class MyCheckBox;
class QFontComboBox;
class QButtonGroup;
class QLineEdit;
class QGraphicsTextItem;
class QFont;
class QToolButton;
class QAbstractButton;
class QGraphicsView;
class QListWidget;
class QStatusBar;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
class QUndoStack;
class QUndoView;
class DrawView;
class QSerialPort;
QT_END_NAMESPACE

class QtVariantProperty;
class QtProperty;

class MyPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit MyPushButton(int index, QWidget *parent = 0) : QPushButton(parent), index(index)
    {
        //        connect(this, SIGNAL(clicked()), this, SLOT(OnClicked()));
    }
public:
    int index;
public slots:
    //    void OnClicked(){
    //        qDebug() << index;//你可以在这里emit(index),槽函数就可以根据index知道是哪个按钮发出的信号}
    //    };
};


class MyCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    explicit MyCheckBox(int index, QWidget *parent = 0) : QCheckBox(parent), index(index)
    {
        //        connect(this, SIGNAL(clicked()), this, SLOT(OnClicked()));
    }
public:
    int index;
public slots:

};
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool openFile(const QString &fileName);

public slots:

    void newFile();
    void open();
    void save();
    DrawView *createMdiChild();
    void updateMenus();
    void updateWindowMenu();
    void setActiveSubWindow(QWidget *window);
    void servo_handle(quint8 *id, quint16 *angle, quint16 *speed, int len);
    void addShape();
    void updateActions();
    void deleteItem();
    void itemSelected();
    void itemMoved(QGraphicsItem * item , const QPointF & oldPosition );
    void itemAdded(QGraphicsItem * item );
    void itemRotate(QGraphicsItem * item , const qreal oldAngle );
    void itemResize(QGraphicsItem * item , int handle , const QPointF& scale );
    void itemControl(QGraphicsItem * item , int handle , const QPointF & newPos , const QPointF& lastPos_ );

    void on_actionBringToFront_triggered();
    void on_actionSendToBack_triggered();
    void on_aglin_triggered();
    void zoomIn();
    void zoomOut();
    void on_group_triggered();
    void on_unGroup_triggered();
    void on_func_test_triggered();
    void on_copy();
    void on_paste();
    void on_cut();
    void dataChanged();
    void positionChanged(int x, int y );

    void about();
    void receiveActionChange(QStringList la,QStringList ls);
    void slot_tableItemSelect();
    void slot_play();
    void slot_com_conf();
    void slot_net_conf();
    void slot_com_connect();
    void slot_getPort(QString port);
    void receiveInfo();
    void slot_inputParameter();
    void sendCommand();
    void slot_stop();
    void slot_com_disconnect();
    void setLineEditValue(int value);
    void slot_loadCom();
    void slot_closeDrawView();
    void slot_play_all_audio();
    void slot_play_partial_audio();
    void slot_load_audio();
    void slt_request_load();
    void slt_request_play();
    void slt_get_wave_vector(QVector<qint16> val);
    void slt_duration(quint64 dur);
    void slt_request_play_all();
    void show_play_pos();
    void slt_fill_graphics(QString info);
    void slt_selected_actiongroup_type(bool isSelected);
    void create_action_list();
    void get_servo_info_main();
    void slt_btn_hides_clicked(bool b);
    void slt_btn_locks_clicked(bool b);
    QString get_servo_name_by_id(int id);
    bool openFile4save(const QString &fileName);
    void slt_check_item_selected();
    void item_control_hide(MyPushButton *btn_action, MyPushButton *btn_hide, bool &flag);
    void item_control_lock(MyPushButton *btn_action, MyPushButton *btn_lock, bool &flag);
    void slt_btn_actions_clicked(bool);
    void actions_control_select(MyPushButton *btn_action, bool &flag);
    void slt_btn_stretchs_clicked(bool b);
    void actions_control_stretch(MyPushButton *btn_action, bool &flag);
    void slt_check_action_status(bool flag);
    void slt_btn_hide_others_clicked(bool b);
    void actions_control_hide_others(MyPushButton *btn_action, MyPushButton *btn_hideothers, bool &flag);
    void slt_ckb_action_stateChanged(int index);
    void get_selected_actions(QVector<MotorAction *> &willsends);
    void slt_reload_audio();
protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    void createActions();
    void createMenus();
    void createToolbars();
    void createPropertyEditor();
    void createToolBox();

    DrawView *activeMdiChild();
    QMdiSubWindow *findMdiChild(const QString &fileName);
public:
    MyPlay *play;
    QMenu *windowMenu;
public:
    QMdiArea *mdiArea;
    QSignalMapper *windowMapper;

    // update ui
    QTimer      m_timer;
    QTimer   *m_check_item_bselected;
    QTimer   *m_check_action_status;
    // toolbox
    QToolBox *toolBox;
    // edit toolbar;
    QToolBar * editToolBar;
    // align toolbar
    QToolBar * alignToolBar;
    QAction  * rightAct;
    QAction  * leftAct;
    QAction  * vCenterAct;
    QAction  * hCenterAct;
    QAction  * upAct;
    QAction  * downAct;
    QAction  * horzAct;
    QAction  * vertAct;
    QAction  * heightAct;
    QAction  * widthAct;
    QAction  * allAct;
    QAction  * bringToFrontAct;
    QAction  * sendToBackAct;

    QAction  * funcAct;
    // file
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *exitAct;
    QAction *comAct;
    QAction *netAct;

    QAction  * groupAct;
    QAction  * unGroupAct;

    // edit action
    QAction  * deleteAct;
    QAction  * undoAct;
    QAction  * redoAct;
    QAction  * copyAct;
    QAction  * pasteAct;
    QAction  * cutAct;
    QAction  * zoomInAct;
    QAction  * zoomOutAct;

    // audio action
    QAction  * loadAct;
    QAction  * playAllAct;
    QAction  * playPartialAct;

    // drawing toolbar
    QToolBar * drawToolBar;
    QActionGroup * drawActionGroup;
    QAction  * selectAct;
    QAction  * lineAct;
    QAction  * rectAct;
    QAction  * roundRectAct;
    QAction  * ellipseAct;
    QAction  * polygonAct;
    QAction  * polylineAct;
    QAction  * bezierAct;
    QAction  * rotateAct;

    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *separatorAct;
    QAction *aboutAct;
    QAction *aboutQtAct;

    //property editor
    QDockWidget *dockProperty;
    ObjectController *propertyEditor;
    QObject *theControlledObject;

    QTableWidget* tableWidget;
    QUndoStack *undoStack;
    QUndoView *undoView;
    DrawView *view;
public:
    QList<ServoInfo*> m_list_servoinfo;
   QVector<MotorAction*> m_selected_action;
public:
    MyPushButton* btn_hides[MAX_ACTION_COUNT]={nullptr};
    MyPushButton* btn_locks[MAX_ACTION_COUNT]={nullptr};
    MyPushButton* btn_stretchs[MAX_ACTION_COUNT]={nullptr};
    MyPushButton* btn_actions[MAX_ACTION_COUNT]={nullptr};
    MyPushButton* btn_hide_others[MAX_ACTION_COUNT]={nullptr};
    MyCheckBox*  ckb_actions[MAX_ACTION_COUNT]={nullptr};
    //    DrawScene *curSence;
    // statusbar label
    QLabel *m_posInfo;
    GraphicsItem *item;
    void prepare_audio();
public:
    QSerialPort * m_serialPort; //串口类
    QStringList m_portNameList;
    QString m_serialPortName;
    QDialog *dlg;
    QList<QGraphicsItem*> items_selected;
    QList<GraphicsItem*> items_selected_t;

    QLineEdit* le_actionID;
    QLineEdit* le_servoID;
    void sendInfo(const QString &info);
    void sendInfo(char *info, int len);
    char convertCharToHex(char ch);
    void convertStringToHex(const QString &str, QByteArray &byteData);
    QTimer* m_t,*m_pos;
    bool bPlay=false;
    QPushButton *btn_play,*btn_stop;
    QList<QList<QStringList>> total_parameter;
    QString param[MAX_ACTION_COUNT][PARAM_COUNT][MAX_POINT_COUNT];
    qint8 Info_sz;
    QString m_audio_src;
    int Info_st[MAX_ACTION_COUNT];
    int Info_ed[MAX_ACTION_COUNT];
    int total_st,total_ed;
    int command_cnt=0;
    quint8 last_servo_ids[MAX_ACTION_COUNT];
    quint16 last_angles[MAX_ACTION_COUNT];
    quint16 last_speeds[MAX_ACTION_COUNT];
    QLineEdit *lineEdit;
    QSlider* slider;
    MyComboBox* combo_port;
    QVector<QGraphicsItem*>lines;
    MyChart*m_chart;
    QLineSeries *m_series;
    quint64 m_curr_dur;
    ActionGroup * main_load_ag,*main_save_ag;

    QString m_graphics_item_info;
    QPointF m_fill_start,m_fill_end,m_fill_start_last,m_fill_end_last;
    bool m_isSelected_servo_type=false;
    bool m_isFill=false;
    void getInfo();
    void intToByte(qint16 i, char*out);
    qint16 bytesToInt(char* in);
    void assemblyCommand(quint8* id, quint16* al, quint16* sp, qint8 len);

    quint8 CheckSum(quint8 *buff, int Size);
    void addActionID();
    void clearTableWidget();



    void addUndoStackDock();
    qint64 writeData_vector(QVector<qint16> &v);
    void drawPalyHeader_ex(qreal pos);
    void get_item_start_end(GraphicsItem *item);
    DrawScene *get_scene();
signals:
    void sig_play(char*url);
    void sig_load(char*url,QString cmd);
    void sig_play_all(QString cmd);
    void sig_send_duration(quint64 dur);
    void sig_graphics_item_selected(QString name);
    void sig_fill_data_to_scene(QVector<QPointF>);
};

#endif // MAINWINDOW_H
