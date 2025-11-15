# 来自BoostConfig.cmake的解释
```md
# A typical use might be
#
# find_package(Boost 1.70 REQUIRED COMPONENTS filesystem regex PATHS C:/Boost)
#
# On success, the above invocation would define the targets Boost::headers,
# Boost::filesystem and Boost::regex. Boost::headers represents all
# header-only libraries. An alias, Boost::boost, for Boost::headers is
# provided for compatibility.
#
# Requesting the special component "ALL" will make all installed components
# available, as in the following example:
#
# find_package(Boost 1.73 REQUIRED COMPONENTS ALL)
#
# Since COMPONENTS is optional when REQUIRED is specified, the above can be
# shortened to
#
# find_package(Boost 1.73 REQUIRED ALL)
#
# When ALL is used, a variable Boost_ALL_TARGETS will be set and will contain
# the names of all created targets.
```
boost-1.89.0(win,msvc) 的 Boost_ALL_TARGETS输出如下：
Boost::headers;Boost::atomic;Boost::bzip2;Boost::charconv;Boost::chrono;Boost::container;Boost::context;Boost::contract;Boost::coroutine;Boost::date_time;Boost::exception;Boost::fiber;Boost::filesystem;Boost::graph;Boost::iostreams;Boost::json;Boost::locale;Boost::log;Boost::log_setup;Boost::math;Boost::math_c99;Boost::math_c99f;Boost::math_c99l;Boost::math_tr1;Boost::math_tr1f;Boost::math_tr1l;Boost::nowide;Boost::prg_exec_monitor;Boost::process;Boost::program_options;Boost::python;Boost::random;Boost::regex;Boost::serialization;Boost::stacktrace_from_exception;Boost::stacktrace_noop;Boost::stacktrace_windbg;Boost::stacktrace_windbg_cached;Boost::test_exec_monitor;Boost::thread;Boost::timer;Boost::type_erasure;Boost::unit_test_framework;Boost::url;Boost::wave;Boost::wserialization;Boost::zlib