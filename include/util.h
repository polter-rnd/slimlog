#pragma once

#include <locale>

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
        : m_old_locale(std::locale::global(
            std::locale(std::forward<Args>(args)...))) // NOLINT(cppcoreguidelines-avoid-c-arrays, \
                                                                 hicpp-avoid-c-arrays, \
                                                                 modernize-avoid-c-arrays)
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