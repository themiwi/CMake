/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/

#ifndef CMakeSetupDialog_h
#define CMakeSetupDialog_h

#include "QCMake.h"
#include <QMainWindow>
#include <QThread>
#include <QSharedPointer>
#include "ui_CMakeSetupDialog.h"

class QCMakeThread;
class CMakeCacheModel;
class QProgressBar;
class QToolButton;

/// Qt user interface for CMake
class CMakeSetupDialog : public QMainWindow, public Ui::CMakeSetupDialog
{
  Q_OBJECT
public:
  CMakeSetupDialog();
  ~CMakeSetupDialog();

public slots:
  void setBinaryDirectory(const QString& dir);
  void setSourceDirectory(const QString& dir);

protected slots: 
  void initialize();
  void doDefaultAction();
  void doConfigure();
  void doGenerate();
  void doBuild();
  void doInstall();
  void doClean();
  void doInstallForCommandLine();
  void doHelp();
  void doAbout();
  void doInterrupt();
  void finishConfigure(int error);
  void finishGenerate(int error);
  void finishBuild(int error);
  void finishInstall(int error);
  void finishClean(int error);
  void error(const QString& message);
  void message(const QString& message);
  
  void doSourceBrowse();
  void doSourceOpen();
  void doBinaryBrowse();
  void doBinaryOpen();
  void doReloadCache();
  void doDeleteCache();
  void updateSourceDirectory(const QString& dir);
  void updateBinaryDirectory(const QString& dir);
  void showProgress(const QString& msg, float percent);
  void setEnabledState(bool);
  bool setupFirstConfigure();
  void updateGeneratorLabel(const QString& gen);
  void setExitAfterGenerate(bool);
  void addBinaryPath(const QString&);
  QStringList loadBuildPaths();
  void saveBuildPaths(const QStringList&);
  void onBinaryDirectoryChanged(const QString& dir);
  void onSourceDirectoryChanged(const QString& dir);
  void setCacheModified();
  void removeSelectedCacheEntries();
  void selectionChanged();
  void addCacheEntry();
  void startSearch();
  void setDebugOutput(bool);
  void setViewType(int);
  void showUserChanges();
  void setSearchFilter(const QString& str);
  void activeConfigurationChanged(bool checked);
  void activeConfigurationChanged(int index);

protected:

  enum State { Interrupting, ReadyConfigure, ReadyGenerate, ReadyBuild,
    ReadyInstall, Configuring, Generating, Building, Installing, Cleaning };
  void enterState(State s);
  void setDefaultAction(QAction* action);
  void manageConfigs();

  void closeEvent(QCloseEvent*);
  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);

  QCMakeThread* CMakeThread;
  bool ExitAfterGenerate;
  bool CacheModified;
  QAction* ReloadCacheAction;
  QAction* DeleteCacheAction;
  QAction* OpenSourceDirectoryAction;
  QAction* OpenBuildDirectoryAction;
  QAction* ExitAction;
  QAction* ConfigureAction;
  QAction* StopAction;
  QAction* GenerateAction;
  QAction* BuildAction;
  QAction* CleanAction;
  QAction* InstallAction;
  QAction* DefaultAction;
  QMenu*   ActiveConfigurationMenu;
  QSharedPointer<QActionGroup> ConfigurationActionsGroup;
  QAction* SuppressDevWarningsAction;
  QAction* InstallForCommandLineAction;
  State CurrentState;
  QList<QAction*> ConfigurationActions;
  QString CurrentConfiguration;

  QTextCharFormat ErrorFormat;
  QTextCharFormat MessageFormat;

};

// QCMake instance on a thread
class QCMakeThread : public QThread
{
  Q_OBJECT
public:
  QCMakeThread(QObject* p);
  QCMake* cmakeInstance() const;
  
signals:  
  void cmakeInitialized();

protected:
  virtual void run();
  QCMake* CMakeInstance;
};

#endif // CMakeSetupDialog_h
