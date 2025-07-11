#include "IdentifyEXE.h"
#include "LoaderError.h"
#include "obse64_common/obse64_version.h"
#include "obse64_common/Log.h"
#include <string>
#include <Windows.h>

bool VersionCheck(const ProcHookInfo &procInfo, u64 RUNTIME_VERSION)
{
	const u64 kCurVersion =
		(u64(GET_EXE_VERSION_MAJOR(RUNTIME_VERSION)) << 48) |
		(u64(GET_EXE_VERSION_MINOR(RUNTIME_VERSION)) << 32) |
		(u64(GET_EXE_VERSION_BUILD(RUNTIME_VERSION)) << 16);

	// convert version resource to internal version format
	u32 versionInternal = MAKE_EXE_VERSION(procInfo.version >> 48, procInfo.version >> 32, procInfo.version >> 16);

	// version mismatch could mean exe type mismatch
	if (procInfo.version != kCurVersion)
	{
#if GET_EXE_VERSION_SUB(RUNTIME_VERSION) == RUNTIME_TYPE_BETHESDA
		const int expectedProcType = kProcType_Steam;
		const char *expectedProcTypeName = "Steam";
#elif GET_EXE_VERSION_SUB(RUNTIME_VERSION) == RUNTIME_TYPE_GOG
		const int expectedProcType = kProcType_GOG;
		const char *expectedProcTypeName = "GOG";
#elif GET_EXE_VERSION_SUB(RUNTIME_VERSION) == RUNTIME_TYPE_MSSTORE
		const int expectedProcType = kProcType_WinStore;
		const char *expectedProcTypeName = "Microsoft Store";
#else
#error unknown runtime type
#endif

		// TODO: add Windows Store support
		// we only care about steam/gog for this check
		const char *foundProcTypeName = nullptr;

		switch (procInfo.procType)
		{
		case kProcType_Steam:
			foundProcTypeName = "Steam";
			break;

		case kProcType_GOG:
			foundProcTypeName = "GOG";
			break;
		}

		if (foundProcTypeName && (procInfo.procType != expectedProcType))
		{
			// different build
			PrintLoaderError(
				"This version of OBSE64 is compatible with the %s version of the game.\n"
				"You have the %s version of the game. Please download the correct version from the website.\n"
				"Runtime: %d.%d.%d\n"
				"OBSE64: %d.%d.%d",
				expectedProcTypeName, foundProcTypeName,
				GET_EXE_VERSION_MAJOR(versionInternal), GET_EXE_VERSION_MINOR(versionInternal), GET_EXE_VERSION_BUILD(versionInternal),
				OBSE_VERSION_INTEGER, OBSE_VERSION_INTEGER_MINOR, OBSE_VERSION_INTEGER_BETA);

			return false;
		}
	}

	if (procInfo.version < kCurVersion)
	{
#if OBSE_TARGETING_BETA_VERSION
		if (versionInternal == CURRENT_RELEASE_RUNTIME)
			PrintLoaderError(
				"You are using the version of OBSE64 intended for the Steam beta branch (%d.%d.%d).\n"
				"Download and install the non-beta branch version (%s) from http://obse.silverlock.org/.",
				OBSE_VERSION_INTEGER, OBSE_VERSION_INTEGER_MINOR, OBSE_VERSION_INTEGER_BETA, CURRENT_RELEASE_OBSE_STR);
		else
			PrintLoaderError(
				"You are using Oblivion Remastered version %d.%d.%d, which is out of date and incompatible with this version of OBSE64 (%d.%d.%d). Update to the latest beta version.",
				GET_EXE_VERSION_MAJOR(versionInternal), GET_EXE_VERSION_MINOR(versionInternal), GET_EXE_VERSION_BUILD(versionInternal),
				OBSE_VERSION_INTEGER, OBSE_VERSION_INTEGER_MINOR, OBSE_VERSION_INTEGER_BETA);
#else
		PrintLoaderError(
			"You are using Oblivion Remastered version %d.%d.%d, which is out of date and incompatible with this version of OBSE64 (%d.%d.%d). Update to the latest version.",
			GET_EXE_VERSION_MAJOR(versionInternal), GET_EXE_VERSION_MINOR(versionInternal), GET_EXE_VERSION_BUILD(versionInternal),
			OBSE_VERSION_INTEGER, OBSE_VERSION_INTEGER_MINOR, OBSE_VERSION_INTEGER_BETA);
#endif
	}
	else if (procInfo.version > kCurVersion)
	{
		PrintLoaderError(
			"You are using a newer version of Oblivion Remastered than this version of OBSE64 supports.\n"
			"If this version just came out, please be patient while we update our code.\n"
			"In the meantime, please check http://obse.silverlock.org for updates.\n"
			"Do not email about this!\n"
			"Runtime: %d.%d.%d\n"
			"OBSE64: %d.%d.%d",
			GET_EXE_VERSION_MAJOR(versionInternal), GET_EXE_VERSION_MINOR(versionInternal), GET_EXE_VERSION_BUILD(versionInternal),
			OBSE_VERSION_INTEGER, OBSE_VERSION_INTEGER_MINOR, OBSE_VERSION_INTEGER_BETA);
	}

	return true;
}

bool VersionStrToInt(const std::string &verStr, u64 *out)
{
	u64 result = 0;
	int parts[4];

	if (sscanf_s(verStr.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) != 4)
		return false;

	for (int i = 0; i < 4; i++)
	{
		if (parts[i] > 0xFFFF)
			return false;

		result <<= 16;
		result |= parts[i];
	}

	*out = result;

	return true;
}

static bool GetFileVersionData(const char *path, u64 *out, std::string *outProductName)
{
	std::string productVersionStr;
	VS_FIXEDFILEINFO versionInfo;
	if (!GetFileVersion(path, &versionInfo, outProductName, &productVersionStr))
		return false;

	DumpVersionInfo(versionInfo);
	_MESSAGE("productVersionStr = %s", productVersionStr.c_str());

	u64 version = 0;
	if (!VersionStrToInt(productVersionStr, &version))
		return false;

	*out = version;

	return true;
}

void DumpVersionInfo(const VS_FIXEDFILEINFO &info)
{
	_MESSAGE("dwSignature = %08X", info.dwSignature);
	_MESSAGE("dwStrucVersion = %08X", info.dwStrucVersion);
	_MESSAGE("dwFileVersionMS = %08X", info.dwFileVersionMS);
	_MESSAGE("dwFileVersionLS = %08X", info.dwFileVersionLS);
	_MESSAGE("dwProductVersionMS = %08X", info.dwProductVersionMS);
	_MESSAGE("dwProductVersionLS = %08X", info.dwProductVersionLS);
	_MESSAGE("dwFileFlagsMask = %08X", info.dwFileFlagsMask);
	_MESSAGE("dwFileFlags = %08X", info.dwFileFlags);
	_MESSAGE("dwFileOS = %08X", info.dwFileOS);
	_MESSAGE("dwFileType = %08X", info.dwFileType);
	_MESSAGE("dwFileSubtype = %08X", info.dwFileSubtype);
	_MESSAGE("dwFileDateMS = %08X", info.dwFileDateMS);
	_MESSAGE("dwFileDateLS = %08X", info.dwFileDateLS);
}

// non-relocated image
const IMAGE_SECTION_HEADER *GetImageSection(const u8 *base, const char *name)
{
	const IMAGE_DOS_HEADER *dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS *ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_SECTION_HEADER *sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	for (u32 i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER *section = &sectionHeader[i];

		if (!strcmp((const char *)section->Name, name))
		{
			return section;
		}
	}

	return NULL;
}

// non-relocated image
bool HasImportedLibrary(const u8 *base, const char *name)
{
	auto *dosHeader = (const IMAGE_DOS_HEADER *)base;
	auto *ntHeader = (const IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	auto *importDir = (const IMAGE_DATA_DIRECTORY *)&ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	if (!importDir->Size || !importDir->VirtualAddress)
		return false;

	// resolve RVA -> file offset
	const auto *sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	auto LookupRVA = [ntHeader, sectionHeader, base](u32 rva) -> const u8 *
	{
		for (u32 i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
		{
			const auto *section = &sectionHeader[i];

			if ((rva >= section->VirtualAddress) &&
				(rva < section->VirtualAddress + section->SizeOfRawData))
			{
				return base + rva - section->VirtualAddress + section->PointerToRawData;
			}
		}

		return nullptr;
	};

	if (const auto *importTable = (const IMAGE_IMPORT_DESCRIPTOR *)LookupRVA(importDir->VirtualAddress))
	{
		for (; importTable->Characteristics; ++importTable)
		{
			auto *dllName = (const char *)LookupRVA(importTable->Name);

			if (dllName && !_stricmp(dllName, name))
			{
				return true;
			}
		}
	}

	return false;
}

// steam EXE will have the .bind section
static bool IsSteamImage(const u8 *base)
{
	return GetImageSection(base, ".bind") != NULL;
}

static bool IsUPXImage(const u8 *base)
{
	return GetImageSection(base, "UPX0") != NULL;
}

static bool IsWinStoreImage(const u8 *base)
{
	return HasImportedLibrary(base, "api-ms-win-core-psm-appnotify-l1-1-0.dll"); // not tested, haven't seen the msstore exe yet
}

static bool IsGOGImage(const u8 *base)
{
	return HasImportedLibrary(base, "Galaxy64.dll");
}

static bool IsEpicImage(const u8 *base)
{
	return HasImportedLibrary(base, "eossdk-win64-shipping.dll");
}

bool ScanEXE(const char *path, ProcHookInfo *hookInfo)
{
	// Check if this is likely an MS Store executable based on filename
	if (strstr(path, "WinGDK") != nullptr)
	{
		_MESSAGE("MS Store executable detected from path, skipping detailed scan");
		hookInfo->procType = kProcType_WinStore;
		return true;
	}

	// open and map the file in to memory
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		_ERROR("ScanEXE: couldn't open file (%d)", err);

		// If access denied and path contains "WindowsApps", it's likely MS Store
		if (err == ERROR_ACCESS_DENIED && (strstr(path, "WindowsApps") != nullptr || strstr(path, "WinGDK") != nullptr))
		{
			_MESSAGE("Access denied, but path suggests MS Store app, assuming MS Store");
			hookInfo->procType = kProcType_WinStore;
			return true;
		}

		return false;
	}

	bool result = false;

	HANDLE mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (mapping)
	{
		const u8 *fileBase = (const u8 *)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
		if (fileBase)
		{
			// scan for packing type
			bool isWinStore = IsWinStoreImage(fileBase);

			if (IsUPXImage(fileBase))
			{
				hookInfo->procType = kProcType_Packed;
			}
			else if (IsSteamImage(fileBase))
			{
				hookInfo->procType = kProcType_Steam;
			}
			else if (isWinStore)
			{
				hookInfo->procType = kProcType_WinStore;
			}
			else if (IsGOGImage(fileBase))
			{
				hookInfo->procType = kProcType_GOG;
			}
			else if (IsEpicImage(fileBase))
			{
				hookInfo->procType = kProcType_Epic;
			}
			else
			{
				hookInfo->procType = kProcType_Normal;
			}

			result = true;

			UnmapViewOfFile(fileBase);
		}
		else
		{
			DWORD err = GetLastError();
			_ERROR("ScanEXE: couldn't map file (%d)", err);

			// If access denied and path contains "WindowsApps", it's likely MS Store
			if (err == ERROR_ACCESS_DENIED && (strstr(path, "WindowsApps") != nullptr || strstr(path, "WinGDK") != nullptr))
			{
				_MESSAGE("Access denied when mapping file, but path suggests MS Store app, assuming MS Store");
				hookInfo->procType = kProcType_WinStore;
				result = true;
			}
		}

		CloseHandle(mapping);
	}
	else
	{
		DWORD err = GetLastError();
		_ERROR("ScanEXE: couldn't create file mapping (%d)", err);

		// If access denied and path contains "WindowsApps", it's likely MS Store
		if (err == ERROR_ACCESS_DENIED && (strstr(path, "WindowsApps") != nullptr || strstr(path, "WinGDK") != nullptr))
		{
			_MESSAGE("Access denied when creating mapping, but path suggests MS Store app, assuming MS Store");
			hookInfo->procType = kProcType_WinStore;
			result = true;
		}
	}

	CloseHandle(file);

	return result;
}

bool IdentifyEXE(const char *procName, bool isEditor, std::string *dllSuffix, ProcHookInfo *hookInfo)
{
	u64 version;
	std::string productName;

	// Check if this is likely an MS Store executable
	bool isMSStore = (strstr(procName, "WinGDK") != nullptr);

	// check file version
	if (!isMSStore && !GetFileVersionData(procName, &version, &productName))
	{
		// For MS Store versions, we can't reliably get the version info
		// So we'll hardcode a known compatible version if this seems to be an MS Store executable
		if (isMSStore)
		{
			_MESSAGE("MS Store executable detected, using hardcoded version info");

			// Use the current known version for MS Store
			// Format: (major << 48) | (minor << 32) | (build << 16) | (sub)
			version = (u64(0) << 48) | (u64(411) << 32) | (u64(140) << 16) | (u64(RUNTIME_TYPE_MSSTORE));
			productName = "Oblivion Remastered MS";

			_MESSAGE("Using version = %016I64X", version);
			_MESSAGE("product name = %s", productName.c_str());
		}
		else
		{
			PrintLoaderError("Couldn't retrieve EXE version information.");
			return false;
		}
	}

	_MESSAGE("version = %016I64X", version);
	_MESSAGE("product name = %s", productName.c_str());

	hookInfo->version = version;
	hookInfo->packedVersion = MAKE_EXE_VERSION(version >> 48, version >> 32, version >> 16);

	if (productName == "OBSE64")
	{
		_MESSAGE("found an OBSE64 component");
		return false;
	}

	// check protection type
	if (!ScanEXE(procName, hookInfo))
	{
		PrintLoaderError("Failed to identify EXE type.");
		return false;
	}

	// For MS Store version, explicitly set the process type
	if (isMSStore)
	{
		hookInfo->procType = kProcType_WinStore;
		_MESSAGE("winstore exe (detected via filename)");
	}
	else
	{
		switch (hookInfo->procType)
		{
		case kProcType_Steam:
			_MESSAGE("steam exe");
			break;
		case kProcType_Normal:
			_MESSAGE("normal exe");
			break;
		case kProcType_Packed:
			_MESSAGE("packed exe");
			break;
		case kProcType_WinStore:
			_MESSAGE("winstore exe");
			break;
		case kProcType_GOG:
			_MESSAGE("gog exe");
			break;
		case kProcType_Epic:
			_MESSAGE("epic exe");
			break;
		case kProcType_Unknown:
		default:
			_MESSAGE("unknown exe type");
			break;
		}
	}

	if (hookInfo->procType == kProcType_Epic)
	{
		PrintLoaderError("The Epic Store version of Oblivion Remastered is not supported.");
		return false;
	}

	bool result = false;

	if (isEditor)
	{
		switch (hookInfo->procType)
		{
		case kProcType_Steam:
		case kProcType_Normal:
		case kProcType_WinStore:
		case kProcType_GOG:
			*dllSuffix = "";

			result = true;

			break;
		case kProcType_Unknown:
		default:
			PrintLoaderError("Unsupported editor executable type.");
			break;
		}
	}
	else
	{
		char versionStr[256];
		sprintf_s(versionStr, "%d_%d_%d", hookInfo->getVersionMajor(), hookInfo->getVersionMinor(), hookInfo->getVersionBuild());
		*dllSuffix = versionStr;

		switch (hookInfo->procType)
		{
		case kProcType_Steam:
		case kProcType_Normal:
			result = true;

			break;

		case kProcType_GOG:
			*dllSuffix += "_gog";

			result = true;
			break;

		case kProcType_WinStore:
			*dllSuffix += "_winstore";

			result = true;

			break;

		case kProcType_Packed:
			PrintLoaderError("Packed versions of Oblivion Remastered are not supported.");
			break;

		case kProcType_Unknown:
		default:
			PrintLoaderError("Unknown executable type.");
			break;
		}
	}

	return result;
}
