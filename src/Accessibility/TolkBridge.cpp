#include "TolkBridge.h"
#include "../Engine/Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace OpenXcom
{

#ifdef _WIN32
namespace
{
	typedef void (__cdecl *TolkLoadFunction)();
	typedef void (__cdecl *TolkUnloadFunction)();
	typedef bool (__cdecl *TolkIsLoadedFunction)();
	typedef const wchar_t *(__cdecl *TolkDetectFunction)();
	typedef bool (__cdecl *TolkOutputFunction)(const wchar_t *, bool);
	typedef bool (__cdecl *TolkSilenceFunction)();

	std::wstring utf8ToWide(const std::string &text)
	{
		if (text.empty()) return std::wstring();
		int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), static_cast<int>(text.size()), 0, 0);
		if (count <= 0)
		{
			count = MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), 0, 0);
			if (count <= 0) return std::wstring();
			std::wstring result(count, L'\0');
			MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), &result[0], count);
			return result;
		}
		std::wstring result(count, L'\0');
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), static_cast<int>(text.size()), &result[0], count);
		return result;
	}

	std::string wideToUtf8(const wchar_t *text)
	{
		if (!text || !*text) return std::string();
		int count = WideCharToMultiByte(CP_UTF8, 0, text, -1, 0, 0, 0, 0);
		if (count <= 1) return std::string();
		std::string result(count, '\0');
		WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], count, 0, 0);
		result.resize(count - 1);
		return result;
	}
}
#endif

TolkBridge::TolkBridge() : _module(0), _loaded(false)
{
}

TolkBridge::~TolkBridge()
{
	shutdown();
}

bool TolkBridge::initialize()
{
#ifdef _WIN32
	if (_module) return _loaded;
	HMODULE module = LoadLibraryW(L"Tolk.dll");
	if (!module)
	{
		Log(LOG_WARNING) << "Accessibility: could not load Tolk.dll (Windows error " << GetLastError() << ").";
		return false;
	}

	TolkLoadFunction load = reinterpret_cast<TolkLoadFunction>(GetProcAddress(module, "Tolk_Load"));
	TolkIsLoadedFunction isLoaded = reinterpret_cast<TolkIsLoadedFunction>(GetProcAddress(module, "Tolk_IsLoaded"));
	TolkDetectFunction detect = reinterpret_cast<TolkDetectFunction>(GetProcAddress(module, "Tolk_DetectScreenReader"));
	TolkOutputFunction output = reinterpret_cast<TolkOutputFunction>(GetProcAddress(module, "Tolk_Output"));
	TolkUnloadFunction unload = reinterpret_cast<TolkUnloadFunction>(GetProcAddress(module, "Tolk_Unload"));
	if (!load || !isLoaded || !detect || !output || !unload)
	{
		Log(LOG_WARNING) << "Accessibility: Tolk.dll is missing one or more required exports.";
		FreeLibrary(module);
		return false;
	}

	_module = module;
	load();
	_loaded = isLoaded();
	if (!_loaded)
	{
		Log(LOG_WARNING) << "Accessibility: Tolk_Load completed, but Tolk_IsLoaded returned false.";
		shutdown();
		return false;
	}
	_screenReader = wideToUtf8(detect());
	Log(LOG_INFO) << "Accessibility: Tolk initialized; screen reader: " << (_screenReader.empty() ? "not detected" : _screenReader) << ".";
	return true;
#else
	return false;
#endif
}

void TolkBridge::shutdown()
{
#ifdef _WIN32
	if (_module)
	{
		HMODULE module = static_cast<HMODULE>(_module);
		TolkUnloadFunction unload = reinterpret_cast<TolkUnloadFunction>(GetProcAddress(module, "Tolk_Unload"));
		if (_loaded && unload) unload();
		FreeLibrary(module);
	}
#endif
	_module = 0;
	_loaded = false;
	_screenReader.clear();
}

bool TolkBridge::isLoaded() const
{
	return _loaded;
}

bool TolkBridge::hasScreenReader() const
{
	return !_screenReader.empty();
}

const std::string &TolkBridge::getScreenReaderName() const
{
	return _screenReader;
}

bool TolkBridge::output(const std::string &text, bool interrupt)
{
#ifdef _WIN32
	if (!_loaded || !_module || text.empty()) return false;
	TolkOutputFunction output = reinterpret_cast<TolkOutputFunction>(GetProcAddress(static_cast<HMODULE>(_module), "Tolk_Output"));
	if (!output) return false;
	const std::wstring wide = utf8ToWide(text);
	const bool accepted = !wide.empty() && output(wide.c_str(), interrupt);
	if (!accepted)
	{
		Log(LOG_WARNING) << "Accessibility: Tolk rejected an output request.";
	}
	return accepted;
#else
	(void)text;
	(void)interrupt;
	return false;
#endif
}

void TolkBridge::silence()
{
#ifdef _WIN32
	if (!_loaded || !_module) return;
	TolkSilenceFunction silenceFunction = reinterpret_cast<TolkSilenceFunction>(GetProcAddress(static_cast<HMODULE>(_module), "Tolk_Silence"));
	if (silenceFunction) silenceFunction();
#endif
}

}
