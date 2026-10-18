// MemPool.h -- fixed-size block memory pool.
// Recovered from the shipped dod.so (public/MemPool.*).
/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or (at your
 * option) any later version (https://www.gnu.org/licenses/).
 */
#pragma once

class CMemoryPool
{
  public:
	CMemoryPool( int blockSize, int numElements );
	~CMemoryPool();

	void *Alloc( unsigned int amount );
	void Free( void *memblock );
	int Count() { return _blocksAllocated; }
	void AddNewBlob();

  public:
	enum
	{
		MAX_BLOBS = 16
	};

	int _blockSize;
	int _blocksPerBlob;
	int _numElements;
	void *_memBlob[MAX_BLOBS];
	void *_headOfFreeList;
	int _numBlobs;
	int _peakAlloc;
	int _blocksAllocated;
};
