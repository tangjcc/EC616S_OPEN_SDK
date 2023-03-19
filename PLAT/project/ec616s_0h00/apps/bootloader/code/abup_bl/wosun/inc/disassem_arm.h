#ifndef disassem_arm
#define disassem_arm

#include "patch.h"
#include "peer_type.h"



typedef enum {
  ARM_OFF8,
  ARM_OFF11,
  ARM_OFF24,
  ARM_OFF25,
  ARM_OFF21,
} ARM_RVA;

typedef uint32_t RVA;

extern int g_oldsize;
//using RVA = uint32_t;

RETCODE_FOTA scan_old_addr(ML_PEER_FILE fp, int frame_size);
int disassem_encode(uint32_t offset, uint8_t* buf, int buf_size, int n_op,uint32_t* op, int n_addr,uint32_t* addr);
int decode_assem(uint32_t offset, uint8_t* buf, int buf_size, int n_addr,uint32_t* addr, int* extra, uint32_t* curr_op, int direction) ;
int scan_file_op(int buf_size, ML_PEER_FILE fp, uint32_t* op, int max_size, int n_addr, uint32_t* addr, int frame_size);
int scan_file_addr(int buf_size, ML_PEER_FILE fp, uint32_t* addr, uint8_t* cnt, int max_size, int frame_size);


/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2014 */
// Disassembler state machine opcodes.

#define ARM_FeatureAAPCS (1ULL << 0)
#define ARM_FeatureAClass (1ULL << 1)
#define ARM_FeatureAPCS (1ULL << 2)
#define ARM_FeatureAvoidMOVsShOp (1ULL << 3)
#define ARM_FeatureAvoidPartialCPSR (1ULL << 4)
#define ARM_FeatureCRC (1ULL << 5)
#define ARM_FeatureCrypto (1ULL << 6)
#define ARM_FeatureD16 (1ULL << 7)
#define ARM_FeatureDB (1ULL << 8)
#define ARM_FeatureDSPThumb2 (1ULL << 9)
#define ARM_FeatureFP16 (1ULL << 10)
#define ARM_FeatureFPARMv8 (1ULL << 11)
#define ARM_FeatureHWDiv (1ULL << 12)
#define ARM_FeatureHWDivARM (1ULL << 13)
#define ARM_FeatureHasRAS (1ULL << 14)
#define ARM_FeatureHasSlowFPVMLx (1ULL << 15)
#define ARM_FeatureMClass (1ULL << 16)
#define ARM_FeatureMP (1ULL << 17)
#define ARM_FeatureNEON (1ULL << 18)
#define ARM_FeatureNEONForFP (1ULL << 19)
#define ARM_FeatureNaClTrap (1ULL << 20)
#define ARM_FeatureNoARM (1ULL << 21)
#define ARM_FeaturePerfMon (1ULL << 22)
#define ARM_FeaturePref32BitThumb (1ULL << 23)
#define ARM_FeatureRClass (1ULL << 24)
#define ARM_FeatureSlowFPBrcc (1ULL << 25)
#define ARM_FeatureT2XtPk (1ULL << 26)
#define ARM_FeatureThumb2 (1ULL << 27)
#define ARM_FeatureTrustZone (1ULL << 28)
#define ARM_FeatureVFP2 (1ULL << 29)
#define ARM_FeatureVFP3 (1ULL << 30)
#define ARM_FeatureVFP4 (1ULL << 31)
#define ARM_FeatureVFPOnlySP (1ULL << 32)
#define ARM_FeatureVMLxForwarding (1ULL << 33)
#define ARM_FeatureVirtualization (1ULL << 34)
#define ARM_FeatureZCZeroing (1ULL << 35)
#define ARM_HasV4TOps (1ULL << 36)
#define ARM_HasV5TEOps (1ULL << 37)
#define ARM_HasV5TOps (1ULL << 38)
#define ARM_HasV6MOps (1ULL << 39)
#define ARM_HasV6Ops (1ULL << 40)
#define ARM_HasV6T2Ops (1ULL << 41)
#define ARM_HasV7Ops (1ULL << 42)
#define ARM_HasV8Ops (1ULL << 43)
#define ARM_ModeThumb (1ULL << 44)
#define ARM_ProcA5 (1ULL << 45)
#define ARM_ProcA7 (1ULL << 46)
#define ARM_ProcA8 (1ULL << 47)
#define ARM_ProcA9 (1ULL << 48)
#define ARM_ProcA12 (1ULL << 49)
#define ARM_ProcA15 (1ULL << 50)
#define ARM_ProcA53 (1ULL << 51)
#define ARM_ProcA57 (1ULL << 52)
#define ARM_ProcKrait (1ULL << 53)
#define ARM_ProcR5 (1ULL << 54)
#define ARM_ProcSwift (1ULL << 55)

enum DecoderOps {
    MCD_OPC_ExtractField = 1, // OPC_ExtractField(uint8_t Start, uint8_t Len)
    MCD_OPC_FilterValue,      // OPC_FilterValue(uleb128 Val, uint16_t NumToSkip)
    MCD_OPC_CheckField,       // OPC_CheckField(uint8_t Start, uint8_t Len,
                              //                uleb128 Val, uint16_t NumToSkip)
    MCD_OPC_CheckPredicate,   // OPC_CheckPredicate(uleb128 PIdx, uint16_t NumToSkip)
    MCD_OPC_Decode,           // OPC_Decode(uleb128 Opcode, uleb128 DIdx)
    MCD_OPC_SoftFail,         // OPC_SoftFail(uleb128 PMask, uleb128 NMask)
    MCD_OPC_Fail              // OPC_Fail()
};




#endif
