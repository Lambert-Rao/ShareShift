/*
 * ConfigFileReader.h
 *
 *  Created on: 2013-7-2
 *      Author: ziteng@mogujie.com
 */

#pragma once
#include <string>
#include <map>

class ConfigFileReader
{
public:
    ConfigFileReader(const char *filename);
    ~ConfigFileReader();

    char *GetConfigName(const char *name);
    int SetConfigValue(const char *name, const char *value);

private:
    void _LoadFile(const char *filename);
    int _WriteFIle(const char *filename = nullptr);
    void _ParseLine(char *line);
    char *_TrimSpace(char *name);

    bool m_load_ok;
    std::map<std::string, std::string> m_config_map;
    std::string m_config_file{};
};

