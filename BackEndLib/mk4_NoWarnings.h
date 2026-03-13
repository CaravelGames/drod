#ifndef __MK4_NO_WARNINGS_H__
#define __MK4_NO_WARNINGS_H__

// The sole purpose of this file is to import mk4.h while also ignoring all
// warnings within it — across GCC, Clang, and MSVC.

#if defined(_MSC_VER)
    // MSVC
    #pragma warning(push, 0)
    #include <mk4.h>
    #pragma warning(pop)

// Clang also defines __GNUC__ so must be handled first
#elif defined(__clang__)
    // Clang (must come before GCC check — Clang also defines __GNUC__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
    #include <mk4.h>
    #pragma clang diagnostic pop

#elif defined(__GNUC__)
    // GCC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wparentheses"
    #pragma GCC diagnostic ignored "-Wdeprecated-copy"
    #include <mk4.h>
    #pragma GCC diagnostic pop

#else
    // Unknown compiler — include without suppression, but don't fail
    #include <mk4.h>
#endif

#endif // __MK4_NO_WARNINGS_H__