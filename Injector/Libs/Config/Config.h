#pragma once
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

class Config {
public:
	Config(std::string folderName);

	std::map<std::string, std::string> ReadConfigFile();

	void WriteConfigFile(const std::map<std::string, std::string>& current);

private:
	std::string configPath;
};