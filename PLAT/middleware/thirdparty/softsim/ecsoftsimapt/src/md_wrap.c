/**
 * \file md_wrap.c
 *
 * \brief Generic message digest wrapper for mbed TLS
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#ifdef SOFTSIM_FEATURE_ENABLE

#include "config.h"

#include "EC_md_internal.h"

#include "sha256.h"

#include "sha512.h"
#include <stdlib.h>
#define mbedtls_calloc    calloc
#define mbedtls_free       free


/*
 * Wrappers for generic message digests
 */

static void sha224_starts_wrap( void *ctx )
{
    mbedtls_sha256_starts( (mbedtls_sha256_context *) ctx, 1 );
}

static void sha224_update_wrap( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    mbedtls_sha256_update( (mbedtls_sha256_context *) ctx, input, ilen );
}

static void sha224_finish_wrap( void *ctx, unsigned char *output )
{
    mbedtls_sha256_finish( (mbedtls_sha256_context *) ctx, output );
}

static void sha224_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    mbedtls_sha256( input, ilen, output, 1 );
}

static void *sha224_ctx_alloc( void )
{
    void *ctx = mbedtls_calloc( 1, sizeof( mbedtls_sha256_context ) );

    if( ctx != NULL )
        mbedtls_sha256_init( (mbedtls_sha256_context *) ctx );

    return( ctx );
}

static void sha224_ctx_free( void *ctx )
{
    mbedtls_sha256_free( (mbedtls_sha256_context *) ctx );
    mbedtls_free( ctx );
}

static void sha224_clone_wrap( void *dst, const void *src )
{
    mbedtls_sha256_clone( (mbedtls_sha256_context *) dst,
                    (const mbedtls_sha256_context *) src );
}

static void sha224_process_wrap( void *ctx, const unsigned char *data )
{
    mbedtls_sha256_process( (mbedtls_sha256_context *) ctx, data );
}

const mbedtls_md_info_t EC_mbedtls_sha224_info = {
    MBEDTLS_MD_SHA224,
    "SHA224",
    28,
    64,
    sha224_starts_wrap,
    sha224_update_wrap,
    sha224_finish_wrap,
    sha224_wrap,
    sha224_ctx_alloc,
    sha224_ctx_free,
    sha224_clone_wrap,
    sha224_process_wrap,
};

static void sha256_starts_wrap( void *ctx )
{
    mbedtls_sha256_starts( (mbedtls_sha256_context *) ctx, 0 );
}

static void sha256_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    mbedtls_sha256( input, ilen, output, 0 );
}

const mbedtls_md_info_t EC_mbedtls_sha256_info = {
    MBEDTLS_MD_SHA256,
    "SHA256",
    32,
    64,
    sha256_starts_wrap,
    sha224_update_wrap,
    sha224_finish_wrap,
    sha256_wrap,
    sha224_ctx_alloc,
    sha224_ctx_free,
    sha224_clone_wrap,
    sha224_process_wrap,
};

#if defined(MBEDTLS_SHA256_HW_ADD)

static void sha256_starts_wrap_hw( void *ctx )
{
    mbedtls_sha256_starts_hw( (mbedtls_sha256_context *) ctx, 0 );
}

static void sha256_wrap_hw( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    mbedtls_sha256_hw( input, ilen, output, 0 );
}

static void sha224_update_wrap_hw( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    mbedtls_sha256_update_hw( (mbedtls_sha256_context *) ctx, input, ilen );
}

static void sha224_finish_wrap_hw( void *ctx, unsigned char *output )
{
    mbedtls_sha256_finish_hw( (mbedtls_sha256_context *) ctx, output );
}

static void *sha224_ctx_alloc_hw( void )
{
    void *ctx = mbedtls_calloc( 1, sizeof( mbedtls_sha256_context ) );

    if( ctx != NULL )
        mbedtls_sha256_init_hw( (mbedtls_sha256_context *) ctx );

    return( ctx );
}

static void sha224_ctx_free_hw( void *ctx )
{
    mbedtls_sha256_free_hw( (mbedtls_sha256_context *) ctx );
    mbedtls_free( ctx );
}

static void sha224_process_wrap_hw( void *ctx, const unsigned char *data )
{
    mbedtls_sha256_process_hw( (mbedtls_sha256_context *) ctx, data );
}

const mbedtls_md_info_t EC_mbedtls_sha256_info_hw = {
    MBEDTLS_MD_SHA256,
    "SHA256",
    32,
    64,
    sha256_starts_wrap_hw,
    sha224_update_wrap_hw,
    sha224_finish_wrap_hw,
    sha256_wrap_hw,
    sha224_ctx_alloc_hw,
    sha224_ctx_free_hw,
    sha224_clone_wrap,
    sha224_process_wrap_hw,
};

#endif


static void sha384_starts_wrap( void *ctx )
{
    mbedtls_sha512_starts( (mbedtls_sha512_context *) ctx, 1 );
}

static void sha384_update_wrap( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    mbedtls_sha512_update( (mbedtls_sha512_context *) ctx, input, ilen );
}

static void sha384_finish_wrap( void *ctx, unsigned char *output )
{
    mbedtls_sha512_finish( (mbedtls_sha512_context *) ctx, output );
}

static void sha384_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    mbedtls_sha512( input, ilen, output, 1 );
}

static void *sha384_ctx_alloc( void )
{
    void *ctx = mbedtls_calloc( 1, sizeof( mbedtls_sha512_context ) );

    if( ctx != NULL )
        mbedtls_sha512_init( (mbedtls_sha512_context *) ctx );

    return( ctx );
}

static void sha384_ctx_free( void *ctx )
{
    mbedtls_sha512_free( (mbedtls_sha512_context *) ctx );
    mbedtls_free( ctx );
}

static void sha384_clone_wrap( void *dst, const void *src )
{
    mbedtls_sha512_clone( (mbedtls_sha512_context *) dst,
                    (const mbedtls_sha512_context *) src );
}

static void sha384_process_wrap( void *ctx, const unsigned char *data )
{
    mbedtls_sha512_process( (mbedtls_sha512_context *) ctx, data );
}

const mbedtls_md_info_t EC_mbedtls_sha384_info = {
    MBEDTLS_MD_SHA384,
    "SHA384",
    48,
    128,
    sha384_starts_wrap,
    sha384_update_wrap,
    sha384_finish_wrap,
    sha384_wrap,
    sha384_ctx_alloc,
    sha384_ctx_free,
    sha384_clone_wrap,
    sha384_process_wrap,
};

static void sha512_starts_wrap( void *ctx )
{
    mbedtls_sha512_starts( (mbedtls_sha512_context *) ctx, 0 );
}

static void sha512_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    mbedtls_sha512( input, ilen, output, 0 );
}

const mbedtls_md_info_t EC_mbedtls_sha512_info = {
    MBEDTLS_MD_SHA512,
    "SHA512",
    64,
    128,
    sha512_starts_wrap,
    sha384_update_wrap,
    sha384_finish_wrap,
    sha512_wrap,
    sha384_ctx_alloc,
    sha384_ctx_free,
    sha384_clone_wrap,
    sha384_process_wrap,
};

#endif
