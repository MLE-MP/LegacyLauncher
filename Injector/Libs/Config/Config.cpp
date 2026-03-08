#include "Config.h"

std::string GetConfigPath(std::string name);
constexpr const char* CURRENT_VERSION = "1";

Config::Config(std::string folderName) {
    this->configPath = GetConfigPath(folderName);

    std::map<std::string, std::string> config;
    if (std::filesystem::exists(this->configPath)) {
        config = ReadConfigFile();
        if (config["version"] != CURRENT_VERSION) {
            WriteConfigFile(config);
        }
    } else {
        WriteConfigFile({});
    }
}

std::map<std::string, std::string> DEFAULT_CONFIG = {
    { "version", CURRENT_VERSION },
    { "key", "" },
};

std::string GetConfigPath(std::string name) {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::string full = std::string(path) + "\\" + name;
        std::filesystem::create_directories(full);
        return full + "\\config.inj";
    }
    return "";
}

std::map<std::string, std::string> Config::ReadConfigFile() {
    std::ifstream in(this->configPath);
    std::map<std::string, std::string> config;
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        config[key] = value;
    }

    return config;
}

void Config::WriteConfigFile(const std::map<std::string, std::string>& current) {
    std::ofstream out(this->configPath);
    out << "version=" << CURRENT_VERSION << "\n";

    for (const auto& [key, defaultVal] : DEFAULT_CONFIG) {
        if (key == "version") continue;
        auto it = current.find(key);
        out << key << "=" << (it != current.end() ? it->second : defaultVal) << "\n";
    }

    for (const auto& [key, val] : current) {
        if (DEFAULT_CONFIG.find(key) == DEFAULT_CONFIG.end() && key != "version") {
            out << "# removed: " << key << "=" << val << "\n";
        }
    }
}