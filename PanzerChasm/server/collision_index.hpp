#pragma once
#include "../map_loader.hpp"
#include "../math_utils.hpp"

namespace PanzerChasm
{

// Class for collisions calculations optimization.
// It can fast fetch only potential-collidable objects.
// Supported only "static walls" and "models" from map data.
// Dynamic walls not supported.
class CollisionIndex final
{
public:
	explicit CollisionIndex( const MapDataConstPtr& map_data );
	~CollisionIndex();

	template<class Func>
	void ProcessElementsInRadius(
		const m_Vec2& pos, float radius,
		const Func& func ) const;

	// Func must return true, if need abort.
	template<class Func>
	void RayCast(
		const m_Vec3& pos, const m_Vec3& dir_normalized,
		const Func& func,
		float max_cast_distance= Constants::max_float ) const;

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

	// Indeces of models, which can`t be placed in index - dynamic, breakable, etc.
	std::vector<unsigned short> dynamic_models_indeces_;

	// Linked lists heads.
	unsigned short index_field_[ MapData::c_map_size * MapData::c_map_size ];
};

} // namespace PanzerChasm
