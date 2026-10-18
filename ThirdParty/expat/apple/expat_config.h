// Native configuration of the bundled parser; retain the Windows build's XML
// namespace, DTD and context features. No archive XML is converted or rewritten.
#pragma once
#define BYTEORDER 1234
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Unsupported Apple byte order
#endif
#define HAVE_MEMMOVE 1
#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024
