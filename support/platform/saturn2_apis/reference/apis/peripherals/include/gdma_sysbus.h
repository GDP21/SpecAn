#if !defined (__GDMA_SYSBUS_H__)
#define __GDMA_SYSBUS_H__

/*!
*******************************************************************************

 This describes a linked list element.

*******************************************************************************/
typedef struct GDMA_sLinkedListDescriptor_tag
{
    /*! Word 0 - used to specify byte swapping, direction and peripheral data width */
    IMG_UINT32	ui32Word0;

    /*! Word 1 - used to specify interrupt behaviour and transfer length */
    IMG_UINT32	ui32Word1;

    /*! Word 2 - used to specify peripheral register address */
    IMG_UINT32	ui32Word2;

    /*! Word 3 - used to specify burst rate */
    IMG_UINT32	ui32Word3;

    /*! Word 4 - unused */
    IMG_UINT32	ui32Word4;

    /*! Word 5 - unused */
    IMG_UINT32	ui32Word5;

    /*! Word 6 - used to specify data start address */
    IMG_UINT32	ui32Word6;

    /*! Word 7 - used to specify location of next list element */
    IMG_UINT32	ui32Word7;

} GDMA_sLinkedListDescriptor;

#endif	/* __GDMA_SYSBUS_H__ */
