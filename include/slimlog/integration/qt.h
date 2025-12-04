#pragma once

#include <slimlog/logger.h>

#include <algorithm>
#include <QDebug>
#include <QIODevice>
#include <QString>

namespace SlimLog {
template<>
struct ConvertString<QString, char> {
    std::string_view operator()(const QString& str, auto& buffer) const
    {
        buffer.append(str.toUtf8());
        return {buffer.data(), buffer.size()};
    }
};

namespace Detail {
template<typename T>
concept HasToString = requires(const T& value) {
    { value.toString() } -> std::convertible_to<QString>;
};

// Helper to format Qt types using QDebug with optimized buffer handling
// Supports both pointer and non-pointer types
template<typename T, typename Char, typename FormatContext>
    requires std::convertible_to<T, QString>
auto format_qt_type(const T& value, FormatContext& ctx)
{
    const QString& str = value;

    if constexpr (std::same_as<Char, char16_t>) {
        // Zero-copy for char16_t - direct access to QString's UTF-16 data
        const auto* data = reinterpret_cast<const char16_t*>(str.utf16());
        return std::copy_n(data, str.size(), ctx.out());
    } else if constexpr (std::same_as<Char, char>) {
        // Direct UTF-8 conversion
        const QByteArray utf8Data = str.toUtf8();
        return std::copy_n(utf8Data.constData(), utf8Data.size(), ctx.out());
    } else if constexpr (std::same_as<Char, char8_t>) {
        // Direct UTF-8 conversion for char8_t
        const QByteArray utf8Data = str.toUtf8();
        const auto* data = reinterpret_cast<const char8_t*>(utf8Data.constData());
        return std::copy_n(data, utf8Data.size(), ctx.out());
    } else {
        // Fallback for other character types
        const auto converted = ConvertString<QString, Char>()(value);
        return std::copy(converted.begin(), converted.end(), ctx.out());
    }
}

// Custom QIODevice that writes directly to format context output iterator
template<typename OutputIt, typename Char>
class DirectOutputDevice : public QIODevice {
    OutputIt* m_out;

public:
    DirectOutputDevice()
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
        if constexpr (std::same_as<Char, char>) {
            // Direct copy for char - QDebug outputs UTF-8
            std::copy_n(data, len, *m_out);
        } else if constexpr (std::same_as<Char, char8_t>) {
            // Direct copy for char8_t - QDebug outputs UTF-8
            const auto* u8data = reinterpret_cast<const char8_t*>(data);
            std::copy_n(u8data, len, *m_out);
        } else if constexpr (std::same_as<Char, char16_t>) {
            // Convert UTF-8 to UTF-16 in small chunks
            const QString temp = QString::fromUtf8(data, len);
            const auto* u16data = reinterpret_cast<const char16_t*>(temp.utf16());
            std::copy_n(u16data, temp.size(), *m_out);
        } else if constexpr (std::same_as<Char, char32_t>) {
            // Convert UTF-8 to UTF-32 in small chunks
            const QString temp = QString::fromUtf8(data, len);
            const auto u32str = temp.toStdU32String();
            std::copy(u32str.begin(), u32str.end(), *m_out);
        }
        return len;
    }

    qint64 readData(char*, qint64) override
    {
        return -1; // Not supported
    }
};

template<typename T, typename Char, typename FormatContext, bool UseAddress = false>
    requires(!std::convertible_to<T, QString>)
auto format_qt_type(const T& value, FormatContext& ctx)
{
    // Use direct output device to avoid intermediate buffer
    static thread_local DirectOutputDevice<decltype(ctx.out()), Char> device;
    static thread_local QDebug debug = QDebug(&device).nospace().noquote();

    auto out_it = ctx.out();
    device.setOut(&out_it);

    if constexpr (UseAddress) {
        debug << &value;
    } else {
        debug << value;
    }

    // Force flushing the stream
    device.aboutToClose();

    return out_it;
    // Use optimized QString buffer with minimal allocations
    /*QString buffer;
    buffer.reserve(64); // Pre-allocate reasonable size

    QDebug debug(&buffer);
    if constexpr (UseAddress) {
        debug.nospace() << &value;
    } else {
        debug.nospace() << value;
    }
    const QByteArray utf8Data = buffer.toUtf8();
    return std::copy_n(utf8Data.constData(), utf8Data.size(), ctx.out());*/
}
} // namespace Detail
} // namespace SlimLog

// Namespace selection for formatter specializations
#ifdef SLIMLOG_FMTLIB
#define SLIMLOG_FORMATTER_NAMESPACE fmt
#else
#define SLIMLOG_FORMATTER_NAMESPACE std
#endif

// Helper macro to generate the formatter template
#define SLIMLOG_QT_FORMATTER(QtType)                                                               \
    class QtType;                                                                                  \
    template<typename Char>                                                                        \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<QtType, Char>                                    \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(const QtType& value, FormatContext& ctx) const                                 \
        {                                                                                          \
            return SlimLog::Detail::format_qt_type<QtType, Char>(value, ctx);                      \
        }                                                                                          \
    };

// Helper macro for pointer-style formatting (takes address of non-pointer types)
#define SLIMLOG_QT_PTR_FORMATTER(QtType)                                                           \
    class QtType;                                                                                  \
    template<typename Char>                                                                        \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<QtType, Char>                                    \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(const QtType& value, FormatContext& ctx) const                                 \
        {                                                                                          \
            return SlimLog::Detail::format_qt_type<QtType, Char, FormatContext, true>(value, ctx); \
        }                                                                                          \
    };

// Helper macro for inheritance-aware formatting
#define SLIMLOG_QT_DERIVED_FORMATTER(QtType)                                                       \
    class QtType;                                                                                  \
    template<typename T, typename Char>                                                            \
        requires std::derived_from<T, QtType>                                                      \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<T, Char>                                         \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(const T& value, FormatContext& ctx) const                                      \
        {                                                                                          \
            return SlimLog::Detail::format_qt_type<T, Char, FormatContext, true>(value, ctx);      \
        }                                                                                          \
    };

// Helper macro to expand parentheses
#define SLIMLOG_EXPAND(...) __VA_ARGS__

// Universal macro for any template type (handles both type and non-type parameters)
#define SLIMLOG_QT_TMPL_FORMATTER(TemplateName, TemplateArgs, ParamDecl)                           \
    template<SLIMLOG_EXPAND ParamDecl, typename Char>                                              \
    struct SLIMLOG_FORMATTER_NAMESPACE::formatter<TemplateName<SLIMLOG_EXPAND TemplateArgs>, Char> \
        : formatter<std::basic_string_view<Char>, Char> {                                          \
        template<typename FormatContext>                                                           \
        auto format(                                                                               \
            const TemplateName<SLIMLOG_EXPAND TemplateArgs>& value, FormatContext& ctx) const      \
        {                                                                                          \
            return SlimLog::Detail::                                                               \
                format_qt_type<TemplateName<SLIMLOG_EXPAND TemplateArgs>, Char>(value, ctx);       \
        }                                                                                          \
    };

// ============================================================================
// QtCore Module Types
// ============================================================================

// Object system - use inheritance-aware formatter for QObject hierarchy
SLIMLOG_QT_DERIVED_FORMATTER(QObject)

// Core data types
SLIMLOG_QT_FORMATTER(QString)
SLIMLOG_QT_FORMATTER(QStringView)
SLIMLOG_QT_FORMATTER(QByteArray)
SLIMLOG_QT_FORMATTER(QBitArray)
SLIMLOG_QT_FORMATTER(QUrl)
SLIMLOG_QT_FORMATTER(QUuid)
SLIMLOG_QT_FORMATTER(QVersionNumber)
SLIMLOG_QT_FORMATTER(QOperatingSystemVersion)
SLIMLOG_QT_FORMATTER(QTypeRevision)
SLIMLOG_QT_FORMATTER(QKeyCombination)
SLIMLOG_QT_FORMATTER(QMetaType)
SLIMLOG_QT_FORMATTER(QAnyStringView)
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
