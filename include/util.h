#pragma once

#include <locale>
#include <utility>

namespace PlainCloud::Util {

class ScopedGlobalLocale {
public:
    ScopedGlobalLocale() = delete;
    ScopedGlobalLocale(ScopedGlobalLocale const&) = delete;
    ScopedGlobalLocale(ScopedGlobalLocale&&) = delete;
    auto operator=(ScopedGlobalLocale const&) -> ScopedGlobalLocale& = delete;
    auto operator=(ScopedGlobalLocale&&) -> ScopedGlobalLocale& = delete;

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

} // namespace PlainCloud::Util