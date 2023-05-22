#ifndef DEFINES_HPP
#define DEFINES_HPP

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG(fmt, ...)                    \
    fprintf(stdout, fmt, ##__VA_ARGS__); \
    fflush(stdout);

#define ASSERT(val, fmt, ...)                    \
    do                                           \
    {                                            \
        if (!(val))                                \
        {                                        \
            fprintf(stdout, fmt, ##__VA_ARGS__); \
            fflush(stdout);                      \
            assert(false);                       \
        }                                        \
    } while(0)                                   \

#define EXIT(fmt, ...)                       \
    do                                       \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
        assert(false);                       \
    } while (0)

#define VK_CHECK(val)                  \
    do                                 \
    {                                  \
        if (val != VK_SUCCESS)         \
        {                              \
            assert(val == VK_SUCCESS); \
        }                              \
    } while (false)

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

#endif // DEFINES_HPP