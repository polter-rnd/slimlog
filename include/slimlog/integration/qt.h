#pragma once

#include <slimlog/logger.h>

#include <algorithm>

// Qt includes
#include <QDebug>
#include <QIODevice>
#include <QLatin1StringView>
#include <QString>
#include <QUtf8StringView>

namespace SlimLog::Detail {

// Custom QIODevice that writes directly to format context output iterator
template<typename OutputIt, typename Char>
class DirectOutputDevice : public QIODevice {
public:
    DirectOutputDevice(OutputIt* out = nullptr)
        : m_out(out)
    {
        open(QIODevice::WriteOnly);
    }

    void setOut(OutputIt* out)
    {
        m_out = out;
    }

protected:
    qint64 writeData(const char* data, qint64 len) override
    {
        if constexpr (std::same_as<Char, char> || std::same_as<Char, char8_t>) {
            // Direct copy for char and char8_t - QDebug outputs UTF-8
            // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            std::copy_n(reinterpret_cast<const Char*>(data), len, *m_out);
        } else if constexpr (std::same_as<Char, char16_t>) {
            // Efficient UTF-8 to UTF-16 conversion using QStringView
            const auto str_view = QUtf8StringView(data, len);
            for (auto codepoint : str_view) {
                if (codepoint <= 0xFFFF) {
                    *(*m_out)++ = static_cast<char16_t>(codepoint);
                } else {
                    // Encode as UTF-16 surrogate pair
                    const auto surrogate = codepoint - 0x10000;
                    *(*m_out)++ = static_cast<char16_t>(0xD800 + (surrogate >> 10));
                    *(*m_out)++ = static_cast<char16_t>(0xDC00 + (surrogate & 0x3FF));
                }
            }
        } else if constexpr (std::same_as<Char, char32_t>) {
            // Efficient UTF-8 to UTF-32 conversion using QStringView
            const auto str_view = QUtf8StringView(data, len);
            for (auto codepoint : str_view) {
                *(*m_out)++ = static_cast<char32_t>(codepoint);
            }
        }
        return len;
    }

    qint64 readData(char*, qint64) override
    {
        return -1; // Not supported
    }

private:
    OutputIt* m_out;
};

template<typename T, typename Char, bool UseAddress = false>
auto format_qt_type(const T& value, auto out_it)
{
    if constexpr (std::is_convertible_v<T, std::basic_string_view<Char>>) {
        // This works for QString/QStringView -> std::u16string_view,
        //                    QUtf8StringView -> std::string_view
        const auto str_view = static_cast<std::basic_string_view<Char>>(value);
        return std::copy_n(str_view.data(), str_view.size(), out_it);
    } else if constexpr (
        std::is_same_v<Char, char> && std::is_convertible_v<T, std::u8string_view>) {
        // In case if we have char and u8string_view, convert to u8string_view first
        const auto str_view = static_cast<std::u8string_view>(value);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return std::copy_n(reinterpret_cast<const Char*>(str_view.data()), str_view.size(), out_it);
    } else if constexpr (
        std::is_same_v<Char, char8_t> && std::is_convertible_v<T, std::string_view>) {
        // In case if we have char8_t and string_view, convert to string_view first
        const auto str_view = static_cast<std::string_view>(value);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return std::copy_n(reinterpret_cast<const Char*>(str_view.data()), str_view.size(), out_it);
    } else if constexpr (std::is_same_v<T, QLatin1StringView>) {
        // Latin1 string view we can copy to any character type directly
        if constexpr (sizeof(Char) == 1) {
            return std::copy_n(value.data(), value.size(), out_it);
        } else {
            for (unsigned char c : value) {
                *out_it++ = static_cast<Char>(c);
            }
            return out_it;
        }
    } else {
        // Use direct output device to avoid intermediate buffer
        static thread_local DirectOutputDevice<decltype(out_it), Char> device;
        static thread_local QDebug debug = QDebug(&device).nospace().noquote();

        device.setOut(&out_it);

        if constexpr (UseAddress) {
            debug << &value;
        } else {
            debug << value;
        }

        // Force flushing the stream
        device.aboutToClose();

        return out_it;
    }
}
} // namespace SlimLog::Detail

// Namespace selection for formatter specializations
#ifdef SLIMLOG_FMTLIB
#define SLIMLOG_FORMATTER_NAMESPACE fmt
#else
#define SLIMLOG_FORMATTER_NAMESPACE std
#endif

// Helper macro to expand parentheses
#define SLIMLOG_EXPAND(...) __VA_ARGS__

// Helper macro to generate the ConvertString specialization
#define SLIMLOG_CONVERT_STRING(QtType, UseAddress)                                                 \
    class QtType;                                                                                  \
    template<typename Char>                                                                        \
    struct SlimLog::ConvertString<QtType, Char> {                                                  \
        std::basic_string_view<Char> operator()(const QtType& str, auto& buffer) const             \
        {                                                                                          \
            Detail::format_qt_type<QtType, Char, UseAddress>(str, std::back_inserter(buffer));     \
            return {buffer.data(), buffer.size()};                                                 \
        }                                                                                          \
    };

// Helper macro to generate the ConvertString specialization for template types
#define SLIMLOG_CONVERT_STRING_TMPL(TemplateName, TemplateArgs, ParamDecl, UseAddress)             \
    template<SLIMLOG_EXPAND ParamDecl, typename Char>                                              \
    struct SlimLog::ConvertString<TemplateName<SLIMLOG_EXPAND TemplateArgs>, Char> {               \
        std::basic_string_view<Char> operator()(                                                   \
            const TemplateName<SLIMLOG_EXPAND TemplateArgs>& str, auto& buffer) const              \
        {                                                                                          \
            Detail::format_qt_type<TemplateName<SLIMLOG_EXPAND TemplateArgs>, Char, UseAddress>(   \
                str, std::back_inserter(buffer));                                                  \
            return {buffer.data(), buffer.size()};                                                 \
        }                                                                                          \
    };

// Helper macro to generate the formatter template
#define SLIMLOG_QT_FORMATTER(QtType)                                                               \
    SLIMLOG_CONVERT_STRING(QtType, false)                                                          \
    template<typename Char>                                                                        \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<QtType, Char>                                    \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(const QtType& value, FormatContext& ctx) const                                 \
        {                                                                                          \
            return SlimLog::Detail::format_qt_type<QtType, Char>(value, ctx.out());                \
        }                                                                                          \
    };

// Helper macro for pointer-style formatting (takes address of non-pointer types)
#define SLIMLOG_QT_PTR_FORMATTER(QtType)                                                           \
    SLIMLOG_CONVERT_STRING(QtType, true)                                                           \
    template<typename Char>                                                                        \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<QtType, Char>                                    \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(const QtType& value, FormatContext& ctx) const                                 \
        {                                                                                          \
            return SlimLog::Detail::format_qt_type<QtType, Char, true>(value, ctx.out());          \
        }                                                                                          \
    };

// Helper macro to generate the formatter template for template types
#define SLIMLOG_QT_TMPL_FORMATTER(TemplateName, TemplateArgs, ParamDecl)                           \
    SLIMLOG_CONVERT_STRING_TMPL(TemplateName, TemplateArgs, ParamDecl, false)                      \
    template<SLIMLOG_EXPAND ParamDecl, typename Char>                                              \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<TemplateName<SLIMLOG_EXPAND TemplateArgs>, Char> \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(                                                                               \
            const TemplateName<SLIMLOG_EXPAND TemplateArgs>& value, FormatContext& ctx) const      \
        {                                                                                          \
            return SlimLog::Detail::                                                               \
                format_qt_type<TemplateName<SLIMLOG_EXPAND TemplateArgs>, Char>(value, ctx.out()); \
        }                                                                                          \
    };

// ============================================================================
// QtCore Module Types
// ============================================================================

// Object system - use inheritance-aware formatter for QObject hierarchy
SLIMLOG_CONVERT_STRING(QObject, true)
template<typename T, typename Char>
    requires std::derived_from<T, QObject>
struct SLIMLOG_FORMATTER_NAMESPACE::formatter<T, Char>
    : formatter<std::basic_string_view<Char>, Char> {
    template<typename FormatContext>
    auto format(const T& value, FormatContext& ctx) const
    {
        return SlimLog::Detail::format_qt_type<T, Char, true>(value, ctx.out());
    }
};

// String data types
SLIMLOG_QT_FORMATTER(QString)
SLIMLOG_QT_FORMATTER(QStringView)
SLIMLOG_QT_FORMATTER(QLatin1String)
SLIMLOG_QT_FORMATTER(QAnyStringView)
SLIMLOG_QT_FORMATTER(QByteArray)
SLIMLOG_QT_FORMATTER(QByteArrayView)
SLIMLOG_QT_TMPL_FORMATTER(QBasicUtf8StringView, (UseChar8T), (bool UseChar8T))

#ifdef SLIMLOG_FMTLIB
// Disable fmt range formatting for Qt containers to avoid conflicts
template<bool UseChar8T, typename Char>
struct fmt::range_format_kind<QBasicUtf8StringView<UseChar8T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Char>
struct fmt::range_format_kind<QLatin1String, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Char>
struct fmt::range_format_kind<QByteArrayView, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
#endif

// Core data types
SLIMLOG_QT_FORMATTER(QBitArray)
SLIMLOG_QT_FORMATTER(QUrl)
SLIMLOG_QT_FORMATTER(QUuid)
SLIMLOG_QT_FORMATTER(QVersionNumber)
SLIMLOG_QT_FORMATTER(QOperatingSystemVersion)
SLIMLOG_QT_FORMATTER(QTypeRevision)
SLIMLOG_QT_FORMATTER(QKeyCombination)
SLIMLOG_QT_FORMATTER(QMetaType)
SLIMLOG_QT_FORMATTER(QPermission)

// Date/Time types
SLIMLOG_QT_FORMATTER(QDateTime)
SLIMLOG_QT_FORMATTER(QDate)
SLIMLOG_QT_FORMATTER(QTime)
SLIMLOG_QT_FORMATTER(QTimeZone)

// File/System types
SLIMLOG_QT_FORMATTER(QFileInfo)
SLIMLOG_QT_FORMATTER(QDir)
SLIMLOG_QT_FORMATTER(QLocale)
SLIMLOG_QT_FORMATTER(QStorageInfo)

// Geometry types
SLIMLOG_QT_FORMATTER(QRect)
SLIMLOG_QT_FORMATTER(QRectF)
SLIMLOG_QT_FORMATTER(QPoint)
SLIMLOG_QT_FORMATTER(QPointF)
SLIMLOG_QT_FORMATTER(QSize)
SLIMLOG_QT_FORMATTER(QSizeF)
SLIMLOG_QT_FORMATTER(QLine)
SLIMLOG_QT_FORMATTER(QLineF)
SLIMLOG_QT_FORMATTER(QMargins)
SLIMLOG_QT_FORMATTER(QMarginsF)

// Regular expressions
SLIMLOG_QT_FORMATTER(QRegularExpression)
SLIMLOG_QT_FORMATTER(QRegularExpressionMatch)

// Animation
SLIMLOG_QT_FORMATTER(QEasingCurve)

// MIME types
SLIMLOG_QT_FORMATTER(QMimeType)

// Model/View types
SLIMLOG_QT_FORMATTER(QModelIndex)
SLIMLOG_QT_FORMATTER(QPersistentModelIndex)

// Item selection
SLIMLOG_QT_FORMATTER(QItemSelectionRange)

// JSON types
SLIMLOG_QT_FORMATTER(QJsonObject)
SLIMLOG_QT_FORMATTER(QJsonArray)
SLIMLOG_QT_FORMATTER(QJsonValue)
SLIMLOG_QT_FORMATTER(QJsonDocument)

#ifdef SLIMLOG_FMTLIB
// Disable fmt range formatting for Qt JSON containers to avoid conflicts
template<typename Char>
struct fmt::range_format_kind<QJsonObject, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Char>
struct fmt::range_format_kind<QJsonArray, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
#endif

// CBOR types
SLIMLOG_QT_FORMATTER(QCborArray)
SLIMLOG_QT_FORMATTER(QCborMap)
SLIMLOG_QT_FORMATTER(QCborValue)

// Container types (templates)
SLIMLOG_QT_TMPL_FORMATTER(QList, (T), (typename T))
SLIMLOG_QT_TMPL_FORMATTER(QSet, (T), (typename T))
SLIMLOG_QT_TMPL_FORMATTER(QContiguousCache, (T), (typename T))
SLIMLOG_QT_TMPL_FORMATTER(QMap, (Key, T), (typename Key, typename T))
SLIMLOG_QT_TMPL_FORMATTER(QMultiMap, (Key, T), (typename Key, typename T))
SLIMLOG_QT_TMPL_FORMATTER(QHash, (Key, T), (typename Key, typename T))
SLIMLOG_QT_TMPL_FORMATTER(QMultiHash, (Key, T), (typename Key, typename T))
SLIMLOG_QT_TMPL_FORMATTER(QVarLengthArray, (T, Prealloc), (typename T, int Prealloc))

#ifdef SLIMLOG_FMTLIB
// Disable fmt range formatting for Qt containers to avoid conflicts
template<typename T, typename Char>
struct fmt::range_format_kind<QList<T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename T, typename Char>
struct fmt::range_format_kind<QSet<T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename T, typename Char>
struct fmt::range_format_kind<QContiguousCache<T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Key, typename T, typename Char>
struct fmt::range_format_kind<QMap<Key, T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Key, typename T, typename Char>
struct fmt::range_format_kind<QMultiMap<Key, T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Key, typename T, typename Char>
struct fmt::range_format_kind<QHash<Key, T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename Key, typename T, typename Char>
struct fmt::range_format_kind<QMultiHash<Key, T>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
template<typename T, int Prealloc, typename Char>
struct fmt::range_format_kind<QVarLengthArray<T, Prealloc>, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};
#endif

// Qt smart pointers and utility types
SLIMLOG_QT_TMPL_FORMATTER(QSharedPointer, (T), (typename T))
SLIMLOG_QT_TMPL_FORMATTER(QTaggedPointer, (T, Tag), (typename T, typename Tag))
SLIMLOG_QT_TMPL_FORMATTER(QFlags, (T), (typename T))

// ============================================================================
// QtGui Module Types
// ============================================================================

// Color and drawing
SLIMLOG_QT_FORMATTER(QColor)
SLIMLOG_QT_FORMATTER(QFont)
SLIMLOG_QT_FORMATTER(QPen)
SLIMLOG_QT_FORMATTER(QBrush)
SLIMLOG_QT_FORMATTER(QPalette)
SLIMLOG_QT_FORMATTER(QFontVariableAxis)

// Images and graphics
SLIMLOG_QT_FORMATTER(QPixmap)
SLIMLOG_QT_FORMATTER(QImage)
SLIMLOG_QT_FORMATTER(QIcon)
SLIMLOG_QT_FORMATTER(QCursor)

// Shapes and paths
SLIMLOG_QT_FORMATTER(QPolygon)
SLIMLOG_QT_FORMATTER(QPolygonF)
SLIMLOG_QT_FORMATTER(QPainterPath)
SLIMLOG_QT_FORMATTER(QRegion)

// Transformations
SLIMLOG_QT_FORMATTER(QTransform)
SLIMLOG_QT_FORMATTER(QMatrix4x4)

// Vectors and math
SLIMLOG_QT_FORMATTER(QVector2D)
SLIMLOG_QT_FORMATTER(QVector3D)
SLIMLOG_QT_FORMATTER(QVector4D)
SLIMLOG_QT_FORMATTER(QQuaternion)

// Input and events
SLIMLOG_QT_FORMATTER(QKeySequence)
SLIMLOG_QT_FORMATTER(QEventPoint)

// Actions and UI
SLIMLOG_QT_PTR_FORMATTER(QAction)

// Windows and devices
SLIMLOG_QT_PTR_FORMATTER(QWindow)
SLIMLOG_QT_PTR_FORMATTER(QScreen)
SLIMLOG_QT_PTR_FORMATTER(QInputDevice)
SLIMLOG_QT_PTR_FORMATTER(QPointingDevice)

// Events
SLIMLOG_QT_PTR_FORMATTER(QEvent)

// Surface and display
SLIMLOG_QT_FORMATTER(QSurfaceFormat)
SLIMLOG_QT_FORMATTER(QColorSpace)

// Printing support
SLIMLOG_QT_FORMATTER(QPageSize)
SLIMLOG_QT_FORMATTER(QPageLayout)
SLIMLOG_QT_FORMATTER(QPageRanges)

// Text formatting
SLIMLOG_QT_FORMATTER(QTextLength)
SLIMLOG_QT_FORMATTER(QTextFormat)

// Accessibility
SLIMLOG_QT_PTR_FORMATTER(QAccessibleInterface)
SLIMLOG_QT_FORMATTER(QAccessibleEvent)

// Vulkan support
SLIMLOG_QT_FORMATTER(QVulkanLayer)
SLIMLOG_QT_FORMATTER(QVulkanExtension)

// OpenGL support
SLIMLOG_QT_PTR_FORMATTER(QOpenGLContext)
SLIMLOG_QT_PTR_FORMATTER(QOpenGLContextGroup)
SLIMLOG_QT_PTR_FORMATTER(QOpenGLTexture)
SLIMLOG_QT_FORMATTER(QOpenGLVersionProfile)
SLIMLOG_QT_FORMATTER(QOpenGLDebugMessage)

// ============================================================================
// QtWidgets Module Types
// ============================================================================

// Widget policies and styles
SLIMLOG_QT_FORMATTER(QSizePolicy)
SLIMLOG_QT_FORMATTER(QStyleOption)

// Widget pointers (for debugging widget hierarchies)
SLIMLOG_QT_PTR_FORMATTER(QWidget)
SLIMLOG_QT_PTR_FORMATTER(QDockWidget)

// Gesture system
SLIMLOG_QT_PTR_FORMATTER(QGesture)
SLIMLOG_QT_PTR_FORMATTER(QGestureEvent)

// Graphics framework pointers
SLIMLOG_QT_PTR_FORMATTER(QGraphicsItem)
SLIMLOG_QT_PTR_FORMATTER(QGraphicsObject)
SLIMLOG_QT_PTR_FORMATTER(QGraphicsSceneEvent)

// ============================================================================
// QtNetwork Module Types
// ============================================================================

// Network addressing
SLIMLOG_QT_FORMATTER(QHostAddress)
SLIMLOG_QT_FORMATTER(QNetworkAddressEntry)
SLIMLOG_QT_FORMATTER(QNetworkInterface)

// Network requests/responses
SLIMLOG_QT_FORMATTER(QNetworkCookie)
SLIMLOG_QT_FORMATTER(QNetworkProxy)
SLIMLOG_QT_FORMATTER(QNetworkProxyQuery)
SLIMLOG_QT_FORMATTER(QNetworkRequestFactory)
SLIMLOG_QT_FORMATTER(QRestReply)
SLIMLOG_QT_FORMATTER(QHttpHeaders)
SLIMLOG_QT_FORMATTER(QHttpPart)

// SSL/TLS support
SLIMLOG_QT_FORMATTER(QSslCertificate)
SLIMLOG_QT_FORMATTER(QSslCipher)
SLIMLOG_QT_FORMATTER(QSslKey)
SLIMLOG_QT_FORMATTER(QSslError)
SLIMLOG_QT_FORMATTER(QSslDiffieHellmanParameters)
SLIMLOG_QT_FORMATTER(QSslEllipticCurve)

// ============================================================================
// QtDBus Module Types
// ============================================================================

SLIMLOG_QT_FORMATTER(QDBusMessage)
SLIMLOG_QT_FORMATTER(QDBusError)
SLIMLOG_QT_FORMATTER(QDBusObjectPath)

// ============================================================================
// QtSql Module Types
// ============================================================================

SLIMLOG_QT_FORMATTER(QSqlDatabase)
SLIMLOG_QT_FORMATTER(QSqlField)
SLIMLOG_QT_FORMATTER(QSqlError)
SLIMLOG_QT_FORMATTER(QSqlRecord)

// ============================================================================
// QtPrintSupport Module Types
// ============================================================================

SLIMLOG_QT_FORMATTER(QPrinterInfo)
