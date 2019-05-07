/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudioVersionedGenerator.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmVSSetupHelper.h"
#include "cmake.h"

#include "cmsys/Glob.hxx"

#if defined(_M_ARM64)
#  define HOST_PLATFORM_NAME "ARM64"
#  define HOST_TOOLS_ARCH ""
#elif defined(_M_ARM)
#  define HOST_PLATFORM_NAME "ARM"
#  define HOST_TOOLS_ARCH ""
#elif defined(_M_IA64)
#  define HOST_PLATFORM_NAME "Itanium"
#  define HOST_TOOLS_ARCH ""
#elif defined(_WIN64)
#  define HOST_PLATFORM_NAME "x64"
#  define HOST_TOOLS_ARCH "x64"
#else
static bool VSIsWow64()
{
  BOOL isWow64 = false;
  return IsWow64Process(GetCurrentProcess(), &isWow64) && isWow64;
}
#endif

static std::string VSHostPlatformName()
{
#ifdef HOST_PLATFORM_NAME
  return HOST_PLATFORM_NAME;
#else
  if (VSIsWow64()) {
    return "x64";
  } else {
    return "Win32";
  }
#endif
}

static std::string VSHostArchitecture()
{
#ifdef HOST_TOOLS_ARCH
  return HOST_TOOLS_ARCH;
#else
  if (VSIsWow64()) {
    return "x64";
  } else {
    return "x86";
  }
#endif
}

static unsigned int VSVersionToMajor(
  cmGlobalVisualStudioGenerator::VSVersion v)
{
  switch (v) {
    case cmGlobalVisualStudioGenerator::VS9:
      return 9;
    case cmGlobalVisualStudioGenerator::VS10:
      return 10;
    case cmGlobalVisualStudioGenerator::VS11:
      return 11;
    case cmGlobalVisualStudioGenerator::VS12:
      return 12;
    case cmGlobalVisualStudioGenerator::VS14:
      return 14;
    case cmGlobalVisualStudioGenerator::VS15:
      return 15;
    case cmGlobalVisualStudioGenerator::VS16:
      return 16;
  }
  return 0;
}

static const char* VSVersionToToolset(
  cmGlobalVisualStudioGenerator::VSVersion v)
{
  switch (v) {
    case cmGlobalVisualStudioGenerator::VS9:
      return "v90";
    case cmGlobalVisualStudioGenerator::VS10:
      return "v100";
    case cmGlobalVisualStudioGenerator::VS11:
      return "v110";
    case cmGlobalVisualStudioGenerator::VS12:
      return "v120";
    case cmGlobalVisualStudioGenerator::VS14:
      return "v140";
    case cmGlobalVisualStudioGenerator::VS15:
      return "v141";
    case cmGlobalVisualStudioGenerator::VS16:
      return "v142";
  }
  return "";
}

static const char vs15generatorName[] = "Visual Studio 15 2017";

// Map generator name without year to name with year.
static const char* cmVS15GenName(const std::string& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs15generatorName,
              sizeof(vs15generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs15generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2017")) {
    p += 5;
  }
  genName = std::string(vs15generatorName) + p;
  return p;
}

class cmGlobalVisualStudioVersionedGenerator::Factory15
  : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const override
  {
    std::string genName;
    const char* p = cmVS15GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudioVersionedGenerator(
        cmGlobalVisualStudioGenerator::VS15, cm, genName, "");
    }
    if (*p++ != ' ') {
      return 0;
    }
    if (strcmp(p, "Win64") == 0) {
      return new cmGlobalVisualStudioVersionedGenerator(
        cmGlobalVisualStudioGenerator::VS15, cm, genName, "x64");
    }
    if (strcmp(p, "ARM") == 0) {
      return new cmGlobalVisualStudioVersionedGenerator(
        cmGlobalVisualStudioGenerator::VS15, cm, genName, "ARM");
    }
    return 0;
  }

  void GetDocumentation(cmDocumentationEntry& entry) const override
  {
    entry.Name = std::string(vs15generatorName) + " [arch]";
    entry.Brief = "Generates Visual Studio 2017 project files.  "
                  "Optional [arch] can be \"Win64\" or \"ARM\".";
  }

  std::vector<std::string> GetGeneratorNames() const override
  {
    std::vector<std::string> names;
    names.push_back(vs15generatorName);
    return names;
  }

  std::vector<std::string> GetGeneratorNamesWithPlatform() const override
  {
    std::vector<std::string> names;
    names.push_back(vs15generatorName + std::string(" ARM"));
    names.push_back(vs15generatorName + std::string(" Win64"));
    return names;
  }

  bool SupportsToolset() const override { return true; }
  bool SupportsPlatform() const override { return true; }

  std::vector<std::string> GetKnownPlatforms() const override
  {
    std::vector<std::string> platforms;
    platforms.emplace_back("x64");
    platforms.emplace_back("Win32");
    platforms.emplace_back("ARM");
    platforms.emplace_back("ARM64");
    return platforms;
  }

  std::string GetDefaultPlatformName() const override { return "Win32"; }
};

cmGlobalGeneratorFactory*
cmGlobalVisualStudioVersionedGenerator::NewFactory15()
{
  return new Factory15;
}

static const char vs16generatorName[] = "Visual Studio 16 2019";

// Map generator name without year to name with year.
static const char* cmVS16GenName(const std::string& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs16generatorName,
              sizeof(vs16generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs16generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2019")) {
    p += 5;
  }
  genName = std::string(vs16generatorName) + p;
  return p;
}

class cmGlobalVisualStudioVersionedGenerator::Factory16
  : public cmGlobalGeneratorFactory
{
public:
  virtual cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                                   cmake* cm) const
  {
    std::string genName;
    const char* p = cmVS16GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudioVersionedGenerator(
        cmGlobalVisualStudioGenerator::VS16, cm, genName, "");
    }
    return 0;
  }

  virtual void GetDocumentation(cmDocumentationEntry& entry) const
  {
    entry.Name = std::string(vs16generatorName);
    entry.Brief = "Generates Visual Studio 2019 project files.  "
                  "Use -A option to specify architecture.";
  }

  std::vector<std::string> GetGeneratorNames() const override
  {
    std::vector<std::string> names;
    names.push_back(vs16generatorName);
    return names;
  }

  std::vector<std::string> GetGeneratorNamesWithPlatform() const override
  {
    return std::vector<std::string>();
  }

  bool SupportsToolset() const override { return true; }
  bool SupportsPlatform() const override { return true; }

  std::vector<std::string> GetKnownPlatforms() const override
  {
    std::vector<std::string> platforms;
    platforms.emplace_back("x64");
    platforms.emplace_back("Win32");
    platforms.emplace_back("ARM");
    platforms.emplace_back("ARM64");
    return platforms;
  }

  std::string GetDefaultPlatformName() const override
  {
    return VSHostPlatformName();
  }
};

cmGlobalGeneratorFactory*
cmGlobalVisualStudioVersionedGenerator::NewFactory16()
{
  return new Factory16;
}

cmGlobalVisualStudioVersionedGenerator::cmGlobalVisualStudioVersionedGenerator(
  VSVersion version, cmake* cm, const std::string& name,
  std::string const& platformInGeneratorName)
  : cmGlobalVisualStudio14Generator(cm, name, platformInGeneratorName)
  , vsSetupAPIHelper(VSVersionToMajor(version))
{
  this->Version = version;
  this->ExpressEdition = false;
  this->DefaultPlatformToolset = VSVersionToToolset(this->Version);
  this->DefaultCLFlagTableName = VSVersionToToolset(this->Version);
  this->DefaultCSharpFlagTableName = VSVersionToToolset(this->Version);
  this->DefaultLinkFlagTableName = VSVersionToToolset(this->Version);
  if (this->Version >= cmGlobalVisualStudioGenerator::VS16) {
    this->DefaultPlatformName = VSHostPlatformName();
    this->DefaultPlatformToolsetHostArchitecture = VSHostArchitecture();
  }
}

bool cmGlobalVisualStudioVersionedGenerator::MatchesGeneratorName(
  const std::string& name) const
{
  std::string genName;
  switch (this->Version) {
    case cmGlobalVisualStudioGenerator::VS9:
    case cmGlobalVisualStudioGenerator::VS10:
    case cmGlobalVisualStudioGenerator::VS11:
    case cmGlobalVisualStudioGenerator::VS12:
    case cmGlobalVisualStudioGenerator::VS14:
      break;
    case cmGlobalVisualStudioGenerator::VS15:
      if (cmVS15GenName(name, genName)) {
        return genName == this->GetName();
      }
      break;
    case cmGlobalVisualStudioGenerator::VS16:
      if (cmVS16GenName(name, genName)) {
        return genName == this->GetName();
      }
      break;
  }
  return false;
}

bool cmGlobalVisualStudioVersionedGenerator::SetGeneratorInstance(
  std::string const& i, cmMakefile* mf)
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
      mf->IssueMessage(MessageType::FATAL_ERROR, e.str());
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
    mf->IssueMessage(MessageType::FATAL_ERROR, e.str());
    return false;
  }

  // Save the selected instance persistently.
  std::string genInstance = mf->GetSafeDefinition("CMAKE_GENERATOR_INSTANCE");
  if (vsInstance != genInstance) {
    this->CMakeInstance->AddCacheEntry(
      "CMAKE_GENERATOR_INSTANCE", vsInstance.c_str(),
      "Generator instance identifier.", cmStateEnums::INTERNAL);
  }

  return true;
}

bool cmGlobalVisualStudioVersionedGenerator::GetVSInstance(
  std::string& dir) const
{
  return vsSetupAPIHelper.GetVSInstanceInfo(dir);
}

bool cmGlobalVisualStudioVersionedGenerator::IsDefaultToolset(
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

std::string cmGlobalVisualStudioVersionedGenerator::GetAuxiliaryToolset() const
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

bool cmGlobalVisualStudioVersionedGenerator::InitializeWindows(cmMakefile* mf)
{
  // If the Win 8.1 SDK is installed then we can select a SDK matching
  // the target Windows version.
  if (this->IsWin81SDKInstalled()) {
    // VS 2019 does not default to 8.1 so specify it explicitly when needed.
    if (this->Version >= cmGlobalVisualStudioGenerator::VS16 &&
        !cmSystemTools::VersionCompareGreater(this->SystemVersion, "8.1")) {
      this->SetWindowsTargetPlatformVersion("8.1", mf);
      return true;
    }
    return cmGlobalVisualStudio14Generator::InitializeWindows(mf);
  }
  // Otherwise we must choose a Win 10 SDK even if we are not targeting
  // Windows 10.
  return this->SelectWindows10SDK(mf, false);
}

bool cmGlobalVisualStudioVersionedGenerator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    if (this->IsWindowsStoreToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = VSVersionToToolset(this->Version);
      return true;
    } else {
      return false;
    }
  }
  return this->cmGlobalVisualStudio14Generator::SelectWindowsStoreToolset(
    toolset);
}

bool cmGlobalVisualStudioVersionedGenerator::IsWindowsDesktopToolsetInstalled()
  const
{
  return vsSetupAPIHelper.IsVSInstalled();
}

bool cmGlobalVisualStudioVersionedGenerator::IsWindowsStoreToolsetInstalled()
  const
{
  return vsSetupAPIHelper.IsWin10SDKInstalled();
}

bool cmGlobalVisualStudioVersionedGenerator::IsWin81SDKInstalled() const
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
        win81Root, cmSystemTools::KeyWOW64_32) ||
      cmSystemTools::ReadRegistryValue(
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\"
        "Windows Kits\\Installed Roots;KitsRoot81",
        win81Root, cmSystemTools::KeyWOW64_32)) {
    return cmSystemTools::FileExists(win81Root + "/include/um/windows.h",
                                     true);
  }
  return false;
}

std::string cmGlobalVisualStudioVersionedGenerator::GetWindows10SDKMaxVersion()
  const
{
  return std::string();
}

std::string cmGlobalVisualStudioVersionedGenerator::FindMSBuildCommand()
{
  std::string msbuild;

  // Ask Visual Studio Installer tool.
  std::string vs;
  if (vsSetupAPIHelper.GetVSInstanceInfo(vs)) {
    msbuild = vs + "/MSBuild/Current/Bin/MSBuild.exe";
    if (cmSystemTools::FileExists(msbuild)) {
      return msbuild;
    }
    msbuild = vs + "/MSBuild/15.0/Bin/MSBuild.exe";
    if (cmSystemTools::FileExists(msbuild)) {
      return msbuild;
    }
  }

  msbuild = "MSBuild.exe";
  return msbuild;
}

std::string cmGlobalVisualStudioVersionedGenerator::FindDevEnvCommand()
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

bool cmGlobalVisualStudioVersionedGenerator::SetGeneratorPlatform(
  std::string const& p, cmMakefile* mf)
{
  if (this->IsAndroidMSVS()) {
    if (p != "ARM64" && p != "ARM" && p != "x86_64" && p != "x86") {
      std::string err;
      err = std::string("Building for Android with '") + this->GetName() +
        "' Platform '" + p + "' not supported. Only ARM, ARM64, x86, x86_64";
      mf->IssueMessage(MessageType::FATAL_ERROR, err.c_str());
      return false;
    }
  }

  return cmGlobalVisualStudio14Generator::SetGeneratorPlatform(p, mf);
}

bool cmGlobalVisualStudioVersionedGenerator::SetGeneratorToolset(
  std::string const& ts, cmMakefile* mf)
{
  if (this->IsAndroidMSVS() && ts.empty() &&
      this->DefaultPlatformToolset.empty()) {
    std::ostringstream e;
    e << this->GetName()
      << " MSVS Android requires CMAKE_GENERATOR_TOOLSET to be set.";
    mf->IssueMessage(MessageType::FATAL_ERROR, e.str());
    return false;
  }

  return cmGlobalVisualStudio14Generator::SetGeneratorToolset(ts, mf);
}

std::string cmGlobalVisualStudioVersionedGenerator::GetAndroidAPILevel() const
{
  return this->AndroidAPILevel;
}

bool cmGlobalVisualStudioVersionedGenerator::FindVCTargetsPath(cmMakefile* mf)
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

bool cmGlobalVisualStudioVersionedGenerator::InitializeSystem(cmMakefile* mf)
{
  if (strcmp(this->SystemName.c_str(), "Android") != 0) {
    this->SystemIsAndroidMSVS = false;
    return cmGlobalVisualStudio14Generator::InitializeSystem(mf);
  }

  if (!InitializeAndroidWorkflow(mf)) {
    if (!GetInstalledNsightTegraVersion().empty()) {
      return cmGlobalVisualStudio14Generator::InitializeSystem(mf);
    }

    mf->IssueMessage(MessageType::FATAL_ERROR,
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
    mf->IssueMessage(MessageType::FATAL_ERROR, e.str());
    return false;
  }

  this->SystemIsAndroidMSVS = true;
  this->DefaultPlatformToolset = GetDefaultAndroidToolChain();

  // even though not targeting windows 10 sdk need a valid one for utility
  // vcxproj files
  this->SelectWindows10SDK(mf, false);

  return true;
}

/// @TODO: These paths are correct for MSVS 2017 but not 2019! tool chains have moved!
bool cmGlobalVisualStudioVersionedGenerator::InitializeAndroidWorkflow(cmMakefile* mf)
{
  if (!VersionAndroidMSVS.empty()) {
    return true;
  }

  std::string vsInstance;
  if (!this->GetVSInstance(vsInstance)) {
    mf->IssueMessage(MessageType::FATAL_ERROR, "VisualStudio not installed!");
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
    mf->IssueMessage(MessageType::FATAL_ERROR, "CMAKE_GENERATOR_PLATFORM not set!");
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
                             generatorPlatform + "/PlatformToolsets/Clang*");

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
    mf->IssueMessage(
      MessageType::FATAL_ERROR,
      "Could not find any clang toolchains for MSVS/Android work flow!");
    return false;
  }

  const char* versionDirName =
    strrchr(highestInstalledWorkflow.c_str(), '/') + 1;
  VersionAndroidMSVS = versionDirName;

  const char* toolChainName = strrchr(highestClangToolChain.c_str(), '/') + 1;
  DefaultAndroidToolChain = toolChainName;

  AndroidAPILevel = mf->GetSafeDefinition("ANDROID_NATIVE_API_LEVEL");

  if (AndroidAPILevel.empty()) {
    AndroidAPILevel = "26";
  }

  return true;
}

std::string cmGlobalVisualStudioVersionedGenerator::GetDefaultAndroidToolChain() const
{
  return DefaultAndroidToolChain;
}
