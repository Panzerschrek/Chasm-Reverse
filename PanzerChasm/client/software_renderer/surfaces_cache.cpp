#include <cmath>

#include "../../assert.hpp"
#include "../../log.hpp"

#include "surfaces_cache.hpp"

namespace PanzerChasm
{

// Returns result in bytes.
inline unsigned int SurfaceDataSizeAligned(
	const unsigned int size_x, const unsigned int size_y )
{
	const unsigned int pixels= size_x * size_y;
	return ( (pixels + 3u) & (~3u) ) * sizeof(uint32_t);
}

SurfacesCache::SurfacesCache( const Size2& viewport_size )
{
	// For lower resolutions we need more surface cache, relative screen area.
	// For bigger resolutions ( 1024x768 or more ) we need less relative cache size.
	const unsigned int viewport_pixels= viewport_size.Width() * viewport_size.Height();
	const float viewport_pixels_f= float(viewport_pixels);
	const unsigned int cache_size_pixels=
		static_cast<unsigned int>( viewport_pixels_f * 2.5f / std::sqrt( viewport_pixels_f / ( 1024.0f * 768.0f ) ) );

	storage_.resize( cache_size_pixels * sizeof(uint32_t) );

	const unsigned int size_kb= (storage_.size() + 1023u) / 1024u;
	Log::Info( "Surfaces cache size: ", size_kb, "kb ( ", size_kb * sizeof(uint32_t), " kilotexels )." );
}

SurfacesCache::~SurfacesCache()
{
}

void SurfacesCache::AllocateSurface(
	const unsigned int size_x, const unsigned int size_y,
	Surface** out_surface_ptr )
{
	unsigned int surface_data_size= sizeof(Surface) + SurfaceDataSizeAligned( size_x, size_y );

	PC_ASSERT( surface_data_size < storage_.size() );

	if( next_allocated_surface_offset_ + surface_data_size > storage_.size() )
	{
		// Recycle surfaces at end.
		while( next_recycled_surface_offset_ < last_surface_in_buffer_end_offset_ )
		{
			Surface* const recycled_surface= reinterpret_cast<Surface*>( storage_.data() + next_recycled_surface_offset_ );
			PC_ASSERT( recycled_surface->owner != nullptr && *recycled_surface->owner == recycled_surface );
			*recycled_surface->owner= nullptr;
			next_recycled_surface_offset_+=
				sizeof(Surface) + SurfaceDataSizeAligned( recycled_surface->size[0], recycled_surface->size[1] );
		}

		last_surface_in_buffer_end_offset_= next_allocated_surface_offset_;
		next_allocated_surface_offset_= 0u;
		next_recycled_surface_offset_= 0u;
	}

	// Set recycled surfaces owner to null.
	while( next_recycled_surface_offset_ < last_surface_in_buffer_end_offset_ &&
		next_recycled_surface_offset_ < next_allocated_surface_offset_ + surface_data_size )
	{
		Surface* const recycled_surface= reinterpret_cast<Surface*>( storage_.data() + next_recycled_surface_offset_ );
		PC_ASSERT( recycled_surface->owner != nullptr && *recycled_surface->owner == recycled_surface );
		*recycled_surface->owner= nullptr;
		next_recycled_surface_offset_+=
			sizeof(Surface) + SurfaceDataSizeAligned( recycled_surface->size[0], recycled_surface->size[1] );
	}

	Surface* const surface= reinterpret_cast<Surface*>( storage_.data() + next_allocated_surface_offset_ );
	surface->size[0]= size_x;
	surface->size[1]= size_y;
	surface->owner= out_surface_ptr;

	*out_surface_ptr= surface;

	next_allocated_surface_offset_+= surface_data_size;
}

void SurfacesCache::Clear()
{
	next_allocated_surface_offset_= 0u;
	last_surface_in_buffer_end_offset_= 0u;
	next_recycled_surface_offset_= ~0u;
}

} // namespace PanzerChasm
