/**
 * @file locale.h
 * @brief Provides utility classes and functions for locale management.
 */

#pragma once

#include <locale>
#include <utility>

namespace SlimLog::Util::Locale {

/**
 * @brief RAII class to temporarily set the global locale.
 *
 * On construction, sets the global locale to the specified locale.
 * On destruction, restores the previous global locale.
 */
class ScopedGlobalLocale {
public:
    ScopedGlobalLocale(ScopedGlobalLocale const&) = delete;
    ScopedGlobalLocale(ScopedGlobalLocale&&) = delete;
    auto operator=(ScopedGlobalLocale const&) -> ScopedGlobalLocale& = delete;
    auto operator=(ScopedGlobalLocale&&) -> ScopedGlobalLocale& = delete;

    /**
     * @brief Constructs a new `ScopedGlobalLocale` object.
     *
     * Sets the global locale to a newly constructed locale using the provided arguments.
     * Stores the previous global locale to restore it later.
     *
     * @tparam Args Types of arguments used to construct a `std::locale`.
     * @param args Arguments forwarded to the `std::locale` constructor.
     */
    template<typename... Args>
    explicit ScopedGlobalLocale(Args&&... args)
        // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
        : m_old_locale(std::locale::global(std::locale(std::forward<Args>(args)...)))
    {
    }

    ~ScopedGlobalLocale()
    {
        std::locale::global(m_old_locale);
    }

private:
    std::locale m_old_locale;
};

} // namespace SlimLog::Util::Locale
