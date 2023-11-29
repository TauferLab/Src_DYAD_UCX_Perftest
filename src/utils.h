#ifndef DYAD_UCX_PERFTEST_UTILS_H
#define DYAD_UCX_PERFTEST_UTILS_H

#include <fmt/core.h>

#define DYAD_PERFTEST_INFO(log_msg, ...) \
    fmt::print(stdout, "{}\n", log_msg, __VA_ARGS__); 
#define DYAD_PERFTEST_ERROR(log_msg, ...) \
    fmt::print(stderr, "{}\n", log_msg, __VA_ARGS__); 

#endif /* DYAD_UCX_PERFTEST_UTILS_H */