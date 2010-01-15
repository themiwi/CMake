/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2009-2009 Michael Wild <themiwi@users.sourceforge.net>
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/

#include "ActionButtonEventFilter.h"
#include <QWidget>
#include <QStyle>
#include <QMouseEvent>

ActionButtonEventFilter::ActionButtonEventFilter(QWidget* parent)
  : QObject(parent)
  , Delay(600)
{
  this->Timer = new QTimer(this);
  this->Timer->setSingleShot(true);
  // chain the timer's timeout() signal to our showMenu() signal
  QObject::connect(this->Timer, SIGNAL(timeout()), this, SIGNAL(showMenu()));
  if(parent)
    {
      // use the style-sheet defined popup-delay
    this->setDelay(parent->style()->styleHint(
          QStyle::SH_ToolButton_PopupDelay, 0, parent));
    }
}

int ActionButtonEventFilter::getDelay() const
{
  return this->Delay;
}

void ActionButtonEventFilter::setDelay(int delay)
{
  this->Delay = delay;
}

bool ActionButtonEventFilter::eventFilter(QObject *obj, QEvent *event)
{
  // only handle mouse-button press and release
  if(event->type() == QEvent::MouseButtonPress ||
     event->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    // only handle the left mouse-button
    if(mouseEvent->button() == Qt::LeftButton)
      {
      if(mouseEvent->type() == QEvent::MouseButtonPress)
        {
        // on press, start timer and eat event
        this->Timer->start(this->Delay);
        return true;
        }
      else if(this->Timer->isActive())
        {
        // on release, stop timer, fire default action and eat event
        this->Timer->stop();
        emit this->activateDefaultAction();
        return true;
        }
      }
    }
  // normal processing
  return QObject::eventFilter(obj, event);
}
