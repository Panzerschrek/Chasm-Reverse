#include "../../assert.hpp"

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
	// TODO - calibrate cache size formula.
	const unsigned int viewport_pixels= viewport_size.Width() * viewport_size.Height();
	const unsigned int cache_size_pixels= viewport_pixels * 4u;

	storage_.resize( cache_size_pixels * 4u );
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
			Surface* recycled_surface= reinterpret_cast<Surface*>( storage_.data() + next_recycled_surface_offset_ );
			PC_ASSERT( recycled_surface->owner != nullptr );
			*recycled_surface->owner= nullptr;
			next_recycled_surface_offset_+=
				sizeof(Surface) + SurfaceDataSizeAligned( recycled_surface->size[0], recycled_surface->size[1] );
		}

		last_surface_in_buffer_end_offset_= next_allocated_surface_offset_;
		next_allocated_surface_offset_= 0u;
		next_recycled_surface_offset_= 0u;
	}

	// Set recycled surfaces owner to null.
	while( next_recycled_surface_offset_ < next_allocated_surface_offset_ + surface_data_size )
	{
		Surface* recycled_surface= reinterpret_cast<Surface*>( storage_.data() + next_recycled_surface_offset_ );
		PC_ASSERT( recycled_surface->owner != nullptr );
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
