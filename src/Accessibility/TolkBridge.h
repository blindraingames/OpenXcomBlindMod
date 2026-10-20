#pragma once

#include <string>

namespace OpenXcom
{

class TolkBridge
{
private:
	void *_module;
	bool _loaded;
	std::string _screenReader;

	TolkBridge(const TolkBridge &);
	TolkBridge &operator=(const TolkBridge &);
public:
	TolkBridge();
	~TolkBridge();
	bool initialize();
	void shutdown();
	bool isLoaded() const;
	bool hasScreenReader() const;
	const std::string &getScreenReaderName() const;
	bool output(const std::string &text, bool interrupt);
	void silence();
};

}
