/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  Copyright (c) 2004 Alexander Neundorf, neundorf@kde.org. All rights reserved.
  Copyright (c) 2007 Miguel A. Figueroa-Villanueva. All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef cmExtraEclipseCDT4Generator_h
#define cmExtraEclipseCDT4Generator_h

#include "cmExternalMakefileProjectGenerator.h"

class cmMakefile;
class cmGeneratedFileStream;

/** \class cmExtraEclipseCDT4Generator
 * \brief Write Eclipse project files for Makefile based projects
 *
 * This generator is in early alpha stage.
 */
class cmExtraEclipseCDT4Generator : public cmExternalMakefileProjectGenerator
{
public:
  cmExtraEclipseCDT4Generator();

  static cmExternalMakefileProjectGenerator* New() {
    return new cmExtraEclipseCDT4Generator;
  }

  virtual const char* GetName() const {
    return cmExtraEclipseCDT4Generator::GetActualName();
  }

  static const char* GetActualName() { return "Eclipse CDT4"; }

  virtual void GetDocumentation(cmDocumentationEntry& entry,
                                const char*           fullName) const;

  virtual void SetGlobalGenerator(cmGlobalGenerator* generator);

  virtual void Generate();

private:
  // create .project file
  void CreateProjectFile() const;

  // create .cproject file
  void CreateCProjectFile() const;

  // Eclipse supported toolchain types
  enum EclipseToolchainType
    {
    EclipseToolchainOther,
    EclipseToolchainLinux,
    EclipseToolchainCygwin,
    EclipseToolchainMinGW,
    EclipseToolchainSolaris,
    EclipseToolchainMacOSX
    };
  static EclipseToolchainType GetToolChainType(const cmMakefile& makefile);

  // If built with cygwin cmake, convert posix to windows path.
  static std::string GetEclipsePath(const std::string& path);

  // Extract basename.
  static std::string GetPathBasename(const std::string& path);

  // Helper functions
  static void AppendStorageScanners(cmGeneratedFileStream& fout);
  static void AppendTarget         (cmGeneratedFileStream& fout,
                                    const std::string&     target,
                                    const std::string&     make);
  static void AppendScannerProfile (cmGeneratedFileStream& fout,
                                    const std::string&   profileID,
                                    bool                 openActionEnabled,
                                    const std::string&   openActionFilePath,
                                    bool                 pParserEnabled,
                                    const std::string&   scannerInfoProviderID,
                                    const std::string&   runActionArguments,
                                    const std::string&   runActionCommand,
                                    bool                 runActionUseDefault,
                                    bool                 sipParserEnabled);
};

#endif