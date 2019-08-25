#ifndef DRAWVIEW_H
#define DRAWVIEW_H
#include <QGraphicsView>

#include "rulebar.h"
#include "drawobj.h"
#include "mainwindow.h"

class QMouseEvent;
class MainWindow;

class DrawView : public QGraphicsView
{
    Q_OBJECT
public:
    DrawView(QGraphicsScene *scene);
 ~DrawView() override;

    void zoomIn();
    void zoomOut();

    void newFile();
    bool loadFile(const QString &fileName);
    bool save();
     bool saveAs();
    bool saveFile(const QString &fileName);
    QString userFriendlyCurrentFile();

    QString currentFile() { return curFile; }
    void setModified( bool value ) { modified = value ; }
    bool isModified() const { return modified; }
    QStringList getAttribute(QXmlStreamReader &xml, QString element, QString attribute);
    QStringList sendInfo2Table(QString fileName, QString element, QString attr);

    void jump2Element(QXmlStreamReader *xml, QString desElement);
    void jump2LowerElement(QXmlStreamReader *xml, QString desElement);
    QList<QStringList> getAttributes_ex(QXmlStreamReader &xml, QString element, QString actid, QString actid_value, QString attribute);
   QList<QStringList> getparameter(QString fileName, QString element, QString actid, QString actid_value, QString attr);
public slots:
   void selectedByTable();
   void slt_receive_duration(quint64 dur);
   void set_actions_button();
signals:
    void positionChanged(int x , int y );
    void actionInfoChanged(QStringList la,QStringList ls);
    void receivedParameter(QList<QList<QStringList>> ts);
    void closeDrawView();
public:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void scrollContentsBy(int dx, int dy) Q_DECL_OVERRIDE;
    void updateRuler();
    QtRuleBar *m_hruler;
    QtRuleBar *m_vruler;
    QtCornerBox * box;
private:
    bool maybeSave();
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);
    void loadCanvas( QXmlStreamReader *xml );
    GraphicsItemGroup * loadGroupFromXML( QXmlStreamReader * xml );

    QString curFile;
    bool isUntitled;
    bool modified;
    quint64 m_dur;
public:
    MainWindow* m_parent_window;
public:
    QList<QList<QStringList>> ts;
    bool saveFile_Ex(const QString &fileName);


    // QWidget interface
    bool loadFile_ex(const QString &fileName);
    bool saveAs_ex();
    bool save_ex();
protected:
//    void paintEvent(QPaintEvent *event);
};

#endif // DRAWVIEW_H
