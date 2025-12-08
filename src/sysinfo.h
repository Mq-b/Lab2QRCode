#pragma once
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <fstream>
#include <string>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/sysctl.h>
#endif

namespace sysinfo {

    struct Bytes {};
    struct KB {};
    struct MB {};
    struct GB {};

    namespace detail{

        inline double getSystemRAM_Bytes()
        {
#ifdef _WIN32

            MEMORYSTATUSEX statex;
            statex.dwLength = sizeof(statex);
            if (GlobalMemoryStatusEx(&statex)) {
                return statex.ullTotalPhys;   // Bytes
            }
            return 0;

#elif defined(__linux__)

            std::ifstream meminfo("/proc/meminfo");
            std::string key;
            uint64_t valueKB = 0;
            std::string unit;

            while (meminfo >> key >> valueKB >> unit) {
                if (key == "MemTotal:") {
                    return valueKB * 1024;    // kB → Bytes
                }
            }
            return 0;

#elif defined(__APPLE__) && defined(__MACH__)

            int mib[2] = { CTL_HW, HW_MEMSIZE };
            double memBytes = 0;
            size_t len = sizeof(memBytes);

            if (sysctl(mib, 2, &memBytes, &len, nullptr, 0) == 0) {
                return memBytes;              // Bytes
            }
            return 0;

#else
            return 0; // Unsupported platform
#endif
        }

    }


    template <typename T = KB>
    double getSystemRAM()
    {
        const double bytes = detail::getSystemRAM_Bytes();

        if constexpr (std::is_same_v<T, Bytes>) {
            return bytes;
        }
        else if constexpr (std::is_same_v<T, KB>) {
            return bytes / 1024;
        }
        else if constexpr (std::is_same_v<T, MB>) {
            return bytes / (1024ull * 1024ull);
        }
        else if constexpr (std::is_same_v<T, GB>) {
            return bytes / (1024ull * 1024ull * 1024ull);
        }
        else {
            static_assert(!sizeof(T), "Unknown unit type");
            return 0;
        }
    }

    inline unsigned int getCPUCoreCount() {
        const unsigned int cores = std::thread::hardware_concurrency();
        return cores;
    }
}