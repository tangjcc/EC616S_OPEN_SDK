/**
 * \file mbedtls_md.c
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

#include <stdlib.h>
#define mbedtls_calloc    calloc
#define mbedtls_free       free

#include <string.h>

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 * Reminder: update profiles in x509_crt.c when adding a new hash!
 */
static const int supported_digests[] = {

#if defined(MBEDTLS_SHA512_C)
        MBEDTLS_MD_SHA512,
        MBEDTLS_MD_SHA384,
#endif

#if defined(MBEDTLS_SHA256_C)
        MBEDTLS_MD_SHA256,
        MBEDTLS_MD_SHA224,
#endif

#if defined(MBEDTLS_SHA1_C)
        MBEDTLS_MD_SHA1,
#endif

        MBEDTLS_MD_NONE
};

const int *EC_mbedtls_md_list( void )
{
    return( supported_digests );
}

const mbedtls_md_info_t *EC_mbedtls_md_info_from_string( const char *md_name )
{
    if( NULL == md_name )
        return( NULL );

    if( !strcmp( "SHA224", md_name ) )
        return EC_mbedtls_md_info_from_type( MBEDTLS_MD_SHA224 );
    if( !strcmp( "SHA256", md_name ) )
        return EC_mbedtls_md_info_from_type( MBEDTLS_MD_SHA256 );
    if( !strcmp( "SHA384", md_name ) )
        return EC_mbedtls_md_info_from_type( MBEDTLS_MD_SHA384 );
    if( !strcmp( "SHA512", md_name ) )
        return EC_mbedtls_md_info_from_type( MBEDTLS_MD_SHA512 );
    return( NULL );
}

const mbedtls_md_info_t *EC_mbedtls_md_info_from_type( mbedtls_md_type_t md_type )
{
    switch( md_type )
    {
        case MBEDTLS_MD_SHA224:
            return( &EC_mbedtls_sha224_info );
        case MBEDTLS_MD_SHA256:
            return( &EC_mbedtls_sha256_info );
        case MBEDTLS_MD_SHA384:
            return( &EC_mbedtls_sha384_info );
        case MBEDTLS_MD_SHA512:
            return( &EC_mbedtls_sha512_info );
        default:
            return( NULL );
    }
}


const mbedtls_md_info_t *mbedtls_md_info_from_type_hw( mbedtls_md_type_t md_type )
{
    switch( md_type )
    {

        case MBEDTLS_MD_SHA224:
            return( &EC_mbedtls_sha224_info );
        case MBEDTLS_MD_SHA256:
            return( &EC_mbedtls_sha256_info );
        default:
            return( NULL );
    }
}

void EC_mbedtls_md_init( mbedtls_md_context_t *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_md_context_t ) );
}

void EC_mbedtls_md_free( mbedtls_md_context_t *ctx )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return;

    if( ctx->md_ctx != NULL )
        ctx->md_info->ctx_free_func( ctx->md_ctx );

    if( ctx->hmac_ctx != NULL )
    {
        mbedtls_zeroize( ctx->hmac_ctx, 2 * ctx->md_info->block_size );
        mbedtls_free( ctx->hmac_ctx );
    }

    mbedtls_zeroize( ctx, sizeof( mbedtls_md_context_t ) );
}

int EC_mbedtls_md_clone( mbedtls_md_context_t *dst,
                      const mbedtls_md_context_t *src )
{
    if( dst == NULL || dst->md_info == NULL ||
        src == NULL || src->md_info == NULL ||
        dst->md_info != src->md_info )
    {
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    }

    dst->md_info->clone_func( dst->md_ctx, src->md_ctx );

    return( 0 );
}

#if ! defined(MBEDTLS_DEPRECATED_REMOVED)
int EC_mbedtls_md_init_ctx( mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info )
{
    return EC_mbedtls_md_setup( ctx, md_info, 1 );
}
#endif

int EC_mbedtls_md_setup( mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info, int hmac )
{
    if( md_info == NULL || ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    if( ( ctx->md_ctx = md_info->ctx_alloc_func() ) == NULL )
        return( MBEDTLS_ERR_MD_ALLOC_FAILED );

    if( hmac != 0 )
    {
        ctx->hmac_ctx = mbedtls_calloc( 2, md_info->block_size );
        if( ctx->hmac_ctx == NULL )
        {
            md_info->ctx_free_func( ctx->md_ctx );
            return( MBEDTLS_ERR_MD_ALLOC_FAILED );
        }
    }

    ctx->md_info = md_info;

    return( 0 );
}

int EC_mbedtls_md_starts( mbedtls_md_context_t *ctx )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->starts_func( ctx->md_ctx );

    return( 0 );
}

int EC_mbedtls_md_update( mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->update_func( ctx->md_ctx, input, ilen );

    return( 0 );
}

int EC_mbedtls_md_finish( mbedtls_md_context_t *ctx, unsigned char *output )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->finish_func( ctx->md_ctx, output );

    return( 0 );
}

int EC_mbedtls_md( const mbedtls_md_info_t *md_info, const unsigned char *input, size_t ilen,
            unsigned char *output )
{
    if( md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    md_info->digest_func( input, ilen, output );

    return( 0 );
}


int EC_mbedtls_md_hmac_starts( mbedtls_md_context_t *ctx, const unsigned char *key, size_t keylen )
{
    unsigned char sum[MBEDTLS_MD_MAX_SIZE];
    unsigned char *ipad, *opad;
    size_t i;

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    if( keylen > (size_t) ctx->md_info->block_size )
    {
        ctx->md_info->starts_func( ctx->md_ctx );
        ctx->md_info->update_func( ctx->md_ctx, key, keylen );
        ctx->md_info->finish_func( ctx->md_ctx, sum );

        keylen = ctx->md_info->size;
        key = sum;
    }

    ipad = (unsigned char *) ctx->hmac_ctx;
    opad = (unsigned char *) ctx->hmac_ctx + ctx->md_info->block_size;

    memset( ipad, 0x36, ctx->md_info->block_size );
    memset( opad, 0x5C, ctx->md_info->block_size );

    for( i = 0; i < keylen; i++ )
    {
        ipad[i] = (unsigned char)( ipad[i] ^ key[i] );
        opad[i] = (unsigned char)( opad[i] ^ key[i] );
    }

    mbedtls_zeroize( sum, sizeof( sum ) );

    ctx->md_info->starts_func( ctx->md_ctx );
    ctx->md_info->update_func( ctx->md_ctx, ipad, ctx->md_info->block_size );

    return( 0 );
}

int EC_mbedtls_md_hmac_update( mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen )
{
#if defined(MBEDTLS_SHA256_HW_ADD)
    const mbedtls_md_info_t *md_info_hw;
#endif

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );
    
#if defined(MBEDTLS_SHA256_HW_ADD)
    md_info_hw = mbedtls_md_info_from_type_hw(ctx->md_info->type);
    if (md_info_hw != NULL)
    {
        md_info_hw->update_func( ctx->md_ctx, input, ilen );        
    }
    else
    {
#endif

    ctx->md_info->update_func( ctx->md_ctx, input, ilen );

#if defined(MBEDTLS_SHA256_HW_ADD)
    }
#endif

    return( 0 );
}

int EC_mbedtls_md_hmac_finish( mbedtls_md_context_t *ctx, unsigned char *output )
{
    unsigned char tmp[MBEDTLS_MD_MAX_SIZE];
    unsigned char *opad;
#if defined(MBEDTLS_SHA256_HW_ADD)
    const mbedtls_md_info_t *md_info_hw;
#endif

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    opad = (unsigned char *) ctx->hmac_ctx + ctx->md_info->block_size;

#if defined(MBEDTLS_SHA256_HW_ADD)
    md_info_hw = mbedtls_md_info_from_type_hw(ctx->md_info->type);
    if (md_info_hw != NULL)
    {
        md_info_hw->finish_func( ctx->md_ctx, tmp );
        md_info_hw->starts_func( ctx->md_ctx );
        md_info_hw->update_func( ctx->md_ctx, opad, ctx->md_info->block_size );
        md_info_hw->update_func( ctx->md_ctx, tmp, ctx->md_info->size );
        md_info_hw->finish_func( ctx->md_ctx, output );
    }
    else
    {
#endif

    ctx->md_info->finish_func( ctx->md_ctx, tmp );
    ctx->md_info->starts_func( ctx->md_ctx );
    ctx->md_info->update_func( ctx->md_ctx, opad, ctx->md_info->block_size );
    ctx->md_info->update_func( ctx->md_ctx, tmp, ctx->md_info->size );
    ctx->md_info->finish_func( ctx->md_ctx, output );
    
#if defined(MBEDTLS_SHA256_HW_ADD)
    }
#endif

    return( 0 );
}

int EC_mbedtls_md_hmac_reset( mbedtls_md_context_t *ctx )
{
    unsigned char *ipad;
#if defined(MBEDTLS_SHA256_HW_ADD)
    const mbedtls_md_info_t *md_info_hw;
#endif

    if( ctx == NULL || ctx->md_info == NULL || ctx->hmac_ctx == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ipad = (unsigned char *) ctx->hmac_ctx;

#if defined(MBEDTLS_SHA256_HW_ADD)
    md_info_hw = mbedtls_md_info_from_type_hw(ctx->md_info->type);
    if (md_info_hw != NULL)
    {
        md_info_hw->starts_func( ctx->md_ctx );
        md_info_hw->update_func( ctx->md_ctx, ipad, ctx->md_info->block_size );
    }
    else
    {
#endif
    ctx->md_info->starts_func( ctx->md_ctx );
    ctx->md_info->update_func( ctx->md_ctx, ipad, ctx->md_info->block_size );
#if defined(MBEDTLS_SHA256_HW_ADD)
    }
#endif

    return( 0 );
}

int EC_mbedtls_md_hmac( const mbedtls_md_info_t *md_info, const unsigned char *key, size_t keylen,
                const unsigned char *input, size_t ilen,
                unsigned char *output )
{
    mbedtls_md_context_t ctx;
    int ret;

    if( md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    EC_mbedtls_md_init( &ctx );

    if( ( ret = EC_mbedtls_md_setup( &ctx, md_info, 1 ) ) != 0 )
        return( ret );

    EC_mbedtls_md_hmac_starts( &ctx, key, keylen );
    EC_mbedtls_md_hmac_update( &ctx, input, ilen );
    EC_mbedtls_md_hmac_finish( &ctx, output );

    EC_mbedtls_md_free( &ctx );

    return( 0 );
}

int EC_mbedtls_md_process( mbedtls_md_context_t *ctx, const unsigned char *data )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( MBEDTLS_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->process_func( ctx->md_ctx, data );

    return( 0 );
}

unsigned char EC_mbedtls_md_get_size( const mbedtls_md_info_t *md_info )
{
    if( md_info == NULL )
        return( 0 );

    return md_info->size;
}

mbedtls_md_type_t EC_mbedtls_md_get_type( const mbedtls_md_info_t *md_info )
{
    if( md_info == NULL )
        return( MBEDTLS_MD_NONE );

    return md_info->type;
}

const char *EC_mbedtls_md_get_name( const mbedtls_md_info_t *md_info )
{
    if( md_info == NULL )
        return( NULL );

    return md_info->name;
}

#endif
