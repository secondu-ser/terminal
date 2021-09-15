// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "VsSetupConfiguration.h"

using namespace winrt::Microsoft::Terminal::Settings::Model;

std::vector<VsSetupConfiguration::VsSetupInstance> VsSetupConfiguration::QueryInstances()
{
    std::vector<VsSetupInstance> instances;

    // SetupConfiguration is only registered if Visual Studio is installed
    ComPtrSetupQuery pQuery{ wil::CoCreateInstanceNoThrow<SetupConfiguration, ISetupConfiguration2>() };
    if (pQuery == nullptr)
    {
        return instances;
    }

    // Enumerate all valid instances of Visual Studio
    wil::com_ptr<IEnumSetupInstances> e;
    THROW_IF_FAILED(pQuery->EnumInstances(&e));

    std::array<ComPtrSetupInstance, 1> rgpInstance;
    auto result = e->Next(1, &rgpInstance[0], NULL);
    while (S_OK == result)
    {
        // wrap the COM pointers in a friendly interface
        instances.emplace_back(VsSetupInstance{ pQuery, rgpInstance[0] });
        result = e->Next(1, &rgpInstance[0], NULL);
    }

    THROW_IF_FAILED(result);

    return instances;
}

/// <summary>
/// Takes a relative path under a Visual Studio installation and returns the absolute path.
/// </summary>
std::wstring VsSetupConfiguration::ResolvePath(ComPtrSetupInstance pInst, std::wstring_view relativePath)
{
    wil::unique_bstr bstrAbsolutePath;
    THROW_IF_FAILED(pInst->ResolvePath(relativePath.data(), &bstrAbsolutePath));
    return bstrAbsolutePath.get();
}

/// <summary>
/// Determines whether a Visual Studio installation version falls within a specified range.
/// The range is specified as a string, ex: "[15.0.0.0,)", "[15.0.0.0, 16.7.0.0)
/// </summary>
bool VsSetupConfiguration::InstallationVersionInRange(ComPtrSetupQuery pQuery, ComPtrSetupInstance pInst, std::wstring_view range)
{
    auto helper = pQuery.query<ISetupHelper>();

    // VS versions in a string format such as "16.3.0.0" can be easily compared
    // by parsing them into 64-bit unsigned integers using the stable algorithm
    // provided by ParseVersion and ParseVersionRange

    unsigned long long minVersion{ 0 };
    unsigned long long maxVersion{ 0 };
    THROW_IF_FAILED(helper->ParseVersionRange(range.data(), &minVersion, &maxVersion));

    wil::unique_bstr bstrVersion;
    THROW_IF_FAILED(pInst->GetInstallationVersion(&bstrVersion));

    unsigned long long ullVersion{ 0 };
    THROW_IF_FAILED(helper->ParseVersion(bstrVersion.get(), &ullVersion));

    return ullVersion >= minVersion && ullVersion <= maxVersion;
}

std::wstring VsSetupConfiguration::GetInstallationVersion(ComPtrSetupInstance pInst)
{
    wil::unique_bstr bstrInstallationVersion;
    THROW_IF_FAILED(pInst->GetInstallationVersion(&bstrInstallationVersion));
    return bstrInstallationVersion.get();
}

std::wstring VsSetupConfiguration::GetInstallationPath(ComPtrSetupInstance pInst)
{
    wil::unique_bstr bstrInstallationPath;
    THROW_IF_FAILED(pInst->GetInstallationPath(&bstrInstallationPath));
    return bstrInstallationPath.get();
}

/// <summary>
/// The instance id is unique for each Visual Studio installation on a system.
/// The instance id is generated by the Visual Studio setup engine and varies from system to system.
/// </summary>
std::wstring VsSetupConfiguration::GetInstanceId(ComPtrSetupInstance pInst)
{
    wil::unique_bstr bstrInstanceId;
    THROW_IF_FAILED(pInst->GetInstanceId(&bstrInstanceId));
    return bstrInstanceId.get();
}

std::wstring VsSetupConfiguration::GetStringProperty(ComPtrPropertyStore pProps, std::wstring_view name)
{
    if (pProps == nullptr)
    {
        return std::wstring{};
    }

    wil::unique_variant var;
    if (FAILED(pProps->GetValue(name.data(), &var)))
    {
        return std::wstring{};
    }

    return var.bstrVal;
}
