#pragma once
#include <algorithm>
#include <type_traits>

#include "vec.hpp"

template<class VecType>
class m_BBox
{
	static_assert(
		std::is_same<VecType, m_Vec2>::value ||
		std::is_same<VecType, m_Vec3>::value,
		"This class can work only with m_Vec2 and m_Vec3 types" );
public:
	m_BBox(){}
	m_BBox( const VecType& in_min, const VecType& in_max );

	m_BBox& operator+=( const VecType& point );
	m_BBox& operator+=( const m_BBox& other );

	m_BBox operator+( const m_BBox& other ) const;

	// TODO - impliment this
	//m_BBox& operator-=( const m_BBox& other );
	//m_BBox operator-( const m_BBox& other ) const;

	VecType Center() const;
	VecType Size() const;

public:
	VecType min;
	VecType max;
};

typedef m_BBox<m_Vec2> m_BBox2;
typedef m_BBox<m_Vec3> m_BBox3;

template<class VecType>
m_BBox<VecType>::m_BBox( const VecType& in_min, const VecType& in_max )
	: min( in_min ), max( in_max )
{}

template<class VecType>
m_BBox<VecType>& m_BBox<VecType>::operator+=( const VecType& point )
{
	for( unsigned int i= 0; i < VecType::size; i++ )
	{
		min.ToArr()[i]= std::min( min.ToArr()[i], point.ToArr()[i] );
		max.ToArr()[i]= std::max( max.ToArr()[i], point.ToArr()[i] );
	}

	return *this;
}

template<class VecType>
m_BBox<VecType>& m_BBox<VecType>::operator+=( const m_BBox& other )
{
	for( unsigned int i= 0; i < VecType::size; i++ )
	{
		min.ToArr()[i]= std::min( min.ToArr()[i], other.min.ToArr()[i] );
		max.ToArr()[i]= std::max( max.ToArr()[i], other.max.ToArr()[i] );
	}

	return *this;
}

template<class VecType>
m_BBox<VecType> m_BBox<VecType>::operator+( const m_BBox& other ) const
{
	m_BBox result;

	for( unsigned int i= 0; i < VecType::size; i++ )
	{
		result.min.toArr()[i]= std::min( min.toArr()[i], other.min.toArr()[i] );
		result.max.toArr()[i]= std::max( max.toArr()[i], other.max.toArr()[i] );
	}

	return result;
}

template<class VecType>
VecType m_BBox<VecType>::Center() const
{
	return 0.5f * ( min + max );
}

template<class VecType>
VecType m_BBox<VecType>::Size() const
{
	return max - min;
}
