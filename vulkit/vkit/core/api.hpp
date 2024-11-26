#pragma once

#include "tkit/core/api.hpp"

#ifdef TKIT_OS_WINDOWS
#    ifdef VKIT_SHARED_LIBRARY
#        ifdef VKIT_EXPORT
#            define VKIT_API __declspec(dllexport)
#        else
#            define VKIT_API __declspec(dllimport)
#        endif
#    else
#        define VKIT_API
#    endif
#elif defined(TKIT_OS_LINUX) || defined(TKIT_OS_APPLE)
#    if defined(VKIT_SHARED_LIBRARY) && defined(VKIT_EXPORT)
#        define VKIT_API __attribute__((visibility("default")))
#    else
#        define VKIT_API
#    endif
#endif
