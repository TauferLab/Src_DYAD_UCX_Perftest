#ifndef DYAD_UCX_PERFTEST_UTILS_H
#define DYAD_UCX_PERFTEST_UTILS_H

#include <fmt/core.h>

#if ENABLE_LOG
#define DYAD_PERFTEST_INFO(log_msg, ...) \
    fmt::print(stdout, std::string(log_msg) + "\n", __VA_ARGS__); 
#define DYAD_PERFTEST_ERROR(log_msg, ...) \
    fmt::print(stderr, std::string(log_msg) + "\n", __VA_ARGS__); 
#else
#define DYAD_PERFTEST_INFO(log_msg, ...)
#define DYAD_PERFTEST_ERROR(log_msg, ...)
#endif

#endif /* DYAD_UCX_PERFTEST_UTILS_H */