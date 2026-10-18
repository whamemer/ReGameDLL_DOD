// MemPool.cpp -- fixed-size block memory pool (see dod.so public/MemPool.*).
/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or (at your
 * option) any later version (https://www.gnu.org/licenses/).
 */
#include "extdll.h"
#include <stdlib.h>
#include <math.h>
#include "MemPool.h"

#define MP_MAX(a, b) ( ( (a) > (b) ) ? (a) : (b) )

CMemoryPool::CMemoryPool( int blockSize, int numElements )
{
	_blocksPerBlob = numElements;
	_blockSize = blockSize;
	_numBlobs = 0;
	_numElements = 0;

	AddNewBlob();

	_peakAlloc = 0;
	_blocksAllocated = 0;
}

CMemoryPool::~CMemoryPool()
{
	for ( int i = 0; i < _numBlobs; i++ )
		free( _memBlob[i] );
}

void CMemoryPool::AddNewBlob()
{
	int sizeMultiplier = (int)pow( 2.0, _numBlobs );
	int nElements = _blocksPerBlob * sizeMultiplier;
	int blobSize = _blockSize * nElements;

	_memBlob[_numBlobs] = malloc( blobSize );
	_headOfFreeList = _memBlob[_numBlobs];

	void **newBlob = (void **)_headOfFreeList;
	for ( int j = 0; j < nElements - 1; j++ )
	{
		newBlob[0] = (char *)newBlob + _blockSize;
		newBlob = (void **)newBlob[0];
	}
	newBlob[0] = NULL;

	_numElements += nElements;
	_numBlobs++;
}

void *CMemoryPool::Alloc( unsigned int amount )
{
	if ( amount > (unsigned int)_blockSize )
		return NULL;

	_blocksAllocated++;
	_peakAlloc = MP_MAX( _peakAlloc, _blocksAllocated );

	if ( _blocksAllocated >= _numElements )
		AddNewBlob();

	void *returnBlock = _headOfFreeList;
	_headOfFreeList = *( (void **)_headOfFreeList );
	return returnBlock;
}

void CMemoryPool::Free( void *memblock )
{
	if ( !memblock )
		return;

	--_blocksAllocated;
	*( (void **)memblock ) = _headOfFreeList;
	_headOfFreeList = memblock;
}
