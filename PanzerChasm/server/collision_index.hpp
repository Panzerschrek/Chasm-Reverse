#pragma once
#include "../map_loader.hpp"

namespace PanzerChasm
{

class CollisionIndex final
{
public:
	explicit CollisionIndex( const MapDataConstPtr& map_data );
	~CollisionIndex();

	template<class Func>
	void ProcessElementsInRadius(
		const m_Vec2& pos, float radius,
		const Func& func ) const;

private:
	void AddElementToIndex( unsigned int x, unsigned int y, const MapData::IndexElement& element );

private:
	struct IndexElement
	{
		MapData::IndexElement index_element;
		unsigned short next; // Index of next element in this linked list.

		static constexpr unsigned short c_dummy_next= 0xFFFFu;
	};

	SIZE_ASSERT( IndexElement, 4u );

private:
	static constexpr float c_fetch_distance_eps_= 0.1f;

private:
	// Linked lists data.
	std::vector<IndexElement> index_elements_;

	// Linked lists heads.
	unsigned short index_field_[ MapData::c_map_size * MapData::c_map_size ];
};

} // namespace PanzerChasm
