#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"

#if !defined(_WIN32)
#define _snprintf snprintf
#endif

#define MAX_ARGUMENTS 64
#define MAX_SPECIFIER_LENGTH 64
#define UNBOUNDED_OUTPUT_SIZE 0x100000

struct FormatSpecifier {
    /* 0000 */ const char* start;
    /* 0004 */ uint32_t length;
    /* 0008 */ int32_t argument;
};

union ArgumentValue {
    /* 0000 */ int32_t intValue;
    /* 0000 */ void* pointerValue;
    /* 0000 */ int64_t longLongValue;
    /* 0000 */ double doubleValue;
};

// VERIFY: 291dacd5 manual
// OFFSET: 0x00773CC0
static int32_t ParseFormatSpecifier(const char** format, ArgumentSize* size, int32_t* orderingPtr) {
    const char* curr = *format;
    int32_t currentNumber = 0;
    *size = e_intSized;

    for (;;) {
        char ch = *curr++;

        if (ch >= '0' && ch <= '9') {
            currentNumber = 10 * currentNumber + ch - '0';
            continue;
        }

        if (ch == '$') {
            if (currentNumber <= 0) {
                return -1;
            }

            *orderingPtr = currentNumber - 1;
            continue;
        }

        currentNumber = 0;
        int32_t result;

        switch (ch) {
        case ' ':
        case '#':
        case '+':
        case '-':
        case '.':
            continue;
        case '%':
            *size = e_takesNoSpace;
            result = 0;
            break;
        case 'C':
        case 'D':
        case 'O':
        case 'U':
        case 'X':
        case 'c':
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
            result = 1;
            break;
        case 'E':
        case 'F':
        case 'G':
        case 'e':
        case 'f':
        case 'g':
            *size = e_doubleSized;
            result = 1;
            break;
        case 'I':
            if (curr[0] != '6' || curr[1] != '4') {
                return -1;
            }

            curr += 2;
            *size = e_longLongSized;
            continue;
        case 'P':
        case 'S':
        case 'p':
        case 's':
            *size = e_pointerSized;
            result = 1;
            break;
        case 'l':
            if (*curr != 'l') {
                return -1;
            }

            curr++;
            *size = e_longLongSized;
            continue;
        default:
            return -1;
        }

        if (curr - *format + 1 >= MAX_SPECIFIER_LENGTH) {
            return -1;
        }

        *format = curr;
        return result;
    }
}

// VERIFY: 7c8d9894 manual
// OFFSET: 0x00773E10
static void FixUpLongLongFormatSpecifier(char* specifier) {
    char* ll = strstr(specifier, "ll");

    if (ll) {
        memmove(ll + 3, ll + 2, strlen(ll) - 1);
        memcpy(ll, "I64", 3);
    }
}

// VERIFY: 33f0aefe manual
// OFFSET: 0x00773E60
int32_t vsnoprintf(char* out, size_t outSize, const char* format, va_list argumentList) {
    if (!outSize) {
        return 0;
    }

    char* dest = out;
    char* end = &out[outSize - 1];

    ArgumentSize sizes[MAX_ARGUMENTS + 1] = {};
    ArgumentValue values[MAX_ARGUMENTS] = {};
    FormatSpecifier specifiers[MAX_ARGUMENTS] = {};
    sizes[MAX_ARGUMENTS] = e_takesNoSpace;

    int32_t orderingPtr = 0;
    int32_t numArguments = 0;
    int32_t numSpecifiers = 0;
    FormatSpecifier* spec = specifiers;
    const char* curr = format;
    char ch;

    do {
        ch = *curr++;

        if (ch != '%') {
            continue;
        }

        if (numSpecifiers >= MAX_ARGUMENTS) {
            *dest = 0;
            return dest - out;
        }

        const char* start = curr - 1;
        ArgumentSize size;
        int32_t result = ParseFormatSpecifier(&curr, &size, &orderingPtr);

        if (result == -1) {
            *dest = 0;
            return dest - out;
        }

        spec->start = start;
        spec->length = curr - start;

        if (result > 0) {
            if (sizes[orderingPtr] && sizes[orderingPtr] != size) {
                *dest = 0;
                return dest - out;
            }

            spec->argument = orderingPtr;
            sizes[orderingPtr] = size;
            orderingPtr++;

            if (numArguments < orderingPtr) {
                numArguments = orderingPtr;
            }
        } else {
            spec->argument = MAX_ARGUMENTS;
        }

        numSpecifiers++;
        spec++;
    } while (ch);

    for (int32_t i = 0; i < numArguments; i++) {
        switch (sizes[i]) {
        case e_unspecified:
            *dest = 0;
            return dest - out;
        case e_intSized:
            values[i].intValue = va_arg(argumentList, int32_t);
            break;
        case e_pointerSized:
            values[i].pointerValue = va_arg(argumentList, void*);
            break;
        case e_longLongSized:
            values[i].longLongValue = va_arg(argumentList, int64_t);
            break;
        case e_doubleSized:
            values[i].doubleValue = va_arg(argumentList, double);
            break;
        default:
            values[i].doubleValue = 0.0;
            break;
        }
    }

    spec = specifiers;

    while (dest < end) {
        ch = *format++;

        if (!ch) {
            break;
        }

        if (ch != '%') {
            *dest++ = ch;
            continue;
        }

        char buffer[MAX_SPECIFIER_LENGTH];
        uint32_t length = spec->length;
        memcpy(buffer, spec->start, length);
        buffer[length] = 0;

        char* specifier = strchr(buffer, '$');

        if (specifier) {
            *specifier = '%';
        } else {
            specifier = buffer;
        }

        int32_t argument = spec->argument;
        spec++;

        int32_t avail = static_cast<int32_t>(end - dest);
        int32_t written = 0;

        switch (sizes[argument]) {
        case e_intSized:
            written = _snprintf(dest, avail, specifier, values[argument].intValue);
            break;
        case e_pointerSized:
            written = _snprintf(dest, avail, specifier, values[argument].pointerValue);
            break;
        case e_longLongSized:
            FixUpLongLongFormatSpecifier(specifier);
            written = _snprintf(dest, avail, specifier, values[argument].longLongValue);
            break;
        case e_doubleSized:
            written = _snprintf(dest, avail, specifier, values[argument].doubleValue);
            break;
        case e_takesNoSpace:
            written = _snprintf(dest, avail, specifier);
            break;
        }

        if (written < 0 || written > avail) {
            written = avail;
        }

        dest += written;
        format += length - 1;
    }

    *dest = 0;
    return dest - out;
}

// VERIFY: 04457973 auto
// OFFSET: 0x007741B0
int32_t vsoprintf(char* out, const char* format, va_list argumentList) {
    return vsnoprintf(out, UNBOUNDED_OUTPUT_SIZE, format, argumentList);
}
