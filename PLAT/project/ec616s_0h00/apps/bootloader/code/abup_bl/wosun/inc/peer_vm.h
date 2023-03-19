#ifndef _PEER_VM_H
#define _PEER_VM_H

#define FRAME_MEMORY_BLOCK_SIZE     4096    
#define FRAME_MEMORY_BLOG           12      /* log2(FRAME_MEMORY_BLOCK_SIZE) */
	
#define FRAME_MEMORY_HIGH	          	0x00000001
#define FRAME_MEMORY_DONOT_COMPACTION	0x00000002
#define FRAME_MEMORY_FRAGSPEC_SIZE	(FRAME_ROUNDUP_P2(sizeof(FRAME_MemFragSpec), 4))
	
#define FRAME_MEMORY_MPTRBLK(h)		((FRAME_MasterPtrBlk *)((ML_U32)(h) & ~(FRAME_MEMORY_BLOCK_SIZE - 1)))
#define FRAME_MEMORY_MPTRS_PER_BLK	(((FRAME_MEMORY_BLOCK_SIZE - FRAME_OFFSETOF(FRAME_MasterPtrBlk, fMasterPtrAndTank)) / sizeof(void *)) & ~1)
#define FRAME_MEMORY_DEF_MPTRBLKS	0 // 10

	
#define FRAME_POSITIVE_MOD(x, y)     ((ML_U32)((ML_U32)(x)%(ML_U32)(y)))
#define FRAME_POSITIVE_MOD_P2(x, y)  ((ML_U32)((ML_U32)(x)&(ML_U32)((y)-1)))
	
/* Converts adress to block number. If address is of fragment, it returns block number of containing block */
#define TTank_A2B(tank, a)	(((ML_U8 *)(a) - (ML_U8 *)((tank)->fHeapBase)) >> FRAME_MEMORY_BLOG)
/* Converts block number to address */
#define TTank_B2A(tank, b)	((void *)(((ML_U32)(b) << FRAME_MEMORY_BLOG) + (ML_U8*)((tank)->fHeapBase)))
	
#define FRAME_HOWMANY(x, y)             (((ML_S32)(x)+((ML_S32)(y)-1))/(ML_S32)(y))	
#define FRAME_OFFSETOF(type, field)     ((ML_S32)&(((type *)0)->field))
	
#define FRAME_ROUNDUP_A_P2(x, y)   ((void *)(((ML_U32)(x)+(ML_U32)(y)-1)&~(ML_U32)((y)-1)))
#define FRAME_ROUNDDOWN_A_P2(x, y) ((void *)((ML_U32)(x)&~(ML_U32)((y)-1)))
#define FRAME_ROUNDUP_P2(x, y)     ((ML_U32)(((ML_U32)(x)+(ML_U32)(y)-1)&~(ML_U32)((y)-1)))
		
struct frame_mem_CallbackTable_ {
	ML_U32 fID;
	ML_BOOL (*fProc)(ML_U32 in_arg, ML_U32 in_size);
	ML_U32 fArg;
};
typedef struct frame_mem_CallbackTable_ FRAME_MEM_CallbackTable;

struct frame_mem_TankInfo_ {
    ML_S32 fMaxTotalBytes;
    ML_S32 fMinSubTankBytes;
    ML_BOOL fAutoExtend;
};
typedef struct frame_mem_TankInfo_ FRAME_MEM_TankInfo;

struct frame_MemBlock_AllocBlock_ {
	ML_S32 fType;	
	ML_S32 fSize;
	ML_S32 fLockCount;
	void **fMasterPtr;
};
typedef struct frame_MemBlock_AllocBlock_ FRAME_MemBlock_AllocBlock;

struct frame_MemBlock_AllocFrag_ {
	ML_S32 fType;	
	ML_S32 fFree;
	ML_S32 fNext;
	ML_S32 fToBeSplit;
	ML_S32 fFreeList;
};
typedef struct frame_MemBlock_AllocFrag_ FRAME_MemBlock_AllocFrag;

struct frame_MemBlock_Free_ {
	ML_S32 fType; 
	ML_S32 fCompactHint;
	ML_S32 fSize;
	ML_S32 fNext;
	ML_S32 fPrev;
};
typedef struct frame_MemBlock_Free_ FRAME_MemBlock_Free;

union frame_MemBlock_ {
	FRAME_MemBlock_AllocBlock uAllocBlock;
	FRAME_MemBlock_AllocFrag uAllocFrag;
	FRAME_MemBlock_Free uFree;
};
typedef union frame_MemBlock_ FRAME_MemBlock;

struct frame_MasterPtrBlk_ {
	ML_S32 fNext;
	ML_S32 fFree;
	void *fMasterPtrAndTank[1];
};
typedef struct frame_MasterPtrBlk_ FRAME_MasterPtrBlk;

struct frame_mem_Tank_ {
	struct frame_mem_Tank_ *fPrev;
	struct frame_mem_Tank_ *fNext;

	void *fHeapNonAlignedAddress;
	ML_BOOL fFreeHeapOnDestroy;
	 
	void *fHeapBase;
	void *fHeapBreak;  	/* aligned to FRAME_MEMORY_BLOCK_SIZE */
	ML_S32 fTankAreaBytes;

	FRAME_MemBlock *fHeapMap;

	ML_S32 fFragBlk[FRAME_MEMORY_BLOG]; /* starting block for the fragment of the tank */
	ML_S32 fCurFragBlk[FRAME_MEMORY_BLOG];

	FRAME_MasterPtrBlk *fMPtrBlk;
	FRAME_MasterPtrBlk *fCurMPtrBlk;
	ML_S32 fMPtrIndex;

	ML_S32 fPersistMPtrBound;
	ML_S32 fRemainingBlocks;
	ML_S32 fBlocks;
};
typedef struct frame_mem_Tank_ FRAME_MEM_Tank;

struct frame_MemFragSpec_ {
	ML_S32 fLockCount;
};
typedef struct frame_MemFragSpec_ FRAME_MemFragSpec;

struct frame_MemFragList_ {
	ML_S32 fNext;	/* next free fragment offset in page */
};
typedef struct frame_MemFragList_ FRAME_MemFragList;

/* TTank_free_block */
#define FRAME_MEM_PURGE_NONE 				    0x00 /* must be false */
#define FRAME_MEM_PURGE_INIT_HEAP			    0xE0
#define FRAME_MEM_PURGE_FREE_BLOCK_RECURSE		0xE1
#define FRAME_MEM_PURGE_REALLOC_BLOCK_FRAGMENT	0xE2
#define FRAME_MEM_PURGE_REALLOC_BLOCK_FREE		0xE3
#define FRAME_MEM_PURGE_REALLOC_BLOCK_NEWFRAG	0xE4
#define FRAME_MEM_PURGE_FREE_MPTR			    0xE5
#define FRAME_MEM_PURGE_HANDLE_SPAREALLOC_X	    0xE6
#define FRAME_MEM_PURGE_MEMORY_SPAREALLOC		0xE7
#define FRAME_MEM_PURGE_MEMORY_FREE			    0xE8
#define FRAME_MEM_PURGE_MEM_REALLOC_INTERTANK	0xE9
#define FRAME_MEM_PURGE_MEM_REALLOC_EXTEND		0xEA
#define FRAME_MEM_PURGE_HANDLE_FREE			    0xEB
#define FRAME_MEM_PURGE_VHANDLE_FREE			0xEC
#define FRAME_MEM_PURGE_VHANDLE_REALLOC_INTERTANK	0xED
#define FRAME_MEM_PURGE_VHANDLE_REALLOC_EXTEND		0xEE
#define FRAME_MEM_PURGE_INTERTANKCOMPACT		    0xEF
#define FRAME_MEM_PURGE_DECOMPACT_FOR_MPTRENTRY	    0xF0

#endif // _PEER_VM_H
