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
    Class(const Class &) = delete;            /* NOLINT */                                                             \
    Class &operator=(const Class &) = delete; /* NOLINT */                                                             \
    Class(Class &&) = delete;                 /* NOLINT */                                                             \
    Class &operator=(Class &&) = delete;      /* NOLINT */
