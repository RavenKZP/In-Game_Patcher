#pragma once

#include "Settings.h"

namespace MCP {

    void Register();
    void __stdcall RenderSettings();
    void __stdcall RenderLog();
    void __stdcall RenderBOSFile();
    void __stdcall RenderdBOSResolver(); 
    void __stdcall RenderKIDFile();

    inline std::vector<std::string> logLines;

};

namespace MCPLog {
    std::filesystem::path GetLogPath();
    std::vector<std::string> ReadLogFile();

    inline bool log_trace = true;
    inline bool log_info = true;
    inline bool log_warning = true;
    inline bool log_error = true;
    inline char custom[255];
};