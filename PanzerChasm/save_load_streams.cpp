#include "assert.hpp"

#include "save_load_streams.hpp"

namespace PanzerChasm
{

SaveStream::SaveStream( SaveLoadBuffer& out_buffer, const Time base_time )
	: buffer_(out_buffer)
	, base_time_(base_time)
{}

SaveStream::~SaveStream()
{}

LoadStream::LoadStream( const SaveLoadBuffer& in_buffer, const unsigned int buffer_pos, const Time base_time )
	: buffer_(in_buffer)
	, buffer_pos_(buffer_pos)
	, base_time_(base_time)
{}

LoadStream::~LoadStream()
{}

unsigned int LoadStream::GetBufferPos() const
{
	return buffer_pos_;
}

template<class T>
void LoadStream::Read( T& t )
{
	static_assert(
		std::is_integral<T>::value || std::is_floating_point<T>::value,
		"Expected basic types" );

	PC_ASSERT( buffer_pos_ + sizeof(T) <= buffer_.size() );

	std::memcpy(
		&t,
		buffer_.data() + buffer_pos_,
		sizeof(T) );

	buffer_pos_+= sizeof(T);
}

void LoadStream::ReadBool( bool& b )
{
	Read(b);
}

void LoadStream::ReadInt8  ( int8_t  & i )
{
	Read(i);
}

void LoadStream::ReadUInt8 ( uint8_t & i )
{
	Read(i);
}

void LoadStream::ReadInt16 ( int16_t & i )
{
	Read(i);
}

void LoadStream::ReadUInt16( uint16_t& i )
{
	Read(i);
}

void LoadStream::ReadInt32 ( int32_t & i )
{
	Read(i);
}

void LoadStream::ReadUInt32( uint32_t& i )
{
	Read(i);
}

void LoadStream::ReadFloat( float& f )
{
	Read(f);
}

void LoadStream::ReadDouble( double& f )
{
	Read(f);
}

void LoadStream::ReadVec2( m_Vec2& v )
{
	Read( v.x );
	Read( v.y );
}

void LoadStream::ReadVec3( m_Vec3& v )
{
	Read( v.x );
	Read( v.y );
	Read( v.z );
}

void LoadStream::ReadTime( Time& time )
{
	int64_t time_internal_representation;
	Read( time_internal_representation );

	time= Time::FromInternalRepresentation( time_internal_representation ) + base_time_;
}

} // namespace PanzerChasm
