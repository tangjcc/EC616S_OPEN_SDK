/*
 *  X.509 Certificate Signing Request (CSR) parsing
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
 *  The ITU-T X.509 standard defines a certificate format for PKI.
 *
 *  http://www.ietf.org/rfc/rfc5280.txt (Certificates and CRLs)
 *  http://www.ietf.org/rfc/rfc3279.txt (Alg IDs for CRLs)
 *  http://www.ietf.org/rfc/rfc2986.txt (CSRs, aka PKCS#10)
 *
 *  http://www.itu.int/ITU-T/studygroups/com17/languages/X.680-0207.pdf
 *  http://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
 */
#ifdef SOFTSIM_FEATURE_ENABLE

#include "config.h"

#include "EC_x509_csr.h"
#include "EC_oid.h"

#include <string.h>


#include "pem.h"

#include <stdlib.h>
#include <stdio.h>
#define mbedtls_free       free
#define mbedtls_calloc    calloc
#define mbedtls_snprintf   snprintf


#if defined(MBEDTLS_FS_IO) || defined(EFIX64) || defined(EFI32)
#include <stdio.h>
#endif

#include "debug_trace.h"
#include "debug_log.h"


/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 *  Version  ::=  INTEGER  {  v1(0)  }
 */
static int x509_csr_get_version( unsigned char **p,
                             const unsigned char *end,
                             int *ver )
{
    int ret;

    if( ( ret = mbedtls_asn1_get_int( p, end, ver ) ) != 0 )
    {
        if( ret == MBEDTLS_ERR_ASN1_UNEXPECTED_TAG )
        {
            *ver = 0;
            return( 0 );
        }

        return( MBEDTLS_ERR_X509_INVALID_VERSION + ret );
    }

    return( 0 );
}

/*
 * Parse a CSR in DER format
 */
int EC_mbedtls_x509_csr_parse_der( mbedtls_x509_csr *csr,
                        const unsigned char *buf, size_t buflen )
{
    int ret;
    size_t len;
    unsigned char *p, *end;
    mbedtls_x509_buf sig_params;

    memset( &sig_params, 0, sizeof( mbedtls_x509_buf ) );
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_0, P_INFO, 0, "mbedtls_x509_csr_parse_der 1" );
    /*
     * Check for valid input
     */
    if( csr == NULL || buf == NULL || buflen == 0 )
        return( MBEDTLS_ERR_X509_BAD_INPUT_DATA );
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_1, P_INFO, 0, "mbedtls_x509_csr_parse_der 2" );
    EC_mbedtls_x509_csr_init( csr );

    /*
     * first copy the raw DER data
     */
    p = mbedtls_calloc( 1, len = buflen );
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_3, P_INFO, 0, "mbedtls_x509_csr_parse_der 3" );
    if( p == NULL )
        return( MBEDTLS_ERR_X509_ALLOC_FAILED );

    memcpy( p, buf, buflen );

    csr->raw.p = p;
    csr->raw.len = len;
    end = p + len;

    /*
     *  CertificationRequest ::= SEQUENCE {
     *       certificationRequestInfo CertificationRequestInfo,
     *       signatureAlgorithm AlgorithmIdentifier,
     *       signature          BIT STRING
     *  }
     */
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_4, P_INFO, 0, "mbedtls_x509_csr_parse_der 4" );
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_INVALID_FORMAT );
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_5, P_INFO, 0, "mbedtls_x509_csr_parse_der 5" );

    if( len != (size_t) ( end - p ) )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_INVALID_FORMAT +
                MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_6, P_INFO, 0, "mbedtls_x509_csr_parse_der 6" );
    /*
     *  CertificationRequestInfo ::= SEQUENCE {
     */
    csr->cri.p = p;

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_INVALID_FORMAT + ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_7, P_INFO, 0, "mbedtls_x509_csr_parse_der 7" );
    end = p + len;
    csr->cri.len = end - csr->cri.p;

    /*
     *  Version  ::=  INTEGER {  v1(0) }
     */
    if( ( ret = x509_csr_get_version( &p, end, &csr->version ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_8, P_INFO, 0, "mbedtls_x509_csr_parse_der 8" );
    if( csr->version != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_UNKNOWN_VERSION );
    }

    csr->version++;

    /*
     *  subject               Name
     */
    csr->subject_raw.p = p;

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_INVALID_FORMAT + ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_9, P_INFO, 0, "mbedtls_x509_csr_parse_der 9" );

    if( ( ret = mbedtls_x509_get_name( &p, p + len, &csr->subject ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_10, P_INFO, 0, "mbedtls_x509_csr_parse_der 10" );
    csr->subject_raw.len = p - csr->subject_raw.p;

    /*
     *  subjectPKInfo SubjectPublicKeyInfo
     */
    if( ( ret = EC_mbedtls_pk_parse_subpubkey( &p, end, &csr->pk ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_11, P_INFO, 0, "mbedtls_x509_csr_parse_der 11" );
    /*
     *  attributes    [0] Attributes
     *
     *  The list of possible attributes is open-ended, though RFC 2985
     *  (PKCS#9) defines a few in section 5.4. We currently don't support any,
     *  so we just ignore them. This is a safe thing to do as the worst thing
     *  that could happen is that we issue a certificate that does not match
     *  the requester's expectations - this cannot cause a violation of our
     *  signature policies.
     */
    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_INVALID_FORMAT + ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_12, P_INFO, 0, "mbedtls_x509_csr_parse_der 12" );
    p += len;

    end = csr->raw.p + csr->raw.len;

    /*
     *  signatureAlgorithm   AlgorithmIdentifier,
     *  signature            BIT STRING
     */
    if( ( ret = mbedtls_x509_get_alg( &p, end, &csr->sig_oid, &sig_params ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_13, P_INFO, 0, "mbedtls_x509_csr_parse_der 13" );
    if( ( ret = mbedtls_x509_get_sig_alg( &csr->sig_oid, &sig_params,
                                  &csr->sig_md, &csr->sig_pk,
                                  &csr->sig_opts ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_14, P_INFO, 0, "mbedtls_x509_csr_parse_der 14" );
    if( ( ret = mbedtls_x509_get_sig( &p, end, &csr->sig ) ) != 0 )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( ret );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_15, P_INFO, 0, "mbedtls_x509_csr_parse_der 15" );
    if( p != end )
    {
        EC_mbedtls_x509_csr_free( csr );
        return( MBEDTLS_ERR_X509_INVALID_FORMAT +
                MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );
    }
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_der_16, P_INFO, 0, "mbedtls_x509_csr_parse_der 16" );
    return( 0 );
}

/*
 * Parse a CSR, allowing for PEM or raw DER encoding
 */
int EC_mbedtls_x509_csr_parse( mbedtls_x509_csr *csr, const unsigned char *buf, size_t buflen )
{
    int ret;
    size_t use_len;
    mbedtls_pem_context pem;
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_00, P_INFO,0, "EC_mbedtls_x509_csr_parse enter");
    /*
     * Check for valid input
     */
    if( csr == NULL || buf == NULL || buflen == 0 )
        return( MBEDTLS_ERR_X509_BAD_INPUT_DATA );

    mbedtls_pem_init( &pem );

    /* Avoid calling mbedtls_pem_read_buffer() on non-null-terminated string */
    if( buf[buflen - 1] != '\0' )
        ret = MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT;
    else
        ret = mbedtls_pem_read_buffer( &pem,
                               "-----BEGIN CERTIFICATE REQUEST-----",
                               "-----END CERTIFICATE REQUEST-----",
                               buf, NULL, 0, &use_len );

                               
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_0, P_INFO,1, "ret :%d" ,ret);
    if( ret == 0 )
    {
        /*
         * Was PEM encoded, parse the result
         */
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_1, P_INFO,1, "ret :%d" ,ret);
        if( ( ret = EC_mbedtls_x509_csr_parse_der( csr, pem.buf, pem.buflen ) ) != 0 )
            return( ret );

        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509_csr_parse_2, P_INFO,1, "ret :%d" ,ret);

        mbedtls_pem_free( &pem );
        return( 0 );
    }
    else if( ret != MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
    {
        mbedtls_pem_free( &pem );
        return( ret );
    }
    else
    return( EC_mbedtls_x509_csr_parse_der( csr, buf, buflen ) );
}

/*
 * Load a CSR into the structure
 */
#define BEFORE_COLON    14
#define BC              "14"
/*
 * Return an informational string about the CSR.
 */
int EC_mbedtls_x509_csr_info( char *buf, size_t size, const char *prefix,
                   const mbedtls_x509_csr *csr )
{
    int ret;
    size_t n;
    char *p;
    char key_size_str[BEFORE_COLON];

    p = buf;
    n = size;

    ret = mbedtls_snprintf( p, n, "%sCSR version   : %d",
                               prefix, csr->version );
    MBEDTLS_X509_SAFE_SNPRINTF;

    ret = mbedtls_snprintf( p, n, "\n%ssubject name  : ", prefix );
    MBEDTLS_X509_SAFE_SNPRINTF;
    ret = mbedtls_x509_dn_gets( p, n, &csr->subject );
    MBEDTLS_X509_SAFE_SNPRINTF;

    ret = mbedtls_snprintf( p, n, "\n%ssigned using  : ", prefix );
    MBEDTLS_X509_SAFE_SNPRINTF;

    ret = mbedtls_x509_sig_alg_gets( p, n, &csr->sig_oid, csr->sig_pk, csr->sig_md,
                             csr->sig_opts );
    MBEDTLS_X509_SAFE_SNPRINTF;

    if( ( ret = mbedtls_x509_key_size_helper( key_size_str, BEFORE_COLON,
                                      EC_mbedtls_pk_get_name( &csr->pk ) ) ) != 0 )
    {
        return( ret );
    }

    ret = mbedtls_snprintf( p, n, "\n%s%-" BC "s: %d bits\n", prefix, key_size_str,
                          (int) EC_mbedtls_pk_get_bitlen( &csr->pk ) );
    MBEDTLS_X509_SAFE_SNPRINTF;

    return( (int) ( size - n ) );
}

/*
 * Initialize a CSR
 */
void EC_mbedtls_x509_csr_init( mbedtls_x509_csr *csr )
{
    memset( csr, 0, sizeof(mbedtls_x509_csr) );
}

/*
 * Unallocate all CSR data
 */
void EC_mbedtls_x509_csr_free( mbedtls_x509_csr *csr )
{
    mbedtls_x509_name *name_cur;
    mbedtls_x509_name *name_prv;

    if( csr == NULL )
        return;

    EC_mbedtls_pk_free( &csr->pk );

#if defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT)
    mbedtls_free( csr->sig_opts );
#endif

    name_cur = csr->subject.next;
    while( name_cur != NULL )
    {
        name_prv = name_cur;
        name_cur = name_cur->next;
        mbedtls_zeroize( name_prv, sizeof( mbedtls_x509_name ) );
        mbedtls_free( name_prv );
    }

    if( csr->raw.p != NULL )
    {
        mbedtls_zeroize( csr->raw.p, csr->raw.len );
        mbedtls_free( csr->raw.p );
    }

    mbedtls_zeroize( csr, sizeof( mbedtls_x509_csr ) );
}
#endif


