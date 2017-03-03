#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include "time.hpp"

namespace PanzerChasm
{

typedef std::vector<char> SaveLoadBuffer;

class SaveStream final
{
public:
	SaveStream( SaveLoadBuffer& out_buffer, Time base_time );
	~SaveStream();

	template<class T>
	void WriteInt8  ( const T& i );
	template<class T>
	void WriteUInt8 ( const T& i );
	template<class T>
	void WriteInt16 ( const T& i );
	template<class T>
	void WriteUInt16( const T& i );
	template<class T>
	void WriteInt32 ( const T& i );
	template<class T>
	void WriteUInt32( const T& i );

	template<class T>
	void WriteFloat( const T& f );
	template<class T>
	void WriteDouble( const T& f );

	void WriteTime( const Time& time );

private:
	template<class T>
	void Write( const T& t );

private:
	SaveLoadBuffer& buffer_;
	const Time base_time_;
};

class LoadStream final
{
public:
	LoadStream( const SaveLoadBuffer& in_buffer, unsigned int buffer_pos );
	~LoadStream();

	unsigned int GetBufferPos() const;

	void ReadInt8  ( int8_t  & i );
	void ReadUInt8 ( uint8_t & i );
	void ReadInt16 ( int16_t & i );
	void ReadUInt16( uint16_t& i );
	void ReadInt32 ( int16_t & i );
	void ReadUInt32( uint16_t& i );

	void ReadFloat( float& f );
	void ReadDouble( double& f );

	void ReadTime( Time& time );

private:
	template<class T>
	void Read( T& t );

private:
	const SaveLoadBuffer& buffer_;
	unsigned int buffer_pos_;
};

template<class T>
void SaveStream::WriteInt8  ( const T& i )
{
	static_assert( std::is_same< T, int8_t   >::value, "Invalid type" );
	Write( i );
}

template<class T>
void SaveStream::WriteUInt8 ( const T& i )
{
	static_assert( std::is_same< T, uint8_t  >::value, "Invalid type" );
	Write( i );
}

template<class T>
void SaveStream::WriteInt16 ( const T& i )
{
	static_assert( std::is_same< T, int16_t  >::value, "Invalid type" );
	Write( i );
}

template<class T>
void SaveStream::WriteUInt16( const T& i )
{
	static_assert( std::is_same< T, uint16_t >::value, "Invalid type" );
	Write( i );
}

template<class T>
void SaveStream::WriteInt32 ( const T& i )
{
	static_assert( std::is_same< T, int32_t  >::value, "Invalid type" );
	Write( i );
}

template<class T>
void SaveStream::WriteUInt32( const T& i )
{
	static_assert( std::is_same< T, uint32_t >::value, "Invalid type" );
	Write( i );
}


template<class T>
void SaveStream::WriteFloat( const T& f )
{
	static_assert( std::is_same< T, float >::value, "Invalid type" );
	Write( f );
}

template<class T>
void SaveStream::WriteDouble( const T& f )
{
	static_assert( std::is_same< T, double >::value, "Invalid type" );
	Write( f );
}

inline void SaveStream::WriteTime( const Time& time )
{
	// Save all times as delta from base time
	Write( ( time - base_time_ ).GetInternalRepresentation() );
}

template<class T>
void SaveStream::Write( const T& t )
{
	static_assert(
		std::is_integral<T>::value || std::is_floating_point<T>::value,
		"Expected basic types" );

	const size_t pos= buffer_.size();
	buffer_.resize( sizeof(T) );
	std::memcpy(
		buffer_.data() + pos,
		&t,
		sizeof(T) );
}

} // PanzerChasm
