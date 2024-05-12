#pragma once

#include "format.h"
#include "level.h"
#include "location.h"

#include <array>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace PlainCloud::Log {

template<typename Char>
class Pattern {
public:
    using CharType = Char;
    using StringType = typename std::basic_string_view<Char>;

    enum class Flag {
        Level = 'l',
        Topic = 't',
        Message = 'm',
        File = 'F',
        Line = 'L',
        Function = 'f'
    };

    struct Levels {
        StringType trace{DefaultTrace.data(), DefaultTrace.size()};
        StringType debug{DefaultDebug.data(), DefaultDebug.size()};
        StringType info{DefaultInfo.data(), DefaultInfo.size()};
        StringType warning{DefaultWarning.data(), DefaultWarning.size()};
        StringType error{DefaultError.data(), DefaultError.size()};
        StringType fatal{DefaultFatal.data(), DefaultFatal.size()};

    private:
        static constexpr std::array<Char, 5> DefaultTrace{'T', 'R', 'A', 'C', 'E'};
        static constexpr std::array<Char, 5> DefaultDebug{'D', 'E', 'B', 'U', 'G'};
        static constexpr std::array<Char, 4> DefaultInfo{'I', 'N', 'F', 'O'};
        static constexpr std::array<Char, 4> DefaultWarning{'W', 'A', 'R', 'N'};
        static constexpr std::array<Char, 5> DefaultError{'E', 'R', 'R', 'O', 'R'};
        static constexpr std::array<Char, 5> DefaultFatal{'F', 'A', 'T', 'A', 'L'};
    };

    template<typename... Args>
    explicit Pattern(const StringType& pattern = {}, Args&&... args)
        : m_pattern(pattern)
    {
        set_levels({std::forward<Args>(args)...});
    }

    [[nodiscard]] auto empty() const -> bool
    {
        return m_pattern.empty();
    }

    template<typename String>
    auto format(
        FormatBuffer<Char>& result,
        const Level level,
        const String& category,
        const Location& caller) const -> void
    {
        if (!m_pattern.empty()) {
            StringType pattern{m_pattern};

            const auto result_pos = result.tellp();
            result.seekg(result_pos);

            auto append = [&pattern, &result](auto pos, auto data) {
                result << pattern.substr(0, pos) << data;
                pattern = pattern.substr(pos + 2);
            };
            auto skip = [&pattern, &result](auto pos, auto flag) {
                result << pattern.substr(0, pos + 1);
                pattern = pattern.substr(pos + (flag == '%' ? 2 : 1));
            };

            for (;;) {
                const auto pos = pattern.find('%');
                const auto len = pattern.size();
                if (pos == pattern.npos || pos == len - 1) {
                    result << pattern;
                    break;
                }

                const auto chr = pattern.at(pos + 1);
                switch (static_cast<Flag>(chr)) {
                case Flag::Level:
                    switch (level) {
                    case Level::Fatal:
                        append(pos, m_levels.fatal);
                        break;
                    case Level::Error:
                        append(pos, m_levels.error);
                        break;
                    case Level::Warning:
                        append(pos, m_levels.warning);
                        break;
                    case Level::Info:
                        append(pos, m_levels.info);
                        break;
                    case Level::Debug:
                        append(pos, m_levels.debug);
                        break;
                    case Level::Trace:
                        append(pos, m_levels.trace);
                        break;
                    }
                    break;
                case Flag::Topic:
                    append(pos, category);
                    break;
                case Flag::Message:
                    result.seekg(0);
                    append(pos, result.view().substr(0, result_pos));
                    result.seekg(result_pos);
                    break;
                    /*case Flag::File:
                    append(pos, caller.file_name());
                    break;*/
                case Flag::Line:
                    append(pos, caller.line());
                    break;
                case Flag::Function:
                    append(pos, caller.function_name());
                    break;
                default:
                    skip(pos, chr);
                    break;
                }
            }
        }
    }

    auto set_pattern(StringType pattern)
    {
        m_pattern = pattern;
    }

    auto set_levels(const std::initializer_list<std::pair<Level, StringType>>& levels)
    {
        for (const auto& level : levels) {
            switch (level.first) {
            case Level::Fatal:
                m_levels.fatal = level.second;
                break;
            case Level::Error:
                m_levels.error = level.second;
                break;
            case Level::Warning:
                m_levels.warning = level.second;
                break;
            case Level::Info:
                m_levels.info = level.second;
                break;
            case Level::Debug:
                m_levels.debug = level.second;
                break;
            case Level::Trace:
                m_levels.trace = level.second;
                break;
            }
        }
    }

private:
    std::basic_string<Char> m_pattern;
    Levels m_levels;
};

} // namespace PlainCloud::Log