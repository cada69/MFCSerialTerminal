#pragma once

// Target Windows 10+
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// Prevent Windows min/max macros from clashing with std::min/std::max
#define NOMINMAX

#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <windows.h>
#include <devguid.h>      // GUID_DEVCLASS_PORTS
#include <setupapi.h>     // SetupDi*
#include <regstr.h>

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>      // std::sort, std::min
#include <fstream>        // std::ifstream, std::ofstream
#include <iterator>       // std::istreambuf_iterator
