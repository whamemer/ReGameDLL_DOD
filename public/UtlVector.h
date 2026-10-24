// UtlVector.h -- growable array of the elements of one type.
// Recovered from the shipped dod.so (public/UtlVector.h).
/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or (at your
 * option) any later version (https://www.gnu.org/licenses/).
 */
#pragma once

#include <new>
#include "UtlMemory.h"

// placement new and the explicit destructor call of the elements
template<class T>
inline T *Construct( T *pMemory )
{
	return ::new ( pMemory ) T;
}

template<class T>
inline T *CopyConstruct( T *pMemory, T const &src )
{
	return ::new ( pMemory ) T( src );
}

template<class T>
inline void Destruct( T *pMemory )
{
	pMemory->~T();
}

//-----------------------------------------------------------------------------
// The CUtlVector class:
// A growable array class which doubles in size by default.
// It will always keep all elements consecutive in memory, and may move the
// elements around in memory (via a PvRealloc) when elements are inserted or
// removed. Clients should therefore refer to the elements of the vector by index
// (they should *never* maintain pointers to elements in the vector).
//-----------------------------------------------------------------------------
template<class T>
class CUtlVector
{
public:
	// constructor, destructor
	CUtlVector( int growSize = 0, int initSize = 0 );
	CUtlVector( T *pMemory, int numElements );
	~CUtlVector();

	// element access
	T &operator[]( int i );
	T const &operator[]( int i ) const;
	T &Element( int i );
	T const &Element( int i ) const;

	// Gets the base address (can change when adding elements!)
	T *Base();
	T const *Base() const;

	// Returns the number of elements in the vector
	int Count() const;
	int Size() const;  // DEPRECATED, use Count

	// Is element index valid?
	bool IsValidIndex( int i ) const;

	// Adds an element, uses default constructor
	int AddToHead();
	int AddToTail();
	int InsertBefore( int elem );
	int InsertAfter( int elem );

	// Adds an element, uses copy constructor
	int AddToHead( T const &src );
	int AddToTail( T const &src );
	int InsertBefore( int elem, T const &src );
	int InsertAfter( int elem, T const &src );

	// Adds multiple elements, uses default constructor
	int AddMultipleToHead( int num );
	int AddMultipleToTail( int num );
	int InsertMultipleBefore( int elem, int num );
	int InsertMultipleAfter( int elem, int num );

	// Calls RemoveAll() then AddMultipleToTail.
	void SetSize( int size );

	// Copies the array
	void CopyArray( T const *pArray, int size );

	// Add the specified array to the tail.
	int AddVectorToTail( CUtlVector<T> const &src );

	// Finds an element (element needs operator== defined)
	int Find( T const &src );
	bool HasElement( T const &src );

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( int num );

	// Element removal
	void FastRemove( int elem );			// doesn't preserve order
	void Remove( int elem );				// preserves order, shifts elements
	void FindAndRemove( T const &src );		// removes first occurrence of src, preserves order, shifts elements
	void RemoveMultiple( int elem, int num );	// preserves order, shifts elements
	void RemoveAll();						// doesn't deallocate memory

	// Memory deallocation
	void Purge();

	// Purges the list and calls delete on each element in it.
	void PurgeAndDeleteElements();

	// Set the size by which it grows when it needs to allocate more memory.
	void SetGrowSize( int size );

protected:
	// Can't copy this unless we explicitly do it!
	CUtlVector( CUtlVector const &vec ) { Assert( 0 ); }

private:
	// Grows the vector
	void GrowVector( int num = 1 );

	// Shifts elements....
	void ShiftElementsRight( int elem, int num = 1 );
	void ShiftElementsLeft( int elem, int num = 1 );

	// For easier access to the elements through the debugger
	void ResetDbgInfo();

	CUtlMemory<T> m_Memory;
	int m_Size;

	// For easier access to the elements through the debugger
	T *m_pElements;
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template<class T>
inline CUtlVector<T>::CUtlVector( int growSize, int initSize ) : m_Memory( growSize, initSize ), m_Size( 0 )
{
	ResetDbgInfo();
}

template<class T>
inline CUtlVector<T>::CUtlVector( T *pMemory, int numElements ) : m_Memory( pMemory, numElements ), m_Size( 0 )
{
	ResetDbgInfo();
}

template<class T>
inline CUtlVector<T>::~CUtlVector()
{
	Purge();
}

//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template<class T>
inline T &CUtlVector<T>::operator[]( int i )
{
	Assert( i < m_Size );
	return m_Memory[i];
}

template<class T>
inline T const &CUtlVector<T>::operator[]( int i ) const
{
	Assert( i < m_Size );
	return m_Memory[i];
}

template<class T>
inline T &CUtlVector<T>::Element( int i )
{
	Assert( i < m_Size );
	return m_Memory[i];
}

template<class T>
inline T const &CUtlVector<T>::Element( int i ) const
{
	Assert( i < m_Size );
	return m_Memory[i];
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template<class T>
inline T *CUtlVector<T>::Base()
{
	return m_Memory.Base();
}

template<class T>
inline T const *CUtlVector<T>::Base() const
{
	return m_Memory.Base();
}

//-----------------------------------------------------------------------------
// Count
//-----------------------------------------------------------------------------
template<class T>
inline int CUtlVector<T>::Size() const
{
	return m_Size;
}

template<class T>
inline int CUtlVector<T>::Count() const
{
	return m_Size;
}

//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template<class T>
inline bool CUtlVector<T>::IsValidIndex( int i ) const
{
	return ( i >= 0 ) && ( i < m_Size );
}

//-----------------------------------------------------------------------------
// Grows the vector
//-----------------------------------------------------------------------------
template<class T>
void CUtlVector<T>::GrowVector( int num )
{
	if ( m_Size + num - 1 >= m_Memory.NumAllocated() )
	{
		m_Memory.Grow( m_Size + num - m_Memory.NumAllocated() );
	}

	m_Size += num;
	ResetDbgInfo();
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template<class T>
void CUtlVector<T>::EnsureCapacity( int num )
{
	m_Memory.EnsureCapacity( num );
	ResetDbgInfo();
}

//-----------------------------------------------------------------------------
// Shifts elements
//-----------------------------------------------------------------------------
template<class T>
void CUtlVector<T>::ShiftElementsRight( int elem, int num )
{
	Assert( IsValidIndex( elem ) || ( m_Size == 0 ) || ( num == 0 ) );
	int numToMove = m_Size - elem - num;
	if ( ( numToMove > 0 ) && ( num > 0 ) )
		memmove( &Element( elem + num ), &Element( elem ), numToMove * sizeof( T ) );
}

template<class T>
void CUtlVector<T>::ShiftElementsLeft( int elem, int num )
{
	Assert( IsValidIndex( elem ) || ( m_Size == 0 ) || ( num == 0 ) );
	int numToMove = m_Size - elem - num;
	if ( ( numToMove > 0 ) && ( num > 0 ) )
		memmove( &Element( elem ), &Element( elem + num ), numToMove * sizeof( T ) );
}

//-----------------------------------------------------------------------------
// Adds an element, uses default constructor
//-----------------------------------------------------------------------------
template<class T>
inline int CUtlVector<T>::AddToHead()
{
	return InsertBefore( 0 );
}

template<class T>
inline int CUtlVector<T>::AddToTail()
{
	return InsertBefore( m_Size );
}

template<class T>
inline int CUtlVector<T>::InsertAfter( int elem )
{
	return InsertBefore( elem + 1 );
}

template<class T>
int CUtlVector<T>::InsertBefore( int elem )
{
	// Can insert at the end
	Assert( ( elem == Count() ) || IsValidIndex( elem ) );

	GrowVector();
	ShiftElementsRight( elem );
	Construct( &Element( elem ) );
	return elem;
}

//-----------------------------------------------------------------------------
// Adds an element, uses copy constructor
//-----------------------------------------------------------------------------
template<class T>
inline int CUtlVector<T>::AddToHead( T const &src )
{
	return InsertBefore( 0, src );
}

template<class T>
inline int CUtlVector<T>::AddToTail( T const &src )
{
	return InsertBefore( m_Size, src );
}

template<class T>
inline int CUtlVector<T>::InsertAfter( int elem, T const &src )
{
	return InsertBefore( elem + 1, src );
}

template<class T>
int CUtlVector<T>::InsertBefore( int elem, T const &src )
{
	// Can insert at the end
	Assert( ( elem == Count() ) || IsValidIndex( elem ) );

	GrowVector();
	ShiftElementsRight( elem );
	CopyConstruct( &Element( elem ), src );
	return elem;
}

//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template<class T>
inline int CUtlVector<T>::AddMultipleToHead( int num )
{
	return InsertMultipleBefore( 0, num );
}

template<class T>
inline int CUtlVector<T>::AddMultipleToTail( int num )
{
	return InsertMultipleBefore( m_Size, num );
}

template<class T>
inline int CUtlVector<T>::InsertMultipleAfter( int elem, int num )
{
	return InsertMultipleBefore( elem + 1, num );
}

template<class T>
void CUtlVector<T>::SetSize( int size )
{
	RemoveAll();
	AddMultipleToTail( size );
}

template<class T>
int CUtlVector<T>::InsertMultipleBefore( int elem, int num )
{
	if ( num == 0 )
		return elem;

	// Can insert at the end
	Assert( ( elem == Count() ) || IsValidIndex( elem ) );

	GrowVector( num );
	ShiftElementsRight( elem, num );

	// Invoke default constructors
	for ( int i = 0; i < num; ++i )
		Construct( &Element( elem + i ) );

	return elem;
}

//-----------------------------------------------------------------------------
// Copies the array
//-----------------------------------------------------------------------------
template<class T>
void CUtlVector<T>::CopyArray( T const *pArray, int size )
{
	RemoveAll();
	AddMultipleToTail( size );
	for ( int i = 0; i < size; i++ )
		( *this )[i] = pArray[i];
}

template<class T>
int CUtlVector<T>::AddVectorToTail( CUtlVector<T> const &src )
{
	int base = Count();

	// Make space.
	AddMultipleToTail( src.Count() );

	// Copy the elements.
	for ( int i = 0; i < src.Count(); i++ )
		( *this )[base + i] = src[i];

	return base;
}

//-----------------------------------------------------------------------------
// Finds an element (element needs operator== defined)
//-----------------------------------------------------------------------------
template<class T>
int CUtlVector<T>::Find( T const &src )
{
	for ( int i = 0; i < Count(); ++i )
	{
		if ( Element( i ) == src )
			return i;
	}
	return -1;
}

template<class T>
bool CUtlVector<T>::HasElement( T const &src )
{
	return ( Find( src ) >= 0 );
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template<class T>
void CUtlVector<T>::FastRemove( int elem )
{
	Assert( IsValidIndex( elem ) );

	Destruct( &Element( elem ) );
	if ( m_Size > 0 )
	{
		memcpy( &Element( elem ), &Element( m_Size - 1 ), sizeof( T ) );
		--m_Size;
	}
}

template<class T>
void CUtlVector<T>::Remove( int elem )
{
	Destruct( &Element( elem ) );
	ShiftElementsLeft( elem );
	--m_Size;
}

template<class T>
void CUtlVector<T>::FindAndRemove( T const &src )
{
	int elem = Find( src );
	if ( elem != -1 )
		Remove( elem );
}

template<class T>
void CUtlVector<T>::RemoveMultiple( int elem, int num )
{
	Assert( IsValidIndex( elem ) );
	Assert( elem + num <= Count() );

	for ( int i = elem + num; --i >= elem; )
		Destruct( &Element( i ) );

	ShiftElementsLeft( elem, num );
	m_Size -= num;
}

template<class T>
void CUtlVector<T>::RemoveAll()
{
	for ( int i = m_Size; --i >= 0; )
		Destruct( &Element( i ) );

	m_Size = 0;
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template<class T>
void CUtlVector<T>::Purge()
{
	RemoveAll();
	m_Memory.Purge();
	ResetDbgInfo();
}

template<class T>
inline void CUtlVector<T>::PurgeAndDeleteElements()
{
	for ( int i = 0; i < m_Size; i++ )
		delete Element( i );

	Purge();
}

template<class T>
void CUtlVector<T>::SetGrowSize( int size )
{
	m_Memory.SetGrowSize( size );
}

//-----------------------------------------------------------------------------
// For easier access to the elements through the debugger
//-----------------------------------------------------------------------------
template<class T>
inline void CUtlVector<T>::ResetDbgInfo()
{
	m_pElements = m_Memory.Base();
}
