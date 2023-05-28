/*
Shy Str -- a single-header C library for advanced string allocation

This library provides facilities for easier creation of dynamically allocated
strings. The included functions utilize the string formatting abilities of
sprintf(), but allocate memory for the string automatically and safely by
calculating the necessary size beforehand, and returning gracefully upon
failure.

AUTHOR: Auul, 2023

USAGE:
        In ONE file where this header will be included, add
                #define SHY_PNM_IMPLEMENTATION
        *before* including the header file


LICENSE:
        This library is in the public domain, no rights reserved. See full
        unlicense text at the end of this file for more detailed information.
*/

#ifndef SHY_STR_H
#define SHY_STR_H

#include <stdarg.h>
#include <stdbool.h>

// Creates a new dynamically allocated string according to the standard
// sprintf() formatting
char *StrCreate(const char *fmt, ...);

// Expands an existing dynamically allocated string, appending the new string to
// it
bool StrAppend(char **dest_p, const char *fmt, ...);

#ifdef SHY_STR_IMPLEMENTATION

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHYSTR_FMTBUF_SIZE 64

enum SHYSTR_FmtFlags {
	SHYSTR_FMTFLAGEMPTY = 0,
	SHYSTR_FMTFLAGMINUS = 1,
	SHYSTR_FMTFLAGPLUS  = 2,
	SHYSTR_FMTFLAGSPACE = 4,
	SHYSTR_FMTFLAGHASH  = 8,
	SHYSTR_FMTFLAGZERO  = 16
};

int SHYSTR_IntSize(int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	switch (length) {
	case -2:
	case -1:
	case 0:
		sprintf(fmtbuf, "%i", va_arg(args, int));
		break;
	case 1:
		sprintf(fmtbuf, "%li", va_arg(args, long int));
		break;
	case 2:
		sprintf(fmtbuf, "%lli", va_arg(args, long long int));
		break;
	case 3:
		sprintf(fmtbuf, "%ji", va_arg(args, intmax_t));
		break;
	case 5:
		sprintf(fmtbuf, "%ti", va_arg(args, ptrdiff_t));
		break;
	}

	return strlen(fmtbuf);
}

int SHYSTR_UIntSize(int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	switch (length) {
	case -2:
	case -1:
	case 0:
		sprintf(fmtbuf, "%u", va_arg(args, unsigned));
		break;
	case 1:
		sprintf(fmtbuf, "%lu", va_arg(args, long unsigned));
		break;
	case 2:
		sprintf(fmtbuf, "%llu", va_arg(args, long long unsigned));
		break;
	case 3:
		sprintf(fmtbuf, "%ju", va_arg(args, uintmax_t));
		break;
	case 4:
		sprintf(fmtbuf, "%tu", va_arg(args, size_t));
		break;
	}

	return strlen(fmtbuf);
}

int SHYSTR_OctalSize(int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	switch (length) {
	case -2:
	case -1:
	case 0:
		sprintf(fmtbuf, "%o", va_arg(args, unsigned));
		break;
	case 1:
		sprintf(fmtbuf, "%lo", va_arg(args, long unsigned));
		break;
	case 2:
		sprintf(fmtbuf, "%llo", va_arg(args, long long unsigned));
		break;
	case 3:
		sprintf(fmtbuf, "%jo", va_arg(args, uintmax_t));
		break;
	case 4:
		sprintf(fmtbuf, "%to", va_arg(args, size_t));
		break;
	}

	return strlen(fmtbuf);
}

int SHYSTR_HexSize(int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	switch (length) {
	case -2:
	case -1:
	case 0:
		sprintf(fmtbuf, "%x", va_arg(args, unsigned));
		break;
	case 1:
		sprintf(fmtbuf, "%lx", va_arg(args, long unsigned));
		break;
	case 2:
		sprintf(fmtbuf, "%llx", va_arg(args, long long unsigned));
		break;
	case 3:
		sprintf(fmtbuf, "%jx", va_arg(args, uintmax_t));
		break;
	case 4:
		sprintf(fmtbuf, "%tx", va_arg(args, size_t));
		break;
	}

	return strlen(fmtbuf);
}

int SHYSTR_FloatSize(unsigned flags, int precision, int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	double      f;
	long double lf;

	if (length == 6) {
		lf = va_arg(args, long double);
	} else {
		f  = va_arg(args, double);
		lf = f;
	}

	if (precision < 0) {
		if (flags & SHYSTR_FMTFLAGHASH) {
			sprintf(fmtbuf, "%#LF", lf);
		} else {
			sprintf(fmtbuf, "%Lf", lf);
		}

		return strlen(fmtbuf);
	} else if (flags & SHYSTR_FMTFLAGHASH) {
		sprintf(fmtbuf, "%#.0Lf", lf);
	} else {
		sprintf(fmtbuf, "%.0Lf", lf);
	}

	return strlen(fmtbuf) + precision;
}

int SHYSTR_SciSize(unsigned flags, int precision, int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	double      f;
	long double lf;

	if (length == 6) {
		lf = va_arg(args, long double);
	} else {
		f  = va_arg(args, double);
		lf = f;
	}

	if (precision < 0) {
		if (flags & SHYSTR_FMTFLAGHASH) {
			sprintf(fmtbuf, "%#Le", lf);
		} else {
			sprintf(fmtbuf, "%Le", lf);
		}

		return strlen(fmtbuf);
	} else if (flags & SHYSTR_FMTFLAGHASH) {
		sprintf(fmtbuf, "%#.0Le", lf);
	} else {
		sprintf(fmtbuf, "%.0Le", lf);
	}

	return strlen(fmtbuf) + precision;
}

int SHYSTR_ShortestSize(unsigned flags, int precision, int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	double      f;
	long double lf;

	if (length == 6) {
		lf = va_arg(args, long double);
	} else {
		f  = va_arg(args, double);
		lf = f;
	}

	if (precision < 0) {
		if (flags & SHYSTR_FMTFLAGHASH) {
			sprintf(fmtbuf, "%#Lg", lf);
		} else {
			sprintf(fmtbuf, "%Lg", lf);
		}

		return strlen(fmtbuf);
	} else if (flags & SHYSTR_FMTFLAGHASH) {
		sprintf(fmtbuf, "%#.0Lg", lf);
	} else {
		sprintf(fmtbuf, "%.0Lg", lf);
	}

	return strlen(fmtbuf) + precision;
}

int SHYSTR_HexFloatSize(unsigned flags, int precision, int length, va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];

	double      f;
	long double lf;

	if (length == 6) {
		lf = va_arg(args, long double);
	} else {
		f  = va_arg(args, double);
		lf = f;
	}

	if (precision < 0) {
		if (flags & SHYSTR_FMTFLAGHASH) {
			sprintf(fmtbuf, "%#La", lf);
		} else {
			sprintf(fmtbuf, "%La", lf);
		}

		return strlen(fmtbuf);
	} else if (flags & SHYSTR_FMTFLAGHASH) {
		sprintf(fmtbuf, "%#.0La", lf);
	} else {
		sprintf(fmtbuf, "%.0La", lf);
	}

	return strlen(fmtbuf) + precision;
}

int SHYSTR_CharSize(int length, va_list args)
{
	int __attribute__((unused)) c;
	if (length == 1) {
		c = va_arg(args, wchar_t);
		return sizeof(wchar_t);
	} else {
		c = va_arg(args, int);
		return sizeof(char);
	}
}

int SHYSTR_PtrSize(va_list args)
{
	char fmtbuf[SHYSTR_FMTBUF_SIZE];
	sprintf(fmtbuf, "%p", va_arg(args, void *));
	return strlen(fmtbuf);
}

int SHYSTR_NoPrintSize(va_list args)
{
	int __attribute__((unused)) n = va_arg(args, int);
	return 0;
}

size_t SHYSTR_StrSize(const char *fmt, va_list args)
{
	size_t   size = 0;
	unsigned flags;
	int      width, precision, length, chunk;

	for (size_t i = 0; fmt[i]; i++) {
		if (fmt[i] == '%') {
			i++;

			flags = SHYSTR_FMTFLAGEMPTY;
			while (fmt[i] == '-' || fmt[i] == '+' || fmt[i] == ' '
			       || fmt[i] == '#' || fmt[i] == '0') {
				switch (fmt[i]) {
				case '-':
					flags |= SHYSTR_FMTFLAGMINUS;
					break;
				case '+':
					flags |= SHYSTR_FMTFLAGPLUS;
					break;
				case ' ':
					flags |= SHYSTR_FMTFLAGSPACE;
					break;
				case '#':
					flags |= SHYSTR_FMTFLAGHASH;
					break;
				case '0':
					flags |= SHYSTR_FMTFLAGZERO;
					break;
				}
				i++;
			}

			width = -1;
			if (fmt[i] == '*') {
				width = va_arg(args, int);
				i++;
			} else if (fmt[i] >= '0' && fmt[i] <= '9') {
				width = 0;
				while (fmt[i] >= '0' && fmt[i] <= '9') {
					width = (width * 10) + (fmt[i] - '0');
					i++;
				}
			}

			precision = -1;
			if (fmt[i] == '.') {
				i++;
				if (fmt[i] == '*') {
					precision = va_arg(args, int);
					i++;
				} else if (fmt[i] >= '0' && fmt[i] <= '9') {
					precision = 0;
					while (fmt[i] >= '0' && fmt[i] <= '9') {
						precision = (precision * 10)
						            + (fmt[i] - '0');
						i++;
					}
				} else {
					precision = 0;
				}
			}

			length = 0;
			switch (fmt[i]) {
			case 'h':
				if (fmt[i + 1] == 'h') {
					length = -2;
					i += 2;
				} else {
					length = -1;
					i++;
				}
				break;
			case 'l':
				if (fmt[i + 1] == 'l') {
					length = 2;
					i += 2;
				} else {
					length = 1;
					i++;
				}
				break;
			case 'j':
				length = 3;
				i++;
				break;
			case 'z':
				length = 4;
				i++;
				break;
			case 't':
				length = 5;
				i++;
				break;
			case 'L':
				length = 6;
				i++;
				break;
			default:
				break;
			}

			switch (fmt[i]) {
			case 'd':
			case 'i':
				if (length == 4) {
					chunk = SHYSTR_UIntSize(length, args);
				} else {
					chunk = SHYSTR_IntSize(length, args);
				}
				if (precision > chunk) {
					chunk = precision;
				}
				if (flags & SHYSTR_FMTFLAGPLUS
				    || flags & SHYSTR_FMTFLAGSPACE) {
					chunk++;
				}
				break;
			case 'u':
				if (length == 5) {
					chunk = SHYSTR_IntSize(length, args);
				} else {
					chunk = SHYSTR_UIntSize(length, args);
				}
				if (precision > chunk) {
					chunk = precision;
				}
				if (flags & SHYSTR_FMTFLAGPLUS
				    || flags & SHYSTR_FMTFLAGSPACE) {
					chunk++;
				}
				break;
			case 'o':
				chunk = SHYSTR_OctalSize(length, args);
				if (precision > chunk) {
					chunk = precision;
				}
				if (flags & SHYSTR_FMTFLAGHASH) {
					chunk++;
				}
				break;
			case 'x':
			case 'X':
				chunk = SHYSTR_HexSize(length, args);
				if (precision > chunk) {
					chunk = precision;
				}
				if (flags & SHYSTR_FMTFLAGHASH) {
					chunk += 2;
				}
				break;
			case 'f':
			case 'F':
				chunk = SHYSTR_FloatSize(flags,
				                         precision,
				                         length,
				                         args);
				if (flags & SHYSTR_FMTFLAGPLUS
				    || flags & SHYSTR_FMTFLAGSPACE) {
					chunk++;
				}
				break;
			case 'e':
			case 'E':
				chunk = SHYSTR_SciSize(flags,
				                       precision,
				                       length,
				                       args);
				if (flags & SHYSTR_FMTFLAGPLUS
				    || flags & SHYSTR_FMTFLAGSPACE) {
					chunk++;
				}
				break;
			case 'g':
			case 'G':
				chunk = SHYSTR_ShortestSize(flags,
				                            precision,
				                            length,
				                            args);
				if (flags & SHYSTR_FMTFLAGPLUS
				    || flags & SHYSTR_FMTFLAGSPACE) {
					chunk++;
				}
				break;
			case 'a':
			case 'A':
				chunk = SHYSTR_HexFloatSize(flags,
				                            precision,
				                            length,
				                            args);
				if (flags & SHYSTR_FMTFLAGPLUS
				    || flags & SHYSTR_FMTFLAGSPACE) {
					chunk++;
				}
				break;
			case 'c':
				chunk = SHYSTR_CharSize(length, args);
				break;
			case 's':
				chunk = strlen(va_arg(args, char *));
				if (precision < chunk) {
					chunk = precision;
				}
				break;
			case 'p':
				chunk = SHYSTR_PtrSize(args);
				break;
			case 'n':
				chunk = SHYSTR_NoPrintSize(args);
				break;
			case '%':
				chunk = 1;
				break;
			}

			if (width > chunk && fmt[i] != 'n') {
				chunk = width;
			}

			size += chunk;
		} else {
			size++;
		}
	}

	return size;
}

char *SHYSTR_vStrCreate(const char *fmt, va_list args_orig)
{
	va_list args;

	va_copy(args, args_orig);
	size_t size = SHYSTR_StrSize(fmt, args);
	va_end(args);

	char *str = malloc(size + 1);
	if (!str) {
		perror(strerror(errno));
		return NULL;
	}

	va_copy(args, args_orig);
	vsprintf(str, fmt, args);
	va_end(args);

	return str;
}

char *StrCreate(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	char *str = SHYSTR_vStrCreate(fmt, args);
	va_end(args);

	return str;
}

bool SHYSTR_vStrAppend(char **dest_p, const char *fmt, va_list args)
{
	char *dest = *dest_p;
	if (!dest) {
		*dest_p = SHYSTR_vStrCreate(fmt, args);
		if (*dest_p) {
			return true;
		} else {
			return false;
		}
	} else if (!fmt) {
		return true;
	}

	char *src = SHYSTR_vStrCreate(fmt, args);
	if (!src) {
		return false;
	}

	size_t dest_len = strlen(dest);
	size_t src_len  = strlen(src);

	dest = realloc(dest, dest_len + src_len + 1);
	if (!dest) {
		perror(strerror(errno));
		return false;
	}
	*dest_p = dest;

	memcpy(dest + dest_len, src, src_len + 1);

	return true;
}

bool StrAppend(char **dest_p, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bool retval = SHYSTR_vStrAppend(dest_p, fmt, args);
	va_end(args);

	return retval;
}

#undef SHYSTR_IMPLEMENTATION
#endif

#endif

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
*/
