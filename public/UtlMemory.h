// UtlMemory.h -- growable memory block, the storage of CUtlVector.
// Recovered from the shipped dod.so (public/UtlMemory.h).
/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or (at your
 * option) any later version (https://www.gnu.org/licenses/).
 */
#pragma once

#include <stdlib.h>
#include <string.h>

#ifndef Assert
#include <assert.h>
#define Assert( _exp ) assert( _exp )
#endif

//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class.
//-----------------------------------------------------------------------------
template<class T>
class CUtlMemory
{
public:
	// constructor, destructor
	CUtlMemory( int nGrowSize = 0, int nInitSize = 0 );
	CUtlMemory( T *pMemory, int numElements );
	~CUtlMemory();

	// element access
	T &operator[]( int i );
	T const &operator[]( int i ) const;

	// Can we use this index?
	bool IsIdxValid( int i ) const;

	// Gets the base address (can change when adding elements!)
	T *Base();
	T const *Base() const;

	// Size
	int NumAllocated() const;

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// is the memory externally allocated?
	bool IsExternallyAllocated() const;

	// Set the size by which the memory grows
	void SetGrowSize( int size );

private:
	enum
	{
		EXTERNAL_BUFFER_MARKER = -1
	};

	T *m_pMemory;
	int m_NumAllocated;
	int m_GrowSize;
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template<class T>
CUtlMemory<T>::CUtlMemory( int nGrowSize, int nInitSize ) : m_pMemory( 0 ), m_NumAllocated( nInitSize ), m_GrowSize( nGrowSize )
{
	Assert( nGrowSize >= 0 );
	if ( m_NumAllocated )
	{
		m_pMemory = (T *) malloc( m_NumAllocated * sizeof( T ) );
	}
}

template<class T>
CUtlMemory<T>::CUtlMemory( T *pMemory, int numElements ) : m_pMemory( pMemory ), m_NumAllocated( numElements )
{
	// Special marker indicating externally supplied memory
	m_GrowSize = EXTERNAL_BUFFER_MARKER;
}

template<class T>
CUtlMemory<T>::~CUtlMemory()
{
	Purge();
}

//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template<class T>
inline T &CUtlMemory<T>::operator[]( int i )
{
	Assert( IsIdxValid( i ) );
	return m_pMemory[i];
}

template<class T>
inline T const &CUtlMemory<T>::operator[]( int i ) const
{
	Assert( IsIdxValid( i ) );
	return m_pMemory[i];
}

template<class T>
inline bool CUtlMemory<T>::IsIdxValid( int i ) const
{
	return ( i >= 0 ) && ( i < m_NumAllocated );
}

//-----------------------------------------------------------------------------
// Gets the base address
//-----------------------------------------------------------------------------
template<class T>
inline T *CUtlMemory<T>::Base()
{
	return m_pMemory;
}

template<class T>
inline T const *CUtlMemory<T>::Base() const
{
	return m_pMemory;
}

//-----------------------------------------------------------------------------
// Size
//-----------------------------------------------------------------------------
template<class T>
inline int CUtlMemory<T>::NumAllocated() const
{
	return m_NumAllocated;
}

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template<class T>
void CUtlMemory<T>::Grow( int num )
{
	if ( IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated
		Assert( 0 );
		return;
	}

	// Grow by the requested amount, without a request by the grow size of this memory or by doubling it
	if ( num > 0 )
		m_NumAllocated += num;
	else if ( m_GrowSize )
		m_NumAllocated += m_GrowSize;
	else
		m_NumAllocated += m_NumAllocated ? m_NumAllocated : 1;

	if ( m_pMemory )
		m_pMemory = (T *) realloc( m_pMemory, m_NumAllocated * sizeof( T ) );
	else
		m_pMemory = (T *) malloc( m_NumAllocated * sizeof( T ) );
}

//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template<class T>
inline void CUtlMemory<T>::EnsureCapacity( int num )
{
	if ( m_NumAllocated >= num )
		return;

	if ( IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated
		Assert( 0 );
		return;
	}

	m_NumAllocated = num;

	if ( m_pMemory )
		m_pMemory = (T *) realloc( m_pMemory, m_NumAllocated * sizeof( T ) );
	else
		m_pMemory = (T *) malloc( m_NumAllocated * sizeof( T ) );
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template<class T>
void CUtlMemory<T>::Purge()
{
	if ( !IsExternallyAllocated() )
	{
		if ( m_pMemory )
		{
			free( (void *) m_pMemory );
			m_pMemory = 0;
		}
		m_NumAllocated = 0;
	}
}

template<class T>
inline bool CUtlMemory<T>::IsExternallyAllocated() const
{
	return m_GrowSize < 0;
}

template<class T>
void CUtlMemory<T>::SetGrowSize( int nSize )
{
	Assert( nSize >= 0 );
	m_GrowSize = nSize;
}
