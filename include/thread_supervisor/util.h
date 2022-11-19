/**
    @file
    @author  Alexander Sherikov
    @copyright 2022 Alexander Sherikov. Licensed under the Apache License,
    Version 2.0. (see LICENSE or http://www.apache.org/licenses/LICENSE-2.0)
    @brief
*/

#pragma once

/// Class definitions that prevent copying
#define THREAD_SUPERVISOR_DISABLE_CLASS_COPY(Class)                                                                    \
public:                                                                                                                \
    Class(const Class &) = delete;                                                                                     \
    Class &operator=(const Class &) = delete;                                                                          \
    Class(Class &&) = delete;                                                                                          \
    Class &operator=(Class &&) = delete;
