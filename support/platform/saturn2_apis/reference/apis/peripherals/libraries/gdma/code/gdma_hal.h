#if !defined (__GDMA_HAL_H__)
#define __GDMA_HAL_H__

#include <MeOS.h>
#include <ioblock_defs.h>
#include <gdma_api.h>

#define	GDMA_HAL__END_OF_LINKED_LIST	0xFFFFFFFF

IMG_RESULT	GDMA_HAL_SetLinkedListDescriptor 
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32						ui32DescriptorIndex,
	IMG_BOOL						bFirstElementOfList,
	GDMA_sTransferObject		*	psTransferObject,
	IMG_UINT32						ui32NextElementIndex
);

IMG_RESULT GDMA_HAL_ReadAndClearInterrupts 
( 
	ioblock_sBlockDescriptor	*	psBlockDesc
);

IMG_RESULT GDMA_HAL_DisableHardware
( 
	ioblock_sBlockDescriptor	*	psBlockDesc 
);

IMG_RESULT GDMA_HAL_EnableTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_BOOL						bListTransfer
);

IMG_RESULT GDMA_HAL_GetCurrentTransferCount
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32					*	pui32Count
);
	
IMG_RESULT GDMA_HAL_ResetHardware
(
	ioblock_sBlockDescriptor	*	psBlockDesc
);

IMG_RESULT GDMA_HAL_PrepareLinkedListTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_UINT32						ui32FirstListElementIndex
);

IMG_RESULT GDMA_HAL_PrepareSingleShotTransfer
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	GDMA_sTransferObject		*	psTransferObject
);

IMG_RESULT GDMA_HAL_GetInterruptStatus
(
	ioblock_sBlockDescriptor	*	psBlockDesc,
	IMG_BOOL *						pbInterrupt
);
										
#endif /* __GDMA_HAL_H__ */
