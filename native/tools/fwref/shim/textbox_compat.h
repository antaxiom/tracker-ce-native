#ifndef FWREF_TEXTBOX_COMPAT_H_
#define FWREF_TEXTBOX_COMPAT_H_

// The unmodified firmware textBox.cpp searches a const buffer but stores the
// result in char*. Host libc++ declares the const overload, so force the
// firmware's intended (non-const) result without editing core/.
#include <cstring>

inline char* fwref_strchr_compat(const char* text, int ch)
{
    return const_cast<char*>(std::strchr(text, ch));
}

#ifndef strchr
#define strchr(text, ch) fwref_strchr_compat((text), (ch))
#endif

#endif // FWREF_TEXTBOX_COMPAT_H_
