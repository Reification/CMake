/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio15Generator_h
#define cmGlobalVisualStudio15Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

#include "cmGlobalVisualStudio14Generator.h"
#include "cmVSSetupHelper.h"

class cmGlobalGeneratorFactory;
class cmake;

/** \class cmGlobalVisualStudio15Generator  */
class cmGlobalVisualStudio15Generator : public cmGlobalVisualStudio14Generator
{
public:
  cmGlobalVisualStudio15Generator(cmake* cm, const std::string& name,
                                  const std::string& platformName);
  static cmGlobalGeneratorFactory* NewFactory();

  bool MatchesGeneratorName(const std::string& name) const override;

  void WriteSLNHeader(std::ostream& fout) override;

  const char* GetToolsVersion() override { return "15.0"; }

  bool SetGeneratorInstance(std::string const& i, cmMakefile* mf) override;

  bool SetGeneratorPlatform( std::string const& p, cmMakefile* mf ) override;

  bool SetGeneratorToolset( std::string const& ts, cmMakefile* mf ) override;

  bool IsAndroidMSVS() const override { return SystemIsAndroidMSVS; }
  std::string GetVersionAndroidMSVS() const override { return VersionAndroidMSVS; }
  std::string GetAndroidAPILevel() const override;

  bool GetVSInstance(std::string& dir) const;

  bool IsDefaultToolset(const std::string& version) const override;
  std::string GetAuxiliaryToolset() const override;

protected:
  bool InitializeWindows(cmMakefile* mf) override;
  bool SelectWindowsStoreToolset(std::string& toolset) const override;

  bool InitializeSystem( cmMakefile* mf ) override;
  bool FindVCTargetsPath( cmMakefile* mf ) override;

  bool InitializeAndroidWorkflow(cmMakefile* mf);

  std::string GetInstalledAndroidWorkflow() const;

  std::string GetDefaultAndroidToolChain() const;

  const char* GetIDEVersion() override { return "15.0"; }

  // Used to verify that the Desktop toolset for the current generator is
  // installed on the machine.
  bool IsWindowsDesktopToolsetInstalled() const override;

  // These aren't virtual because we need to check if the selected version
  // of the toolset is installed
  bool IsWindowsStoreToolsetInstalled() const;

  // Check for a Win 8 SDK known to the registry or VS installer tool.
  bool IsWin81SDKInstalled() const;

  std::string GetWindows10SDKMaxVersion() const override;

  std::string FindMSBuildCommand() override;
  std::string FindDevEnvCommand() override;

private:
  bool SystemIsAndroidMSVS;
  mutable std::string VersionAndroidMSVS;
  class Factory;
  mutable cmVSSetupAPIHelper vsSetupAPIHelper;
  std::string AndroidAPILevel;
  std::string DefaultAndroidToolChain;
};
#endif
