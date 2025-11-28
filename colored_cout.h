#pragma once
#define COLORED_COUT_H
#include <cstdint>

// usage:
// std::cout << clr::red      << clr::on_cyan     << " red "
//           << clr::yellow   << clr::on_blue     << " yellow "
//           << clr::green    << clr::on_magenta  << " green "
//           << clr::cyan     << clr::on_red      << " cyan "
//           << clr::blue     << clr::on_yellow   << " blue "
//           << clr::magenta  << clr::on_green    << " magenta "
//           << clr::white    << clr::on_black    << " white "
//           << clr::gray     << clr::on_darkgray << " gray "
//           << clr::darkgray << clr::on_gray     << " darkgray "
//           << clr::black    << clr::on_white    << " black "
//           << clr::reset                        << " compliant\n";
// std::cout << CLR_RED      CLR_ON_CYAN     " red "
//           << CLR_YELLOW   CLR_ON_BLUE     " yellow "
//           << CLR_GREEN    CLR_ON_MAGENTA  " green "
//           << CLR_CYAN     CLR_ON_RED      " cyan "
//           << CLR_BLUE     CLR_ON_YELLOW   " blue "
//           << CLR_MAGENTA  CLR_ON_GREEN    " magenta "
//           << CLR_WHITE    CLR_ON_BLACK    " white "
//           << CLR_GRAY     CLR_ON_DARKGRAY " gray "
//           << CLR_DARKGRAY CLR_ON_GRAY     " darkgray "
//           << CLR_BLACK    CLR_ON_WHITE    " black "
//           << CLR_RESET                    " compliant\n";

#define CLR_RED             "\033[91m" // 31
#define CLR_YELLOW          "\033[93m" // 33
#define CLR_GREEN           "\033[92m" // 32
#define CLR_CYAN            "\033[96m" // 36
#define CLR_BLUE            "\033[94m" // 34
#define CLR_MAGENTA         "\033[95m" // 35
#define CLR_WHITE           "\033[97m"
#define CLR_GRAY            "\033[37m"
#define CLR_DARKGRAY        "\033[90m"
#define CLR_BLACK           "\033[30m"
#define CLR_ON_RED          "\033[41m"
#define CLR_ON_YELLOW       "\033[103m" // 43
#define CLR_ON_GREEN        "\033[42m" // 102
#define CLR_ON_CYAN         "\033[46m" // 106
#define CLR_ON_BLUE         "\033[44m"
#define CLR_ON_MAGENTA      "\033[45m"
#define CLR_ON_WHITE        "\033[107m"
#define CLR_ON_GRAY         "\033[47m"
#define CLR_ON_DARKGRAY     "\033[100m"
#define CLR_ON_BLACK        "\033[40m"
#define CLR_RESET           "\033[m"

#if defined(_WIN32)
enum class clr : uint8_t {
    __FOREGROUND_BLUE = 0x0001,
    __FOREGROUND_GREEN = 0x0002,
    __FOREGROUND_RED = 0x0004,
    __FOREGROUND_INTENSITY = 0x0008,
    __BACKGROUND_BLUE = 0x0010,
    __BACKGROUND_GREEN = 0x0020,
    __BACKGROUND_RED = 0x0040,
    __BACKGROUND_INTENSITY = 0x0080,

    red = __FOREGROUND_RED | __FOREGROUND_INTENSITY,
    yellow = __FOREGROUND_GREEN | __FOREGROUND_RED | __FOREGROUND_INTENSITY,
    green = __FOREGROUND_GREEN | __FOREGROUND_INTENSITY,
    cyan = __FOREGROUND_BLUE | __FOREGROUND_GREEN | __FOREGROUND_INTENSITY,
    blue = __FOREGROUND_BLUE | __FOREGROUND_INTENSITY,
    magenta = __FOREGROUND_BLUE | __FOREGROUND_RED | __FOREGROUND_INTENSITY,
    white = __FOREGROUND_BLUE | __FOREGROUND_GREEN | __FOREGROUND_RED | __FOREGROUND_INTENSITY,
    gray = __FOREGROUND_BLUE | __FOREGROUND_GREEN | __FOREGROUND_RED,
    darkgray = __FOREGROUND_INTENSITY,
    black = 0x1F,
    on_red = __BACKGROUND_RED,//| __BACKGROUND_INTENSITY
    on_yellow = __BACKGROUND_GREEN | __BACKGROUND_RED | __BACKGROUND_INTENSITY,
    on_green = __BACKGROUND_GREEN,//| __BACKGROUND_INTENSITY,
    on_cyan = __BACKGROUND_BLUE | __BACKGROUND_GREEN,//| __BACKGROUND_INTENSITY,
    on_blue = __BACKGROUND_BLUE,//| __BACKGROUND_INTENSITY
    on_magenta = __BACKGROUND_BLUE | __BACKGROUND_RED,//| __BACKGROUND_INTENSITY
    on_white = __BACKGROUND_BLUE | __BACKGROUND_GREEN | __BACKGROUND_RED | __BACKGROUND_INTENSITY,
    on_gray = __BACKGROUND_BLUE | __BACKGROUND_GREEN | __BACKGROUND_RED,
    on_darkgray = __BACKGROUND_INTENSITY,
    on_black = 0xF1,
    reset = 0xFF,
};

#if defined(_WIN32)
namespace colored_cout_impl {
# ifndef WINBASEAPI
    extern "C" {
        struct CONSOLE_SCREEN_BUFFER_INFO {
            int16_t dwSize_X, dwSize_Y;
            int16_t dwCursorPosition_X, dwCursorPosition_Y;
            uint16_t wAttributes;
            int16_t srWindow_Left, srWindow_Top, srWindow_Right, srWindow_Bottom;
            int16_t dwMaximumWindowSize_X, dwMaximumWindowSize_Y;
        };
        static_assert(sizeof(CONSOLE_SCREEN_BUFFER_INFO) == 22, "");

        //NOTE: Don't include `colored_cout.h` before `Windows.h`!

        extern void* __stdcall GetStdHandle(uint32_t nStdHandle);
        extern int32_t __stdcall GetConsoleScreenBufferInfo(void* hConsoleOutput,
            CONSOLE_SCREEN_BUFFER_INFO* lpConsoleScreenBufferInfo);
        extern int32_t __stdcall SetConsoleTextAttribute(void* hConsoleOutput, uint32_t wAttributes);
    } // extern "C"
# endif // WINADVAPI

    inline uint16_t getConsoleTextAttr() {
#     ifndef WINBASEAPI
        constexpr uint32_t STD_OUTPUT_HANDLE = -11;
#     endif // WINBASEAPI
        CONSOLE_SCREEN_BUFFER_INFO buffer_info;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &buffer_info);
        return buffer_info.wAttributes;
    }
    inline void setConsoleTextAttr(const uint16_t attr) {
#     ifndef WINBASEAPI
        constexpr uint32_t STD_OUTPUT_HANDLE = -11;
#     endif // WINBASEAPI
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
    }
} // namespace colored_cout_impl
#endif

template <typename type>
type& operator<<(type& ostream, const clr color) {
# if defined(_WIN32)
    //static const uint16_t initial_attributes = colored_cout_impl::getConsoleTextAttr();
    static const uint16_t initial_attributes = static_cast<uint16_t>(clr::gray);
    static uint16_t background = initial_attributes & 0x00F0;
    static uint16_t foreground = initial_attributes & 0x000F;
# endif
    if (color == clr::reset) {
#     if defined(_WIN32)
        ostream.flush();
        colored_cout_impl::setConsoleTextAttr(initial_attributes);
        background = initial_attributes & 0x00F0;
        foreground = initial_attributes & 0x000F;
#     elif defined(__unix__) || defined(__APPLE__)
        ostream << "\033[m";
#     endif
    }
    else {
#     if defined(_WIN32)
        const uint16_t colorCode = static_cast<uint16_t>(color);
        if (color == clr::on_black) {
            background = 0;
        }
        else if (color == clr::black) {
            foreground = 0;
        }
        else if (colorCode & 0x00F0) {
            background = colorCode;
        }
        else if (colorCode & 0x000F) {
            foreground = colorCode;
        }
        ostream.flush();
        colored_cout_impl::setConsoleTextAttr(background | foreground);
#     elif defined(__unix__) || defined(__APPLE__)
        ostream << "\033[" << static_cast<uint32_t>(color) << "m";
#     endif
    }
    return ostream;
}

#endif // COLORED_COUT_H