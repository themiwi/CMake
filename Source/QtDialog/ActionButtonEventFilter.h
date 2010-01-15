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

#ifndef ActionButtonEventFilter_h
#define ActionButtonEventFilter_h

#include <QEvent>
#include <QTimer>

class QWidget;

/// Event filter to create QToolButton::DelayedPopupMode behavior for a
/// QPushButton
class ActionButtonEventFilter : public QObject
{
  Q_OBJECT

  /// Popup-delay, defaults to the style-defined delay of the parent if not
  /// NULL, 600ms otherwise.
  Q_PROPERTY(int Delay READ getDelay WRITE setDelay)

public:
  ActionButtonEventFilter(QWidget* parent);

  int getDelay() const;
  void setDelay(int delay);

signals:
  /// Emitted when the default action should be used
  void activateDefaultAction();
  /// Emitted when the button menu should be displayed
  void showMenu();

protected:
  bool eventFilter(QObject *obj, QEvent *event);

private:
  int     Delay;
  QTimer* Timer;

};

#endif // ActionButtonEventFilter_h
