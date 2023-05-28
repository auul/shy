/*
Shy PNM -- a single-header C library for reading PNM (Portable aNy Map) files

AUTHOR: Auul, 2023

USAGE:
        In ONE file where this header will be included, add
                #define SHY_PNM_IMPLEMENTATION
        *before* including the header file

        To load a PNM file, use

        int w, h;
        uint32_t *pix = PnmLoad(filename, &w, &h);

        filename is the path to the PNM file as a string.
        w and h are integers destinations for the width and height of the image
        pix is a pointer to the newly allocated pixel data, in 32-bit RGBA form


LICENSE:
        This library is in the public domain, no rights reserved. See full
        unlicense text at the end of this file for more detailed information.
*/

#ifndef SHY_PNM_H
#define SHY_PNM_H

#include <stdint.h>

uint32_t *PnmLoad(const char *filename, int *w, int *h);

#ifdef SHY_PNM_IMPLEMENTATION

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void FindToken(FILE *f)
{
	int c = fgetc(f);

	while (c != -1) {
		if (c == '#') {
			while (!feof(f) && fgetc(f) != '\n')
				;
		} else if (!isspace(c)) {
			fseek(f, -1, SEEK_CUR);
			return;
		}
		c = fgetc(f);
	}
}

bool TokenTerminator(FILE *f)
{
	int c = fgetc(f);
	if (c == '#') {
		while (!feof(f) && fgetc(f) != '\n')
			;
		return true;
	} else {
		return (c == -1 || isspace(c));
	}
}

void SkipToken(FILE *f)
{
	while (!TokenTerminator(f))
		;
}

bool TokenMatch(FILE *f, const char *str)
{
	// Returns true if the current token in the file matches the given
	// string. If the tokens match, the token will be consumed by the file
	// pointer, but if the tokens do not match, the file pointer will be
	// returned to the beginning of the token.

	size_t bookmark = ftell(f);

	for (int i = 0; str[i]; i++) {
		if (feof(f) || fgetc(f) != str[i]) {
			fseek(f, bookmark, SEEK_SET);
			return false;
		}
	}
	if (TokenTerminator(f)) {
		return true;
	} else {
		fseek(f, bookmark, SEEK_SET);
		return false;
	}
}

int GrabInt(FILE *f)
{
	FindToken(f);

	if (feof(f)) {
		fprintf(stderr,
		        "Error reading pnm file; unexpected end-of-file "
		        "reached while reading integer.\n");
		return -1;
	}

	int c;
	int n = 0;

	for (c = fgetc(f); c != -1 && !isspace(c) && c != '#'; c = fgetc(f)) {
		if (c >= '0' && c <= '9') {
			n = (n * 10) + (c - '0');
		} else {
			fprintf(stderr,
			        "Error reading Pnm file; invalid character "
			        "encountered in integer.\n");
			return -1;
		}
	}

	if (c == '#') {
		while (fgetc(f) != '\n')
			;
	}

	return n;
}

uint32_t *ReadPamHeader(FILE *f, int *w, int *h, int *depth, int *maxval)
{
	for (bool header = true; header;) {
		FindToken(f);
		if (TokenMatch(f, "DEPTH")) {
			*depth = GrabInt(f);
			if (*depth < 0) {
				*w = -1;
				*h = -1;
				return NULL;
			}
		} else if (TokenMatch(f, "MAXVAL")) {
			*maxval = GrabInt(f);
			if (*maxval < 0) {
				*w = -1;
				*h = -1;
				return NULL;
			}
		} else if (TokenMatch(f, "HEIGHT")) {
			*h = GrabInt(f);
			if (*h < 0) {
				*w = -1;
				return NULL;
			}
		} else if (TokenMatch(f, "WIDTH")) {
			*w = GrabInt(f);
			if (*w < 0) {
				*h = -1;
				return NULL;
			}
		} else if (TokenMatch(f, "ENDHDR")) {
			header = false;
		} else {
			// Unknown tokens will be skipped, along with their
			// corresponding value token. Currently, TUPLTYPE tokens
			// default to this, as the tuple type value is
			// unnecessary for loading image files.
			SkipToken(f);
			FindToken(f);
			SkipToken(f);
		}
	}

	if (*depth < 1 || *depth > 4) {
		fprintf(stderr,
		        "Error reading Pnm file; depth must be between 1-4.\n");
		*w = -1;
		*h = -1;
		return NULL;
	}
	if (*maxval < 1 || *maxval > UINT16_MAX) {
		fprintf(
		    stderr,
		    "Error reading Pnm file; maxval must be between 1-%u.\n",
		    UINT16_MAX);
		*w = -1;
		*h = -1;
		return NULL;
	}
	if (*w < 1) {
		fprintf(stderr,
		        "Error reading Pnm file; width must be at least 1.\n");
		*h = -1;
		return NULL;
	}
	if (*h < 1) {
		fprintf(stderr,
		        "Error reading Pnm file; height must be at least 1.\n");
		*w = -1;
		return NULL;
	}

	uint32_t *pix = malloc((*w) * (*h) * sizeof(uint32_t));
	if (!pix) {
		perror(strerror(errno));
		*w = -1;
		*h = -1;
		return NULL;
	}

	return pix;
}

uint32_t *ReadHeader(FILE *f, int *w, int *h, int *maxval)
{
	*w = GrabInt(f);
	if (*w < 1) {
		if (*w == 0) {
			fprintf(stderr,
			        "Error reading Pnm file; width must be at "
			        "least 1.\n");
		}
		*w = -1;
		*h = -1;
		return NULL;
	}

	*h = GrabInt(f);
	if (*h < 1) {
		if (*h == 0) {
			fprintf(stderr,
			        "Error reading Pnm file; height must be at "
			        "least 1.\n");
		}
		*w = -1;
		*h = -1;
		return NULL;
	}

	*maxval = GrabInt(f);
	if (*maxval < 1 || *maxval > UINT16_MAX) {
		if (*maxval > 0) {
			fprintf(stderr,
			        "Error reading Pnm file; maxval must be "
			        "between 1-%u.\n",
			        UINT16_MAX);
		}
		*w = -1;
		*h = -1;
		return NULL;
	}

	uint32_t *pix = malloc((*w) * (*h) * sizeof(uint32_t));
	if (!pix) {
		perror(strerror(errno));
		*w = -1;
		*h = -1;
		return NULL;
	}

	return pix;
}

uint32_t *ReadPbmHeader(FILE *f, int *w, int *h)
{
	*w = GrabInt(f);
	if (*w < 1) {
		if (*w == 0) {
			fprintf(stderr,
			        "Error reading Pnm file; width must be at "
			        "least 1.\n");
		}
		*w = -1;
		*h = -1;
		return NULL;
	}

	*h = GrabInt(f);
	if (*h < 1) {
		if (*h == 0) {
			fprintf(stderr,
			        "Error reading Pnm file; height must be at "
			        "least 1.\n");
		}
		*w = -1;
		*h = -1;
		return NULL;
	}

	uint32_t *pix = malloc((*w) * (*h) * sizeof(uint32_t));
	if (!pix) {
		perror(strerror(errno));
		*w = -1;
		*h = -1;
		return NULL;
	}

	return pix;
}

bool GrabAsciiValue(FILE *f, int maxval, uint32_t *dest)
{
	int n = GrabInt(f);
	if (n < 0) {
		return false;
	} else if (n > maxval) {
		fprintf(stderr,
		        "Error reading Pnm file; pixel value greater than "
		        "maxval encountered.\n");
		return false;
	}

	*dest = ((uint32_t)n * 255) / maxval;
	return true;
}

bool GrabBinValue(FILE *f, int maxval, uint32_t *dest)
{
	if (feof(f)) {
		fprintf(stderr,
		        "Error reading Pnm file; unexpected end-of-file "
		        "reached while reading pixel data.\n");
		return false;
	}
	*dest = fgetc(f);
	if (maxval > UINT8_MAX) {
		if (feof(f)) {
			fprintf(
			    stderr,
			    "Error reading Pnm file; unexpected end-of-file "
			    "reached while reading pixel data.\n");
			return false;
		}
		*dest = (*dest << 8) | fgetc(f);
	}

	if (*dest > (uint32_t)maxval) {
		fprintf(stderr,
		        "Error reading Pnm file; pixel value greater than "
		        "maxval encountered.\n");
		return false;
	}

	*dest = (*dest * 255) / (uint32_t)maxval;
	return true;
}

bool GrayscaleLoad(FILE *    f,
                   uint32_t *pix,
                   int       w,
                   int       h,
                   int       maxval,
                   bool      get_alpha)
{
	int      size = w * h;
	uint32_t gray, alpha;

	if (get_alpha) {
		for (int i = 0; i < size; i++) {
			if (!GrabBinValue(f, maxval, &gray)
			    || !GrabBinValue(f, maxval, &alpha)) {
				return false;
			}
			pix[i]
			    = (gray << 24) | (gray << 16) | (gray << 8) | alpha;
		}
	} else {
		for (int i = 0; i < size; i++) {
			if (!GrabBinValue(f, maxval, &gray)) {
				return false;
			}
			pix[i]
			    = (gray << 24) | (gray << 16) | (gray << 8) | 0xff;
		}
	}

	return true;
}

bool ColorLoad(FILE *f, uint32_t *pix, int w, int h, int maxval, bool get_alpha)
{
	int      size = w * h;
	uint32_t r, g, b, a;

	if (get_alpha) {
		for (int i = 0; i < size; i++) {
			if (!GrabBinValue(f, maxval, &r)
			    || !GrabBinValue(f, maxval, &g)
			    || !GrabBinValue(f, maxval, &b)
			    || !GrabBinValue(f, maxval, &a)) {
				return false;
			}
			pix[i] = (r << 24) | (g << 16) | (b << 8) | a;
		}
	} else {
		for (int i = 0; i < size; i++) {
			if (!GrabBinValue(f, maxval, &r)
			    || !GrabBinValue(f, maxval, &g)
			    || !GrabBinValue(f, maxval, &b)) {
				return false;
			}
			pix[i] = (r << 24) | (g << 16) | (b << 8) | 0xff;
		}
	}

	return true;
}

uint32_t *PamLoad(FILE *f, int *w, int *h)
{
	int       depth, maxval;
	uint32_t *pix = ReadPamHeader(f, w, h, &depth, &maxval);
	if (!pix) {
		return NULL;
	}

	switch (depth) {
	case 1:
		if (!GrayscaleLoad(f, pix, *w, *h, maxval, false)) {
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		}
		break;
	case 2:
		if (!GrayscaleLoad(f, pix, *w, *h, maxval, true)) {
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		}
		break;
	case 3:
		if (!ColorLoad(f, pix, *w, *h, maxval, false)) {
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		}
		break;
	case 4:
		if (!ColorLoad(f, pix, *w, *h, maxval, true)) {
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		}
		break;
	default:
		break;
	}

	return pix;
}

uint32_t *PpmRawLoad(FILE *f, int *w, int *h)
{
	int       maxval;
	uint32_t *pix = ReadHeader(f, w, h, &maxval);
	if (!pix) {
		return NULL;
	}

	if (!ColorLoad(f, pix, *w, *h, maxval, false)) {
		*w = -1;
		*h = -1;
		free(pix);
		return NULL;
	}

	return pix;
}

uint32_t *PgmRawLoad(FILE *f, int *w, int *h)
{
	int       maxval;
	uint32_t *pix = ReadHeader(f, w, h, &maxval);
	if (!pix) {
		return NULL;
	}

	if (!GrayscaleLoad(f, pix, *w, *h, maxval, false)) {
		*w = -1;
		*h = -1;
		free(pix);
		return NULL;
	}

	return pix;
}

uint32_t *PbmRawLoad(FILE *f, int *w, int *h)
{
	uint32_t *pix = ReadPbmHeader(f, w, h);
	if (!pix) {
		return NULL;
	}

	int     size = (*w) * (*h);
	uint8_t byte;

	for (int i = 0; i < size; i++) {
		if (i % 8 == 0) {
			if (feof(f)) {
				fprintf(stderr,
				        "Error reading Pnm file; unexpected "
				        "end-of-file encountered while reading "
				        "pixel data.\n");
				*w = -1;
				*h = -1;
				free(pix);
				return NULL;
			}

			byte = fgetc(f);
		}
		if (byte & (0x01 << (7 - (i % 8)))) {
			pix[i] = 0x000000ff;
		} else {
			pix[i] = 0xffffffff;
		}
	}

	return pix;
}

uint32_t *PpmAsciiLoad(FILE *f, int *w, int *h)
{
	int       maxval;
	uint32_t *pix = ReadHeader(f, w, h, &maxval);
	if (!pix) {
		return NULL;
	}

	int      size = (*w) * (*h);
	uint32_t r, g, b;

	for (int i = 0; i < size; i++) {
		if (!GrabAsciiValue(f, maxval, &r)
		    || !GrabAsciiValue(f, maxval, &g)
		    || !GrabAsciiValue(f, maxval, &b)) {
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		}
		pix[i] = (r << 24) | (g << 16) | (b << 8) | 0xff;
	}

	return pix;
}

uint32_t *PgmAsciiLoad(FILE *f, int *w, int *h)
{
	int       maxval;
	uint32_t *pix = ReadHeader(f, w, h, &maxval);
	if (!pix) {
		return NULL;
	}

	int      size = (*w) * (*h);
	uint32_t gray;

	for (int i = 0; i < size; i++) {
		if (!GrabAsciiValue(f, maxval, &gray)) {
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		}

		pix[i] = (gray << 24) | (gray << 16) | (gray << 8) | 0xff;
	}

	return pix;
}

uint32_t *PbmAsciiLoad(FILE *f, int *w, int *h)
{
	uint32_t *pix = ReadPbmHeader(f, w, h);
	if (!pix) {
		return NULL;
	}

	int size = (*w) * (*h);

	for (int i = 0; i < size;) {
		switch (fgetc(f)) {
		case -1:
			fprintf(
			    stderr,
			    "Error reading Pnm file; unexpected end-of-file "
			    "encountered while reading pixel data.\n");
			*w = -1;
			*h = -1;
			free(pix);
			return NULL;
		case '#':
			while (fgetc(f) != '\n')
				;
			break;
		case '0':
			pix[i] = 0xffffffff;
			i++;
			break;
		case '1':
			pix[i] = 0x000000ff;
			i++;
			break;
		default:
			break;
		}
	}

	return pix;
}

uint32_t *PnmLoad(const char *filename, int *w, int *h)
{
	FILE *f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Error opening file '%s'.\n", filename);
		return NULL;
	}

	if (fgetc(f) != 'P') {
		fprintf(stderr,
		        "File '%s' is not a valid pnm file. Invalid magic "
		        "number encountered.\n",
		        filename);
		fclose(f);
		return NULL;
	}

	uint32_t *pix = NULL;

	switch (fgetc(f)) {
	case '1':
		pix = PbmAsciiLoad(f, w, h);
		break;
	case '2':
		pix = PgmAsciiLoad(f, w, h);
		break;
	case '3':
		pix = PpmAsciiLoad(f, w, h);
		break;
	case '4':
		pix = PbmRawLoad(f, w, h);
		break;
	case '5':
		pix = PgmRawLoad(f, w, h);
		break;
	case '6':
		pix = PpmRawLoad(f, w, h);
		break;
	case '7':
		pix = PamLoad(f, w, h);
		break;
	default:
		fprintf(stderr,
		        "File '%s' is not a valid pnm file. Invalid magic "
		        "number encountered.\n",
		        filename);
		break;
	}

	fclose(f);

	return pix;
}

#undef SHY_PNM_IMPLEMENTATION

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
