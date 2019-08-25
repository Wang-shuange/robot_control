#ifndef ACTIONGROUP_H
#define ACTIONGROUP_H

#include <QObject>
#include "motoraction.h"
class MotorAction;
class CarAction:public QObject
{
    Q_OBJECT
public:
    explicit CarAction(QObject *parent=nullptr);
    int getId() const;
    void setId(int value);

private:
    int id;
};
class CarAction;
class ActionGroup : public QObject
{
    Q_OBJECT
public:
    explicit ActionGroup(QObject *parent = nullptr);
    QString getMusic_src() const;
    void setMusic_src(const QString &value);

    QVector<MotorAction *> getActions() const;
    void setActions(const QVector<MotorAction *> &actions);

private:
    QString music_src;
public:
    QVector<MotorAction*> m_actions;
    QVector<CarAction*> m_car_actions;

signals:

public slots:
};

#endif // ACTIONGROUP_H
