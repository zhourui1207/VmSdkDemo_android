/*
base64encode.h - modified by David Lazar

Originally:
cencode.h - c header for a base64 encoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#ifndef _BASE64_ENCODE_H
#define _BASE64_ENCODE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    step_A, step_B, step_C
} base64_encodestep;

typedef struct {
    base64_encodestep step;
    uint8_t result;
} base64_encodestate;

void base64_encode_init(base64_encodestate *);

char base64_encode_value(uint8_t);

ptrdiff_t base64_encode_update(base64_encodestate *, const uint8_t *, size_t, char *);

ptrdiff_t base64_encode_final(base64_encodestate *, char *);

ptrdiff_t base64_encode(const uint8_t *, size_t, char *);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_ENCODE_H */
