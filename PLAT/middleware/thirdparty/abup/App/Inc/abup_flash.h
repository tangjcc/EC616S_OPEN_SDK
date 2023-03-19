#ifndef  ABUP_FLASH_H
#define  ABUP_FLASH_H

#include "commontypedef.h"
#include "abup_typedef.h"

int32_t abup_init_flash(void);
BOOL Adups_get_break(void);
int32_t abup_erase_delata(void);
int32_t abup_erase_backup(void);
int32_t Adups_destroy_break(void);
uint32_t abup_get_block_size(void);
int32_t abup_write_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len);
int32_t abup_read_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len);
uint8_t abup_get_upgrade_result(void);
int32_t Adups_get_delta_id(void);
int32_t Adups_get_udp_block_num(void);
int32_t Adups_set_udp_block_num(uint32_t num);
int32_t abup_write_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len);
int32_t abup_read_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len);
void Adups_clear_update_result(void);
int32_t Adups_erase_block(uint32_t block);
void Adups_set_break(void);
int32_t Adups_set_delta_id(uint32_t id);
uint32_t abup_get_data_base(void);
void abup_check_upgrade_result(void);
adups_int32 Adups_operation_param(adups_uint8 type, adups_uint8 op, adups_uint8 * value);

#endif
