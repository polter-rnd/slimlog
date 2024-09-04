/**
 * @file util.h
 * @brief Various utility classes and functions.
 */

#pragma once

#include <locale>
#include <utility>

namespace PlainCloud::Util::Locale {

/**
 * @brief RAII wrapper for `std::locale::global`
 *
 * Used to set locale information once created, and roll back on destruction.
 */
class ScopedGlobalLocale {
public:
    ScopedGlobalLocale() = delete;
    ScopedGlobalLocale(ScopedGlobalLocale const&) = delete;
    ScopedGlobalLocale(ScopedGlobalLocale&&) = delete;
    auto operator=(ScopedGlobalLocale const&) -> ScopedGlobalLocale& = delete;
    auto operator=(ScopedGlobalLocale&&) -> ScopedGlobalLocale& = delete;

    /**
     * @brief Construct a new global locale object
     *
     * @tparam Args Types for `std::locale::global` constructor arguments.
     * @param args Arguments for `std::locale::global` constructor.
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
