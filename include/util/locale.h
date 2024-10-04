/**
 * @file locale.h
 * @brief Provides utility classes and functions for locale management.
 */

#pragma once

#include <locale>
#include <utility>

namespace PlainCloud::Util::Locale {

/**
 * @brief RAII wrapper for `std::locale::global`.
 *
 * This class sets the global locale upon creation
 * and restores the previous locale upon destruction.
 */
class ScopedGlobalLocale {
public:
    ScopedGlobalLocale() = delete;
    ScopedGlobalLocale(ScopedGlobalLocale const&) = delete;
    ScopedGlobalLocale(ScopedGlobalLocale&&) = delete;
    auto operator=(ScopedGlobalLocale const&) -> ScopedGlobalLocale& = delete;
    auto operator=(ScopedGlobalLocale&&) -> ScopedGlobalLocale& = delete;

    /**
     * @brief Constructs a new ScopedGlobalLocale object.
     *
     * Sets the global locale to the specified locale and stores the previous locale.
     *
     * @tparam Args Types for `std::locale` constructor arguments.
     * @param args Arguments for `std::locale` constructor.
     */
    template<typename... Args>
    explicit ScopedGlobalLocale(Args&&... args)
        : m_old_locale(std::locale::global(std::locale(
              std::forward<Args>(args)...))) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
    {
    }

    ~ScopedGlobalLocale()
    {
        std::locale::global(m_old_locale);
    }

private:
    std::locale m_old_locale;
};

} // namespace PlainCloud::Util::Locale
