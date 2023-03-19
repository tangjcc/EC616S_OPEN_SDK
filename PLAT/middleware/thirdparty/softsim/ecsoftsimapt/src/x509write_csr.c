/*
 *  X.509 Certificate Signing Request writing
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
/*
 * References:
 * - CSRs: PKCS#10 v1.7 aka RFC 2986
 * - attributes: PKCS#9 v2.0 aka RFC 2985
 */
#ifdef SOFTSIM_FEATURE_ENABLE

#include "config.h"

#include "x509_csr.h"
#include "EC_oid.h"
#include "EC_asn1write.h"

#include <string.h>
#include <stdlib.h>

#include "EC_pem.h"

#include "debug_trace.h"
#include "debug_log.h"

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}


void EC_mbedtls_x509write_csr_init( mbedtls_x509write_csr *ctx )
{
    memset( ctx, 0, sizeof(mbedtls_x509write_csr) );
}

void EC_mbedtls_x509write_csr_free( mbedtls_x509write_csr *ctx )
{
    mbedtls_asn1_free_named_data_list( &ctx->subject );
    mbedtls_asn1_free_named_data_list( &ctx->extensions );

    mbedtls_zeroize( ctx, sizeof(mbedtls_x509write_csr) );
}

void EC_mbedtls_x509write_csr_set_md_alg( mbedtls_x509write_csr *ctx, mbedtls_md_type_t md_alg )
{
    ctx->md_alg = md_alg;
}

void EC_mbedtls_x509write_csr_set_key( mbedtls_x509write_csr *ctx, mbedtls_pk_context *key )
{
    ctx->key = key;
}

int EC_mbedtls_x509write_csr_set_subject_name( mbedtls_x509write_csr *ctx,
                                    const char *subject_name )
{
    return mbedtls_x509_string_to_names( &ctx->subject, subject_name );
}

int EC_mbedtls_x509write_csr_set_extension( mbedtls_x509write_csr *ctx,
                                 const char *oid, size_t oid_len,
                                 const unsigned char *val, size_t val_len )
{
    return mbedtls_x509_set_extension( &ctx->extensions, oid, oid_len,
                               0, val, val_len );
}

int EC_mbedtls_x509write_csr_set_key_usage( mbedtls_x509write_csr *ctx, unsigned char key_usage )
{
    unsigned char buf[4];
    unsigned char *c;
    int ret;

    c = buf + 4;

    if( ( ret = EC_mbedtls_asn1_write_bitstring( &c, buf, &key_usage, 7 ) ) != 4 )
        return( ret );

    ret = EC_mbedtls_x509write_csr_set_extension( ctx, MBEDTLS_OID_KEY_USAGE,
                                       MBEDTLS_OID_SIZE( MBEDTLS_OID_KEY_USAGE ),
                                       buf, 4 );
    if( ret != 0 )
        return( ret );

    return( 0 );
}

int EC_mbedtls_x509write_csr_set_ns_cert_type( mbedtls_x509write_csr *ctx,
                                    unsigned char ns_cert_type )
{
    unsigned char buf[4];
    unsigned char *c;
    int ret;

    c = buf + 4;

    if( ( ret = EC_mbedtls_asn1_write_bitstring( &c, buf, &ns_cert_type, 8 ) ) != 4 )
        return( ret );

    ret = EC_mbedtls_x509write_csr_set_extension( ctx, MBEDTLS_OID_NS_CERT_TYPE,
                                       MBEDTLS_OID_SIZE( MBEDTLS_OID_NS_CERT_TYPE ),
                                       buf, 4 );
    if( ret != 0 )
        return( ret );

    return( 0 );
}



#endif

