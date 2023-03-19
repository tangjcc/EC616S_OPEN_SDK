/*******************************************************************************
 *
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Olaf Bergmann (TZI) and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v. 1.0 which accompanies this distribution.
 *
 * The Eclipse Public License is available at http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Olaf Bergmann  - initial API and implementation
 *    Hauke Mehrtens - memory optimization, ECC integration
 *
 *******************************************************************************/

/** 
 * @file prng.h
 * @brief Pseudo Random Numbers
 */

#ifndef _DTLS_PRNG_H_
#define _DTLS_PRNG_H_

#include "tinydtls.h"

/** 
 * @defgroup prng Pseudo Random Numbers
 * @{
 */

#ifndef WITH_CONTIKI
#include <stdlib.h>
#include "bsp.h"//EC add
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "rng_ec616.h"
#elif defined CHIP_EC616S
#include "rng_ec616s.h"
#endif

static inline uint32_t randGet()
{
    uint32_t random=0;
    uint32_t a=0;
    uint32_t b=0;
    uint8_t i=0;
	uint8_t Rand[24];
	RngGenRandom(Rand);
    for(i=0; i<24; i++){
        a += Rand[i];
        b ^= Rand[i];
    }
    random = (a<<8) + b;
    ECOMM_TRACE(UNILOG_ONENET, rand_ec616_1, P_SIG, 1, "rand:%d",random);
    return random;
}

/**
 * Fills \p buf with \p len random bytes. This is the default
 * implementation for prng().  You might want to change prng() to use
 * a better PRNG on your specific platform.
 */
static inline int
dtls_prng(unsigned char *buf, size_t len) {
  while (len--)
    //*buf++ = rand() & 0xFF;
    *buf++ = randGet() & 0xFF;
  return 1;
}

static inline void
dtls_prng_init(unsigned short seed) {
    srand(seed);
}
#else /* WITH_CONTIKI */
#include <string.h>
#include "random.h"

#ifdef HAVE_PRNG
static inline int
dtls_prng(unsigned char *buf, size_t len)
{
	return contiki_prng_impl(buf, len);
}
#else
/**
 * Fills \p buf with \p len random bytes. This is the default
 * implementation for prng().  You might want to change prng() to use
 * a better PRNG on your specific platform.
 */
static inline int
dtls_prng(unsigned char *buf, size_t len) {
  unsigned short v = random_rand();
  while (len > sizeof(v)) {
    memcpy(buf, &v, sizeof(v));
    len -= sizeof(v);
    buf += sizeof(v);
    v = random_rand();
  }

  memcpy(buf, &v, len);
  return 1;
}
#endif /* HAVE_PRNG */

static inline void
dtls_prng_init(unsigned short seed) {
  /* random_init() messes with the radio interface of the CC2538 and
   * therefore must not be called after the radio has been
   * initialized. */
#ifndef CONTIKI_TARGET_CC2538DK
	random_init(seed);
#endif
}
#endif /* WITH_CONTIKI */

/** @} */

#endif /* _DTLS_PRNG_H_ */
