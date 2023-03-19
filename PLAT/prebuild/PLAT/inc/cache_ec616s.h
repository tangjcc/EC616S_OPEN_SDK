/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    cache_ec616s.h
 * Description:  EC616S cache controller driver header file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

#ifndef _CACHE_EC616S_H
#define _CACHE_EC616S_H
#include "Driver_Common.h"

 /**
   \addtogroup icache_interface_gr
   \{
  */

#ifdef __cplusplus
 extern "C" {
#endif

/**
  \fn    void EnableICache(void)
  \brief Enables Instrution cache
  \param  void
  \return void
 */
void EnableICache(void);
/**
  \fn    void DisableICache(void)
  \brief Disables Instrution cache
  \param  void
  \return void
 */
void DisableICache(void);
/**
  \fn    bool IsICacheEnabled(void)
  \brief Check whether cache is enabled
  \param  void
  \return enabled or not
 */
bool IsICacheEnabled(void);

    /** \}*/

#ifdef __cplusplus
}
#endif

#endif /* _CACHE_EC616S_H */

