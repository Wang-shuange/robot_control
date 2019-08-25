#include "actiongroup.h"

ActionGroup::ActionGroup(QObject *parent) : QObject(parent)
{
   this->music_src=" ";
    this->m_actions.clear();
    this->m_car_actions.clear();
}

QString ActionGroup::getMusic_src() const
{
    return music_src;
}

void ActionGroup::setMusic_src(const QString &value)
{
    music_src = value;
}

QVector<MotorAction *> ActionGroup::getActions() const
{
    return m_actions;
}

void ActionGroup::setActions(const QVector<MotorAction *> &actions)
{
    m_actions = actions;
}

CarAction::CarAction(QObject *parent):QObject(parent)
{
    this->id=0;
}

int CarAction::getId() const
{
    return id;
}

void CarAction::setId(int value)
{
    id = value;
}
