/******************************************************************************/
/** \file       memPoolService.c
 *******************************************************************************
 *
 *  \brief      This module allows you to obtain/return fixed-size memory
 *              blocks from  a previously created pool of a contiguous memory
 *              area. All memory blocks have the same size and a pool contains
 *              a defined number of memory blocks. More than one memory pool
 *              can exist, so your application can obtain memory blocks of
 *              different size from different pools. However, a specific
 *              memory block must be returned to the memory pool from which
 *              it came from.
 *              This module contains three files:
 *              memPoolService.c --> Implementation file
 *              memPoolService.h --> Declaration file
 *              memPoolServiceConfig.h --> Module configuration file
 *
 *  \author     wht4
 *
 *  \date       26.08.2011
 *
 *  \remark     Last Modification
 *               \li wht4, 26.08.2011, Created
 *               \li wbr1, 02.09.2011, Reviewed
 *
 ******************************************************************************/
/*
 *  functions  global:
 *              eMemCreateMemoryPool
 *              eMemTakeBlock
 *              eMemTakeBlockWithTimeout
 *              eMemTakeBlockFromISR
 *              eMemGiveBlock
 *              eMemGiveBlockFromISR
 *  functions  local:
 *              .
 *
 ******************************************************************************/

//----- Header-Files -----------------------------------------------------------
#include <memPoolService.h>

//----- Macros -----------------------------------------------------------------

//----- Data types -------------------------------------------------------------

//----- Function prototypes ----------------------------------------------------

//----- Data -------------------------------------------------------------------

//----- Implementation ---------------------------------------------------------

/*******************************************************************************
 *  function :    eMemCreateMemoryPool
 ******************************************************************************/
/** \brief        Creates a memory pool. The memory must be a contiguous memory
 *                area. The memory pool contains a defined number of blocks.
 *                All memory blocks have the same size. One block must have at
 *                least memory for a void pointer. This function should not be
 *                called out  of an ISR.
 *
 *  \type         global
 *
 *  \param[in]	  psMemPoolManager     Structure used for managing the
 *                                     memory pool
 *  \param[in]	  pvMemAddress         Pointer to the start of the contiguous
 *                                     memory area
 *  \param[in]	  u32MemBlockSize      Blocksize in number of Bytes
 *  \param[in]	  u32MemNumberOfBlocks Number of blocks in the pool
 *  \param[in]	  pcMemName            Name of the memory pool (NULL if not used)
 *
 *  \return       MEM_NO_ERROR         If the memory pool was created correctly
 *                MEM_INVALID_ADDRESS  If psMemPoolManager or pvMemAddress is
 *                                     not a valid address
 *                MEM_INVALID_NBRBLOCKS
 *                                     If the number of blocks < 2
 *                MEM_INVALID_BLOCK_SIZE
 *                                     If the block size is not valid
 *                MEM_COULDNT_CREATE_SEMAPHORE
 *                                     If counting semaphore couldn't be created
 *
 ******************************************************************************/
enumMemError eMemCreateMemoryPool(MemPoolManager    *psMemPoolManager,
                                  void              *pvMemAddress,
                                  unsigned portLONG  u32MemBlockSize,
                                  unsigned portLONG  u32MemNumberOfBlocks,
                                  const portCHAR    *pcMemName) {

    enumMemError       eReturnValue = MEM_NO_ERROR; /* Return Value           */
    unsigned portCHAR  i;                           /* Loop variable          */
    unsigned portCHAR *pu8MemBlock;           /* Temporary pointer to memory
                                                 block                        */
    void             **ppvMemLink;            /* Temporary link pointer       */
#if (MEM_POOL_NAME == 1)
    portCHAR          *pcMemTempName;         /* Temporary pointer to the
                                                 name                         */
#endif

#if(MEM_ARGUMENT_CHECK == 1)
    /* We need a memory pool manager */
    if (psMemPoolManager == (void *) 0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    /* We need a memory pool */
    if (pvMemAddress == (void *)0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    /* Must be pointer size aligned */
    if (((unsigned portLONG)pvMemAddress & (sizeof(void *) - 1)) != 0) {
        eReturnValue = MEM_INVALID_ALIGNMENT;
    }
    /* What's the sense of a memory pool if we don't have at least two blocks */
    if (u32MemNumberOfBlocks < ((unsigned portLONG) 2)) {
        eReturnValue = MEM_INVALID_NUMBER_OF_BLOCKS;
    }
    /* We need at least memory in each block for a pointer  */
    if (u32MemBlockSize < ((unsigned portLONG) sizeof(void*))) {
        eReturnValue = MEM_INVALID_BLOCK_SIZE;
    }
    /* Block size must be a multiple address size */
    if ((u32MemBlockSize & (sizeof(void *) - 1)) != 0) {
        eReturnValue = MEM_INVALID_BLOCK_SIZE;
    }
#endif

    if (eReturnValue == MEM_NO_ERROR) {

        /* Enter the critical section */
        vPortEnterCritical();

        /* Prepare the two temporary pointer. Let's point them to the first  */
        /* memory block. */
        pu8MemBlock = (unsigned portCHAR *)pvMemAddress;
        ppvMemLink = (void **)pvMemAddress;

        /* Always increment the pointer by one block size. We will link all  */
        /* blocks between each other. */
        for (i = 0; i < (u32MemNumberOfBlocks - 1); i++) {
            pu8MemBlock +=  u32MemBlockSize;
            *ppvMemLink = (void  *)pu8MemBlock;
            ppvMemLink = (void **)pu8MemBlock;
        }
        /* Signal that this is the last block --> point to NULL              */
        *ppvMemLink = (void *)0;

        /* Copy all relevant data to the memory pool manager data structure  */
        psMemPoolManager->pvMemAddress = pvMemAddress;
        psMemPoolManager->pvMemFreeList = pvMemAddress;
        psMemPoolManager->u32MemBlockSize = u32MemBlockSize;
        psMemPoolManager->u32MemNumberOfBlocks = u32MemNumberOfBlocks;
        psMemPoolManager->u32MemNumberOfFreeBlocks = u32MemNumberOfBlocks;
#if (MEM_POOL_NAME == 1)
        if (pcMemName != (void *) 0) {
            pcMemTempName = (char *) (psMemPoolManager->pcMemName);
            for (i = 0; (i < (MEM_POOL_NAME_LENGTH - 1)) && (pcMemName[i] != '\0'); i++)   {
                pcMemTempName[i] = pcMemName[i];
            }
            pcMemTempName[i] = '\0';
        }
#endif
        /* Create a counting semaphore. With a counting semaphore, we can     */
        /* take/give a memory block with an additional timeout specified      */
#if (MEM_USE_COUNTING_SEMAPHORE == 1)
        psMemPoolManager->semaphoreMemoryPool = MEM_SEMAPHORE_CREATE(u32MemNumberOfBlocks,
                                                u32MemNumberOfBlocks);
        /* Signal that the semaphore couldn't be created */
        if (psMemPoolManager->semaphoreMemoryPool == (void *) 0) {
            eReturnValue = MEM_COULDNT_CREATE_SEMAPHORE;
        }
#endif
        /* Exit the critical section */
        vPortExitCritical();
    }
    return (eReturnValue);
}

/*******************************************************************************
 *  function :    eMemTakeBlock
 ******************************************************************************/
/** \brief        Take a memory block of a previously created memory pool. If
 *                there are no free memory blocks, then this call will
 *                return immediately. This function should not be called out
 *                of an ISR
 *
 *  \type         global
 *
 *  \param[in]	  psMemPoolManager     Structure used for managing the
 *                                     memory pool
 *  \param[in]	  ppvMemBlock          Requested memory block
 *
 *  \return       MEM_NO_ERROR         If a memory block has been returned
 *                MEM_INVALID_ADDRESS  If psMemPoolManager or *ppvMemBlock
 *                                     are not valid addresses
 *                MEM_NO_FREE_BLOCKS   No more memory blocks left
 *                MEM_SEM_UNKNOWN_ERROR Unknown error by accessing the semaphore
 *
 ******************************************************************************/
enumMemError eMemTakeBlock(MemPoolManager  *psMemPoolManager,
                           void           **ppvMemBlock) {

    enumMemError      eReturnValue = MEM_NO_ERROR; /* Return Value            */

#if(MEM_ARGUMENT_CHECK == 1)
    /* Memory pool must be previously created */
    if (psMemPoolManager == (void *) 0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    if (ppvMemBlock == (void *) 0) {
        /* Must be a valid address */
        eReturnValue = MEM_INVALID_ADDRESS;
    }
#endif

    if ((eReturnValue == MEM_NO_ERROR)) {

        /* Enter the critical section */
        vPortEnterCritical();

        /* Are there any blocks left? */
        if (psMemPoolManager->u32MemNumberOfFreeBlocks > 0) {
            /* Give free a memory block and adjust pointer to the next block */
            *ppvMemBlock = psMemPoolManager->pvMemFreeList;
            psMemPoolManager->pvMemFreeList = *(void **)(*ppvMemBlock);
            /* One less memory block in the pool */
            psMemPoolManager->u32MemNumberOfFreeBlocks--;

#if (MEM_USE_COUNTING_SEMAPHORE == 1)
            /* Adjust the semaphore counter by taking the semaphore */
            if (MEM_SEMAPHORE_TAKE(psMemPoolManager->semaphoreMemoryPool,
                                   0) != pdTRUE)   {

                eReturnValue = MEM_SEM_UNKNOWN_ERROR;
                *ppvMemBlock = (void *) 0;
            }
#endif
        } else {
            /* There are no more memory blocks left */
            eReturnValue = MEM_NO_FREE_BLOCKS;
            *ppvMemBlock = (void *) 0;
        }

        /* Exit the critical section */
        vPortExitCritical();
    }

    return (eReturnValue);
}

#if (MEM_USE_COUNTING_SEMAPHORE == 1)
/*******************************************************************************
 *  function :    eMemTakeBlockWithTimeout
 ******************************************************************************/
/** \brief        Take a memory block of a previously created memory pool. If
 *                there are no free memory blocks, then this call will
 *                block for maximum the specified timeout [in number of OS-Tick]
 *                or until a block is available. This function should not
 *                be called out of an ISR
 *
 *  \type         global
 *
 *  \param[in]	  psMemPoolManager     Structure used for managing the
 *                                     memory pool
 *  \param[in]	  ppvMemBlock          Requested memory block
 *  \param[in]	  u32Timeout           Maximum blocking time [in number of OS-Tick]
 *
 *  \return       MEM_NO_ERROR         If a memory block has been returned
 *                MEM_INVALID_ADDRESS  If psMemPoolManager or *ppvMemBlock
 *                                     are not valid addresses
 *                MEM_TIMEOUT_ELAPSED  Timeout elapsed and no memory block
 *                                     is free
 *
 ******************************************************************************/
enumMemError eMemTakeBlockWithTimeout(MemPoolManager     *psMemPoolManager,
                                      void              **ppvMemBlock,
                                      unsigned portLONG   u32Timeout) {

    enumMemError      eReturnValue = MEM_NO_ERROR; /* Return Value            */

#if(MEM_ARGUMENT_CHECK == 1)
    /* Memory pool must be previously created */
    if (psMemPoolManager == (void *) 0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    if (ppvMemBlock == (void *) 0) {
        /* Must be a valid address */
        eReturnValue = MEM_INVALID_ADDRESS;
    }
#endif

    if ((eReturnValue == MEM_NO_ERROR)) {
        /* Are there any blocks left? */
        if (MEM_SEMAPHORE_TAKE(psMemPoolManager->semaphoreMemoryPool,
                               u32Timeout) == pdTRUE)   {

            /* Enter the critical section */
       	 	vPortEnterCritical();

            /* Give free a memory block and adjust pointer to the next block */
            *ppvMemBlock = psMemPoolManager->pvMemFreeList;
            psMemPoolManager->pvMemFreeList = *(void **)(*ppvMemBlock);
            /* One less memory block in the pool */
            psMemPoolManager->u32MemNumberOfFreeBlocks--;

            /* Exit the critical section */
        	vPortExitCritical();

        } else {
            /* Defined timeout has elapsed and no memory block is free */
            eReturnValue = MEM_TIMEOUT_ELAPSED;
            *ppvMemBlock = (void *) 0;
        }
    }

    return (eReturnValue);
}
#endif /* (MEM_USE_COUNTING_SEMAPHORE == 1) */

/*******************************************************************************
 *  function :    eMemTakeBlockFromISR
 ******************************************************************************/
/** \brief        Take a memory block of a previously created memory pool. If
 *                there are no free memory blocks, then this call will
 *                return immediately. This function can be used from an ISR.
 *
 *  \type         global
 *
 *  \param[in]	  psMemPoolManager     Structure used for managing the
 *                                     memory pool
 *  \param[in]	  ppvMemBlock          Requested memory block
 *  \param[in]	  pxTaskWoken          Caused a higher Prio task to unblock.
 *                                     This variable is only used if
 *                                     (MEM_USE_COUNTING_SEMAPHORE == 1). You
 *                                     can pass a NULL Pointer if your not
 *                                     interested in this information.
 *
 *  \return       MEM_NO_ERROR         If a memory block has been returned
 *                MEM_INVALID_ADDRESS  If psMemPoolManager or *ppvMemBlock
 *                                     are not valid addresses
 *                MEM_NO_FREE_BLOCKS   No more memory blocks left
 *
 ******************************************************************************/
enumMemError eMemTakeBlockFromISR(MemPoolManager  *psMemPoolManager,
                                  void           **ppvMemBlock,
                                  portBASE_TYPE   *ps32TaskWoken) {

    enumMemError      eReturnValue = MEM_NO_ERROR; /* Return Value            */

#if(MEM_ARGUMENT_CHECK == 1)
    /* Memory pool must be previously created */
    if (psMemPoolManager == (void *) 0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    if (ppvMemBlock == (void *) 0) {
        /* Must be a valid address */
        eReturnValue = MEM_INVALID_ADDRESS;
    }
#endif

    if ((eReturnValue == MEM_NO_ERROR)) {

        /* Enter the critical section */
        vPortEnterCritical();

        /* Are there any blocks left? */
        if (psMemPoolManager->u32MemNumberOfFreeBlocks > 0) {
            /* Give free a memory block and adjust pointer to the next block */
            *ppvMemBlock = psMemPoolManager->pvMemFreeList;
            psMemPoolManager->pvMemFreeList = *(void **)(*ppvMemBlock);
            /* One less memory block in the pool */
            psMemPoolManager->u32MemNumberOfFreeBlocks--;

#if (MEM_USE_COUNTING_SEMAPHORE == 1)
            /* Adjust the semaphore counter by taking the semaphore */
            if (MEM_SEMAPHORE_TAKE_ISR(psMemPoolManager->semaphoreMemoryPool,
                                       ps32TaskWoken)  != pdTRUE)   {

                eReturnValue = MEM_NO_FREE_BLOCKS;
                *ppvMemBlock = (void *) 0;
            }
#else
            /* We don't know if we have woken up a high priority task */
            *ps32TaskWoken = pdFALSE;
#endif
        } else {
            /* There are no more memory blocks left */
            eReturnValue = MEM_NO_FREE_BLOCKS;
            *ppvMemBlock = (void *) 0;
        }

        /* Exit the critical section */
        vPortExitCritical();
    }

    return (eReturnValue);
}

/*******************************************************************************
 *  function :    eMemGiveBlock
 ******************************************************************************/
/** \brief        Give back a memory block of a previously created memory pool.
 *                This function should not be called out of an ISR
 *
 *  \type         global
 *
 *  \param[in]	  psMemPoolManager     Structure used for managing the
 *                                     memory pool
 *  \param[in]	  pvMemBlock           Returned memory block
 *
 *  \return       MEM_NO_ERROR         Successful returned memory block
 *                MEM_INVALID_ADDRESS  psMemPoolManager or pvMemBlock are
 *                                     invalid addresses
 *                MEM_SEM_UNKNOWN_ERROR Unknown error by accessing the semaphore
 *                MEM_POOL_FULL        Memory pool is already full
 *
 ******************************************************************************/
enumMemError eMemGiveBlock(MemPoolManager *psMemPoolManager, void *pvMemBlock) {

    enumMemError      eReturnValue = MEM_NO_ERROR; /* Return Value            */

#if(MEM_ARGUMENT_CHECK == 1)
    /* Memory pool must be previously created */
    if (psMemPoolManager == (void *) 0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    if (pvMemBlock == (void *) 0) {
        /* Must be a valid address */
        eReturnValue = MEM_INVALID_ADDRESS;
    }
#endif

    if ((eReturnValue == MEM_NO_ERROR)) {

        /* Enter the critical section */
        vPortEnterCritical();

        /* Is the memory pool already full. Is this the case, the memory pool */
        /* is corrupted. */
        if ((psMemPoolManager->u32MemNumberOfFreeBlocks) <
                (psMemPoolManager->u32MemNumberOfBlocks)) {

            /* Insert free block into the free list */
            *(void **)pvMemBlock = psMemPoolManager->pvMemFreeList;
            psMemPoolManager->pvMemFreeList = pvMemBlock;
            /* One more memory block in the pool */
            psMemPoolManager->u32MemNumberOfFreeBlocks++;
#if (MEM_USE_COUNTING_SEMAPHORE == 1)
            /* Adjust the semaphore counter by giving back the semaphore */
            if (MEM_SEMAPHORE_GIVE(psMemPoolManager->semaphoreMemoryPool)  != pdTRUE)   {

                eReturnValue = MEM_SEM_UNKNOWN_ERROR;
            }
#endif
        } else {
            /* All blocks are already returned. Pool is corrupt */
            eReturnValue = MEM_POOL_FULL;
        }
        /* Exit the critical section */
        vPortExitCritical();
    }

    return (eReturnValue);
}

/*******************************************************************************
 *  function :    eMemGiveBlockFromISR
 ******************************************************************************/
/** \brief        Give back a memory block of a previously created memory pool.
 *                This function can be used from an ISR.
 *
 *  \type         global
 *
 *  \param[in]	  psMemPoolManager     Structure used for managing the
 *                                     memory pool
 *  \param[in]	  pvMemBlock           Returned memory block
 *  \param[in]	  pxTaskWoken          Caused a higher Prio task to unblock.
 *                                     This variable is only used if
 *                                     (MEM_USE_COUNTING_SEMAPHORE == 1). You
 *                                     can pass a NULL Pointer if your not
 *                                     interested in this information.
 *
 *  \return       MEM_NO_ERROR         Successful returned memory block
 *                MEM_INVALID_ADDRESS  psMemPoolManager or pvMemBlock are
 *                                     invalid addresses
 *                MEM_SEM_UNKNOWN_ERROR Unknown error by accessing the semaphore
 *                MEM_POOL_FULL        Memory pool is already full
 *
 *
 ******************************************************************************/
enumMemError eMemGiveBlockFromISR(MemPoolManager *psMemPoolManager,
                                  void           *pvMemBlock,
                                  portBASE_TYPE  *ps32TaskWoken) {

    enumMemError      eReturnValue = MEM_NO_ERROR; /* Return Value            */

#if(MEM_ARGUMENT_CHECK == 1)
    /* Memory pool must be previously created */
    if (psMemPoolManager == (void *) 0) {
        eReturnValue = MEM_INVALID_ADDRESS;
    }
    if (pvMemBlock == (void *) 0) {
        /* Must be a valid address */
        eReturnValue = MEM_INVALID_ADDRESS;
    }
#endif

    if ((eReturnValue == MEM_NO_ERROR)) {

        /* Enter the critical section */
        vPortEnterCritical();

        /* Is the memory pool already full. Is this the case, the memory pool */
        /* is corrupted. */
        if ((psMemPoolManager->u32MemNumberOfFreeBlocks) <
                (psMemPoolManager->u32MemNumberOfBlocks)) {

            /* Insert free block into the free list */
            *(void **)pvMemBlock = psMemPoolManager->pvMemFreeList;
            psMemPoolManager->pvMemFreeList = pvMemBlock;
            /* One more memory block in the pool */
            psMemPoolManager->u32MemNumberOfFreeBlocks++;
#if (MEM_USE_COUNTING_SEMAPHORE == 1)
            /* Adjust the semaphore counter by giving back the semaphore */
            if (MEM_SEMAPHORE_GIVE_ISR(psMemPoolManager->semaphoreMemoryPool,
                                       ps32TaskWoken)  != pdTRUE)   {

                eReturnValue = MEM_SEM_UNKNOWN_ERROR;
            }
#else
            /* We don't know if we have woken up a high priority task */
            *ps32TaskWoken = pdFALSE;
#endif
        } else {
            /* All blocks are already returned. Pool is corrupt */
            eReturnValue = MEM_POOL_FULL;
        }
        /* Exit the critical section */
        vPortExitCritical();
    }

    return (eReturnValue);
}


