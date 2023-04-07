#pragma once
#include <cstdint>
#include <vector>

#include "../../size.hpp"

namespace PanzerChasm
{

class SurfacesCache final
{
public:
	struct Surface
	{
		unsigned int size[2];

		// Pointer to pointer to this surface.
		// Reset, when surface is recycled.
		// If zero - surface was freed.
		Surface** owner;

		uint32_t* GetData()
		{
			return reinterpret_cast<uint32_t*>(this + 1);
		}

		const uint32_t* GetData() const
		{
			return reinterpret_cast<const uint32_t*>(this + 1);
		}
	};

public:
	explicit SurfacesCache( const Size2& viewport_size );
	~SurfacesCache();

	void AllocateSurface( unsigned int size_x, unsigned int size_y, Surface** out_surface_ptr );

	// Clears surface cache, but not notify surfaces owners.
	void Clear();

private:
	std::vector<uint8_t> storage_;
	unsigned int next_allocated_surface_offset_= 0u;
	unsigned int last_surface_in_buffer_end_offset_= 0u;
	unsigned int next_recycled_surface_offset_= ~0u;
};

} // namespace PanzerChasm
