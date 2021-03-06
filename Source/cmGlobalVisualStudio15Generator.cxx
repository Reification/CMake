/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio15Generator.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmVS141CLFlagTable.h"
#include "cmVS141CSharpFlagTable.h"
#include "cmVS141LinkFlagTable.h"
#include "cmVSSetupHelper.h"

#include "cmsys/Glob.hxx"

static const char vs15generatorName[] = "Visual Studio 15 2017";

// Map generator name without year to name with year.
static const char* cmVS15GenName(const std::string& name, std::string& genName)
{
  if (strncmp(
        name.c_str(), vs15generatorName, sizeof(vs15generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs15generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2017")) {
    p += 5;
  }
  genName = std::string(vs15generatorName) + p;
  return p;
}

class cmGlobalVisualStudio15Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  virtual cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                                   cmake* cm) const
  {
    std::string genName;
    const char* p = cmVS15GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "");
    }
    if (*p++ != ' ') {
      return 0;
    }
    if (strcmp(p, "Win64") == 0) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "x64");
    }
    if (strcmp(p, "ARM") == 0) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "ARM");
    }
    if (strcmp(p, "ARM64") == 0) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "ARM64");
    }
    return 0;
  }

  virtual void GetDocumentation(cmDocumentationEntry& entry) const
  {
    entry.Name = std::string(vs15generatorName) + " [arch]";

    entry.Brief = "Generates Visual Studio 2017 project files.  "
                  "Optional [arch] can be \"Win64\", \"ARM\", or \"ARM64\".";
  }

  virtual void GetGenerators(std::vector<std::string>& names) const
  {
    names.push_back(vs15generatorName);
    names.push_back(vs15generatorName + std::string(" ARM"));
    names.push_back(vs15generatorName + std::string(" ARM64"));
    names.push_back(vs15generatorName + std::string(" Win64"));
  }

  bool SupportsToolset() const override { return true; }
  bool SupportsPlatform() const override { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio15Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio15Generator::cmGlobalVisualStudio15Generator(
  cmake* cm,
  const std::string& name,
  const std::string& platformName)
  : cmGlobalVisualStudio14Generator(cm, name, platformName)
{
  this->ExpressEdition = false;
  this->DefaultPlatformToolset = "v141";
  this->DefaultClFlagTable = cmVS141CLFlagTable;
  this->DefaultCSharpFlagTable = cmVS141CSharpFlagTable;
  this->DefaultLinkFlagTable = cmVS141LinkFlagTable;
  this->Version = VS15;
  this->SystemIsAndroidMSVS = false;
}

bool cmGlobalVisualStudio15Generator::MatchesGeneratorName(
  const std::string& name) const
{
  std::string genName;
  if (cmVS15GenName(name, genName)) {
    return genName == this->GetName();
  }
  return false;
}

void cmGlobalVisualStudio15Generator::WriteSLNHeader(std::ostream& fout)
{
  // Visual Studio 15 writes .sln format 12.00
  fout << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
  if (this->ExpressEdition) {
    fout << "# Visual Studio Express 15 for Windows Desktop\n";
  } else {
    fout << "# Visual Studio 15\n";
  }
}

bool cmGlobalVisualStudio15Generator::SetGeneratorInstance(
  std::string const& i,
  cmMakefile* mf)
{
  if (!i.empty()) {
    if (!this->vsSetupAPIHelper.SetVSInstance(i)) {
      std::ostringstream e;
      /* clang-format off */
      e <<
        "Generator\n"
        "  " << this->GetName() << "\n"
        "could not find specified instance of Visual Studio:\n"
        "  " << i;
      /* clang-format on */
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
  }

  std::string vsInstance;
  if (!this->vsSetupAPIHelper.GetVSInstanceInfo(vsInstance)) {
    std::ostringstream e;
    /* clang-format off */
    e <<
      "Generator\n"
      "  " << this->GetName() << "\n"
      "could not find any instance of Visual Studio.\n";
    /* clang-format on */
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  // Save the selected instance persistently.
  std::string genInstance = mf->GetSafeDefinition("CMAKE_GENERATOR_INSTANCE");
  if (vsInstance != genInstance) {
    this->CMakeInstance->AddCacheEntry("CMAKE_GENERATOR_INSTANCE",
                                       vsInstance.c_str(),
                                       "Generator instance identifier.",
                                       cmStateEnums::INTERNAL);
  }

  return true;
}

bool cmGlobalVisualStudio15Generator::GetVSInstance(std::string& dir) const
{
  return vsSetupAPIHelper.GetVSInstanceInfo(dir);
}

bool cmGlobalVisualStudio15Generator::IsDefaultToolset(
  const std::string& version) const
{
  if (version.empty()) {
    return true;
  }

  std::string vcToolsetVersion;
  if (this->vsSetupAPIHelper.GetVCToolsetVersion(vcToolsetVersion)) {

    cmsys::RegularExpression regex("[0-9][0-9]\\.[0-9]+");
    if (regex.find(version) && regex.find(vcToolsetVersion)) {
      const auto majorMinorEnd = vcToolsetVersion.find('.', 3);
      const auto majorMinor = vcToolsetVersion.substr(0, majorMinorEnd);
      return version == majorMinor;
    }
  }

  return false;
}

std::string cmGlobalVisualStudio15Generator::GetAuxiliaryToolset() const
{
  const char* version = this->GetPlatformToolsetVersion();
  if (version) {
    std::string instancePath;
    GetVSInstance(instancePath);
    std::stringstream path;
    path << instancePath;
    path << "/VC/Auxiliary/Build/";
    path << version;
    path << "/Microsoft.VCToolsVersion." << version << ".props";

    std::string toolsetPath = path.str();
    cmSystemTools::ConvertToUnixSlashes(toolsetPath);
    return toolsetPath;
  }
  return {};
}

bool cmGlobalVisualStudio15Generator::InitializeWindows(cmMakefile* mf)
{
  // If the Win 8.1 SDK is installed then we can select a SDK matching
  // the target Windows version.
  if (this->IsWin81SDKInstalled()) {
    return cmGlobalVisualStudio14Generator::InitializeWindows(mf);
  }
  // Otherwise we must choose a Win 10 SDK even if we are not targeting
  // Windows 10.
  return this->SelectWindows10SDK(mf, false);
}

bool cmGlobalVisualStudio15Generator::SetGeneratorPlatform(
  std::string const& p,
  cmMakefile* mf)
{
  if (this->IsAndroidMSVS()) {
    if (p != "ARM64" && p != "ARM" && p != "x86_64" && p != "x86") {
      std::string err;
      err = std::string("Building for Android with '") + this->GetName() +
        "' Platform '" + p + "' not supported. Only ARM, ARM64, x86, x86_64";
      mf->IssueMessage(cmake::FATAL_ERROR, err.c_str());
      return false;
    }
  }

  return cmGlobalVisualStudio14Generator::SetGeneratorPlatform(p, mf);
}

bool cmGlobalVisualStudio15Generator::SetGeneratorToolset(
  std::string const& ts,
  cmMakefile* mf)
{
  if (this->IsAndroidMSVS() && ts.empty() &&
      this->DefaultPlatformToolset.empty()) {
    std::ostringstream e;
    e << this->GetName()
      << " MSVS Android requires CMAKE_GENERATOR_TOOLSET to be set.";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  return cmGlobalVisualStudio14Generator::SetGeneratorToolset(ts, mf);
}

bool cmGlobalVisualStudio15Generator::FindVCTargetsPath(cmMakefile* mf)
{
  static std::string s_targetsPath;

  if (s_targetsPath.empty()) {
    this->GetVSInstance(s_targetsPath);
    s_targetsPath += "/Common7/IDE/VC/VCTargets";

    if (!cmSystemTools::FileIsDirectory(s_targetsPath)) {
      s_targetsPath = "#";
    }
  }

  if (s_targetsPath[0] == '#') {
    return false;
  }

  if (this->VCTargetsPath.empty()) {
    this->VCTargetsPath = s_targetsPath;
  }

  return true;
}

bool cmGlobalVisualStudio15Generator::InitializeSystem(cmMakefile* mf)
{
  if (strcmp(this->SystemName.c_str(), "Android") != 0) {
    this->SystemIsAndroidMSVS = false;
    return cmGlobalVisualStudio14Generator::InitializeSystem(mf);
  }

  if (!InitializeAndroidWorkflow(mf)) {
    if (!GetInstalledNsightTegraVersion().empty()) {
      return cmGlobalVisualStudio14Generator::InitializeSystem(mf);
    }

    mf->IssueMessage(cmake::FATAL_ERROR,
                     std::string("CMAKE_SYSTEM_NAME is '") + this->SystemName +
                       "' but "
                       "'Visual C++ for Cross Platform Mobile Development "
                       "(Android)' is not installed.");
    return false;
  }

  if (this->DefaultPlatformName != "Win32") {
    std::ostringstream e;
    e << "CMAKE_SYSTEM_NAME is '" << this->SystemName
      << "' but CMAKE_GENERATOR "
      << "specifies a platform too: '" << this->GetName() << "'";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  this->SystemIsAndroidMSVS = true;
  this->DefaultPlatformToolset = GetDefaultAndroidToolChain();

  // even though not targeting windows 10 sdk need a valid one for utility
  // vcxproj files
  this->SelectWindows10SDK(mf, false);

  return true;
}

bool cmGlobalVisualStudio15Generator::InitializeAndroidWorkflow(cmMakefile* mf)
{
  if (!VersionAndroidMSVS.empty()) {
    return true;
  }

  std::string vsInstance;
  if (!this->GetVSInstance(vsInstance)) {
    mf->IssueMessage(cmake::FATAL_ERROR, "VisualStudio not installed!");
    return false;
  }

  std::string testPath =
    vsInstance + "/Common7/IDE/VC/VCTargets/Application Type/Android";

  if (!cmSystemTools::FileIsDirectory(testPath)) {
    return false;
  }

  std::string generatorPlatform =
  mf->GetSafeDefinition("CMAKE_GENERATOR_PLATFORM");

  if (generatorPlatform.empty()) {
    mf->IssueMessage(cmake::FATAL_ERROR, "CMAKE_GENERATOR_PLATFORM not set!");
    return false;
  }

  std::string highestInstalledWorkflow;
  std::string highestClangToolChain;

  {
    cmsys::Glob versionDirGlob;
    versionDirGlob.SetListDirs(true);
    versionDirGlob.FindFiles(testPath + "/[0-9]*.[0-9]*");

    std::vector<std::string>& dirs = versionDirGlob.GetFiles();

    for (size_t i = dirs.size(); i-- > 0;) {
      const std::string& versionDir = dirs[i];

      if (cmSystemTools::FileIsDirectory(versionDir)) {
        highestInstalledWorkflow = versionDir;
        break;
      }
    }
  }

  if (highestInstalledWorkflow.empty()) {
    return false;
  }

  {
    cmsys::Glob versionDirGlob;
    versionDirGlob.SetListDirs(true);

    versionDirGlob.FindFiles(highestInstalledWorkflow + "/Platforms/" +
                             generatorPlatform +
                             "/PlatformToolsets/Clang*");

    std::vector<std::string>& dirs = versionDirGlob.GetFiles();

    for (uint32_t i = dirs.size(); i-- > 0;) {
      const std::string& versionDir = dirs[i];

      if (cmSystemTools::FileIsDirectory(versionDir)) {
        highestClangToolChain = versionDir;
        break;
      }
    }
  }

  if (highestClangToolChain.empty()) {
    mf->IssueMessage(cmake::FATAL_ERROR, "Could not find any clang toolchains for MSVS/Android work flow!");
    return false;
  }

  const char* versionDirName =
    strrchr(highestInstalledWorkflow.c_str(), '/') + 1;
  VersionAndroidMSVS = versionDirName;

  const char* toolChainName = strrchr(highestClangToolChain.c_str(), '/') + 1;
  DefaultAndroidToolChain = toolChainName;

  AndroidAPILevel = mf->GetSafeDefinition("ANDROID_NATIVE_API_LEVEL");

  if ( AndroidAPILevel.empty() )
  {
    AndroidAPILevel = "26";
  }

  return true;
}

std::string cmGlobalVisualStudio15Generator::GetDefaultAndroidToolChain() const
{
  return DefaultAndroidToolChain;
}

std::string cmGlobalVisualStudio15Generator::GetAndroidAPILevel() const
{
  return this->AndroidAPILevel;
}

bool cmGlobalVisualStudio15Generator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    if (this->IsWindowsStoreToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = "v141"; // VS 15 uses v141 toolset
      return true;
    } else {
      return false;
    }
  }
  return this->cmGlobalVisualStudio14Generator::SelectWindowsStoreToolset(
    toolset);
}

bool cmGlobalVisualStudio15Generator::IsWindowsDesktopToolsetInstalled() const
{
  return vsSetupAPIHelper.IsVS2017Installed();
}

bool cmGlobalVisualStudio15Generator::IsWindowsStoreToolsetInstalled() const
{
  return vsSetupAPIHelper.IsWin10SDKInstalled();
}

bool cmGlobalVisualStudio15Generator::IsWin81SDKInstalled() const
{
  // Does the VS installer tool know about one?
  if (vsSetupAPIHelper.IsWin81SDKInstalled()) {
    return true;
  }

  // Does the registry know about one (e.g. from VS 2015)?
  std::string win81Root;
  if (cmSystemTools::ReadRegistryValue(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
        "Windows Kits\\Installed Roots;KitsRoot81",
        win81Root,
        cmSystemTools::KeyWOW64_32) ||
      cmSystemTools::ReadRegistryValue(
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\"
        "Windows Kits\\Installed Roots;KitsRoot81",
        win81Root,
        cmSystemTools::KeyWOW64_32)) {
    return cmSystemTools::FileExists(win81Root + "/um/windows.h", true);
  }
  return false;
}

std::string cmGlobalVisualStudio15Generator::GetWindows10SDKMaxVersion() const
{
  return std::string();
}

std::string cmGlobalVisualStudio15Generator::FindMSBuildCommand()
{
  std::string msbuild;

  // Ask Visual Studio Installer tool.
  std::string vs;
  if (vsSetupAPIHelper.GetVSInstanceInfo(vs)) {
    msbuild = vs + "/MSBuild/15.0/Bin/MSBuild.exe";
    if (cmSystemTools::FileExists(msbuild)) {
      return msbuild;
    }
  }

  msbuild = "MSBuild.exe";
  return msbuild;
}

std::string cmGlobalVisualStudio15Generator::FindDevEnvCommand()
{
  std::string devenv;

  // Ask Visual Studio Installer tool.
  std::string vs;
  if (vsSetupAPIHelper.GetVSInstanceInfo(vs)) {
    devenv = vs + "/Common7/IDE/devenv.com";
    if (cmSystemTools::FileExists(devenv)) {
      return devenv;
    }
  }

  devenv = "devenv.com";
  return devenv;
}
