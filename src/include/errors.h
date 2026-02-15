#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    ERR_SUCCESS,
    ERR_FAIL,
    ERR_INVALID_ARGUMENT,
    ERR_INVALID_ALIGN,
    ERR_OUT_OF_BOUNDS,
    ERR_BIT_CLEAR,
    ERR_UNINITIALIZED,
    ERR_NULL_PTR,
    ERR_NO_RECORD
} flash_error;

#endif