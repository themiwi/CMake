/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/

#include "CMakeSetupDialog.h"
#include <QFileDialog>
#include <QProgressBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QSettings>
#include <QMenu>
#include <QMenuBar>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QShortcut>
#include <QDesktopServices>
#include <QMacInstallDialog.h>

#include "QCMake.h"
#include "QCMakeCacheView.h"
#include "AddCacheEntry.h"
#include "FirstConfigure.h"
#include "ActionButtonEventFilter.h"
#include "cmVersion.h"

QCMakeThread::QCMakeThread(QObject* p) 
  : QThread(p), CMakeInstance(NULL)
{
}

QCMake* QCMakeThread::cmakeInstance() const
{
  return this->CMakeInstance;
}

void QCMakeThread::run()
{
  this->CMakeInstance = new QCMake;
  // emit that this cmake thread is ready for use
  emit this->cmakeInitialized();
  this->exec();
  delete this->CMakeInstance;
  this->CMakeInstance = NULL;
}

CMakeSetupDialog::CMakeSetupDialog()
  : ExitAfterGenerate(true), CacheModified(false), CurrentState(Interrupting),
    CurrentConfiguration()
{
  QString title = QString(tr("CMake %1"));
  title = title.arg(cmVersion::GetCMakeVersion());
  this->setWindowTitle(title);

  // create the GUI
  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  int h = settings.value("Height", 500).toInt();
  int w = settings.value("Width", 700).toInt();
  this->resize(w, h);

  QWidget* cont = new QWidget(this);
  this->setupUi(cont);
  this->Splitter->setStretchFactor(0, 3);
  this->Splitter->setStretchFactor(1, 1);
  this->setCentralWidget(cont);
  this->ProgressBar->reset();
  this->RemoveEntry->setEnabled(false);
  this->AddEntry->setEnabled(false);
  
  QByteArray p = settings.value("SplitterSizes").toByteArray();
  this->Splitter->restoreState(p);

  bool groupView = settings.value("GroupView", false).toBool();
  if(groupView)
  {
    this->setViewType(2);
    this->ViewType->setCurrentIndex(2);
  }

  QMenu* FileMenu = this->menuBar()->addMenu(tr("&File"));
  this->ReloadCacheAction = FileMenu->addAction(tr("&Reload Cache"));
  QObject::connect(this->ReloadCacheAction, SIGNAL(triggered(bool)), 
                   this, SLOT(doReloadCache()));
  this->DeleteCacheAction = FileMenu->addAction(tr("&Delete Cache"));
  QObject::connect(this->DeleteCacheAction, SIGNAL(triggered(bool)), 
                   this, SLOT(doDeleteCache()));
  FileMenu->addSeparator();
  this->OpenSourceDirectoryAction = FileMenu->addAction(tr("Open &Source Directory"));
  QObject::connect(this->OpenSourceDirectoryAction, SIGNAL(triggered(bool)),
                   this, SLOT(doSourceOpen()));
  this->OpenBuildDirectoryAction = FileMenu->addAction(tr("Open &Build Directory"));
  QObject::connect(this->OpenBuildDirectoryAction, SIGNAL(triggered(bool)),
                   this, SLOT(doBinaryOpen()));
#if !defined(Q_WS_MAC)
  FileMenu->addSeparator();
#endif
  this->ExitAction = FileMenu->addAction(tr("E&xit"));
  QObject::connect(this->ExitAction, SIGNAL(triggered(bool)), 
                   this, SLOT(close()));

  QMenu* ToolsMenu = this->menuBar()->addMenu(tr("&Tools"));
  this->ConfigureAction = ToolsMenu->addAction(tr("&Configure"));
  // prevent merging with Preferences menu item on Mac OS X
  this->ConfigureAction->setMenuRole(QAction::NoRole);
  QObject::connect(this->ConfigureAction, SIGNAL(triggered(bool)), 
                   this, SLOT(doConfigure()));
  this->StopAction = ToolsMenu->addAction(tr("&Stop"));
  this->StopAction->setEnabled(false);
  this->StopAction->setVisible(false);
  QObject::connect(this->StopAction, SIGNAL(triggered(bool)),
                   this, SLOT(doInterrupt()));
  this->GenerateAction = ToolsMenu->addAction(tr("&Generate"));
  QObject::connect(this->GenerateAction, SIGNAL(triggered(bool)), 
                   this, SLOT(doGenerate()));
  ToolsMenu->addSeparator();
  this->BuildAction = ToolsMenu->addAction(tr("&Build Project"));
  QObject::connect(this->BuildAction, SIGNAL(triggered(bool)),
                   this, SLOT(doBuild()));
  this->InstallAction = ToolsMenu->addAction(tr("&Install Project"));
  QObject::connect(this->InstallAction, SIGNAL(triggered(bool)),
                   this, SLOT(doInstall()));
  this->CleanAction = ToolsMenu->addAction(tr("&Clean Project"));
  QObject::connect(this->CleanAction, SIGNAL(triggered(bool)),
                   this, SLOT(doClean()));
  ToolsMenu->addSeparator();
  this->ActiveConfigurationMenu = ToolsMenu->addMenu(tr("Active Configuration"));
#if !(defined(QT_MAC_USE_COCOA) && QT_VERSION == 0x040503)
  // Don't disable the menu when using Qt-Cocoa 4.5.3:
  // http://bugreports.qt.nokia.com/browse/QTBUG-5313
  this->ActiveConfigurationMenu->setEnabled(false);
#endif
  ToolsMenu->addSeparator();
  QAction* showChangesAction = ToolsMenu->addAction(tr("&Show My Changes"));
  QObject::connect(showChangesAction, SIGNAL(triggered(bool)), 
                   this, SLOT(showUserChanges()));
#if defined(Q_WS_MAC)
  this->InstallForCommandLineAction 
    = ToolsMenu->addAction(tr("&Install For Command Line Use"));
  QObject::connect(this->InstallForCommandLineAction, SIGNAL(triggered(bool)), 
                   this, SLOT(doInstallForCommandLine()));
#endif  
  QMenu* OptionsMenu = this->menuBar()->addMenu(tr("&Options"));
  this->SuppressDevWarningsAction = OptionsMenu->addAction(tr("&Suppress dev Warnings (-Wno-dev)"));
  this->SuppressDevWarningsAction->setCheckable(true);

  QAction* debugAction = OptionsMenu->addAction(tr("&Debug Output"));
  debugAction->setCheckable(true);
  QObject::connect(debugAction, SIGNAL(toggled(bool)), 
                   this, SLOT(setDebugOutput(bool)));
  
  OptionsMenu->addSeparator();
  QAction* expandAction = OptionsMenu->addAction(tr("&Expand Grouped Entries"));
  QObject::connect(expandAction, SIGNAL(triggered(bool)), 
                   this->CacheValues, SLOT(expandAll()));
  QAction* collapseAction = OptionsMenu->addAction(tr("&Collapse Grouped Entries"));
  QObject::connect(collapseAction, SIGNAL(triggered(bool)), 
                   this->CacheValues, SLOT(collapseAll()));

  QMenu* HelpMenu = this->menuBar()->addMenu(tr("&Help"));
  QAction* a = HelpMenu->addAction(tr("About"));
  QObject::connect(a, SIGNAL(triggered(bool)),
                   this, SLOT(doAbout()));
  a = HelpMenu->addAction(tr("Help"));
  QObject::connect(a, SIGNAL(triggered(bool)),
                   this, SLOT(doHelp()));
  
  QShortcut* filterShortcut = new QShortcut(QKeySequence::Find, this);
  QObject::connect(filterShortcut, SIGNAL(activated()), 
                   this, SLOT(startSearch()));
  
  this->setAcceptDrops(true);
  
  // get the saved binary directories
  QStringList buildPaths = this->loadBuildPaths();
  this->BinaryDirectory->addItems(buildPaths);
 
  this->BinaryDirectory->setCompleter(new QCMakeFileCompleter(this, true));
  this->SourceDirectory->setCompleter(new QCMakeFileCompleter(this, true));

  // fixed pitch font in output window
  QFont outputFont("Courier");
  this->Output->setFont(outputFont);
  this->ErrorFormat.setForeground(QBrush(Qt::red));

  // set up the action-button
  QMenu* ActionMenu = new QMenu(this->ActionButton);
  ActionMenu->addAction(this->ConfigureAction);
  ActionMenu->addAction(this->StopAction);
  ActionMenu->addAction(this->GenerateAction);
  ActionMenu->addAction(this->BuildAction);
  ActionMenu->addAction(this->InstallAction);
  ActionMenu->addAction(this->CleanAction);
  this->ActionButton->setMenu(ActionMenu);
  ActionButtonEventFilter* actionEventFilter = new ActionButtonEventFilter(this->ActionButton);
  QObject::connect(actionEventFilter, SIGNAL(activateDefaultAction()),
      this, SLOT(doDefaultAction()));
  QObject::connect(actionEventFilter, SIGNAL(showMenu()),
      this->ActionButton, SLOT(showMenu()));
  this->ActionButton->installEventFilter(actionEventFilter);
  this->setDefaultAction(this->ConfigureAction);

  this->ActiveConfigurationPopup->setVisible(false);

  // start the cmake worker thread
  this->CMakeThread = new QCMakeThread(this);
  QObject::connect(this->CMakeThread, SIGNAL(cmakeInitialized()),
                   this, SLOT(initialize()), Qt::QueuedConnection);  
  this->CMakeThread->start();
  
  this->enterState(ReadyConfigure);
}

void CMakeSetupDialog::initialize()
{
  // now the cmake worker thread is running, lets make our connections to it
  QObject::connect(this->CMakeThread->cmakeInstance(), 
      SIGNAL(propertiesChanged(const QCMakePropertyList&)),
      this->CacheValues->cacheModel(),
      SLOT(setProperties(const QCMakePropertyList&)));

  QObject::connect(this->CMakeThread->cmakeInstance(), 
                   SIGNAL(configureDone(int)),
                   this, SLOT(finishConfigure(int)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(generateDone(int)),
                   this, SLOT(finishGenerate(int)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(buildDone(int)),
                   this, SLOT(finishBuild(int)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(installDone(int)),
                   this, SLOT(finishInstall(int)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(cleanDone(int)),
                   this, SLOT(finishClean(int)));
  
  QObject::connect(this->BrowseSourceDirectoryButton, SIGNAL(clicked(bool)),
                   this, SLOT(doSourceBrowse()));
  QObject::connect(this->OpenSourceDirectoryButton, SIGNAL(clicked(bool)),
                   this, SLOT(doSourceOpen()));
  QObject::connect(this->BrowseBinaryDirectoryButton, SIGNAL(clicked(bool)),
                   this, SLOT(doBinaryBrowse()));
  QObject::connect(this->OpenBinaryDirectoryButton, SIGNAL(clicked(bool)),
                   this, SLOT(doBinaryOpen()));
  
  QObject::connect(this->BinaryDirectory, SIGNAL(editTextChanged(QString)),
                   this, SLOT(onBinaryDirectoryChanged(QString)));
  QObject::connect(this->SourceDirectory, SIGNAL(textChanged(QString)),
                   this, SLOT(onSourceDirectoryChanged(QString)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(sourceDirChanged(QString)),
                   this, SLOT(updateSourceDirectory(QString)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(binaryDirChanged(QString)),
                   this, SLOT(updateBinaryDirectory(QString)));
  
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(progressChanged(QString, float)),
                   this, SLOT(showProgress(QString,float)));
  
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(errorMessage(QString)),
                   this, SLOT(error(QString)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(outputMessage(QString)),
                   this, SLOT(message(QString)));

  QObject::connect(this->ViewType, SIGNAL(currentIndexChanged(int)), 
                   this, SLOT(setViewType(int)));
  QObject::connect(this->Search, SIGNAL(textChanged(QString)), 
                   this, SLOT(setSearchFilter(QString)));
  
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(generatorChanged(QString)),
                   this, SLOT(updateGeneratorLabel(QString)));
  this->updateGeneratorLabel(QString());
  
  QObject::connect(this->CacheValues->cacheModel(),
                   SIGNAL(dataChanged(QModelIndex,QModelIndex)), 
                   this, SLOT(setCacheModified()));
  
  QObject::connect(this->CacheValues->selectionModel(),
                   SIGNAL(selectionChanged(QItemSelection,QItemSelection)), 
                   this, SLOT(selectionChanged()));
  QObject::connect(this->RemoveEntry, SIGNAL(clicked(bool)), 
                   this, SLOT(removeSelectedCacheEntries()));
  QObject::connect(this->AddEntry, SIGNAL(clicked(bool)), 
                   this, SLOT(addCacheEntry()));

  QObject::connect(this->SuppressDevWarningsAction, SIGNAL(triggered(bool)), 
                   this->CMakeThread->cmakeInstance(), SLOT(setSuppressDevWarnings(bool)));
  
  if(!this->SourceDirectory->text().isEmpty() ||
     !this->BinaryDirectory->lineEdit()->text().isEmpty())
    {
    this->onSourceDirectoryChanged(this->SourceDirectory->text());
    this->onBinaryDirectoryChanged(this->BinaryDirectory->lineEdit()->text());
    }
  else
    {
    this->onBinaryDirectoryChanged(this->BinaryDirectory->lineEdit()->text());
    }
}

CMakeSetupDialog::~CMakeSetupDialog()
{
  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  settings.setValue("Height", this->height());
  settings.setValue("Width", this->width());
  settings.setValue("SplitterSizes", this->Splitter->saveState());

  // wait for thread to stop
  this->CMakeThread->quit();
  this->CMakeThread->wait(2000);
}

void CMakeSetupDialog::doDefaultAction()
{
  // doing it this way so we don't have to disconnect/connect all the time when
  // changing the default action. also prevent the action from triggering if it
  // is disabled.
  if(this->DefaultAction->isEnabled())
    {
    this->DefaultAction->trigger();
    }
}

void CMakeSetupDialog::doConfigure()
{
  // make sure build directory exists
  QString bindir = this->CMakeThread->cmakeInstance()->binaryDirectory();
  QDir dir(bindir);
  if(!dir.exists())
    {
    QString msg = tr("Build directory does not exist, "
                         "should I create it?")
                      + "\n\n"
                      + tr("Directory: ");
    msg += bindir;
    QString title = tr("Create Directory");
    QMessageBox::StandardButton btn;
    btn = QMessageBox::information(this, title, msg, 
                                   QMessageBox::Yes | QMessageBox::No);
    if(btn == QMessageBox::No)
      {
      return;
      }
    if(!dir.mkpath("."))
      {
      QMessageBox::information(this, tr("Create Directory Failed"), 
        QString(tr("Failed to create directory %1")).arg(dir.path()), 
        QMessageBox::Ok);

      return;
      }
    }

  // if no generator, prompt for it and other setup stuff
  if(this->CMakeThread->cmakeInstance()->generator().isEmpty())
    {
    if(!this->setupFirstConfigure())
      {
      return;
      }
    }

  // remember path
  this->addBinaryPath(dir.absolutePath());
    
  this->enterState(Configuring);

  this->CacheValues->selectionModel()->clear();
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "setProperties", Qt::QueuedConnection, 
    Q_ARG(QCMakePropertyList,
      this->CacheValues->cacheModel()->properties()));
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "configure", Qt::QueuedConnection);
}

void CMakeSetupDialog::finishConfigure(int err)
{
  if(0 == err && !this->CacheValues->cacheModel()->newPropertyCount())
    {
    this->enterState(ReadyGenerate);
    }
  else
    {
    this->enterState(ReadyConfigure);
    this->CacheValues->scrollToTop();
    }
  
  if(err != 0)
    {
    QMessageBox::critical(this, tr("Error"), 
      tr("Error in configuration process, project files may be invalid"), 
      QMessageBox::Ok);
    }
}

void CMakeSetupDialog::finishGenerate(int err)
{
  if(err == 0)
    {
    this->enterState(ReadyBuild);
    }
  else
    {
    this->enterState(ReadyConfigure);
    QMessageBox::critical(this, tr("Error"), 
      tr("Error in generation process, project files may be invalid"),
      QMessageBox::Ok);
    }
}

void CMakeSetupDialog::finishBuild(int err)
{
  this->ProgressBar->setMaximum(100);
  if(err == 0)
    {
    this->enterState(ReadyInstall);
    }
  else
    {
    this->enterState(ReadyBuild);
    QMessageBox::critical(this, tr("Error"),
      tr("Error in build process"),
      QMessageBox::Ok);
    }
}

void CMakeSetupDialog::finishInstall(int err)
{
  this->ProgressBar->setMaximum(100);
  if(err != 0)
    {
    QMessageBox::critical(this, tr("Error"),
      tr("Error in installation process"),
      QMessageBox::Ok);
    }
  this->enterState(ReadyInstall);
}

void CMakeSetupDialog::finishClean(int err)
{
  this->ProgressBar->setMaximum(100);
  if(err != 0)
    {
    QMessageBox::critical(this, tr("Error"),
      tr("Error in cleaning process"),
      QMessageBox::Ok);
    }
  this->enterState(ReadyBuild);
}

void CMakeSetupDialog::doInstallForCommandLine()
{
  QMacInstallDialog setupdialog(0);
  setupdialog.exec();
}

void CMakeSetupDialog::doGenerate()
{
  this->enterState(Generating);
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "generate", Qt::QueuedConnection);
}

void CMakeSetupDialog::doBuild()
{
  this->enterState(Building);
  this->ProgressBar->setMaximum(0);
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "build", Qt::QueuedConnection, Q_ARG(QString, this->CurrentConfiguration));
}

void CMakeSetupDialog::doInstall()
{
  this->enterState(Installing);
  this->ProgressBar->setMaximum(0);
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "install", Qt::QueuedConnection, Q_ARG(QString, this->CurrentConfiguration));
}

void CMakeSetupDialog::doClean()
{
  this->enterState(Cleaning);
  this->ProgressBar->setMaximum(0);
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "clean", Qt::QueuedConnection, Q_ARG(QString, this->CurrentConfiguration));
}

void CMakeSetupDialog::closeEvent(QCloseEvent* e)
{
  // prompt for close if there are unsaved changes, and we're not busy
  if(this->CacheModified)
    {
    QString msg = tr("You have changed options but not rebuilt, "
                    "are you sure you want to exit?");
    QString title = tr("Confirm Exit");
    QMessageBox::StandardButton btn;
    btn = QMessageBox::critical(this, title, msg,
                                QMessageBox::Yes | QMessageBox::No);
    if(btn == QMessageBox::No)
      {
      e->ignore();
      }
    }

  // don't close if we're busy, unless the user really wants to
  if(this->CurrentState == Configuring)
    {
    QString msg = "You are in the middle of a Configure.\n"
                   "If you Exit now the configure information will be lost.\n"
                   "Are you sure you want to Exit?";
    QString title = tr("Confirm Exit");
    QMessageBox::StandardButton btn;
    btn = QMessageBox::critical(this, title, msg,
                                QMessageBox::Yes | QMessageBox::No);
    if(btn == QMessageBox::No)
      {
      e->ignore();
      }
    else
      {
      this->doInterrupt();
      }
    }

  // let the generate finish
  if(this->CurrentState == Generating)
    {
    e->ignore();
    }
}

void CMakeSetupDialog::doHelp()
{
  QString msg = tr("CMake is used to configure and generate build files for "
    "software projects.   The basic steps for configuring a project are as "
    "follows:\r\n\r\n1. Select the source directory for the project.  This should "
    "contain the CMakeLists.txt files for the project.\r\n\r\n2. Select the build "
    "directory for the project.   This is the directory where the project will be "
    "built.  It can be the same or a different directory than the source "
    "directory.   For easy clean up, a separate build directory is recommended. "
    "CMake will create the directory if it does not exist.\r\n\r\n3. Once the "
    "source and binary directories are selected, it is time to press the "
    "Configure button.  This will cause CMake to read all of the input files and "
    "discover all the variables used by the project.   The first time a variable "
    "is displayed it will be in Red.   Users should inspect red variables making "
    "sure the values are correct.   For some projects the Configure process can "
    "be iterative, so continue to press the Configure button until there are no "
    "longer red entries.\r\n\r\n4. Once there are no longer red entries, you "
    "should click the Generate button.  This will write the build files to the build "
    "directory.");

  QDialog dialog;
  QFontMetrics met(this->font());
  int msgWidth = met.width(msg);
  dialog.setMinimumSize(msgWidth/15,20);
  dialog.setWindowTitle(tr("Help"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QLabel* lab = new QLabel(&dialog);
  lab->setText(msg);
  lab->setWordWrap(true);
  QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok,
                                                Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  l->addWidget(lab);
  l->addWidget(btns);
  dialog.exec();
}

void CMakeSetupDialog::doInterrupt()
{
  this->enterState(Interrupting);
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "interrupt", Qt::QueuedConnection);
}

void CMakeSetupDialog::doSourceBrowse()
{
  QString dir = QFileDialog::getExistingDirectory(this, 
    tr("Enter Path to Source"), this->SourceDirectory->text());
  if(!dir.isEmpty())
    {
    this->setSourceDirectory(dir);
    }
}

void CMakeSetupDialog::doSourceOpen()
{
  QString srcdir = this->SourceDirectory->text();
  if(!srcdir.isEmpty())
    {
    QDesktopServices::openUrl(QUrl("file:/"+srcdir, QUrl::TolerantMode));
    }
}

void CMakeSetupDialog::updateSourceDirectory(const QString& dir)
{
  if(this->SourceDirectory->text() != dir)
    {
    this->SourceDirectory->blockSignals(true);
    this->SourceDirectory->setText(dir);
    this->SourceDirectory->blockSignals(false);
    }
}

void CMakeSetupDialog::updateBinaryDirectory(const QString& dir)
{
  if(this->BinaryDirectory->currentText() != dir)
    {
    this->BinaryDirectory->blockSignals(true);
    this->BinaryDirectory->setEditText(dir);
    this->BinaryDirectory->blockSignals(false);
    }
}

void CMakeSetupDialog::doBinaryBrowse()
{
  QString dir = QFileDialog::getExistingDirectory(this, 
    tr("Enter Path to Build"), this->BinaryDirectory->currentText());
  if(!dir.isEmpty() && dir != this->BinaryDirectory->currentText())
    {
    this->setBinaryDirectory(dir);
    }
}

void CMakeSetupDialog::doBinaryOpen()
{
  QString bindir = this->BinaryDirectory->currentText();
  if(!bindir.isEmpty())
    {
    QDesktopServices::openUrl(QUrl("file:/"+bindir, QUrl::TolerantMode));
    }
}

void CMakeSetupDialog::setBinaryDirectory(const QString& dir)
{
  this->BinaryDirectory->setEditText(dir);
}

void CMakeSetupDialog::onSourceDirectoryChanged(const QString& dir)
{
  this->Output->clear();
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "setSourceDirectory", Qt::QueuedConnection, Q_ARG(QString, dir));
}

void CMakeSetupDialog::onBinaryDirectoryChanged(const QString& dir)
{
  QString title = QString(tr("CMake %1 - %2"));
  title = title.arg(cmVersion::GetCMakeVersion());
  title = title.arg(dir);
  this->setWindowTitle(title);

  this->CacheModified = false;
  this->CacheValues->cacheModel()->clear();
  qobject_cast<QCMakeCacheModelDelegate*>(this->CacheValues->itemDelegate())->clearChanges();
  this->Output->clear();
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "setBinaryDirectory", Qt::QueuedConnection, Q_ARG(QString, dir));

  this->enterState(ReadyConfigure);
}

void CMakeSetupDialog::setSourceDirectory(const QString& dir)
{
  this->SourceDirectory->setText(dir);
}

void CMakeSetupDialog::showProgress(const QString& /*msg*/, float percent)
{
  this->ProgressBar->setMaximum(100);
  this->ProgressBar->setValue(qRound(percent * 100));
}

void CMakeSetupDialog::error(const QString& msg)
{
  this->Output->setCurrentCharFormat(this->ErrorFormat);
  this->Output->append(msg);
}

void CMakeSetupDialog::message(const QString& msg)
{
  this->Output->setCurrentCharFormat(this->MessageFormat);
  this->Output->append(msg);
}

void CMakeSetupDialog::setEnabledState(bool enabled)
{
  // disable parts of the GUI during configure/generate/build/install/clean
  this->CacheValues->cacheModel()->setEditEnabled(enabled);
  this->SourceDirectory->setEnabled(enabled);
  this->BrowseSourceDirectoryButton->setEnabled(enabled);
  this->BinaryDirectory->setEnabled(enabled);
  this->BrowseBinaryDirectoryButton->setEnabled(enabled);
  this->ReloadCacheAction->setEnabled(enabled);
  this->DeleteCacheAction->setEnabled(enabled);
  this->ExitAction->setEnabled(enabled);
  this->ConfigureAction->setEnabled(enabled);
  this->ActionButton->setEnabled(true);
  this->GenerateAction->setEnabled(false);
  this->BuildAction->setEnabled(false);
  this->InstallAction->setEnabled(false);
  this->CleanAction->setEnabled(false);
  this->StopAction->setVisible(false);
  this->StopAction->setEnabled(false);
  this->AddEntry->setEnabled(enabled);
  this->RemoveEntry->setEnabled(false);  // let selection re-enable it
  if(enabled)
    {
    this->manageConfigs();
    }
  else
    {
    this->ActiveConfigurationPopup->setEnabled(false);
#if !(defined(QT_MAC_USE_COCOA) && QT_VERSION == 0x040503)
    // don't disable the menu when using Qt-Cocoa 4.5.3:
    // http://bugreports.qt.nokia.com/browse/QTBUG-5313
    this->ActiveConfigurationMenu->setEnabled(false);
#else
    // do it the dedious way: disable all child-actions
    foreach(QAction* a, this->ActiveConfigurationMenu->actions())
      a->setEnabled(false);
#endif
    }
}

bool CMakeSetupDialog::setupFirstConfigure()
{
  FirstConfigure dialog;

  // initialize dialog and restore saved settings

  // add generators
  dialog.setGenerators(this->CMakeThread->cmakeInstance()->availableGenerators());

  // restore from settings
  dialog.loadFromSettings();

  if(dialog.exec() == QDialog::Accepted)
    {
    dialog.saveToSettings();
    this->CMakeThread->cmakeInstance()->setGenerator(dialog.getGenerator());
    
    QCMakeCacheModel* m = this->CacheValues->cacheModel();

    if(dialog.compilerSetup())
      {
      QString fortranCompiler = dialog.getFortranCompiler();
      if(!fortranCompiler.isEmpty())
        {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_Fortran_COMPILER", 
                          "Fortran compiler.", fortranCompiler, false);
        }
      QString cxxCompiler = dialog.getCXXCompiler();
      if(!cxxCompiler.isEmpty())
        {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_CXX_COMPILER", 
                          "CXX compiler.", cxxCompiler, false);
        }
      
      QString cCompiler = dialog.getCCompiler();
      if(!cCompiler.isEmpty())
        {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_C_COMPILER", 
                          "C compiler.", cCompiler, false);
        }
      }
    else if(dialog.crossCompilerSetup())
      {
      QString fortranCompiler = dialog.getFortranCompiler();
      if(!fortranCompiler.isEmpty())
        {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_Fortran_COMPILER", 
                          "Fortran compiler.", fortranCompiler, false);
        }

      QString mode = dialog.getCrossIncludeMode();
      m->insertProperty(QCMakeProperty::STRING, "CMAKE_FIND_ROOT_PATH_MODE_INCLUDE", 
                        "CMake Find Include Mode", mode, false);
      mode = dialog.getCrossLibraryMode();
      m->insertProperty(QCMakeProperty::STRING, "CMAKE_FIND_ROOT_PATH_MODE_LIBRARY", 
                        "CMake Find Library Mode", mode, false);
      mode = dialog.getCrossProgramMode();
      m->insertProperty(QCMakeProperty::STRING, "CMAKE_FIND_ROOT_PATH_MODE_PROGRAM", 
                        "CMake Find Program Mode", mode, false);
      
      QString rootPath = dialog.getCrossRoot();
      m->insertProperty(QCMakeProperty::PATH, "CMAKE_FIND_ROOT_PATH", 
                        "CMake Find Root Path", rootPath, false);

      QString systemName = dialog.getSystemName();
      m->insertProperty(QCMakeProperty::STRING, "CMAKE_SYSTEM_NAME", 
                        "CMake System Name", systemName, false);
      QString cxxCompiler = dialog.getCXXCompiler();
      m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_CXX_COMPILER", 
                        "CXX compiler.", cxxCompiler, false);
      QString cCompiler = dialog.getCCompiler();
      m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_C_COMPILER", 
                        "C compiler.", cCompiler, false);
      }
    else if(dialog.crossCompilerToolChainFile())
      {
      QString toolchainFile = dialog.getCrossCompilerToolChainFile();
      m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_TOOLCHAIN_FILE", 
                        "Cross Compile ToolChain File", toolchainFile, false);
      }
    return true;
    }

  return false;
}

void CMakeSetupDialog::updateGeneratorLabel(const QString& gen)
{
  QString str = tr("Current Generator: ");
  if(gen.isEmpty())
    {
    str += tr("None");
    }
  else
    {
    str += gen;
    }
  this->Generator->setText(str);
}

void CMakeSetupDialog::doReloadCache()
{
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "reloadCache", Qt::QueuedConnection);
}

void CMakeSetupDialog::doDeleteCache()
{   
  QString title = tr("Delete Cache");
  QString msg = "Are you sure you want to delete the cache?";
  QMessageBox::StandardButton btn;
  btn = QMessageBox::information(this, title, msg, 
                                 QMessageBox::Yes | QMessageBox::No);
  if(btn == QMessageBox::No)
    {
    return;
    }
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
                            "deleteCache", Qt::QueuedConnection);
}

void CMakeSetupDialog::doAbout()
{
  QString msg = "CMake %1\n"
                "Using Qt %2\n"
                "www.cmake.org";

  msg = msg.arg(cmVersion::GetCMakeVersion());
  msg = msg.arg(qVersion());

  QDialog dialog;
  dialog.setWindowTitle(tr("About"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QLabel* lab = new QLabel(&dialog);
  l->addWidget(lab);
  lab->setText(msg);
  lab->setWordWrap(true);
  QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok,
                                                Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  l->addWidget(btns);
  dialog.exec();
}

void CMakeSetupDialog::setExitAfterGenerate(bool b)
{
  this->ExitAfterGenerate = b;
}

void CMakeSetupDialog::addBinaryPath(const QString& path)
{
  QString cleanpath = QDir::cleanPath(path);
  
  // update UI
  this->BinaryDirectory->blockSignals(true);
  int idx = this->BinaryDirectory->findText(cleanpath);
  if(idx != -1)
    {
    this->BinaryDirectory->removeItem(idx);
    }
  this->BinaryDirectory->insertItem(0, cleanpath);
  this->BinaryDirectory->setCurrentIndex(0);
  this->BinaryDirectory->blockSignals(false);
  
  // save to registry
  QStringList buildPaths = this->loadBuildPaths();
  buildPaths.removeAll(cleanpath);
  buildPaths.prepend(cleanpath);
  this->saveBuildPaths(buildPaths);
}

void CMakeSetupDialog::dragEnterEvent(QDragEnterEvent* e)
{
  if(!(this->CurrentState == ReadyConfigure || 
     this->CurrentState == ReadyGenerate ||
     this->CurrentState == ReadyBuild ||
     this->CurrentState == ReadyInstall))
    {
    e->ignore();
    return;
    }

  const QMimeData* dat = e->mimeData();
  QList<QUrl> urls = dat->urls();
  QString file = urls.count() ? urls[0].toLocalFile() : QString();
  if(!file.isEmpty() && 
    (file.endsWith("CMakeCache.txt", Qt::CaseInsensitive) ||
    file.endsWith("CMakeLists.txt", Qt::CaseInsensitive) ) )
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void CMakeSetupDialog::dropEvent(QDropEvent* e)
{
  if(!(this->CurrentState == ReadyConfigure || 
     this->CurrentState == ReadyGenerate ||
     this->CurrentState == ReadyBuild ||
     this->CurrentState == ReadyInstall))
    {
    return;
    }

  const QMimeData* dat = e->mimeData();
  QList<QUrl> urls = dat->urls();
  QString file = urls.count() ? urls[0].toLocalFile() : QString();
  if(file.endsWith("CMakeCache.txt", Qt::CaseInsensitive))
    {
    QFileInfo info(file);
    if(this->CMakeThread->cmakeInstance()->binaryDirectory() != info.absolutePath())
      {
      this->setBinaryDirectory(info.absolutePath());
      }
    }
  else if(file.endsWith("CMakeLists.txt", Qt::CaseInsensitive))
    {
    QFileInfo info(file);
    if(this->CMakeThread->cmakeInstance()->binaryDirectory() != info.absolutePath())
      {
      this->setSourceDirectory(info.absolutePath());
      this->setBinaryDirectory(info.absolutePath());
      }
    }
}

QStringList CMakeSetupDialog::loadBuildPaths()
{
  QSettings settings;
  settings.beginGroup("Settings/StartPath");

  QStringList buildPaths;
  for(int i=0; i<10; i++)
    { 
      QString p = settings.value(QString("WhereBuild%1").arg(i)).toString();
      if(!p.isEmpty())
        {
        buildPaths.append(p);
        }
    }
  return buildPaths;
}

void CMakeSetupDialog::saveBuildPaths(const QStringList& paths)
{
  QSettings settings;
  settings.beginGroup("Settings/StartPath");

  int num = paths.count();
  if(num > 10)
    {
    num = 10;
    }

  for(int i=0; i<num; i++)
    { 
    settings.setValue(QString("WhereBuild%1").arg(i), paths[i]);
    }
}
  
void CMakeSetupDialog::setCacheModified()
{
  this->CacheModified = true;
  this->enterState(ReadyConfigure);
}

void CMakeSetupDialog::removeSelectedCacheEntries()
{
  QModelIndexList idxs = this->CacheValues->selectionModel()->selectedRows();
  QList<QPersistentModelIndex> pidxs;
  foreach(QModelIndex i, idxs)
    {
    pidxs.append(i);
    }
  foreach(QPersistentModelIndex pi, pidxs)
    {
    this->CacheValues->model()->removeRow(pi.row(), pi.parent());
    }
}

void CMakeSetupDialog::selectionChanged()
{
  QModelIndexList idxs = this->CacheValues->selectionModel()->selectedRows();
  if(idxs.count() && 
      (this->CurrentState == ReadyConfigure || 
       this->CurrentState == ReadyGenerate ||
       this->CurrentState == ReadyBuild ||
       this->CurrentState == ReadyInstall) )
    {
    this->RemoveEntry->setEnabled(true);
    }
  else
    {
    this->RemoveEntry->setEnabled(false);
    }
}
  
void CMakeSetupDialog::enterState(CMakeSetupDialog::State s)
{
  if(s == this->CurrentState)
    {
    return;
    }

  this->CurrentState = s;

  switch(s)
    {
    case Interrupting:
      this->ActionButton->setEnabled(false);
      break;
    case Configuring:
    case Generating:
    case Building:
    case Installing:
    case Cleaning:
      this->Output->clear();
      this->setEnabledState(false);
      this->StopAction->setEnabled(true);
      this->StopAction->setVisible(true);
      this->setDefaultAction(this->StopAction);
      break;
    case ReadyConfigure:
      this->ProgressBar->reset();
      this->setEnabledState(true);
      this->setDefaultAction(this->ConfigureAction);
      break;
    case ReadyGenerate:
      this->ProgressBar->reset();
      this->setEnabledState(true);
      this->GenerateAction->setEnabled(true);
      this->CleanAction->setEnabled(false);
      this->setDefaultAction(this->GenerateAction);
      break;
    case ReadyBuild:
      this->ProgressBar->reset();
      this->setEnabledState(true);
      this->GenerateAction->setEnabled(true);
      this->BuildAction->setEnabled(true);
      this->CleanAction->setEnabled(true);
      this->setDefaultAction(this->BuildAction);
      break;
    case ReadyInstall:
      this->ProgressBar->reset();
      this->setEnabledState(true);
      this->GenerateAction->setEnabled(true);
      this->BuildAction->setEnabled(true);
      this->InstallAction->setEnabled(true);
      this->CleanAction->setEnabled(true);
      this->setDefaultAction(this->InstallAction);
      break;
    }
}

void CMakeSetupDialog::setDefaultAction(QAction* action)
{
  this->DefaultAction = action;
  this->ActionButton->setText(action->text());
}

void CMakeSetupDialog::manageConfigs()
{
  QCMakePropertyList props = this->CacheValues->cacheModel()->properties();
  QStringList configs;
  foreach(QCMakeProperty p, props)
    {
    if(p.Key == "CMAKE_CONFIGURATION_TYPES")
      {
      // p.Strings seems to be always empty...
      configs = p.Value.toString().split(";");
      break;
      }
    }
  // always rebuild menu and combobox from scratch (easier...)
  // first check that we have in fact a multi-config generator
  if(configs.length() > 0)
    {
    // figure out what the index of the current configuration is in the new set
    int curIdx = -1;
    if(!this->CurrentConfiguration.isEmpty())
      {
      curIdx = configs.indexOf(this->CurrentConfiguration);
      }
    if(curIdx < 0)
      {
      this->CurrentConfiguration = "";
      curIdx = 0;
      }
    // now clear out the menu, the action group and the combo box
    this->ActiveConfigurationMenu->clear();
    this->ActiveConfigurationPopup->clear();
    this->ConfigurationActions.clear();
    // this also deletes the actions (they are children of the old ActionGroup)!
    this->ConfigurationActionsGroup = QSharedPointer<QActionGroup>(new QActionGroup(NULL));
    this->ConfigurationActionsGroup->setExclusive(true);
    // now populate
    size_t i = 0;
    foreach(QString c, configs)
      {
      QAction* act = new QAction(c, this->ConfigurationActionsGroup.data());
      act->setCheckable(true);
      this->ConfigurationActionsGroup->addAction(act);
      this->ConfigurationActions.append(act);
      this->ActiveConfigurationMenu->addAction(act);
      this->ActiveConfigurationPopup->addItem(c);
      QObject::connect(act, SIGNAL(triggered(bool)), this, SLOT(activeConfigurationChanged(bool)));
      ++i;
      }
    this->ConfigurationActions[curIdx]->setChecked(true);
    this->ActiveConfigurationPopup->setCurrentIndex(curIdx);
    QObject::connect(this->ActiveConfigurationPopup, SIGNAL(currentIndexChanged(int)),
        this, SLOT(activeConfigurationChanged(int)));
    this->ActiveConfigurationPopup->setVisible(true);
    this->ActiveConfigurationPopup->setEnabled(true);
    this->ActiveConfigurationMenu->setEnabled(true);
    }
  else
    {
    // nope, disable
    this->ActiveConfigurationPopup->clear();
    this->ActiveConfigurationMenu->clear();
    this->ActiveConfigurationPopup->setVisible(false);
    this->ActiveConfigurationPopup->setEnabled(false);
#if !(defined(QT_MAC_USE_COCOA) && QT_VERSION == 0x040503)
  // Don't disable the menu when using Qt-Cocoa 4.5.3:
  // http://bugreports.qt.nokia.com/browse/QTBUG-5313
    this->ActiveConfigurationMenu->setEnabled(false);
#endif
    this->CurrentConfiguration = "";
    }
}

void CMakeSetupDialog::addCacheEntry()
{
  QDialog dialog(this);
  dialog.resize(400, 200);
  dialog.setWindowTitle(tr("Add Cache Entry"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  AddCacheEntry* w = new AddCacheEntry(&dialog);
  QDialogButtonBox* btns = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
      Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  QObject::connect(btns, SIGNAL(rejected()), &dialog, SLOT(reject()));
  l->addWidget(w);
  l->addStretch();
  l->addWidget(btns);
  if(QDialog::Accepted == dialog.exec())
    {
    QCMakeCacheModel* m = this->CacheValues->cacheModel();
    m->insertProperty(w->type(), w->name(), w->description(), w->value(), false);
    }
}

void CMakeSetupDialog::startSearch()
{
  this->Search->setFocus(Qt::OtherFocusReason);
  this->Search->selectAll();
}

void CMakeSetupDialog::setDebugOutput(bool flag)
{
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
    "setDebugOutput", Qt::QueuedConnection, Q_ARG(bool, flag));
}

void CMakeSetupDialog::setViewType(int v)
{
  if(v == 0)  // simple view
    {
    this->CacheValues->cacheModel()->setViewType(QCMakeCacheModel::FlatView);
    this->CacheValues->setRootIsDecorated(false);
    this->CacheValues->setShowAdvanced(false);
    }
  else if(v == 1)  // advanced view
    {
    this->CacheValues->cacheModel()->setViewType(QCMakeCacheModel::FlatView);
    this->CacheValues->setRootIsDecorated(false);
    this->CacheValues->setShowAdvanced(true);
    }
  else if(v == 2)  // grouped view
    {
    this->CacheValues->cacheModel()->setViewType(QCMakeCacheModel::GroupView);
    this->CacheValues->setRootIsDecorated(true);
    this->CacheValues->setShowAdvanced(true);
    }
  
  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  settings.setValue("GroupView", v == 2);
  
}

void CMakeSetupDialog::showUserChanges()
{
  QSet<QCMakeProperty> changes =
    qobject_cast<QCMakeCacheModelDelegate*>(this->CacheValues->itemDelegate())->changes();

  QDialog dialog(this);
  dialog.setWindowTitle(tr("My Changes"));
  dialog.resize(600, 400);
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QTextEdit* textedit = new QTextEdit(&dialog);
  textedit->setReadOnly(true);
  l->addWidget(textedit);
  QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Close,
                                                Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(rejected()), &dialog, SLOT(accept()));
  l->addWidget(btns);

  QString command;
  QString cache;
  
  foreach(QCMakeProperty prop, changes)
    {
    QString type;
    switch(prop.Type)
      {
      case QCMakeProperty::BOOL:
        type = "BOOL";
        break;
      case QCMakeProperty::PATH:
        type = "PATH";
        break;
      case QCMakeProperty::FILEPATH:
        type = "FILEPATH";
        break;
      case QCMakeProperty::STRING:
        type = "STRING";
        break;
      }
    QString value;
    if(prop.Type == QCMakeProperty::BOOL)
      {
      value = prop.Value.toBool() ? "1" : "0";
      }
    else
      {
      value = prop.Value.toString();
      }

    QString line("%1:%2=");
    line = line.arg(prop.Key);
    line = line.arg(type);

    command += QString("-D%1\"%2\" ").arg(line).arg(value);
    cache += QString("%1%2\n").arg(line).arg(value);
    }
  
  textedit->append(tr("Commandline options:"));
  textedit->append(command);
  textedit->append("\n");
  textedit->append(tr("Cache file:"));
  textedit->append(cache);
  
  dialog.exec();
}

void CMakeSetupDialog::setSearchFilter(const QString& str)
{
  this->CacheValues->selectionModel()->clear();
  this->CacheValues->setSearchFilter(str);
}

void CMakeSetupDialog::activeConfigurationChanged(bool checked)
{
  if(checked)
    {
    for(int i=0; i < this->ConfigurationActions.length(); ++i)
      if(this->ConfigurationActions[i]->isChecked())
        {
        this->activeConfigurationChanged(i);
        break;
        }
    }
}

void CMakeSetupDialog::activeConfigurationChanged(int index)
{
  if(index > -1)
    {
    this->ConfigurationActions[index]->setChecked(true);
    this->ActiveConfigurationPopup->setCurrentIndex(index);
    this->CurrentConfiguration = this->ActiveConfigurationPopup->currentText();
    }
}

