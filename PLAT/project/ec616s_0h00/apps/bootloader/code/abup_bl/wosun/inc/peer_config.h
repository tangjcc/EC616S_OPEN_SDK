#ifndef PEER_CONFIG_H_
#define PEER_CONFIG_H_

#define VM_MEMPOOL_SIZE  (1024*1024*1024)

#define ADUPS_DIFF_TOOLS_VERSION 311

/* CHANGES
2.05    -- Header: remove total memory, add platform id, tools version, datetime.
2.06    -- Change LZMA2 header
2.07    -- No frame size check
2.08    -- CRC table overflow when file length is 64K aligned
2.09    -- Auto patch backward or forward, op mapped
2.10    -- PrePatch default is ON
2.11    -- max_n_pos_var_len == 0 bug fix
2.12    -- addr_thres, 自适应匹配长度, IsPatched_pre_check_oldbin
2.13    -- memory pool size: 1024M to update patchfile
2.20    -- 整理代码,删除bzlib,重命名lzma, PEER_HAVING_LZMA_CHECK in peer.h
2.21    -- revert bzlib
3.01    -- wosun08 差分包格式变化, 兼容bzlib与lzma, remove PEER_HAVING_LZMA_CHECK
3.02    -- fix memory return null
3.03    -- memory reduce during backup, wosun07, PEER_HAVING_LOGOUT
3.04    -- vm 参数修改 update , patch, verify
3.05    -- vm内存限制检测, patch读写校验
3.06    -- checkbackup, lzma_decode_init, pad_trailing_zeros
3.07    -- new_op_var_len==0 error, pad_trailing_data
3.08    -- WRITE_SINCE_FRAME_NO lookup
3.09    -- backup/flush 恢复old crc相同为判据, 增加badcrc:crc32 old algorithm failed, gtest集成
3.10    -- add patch file crc
3.11    -- pad_trailing_zeros last block read content error, appended_patch_filecrc_flag default: off

*/

//#define PEER_HAVING_BACKUP_REGION
//#define PEER_HAVING_FAT_SUPPORT

#define PEER_FLASH_BLOCKSIZE    65536 //4096 // 8192

//#define PEER_HAVING_LOGOUT

#if defined(WIN32) || defined(LINUX)
#define PEER_HAVING_MEM_TEST

// put this definition at 工程属性中：预编译定义
//#define GTEST

#ifdef GTEST

#undef PEER_HAVING_LOGOUT

enum WOSUN_TEST_ENV {
	TEST_NORMAL = 0,
	TEST_CHECKBIN,
	TEST_POWERFAILED_SIMULATE_TESTCASE_patch_readback_error,
	TEST_POWERFAILED_SIMULATE_TESTCASE_backup_before_patch,
	TEST_POWERFAILED_SIMULATE_TESTCASE_pad_trailing_zeros,
	TEST_POWERFAILED_SIMULATE_TESTCASE_pad_trailing_data,
	TEST_POWERFAILED_UPGRADE
};

int peer_gtest_getTotalFrameNumber();
void peer_gtest_setErrorFrameNo(int frameno);
void peer_gtest_setTestEnv(int env);
int peer_gtest_getTestEnv();
#endif

//#define MBIN_PATCHFILE

#endif

#endif // end of PEER_CONFIG_H_



