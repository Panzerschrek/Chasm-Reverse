#include "../assert.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"

#include "objects_sounds_processor.hpp"

namespace PanzerChasm
{

namespace Sound
{

ObjectsSoundsProcessor::ObjectsSoundsProcessor( const GameResourcesConstPtr& game_resources )
	: game_resources_(game_resources)
	, prev_tick_time_( Time::CurrentTime() )
{
	PC_ASSERT( game_resources_ != nullptr );
}

ObjectsSoundsProcessor::~ObjectsSoundsProcessor()
{}

void ObjectsSoundsProcessor::SetMap( const MapDataConstPtr& map_data )
{
	map_data_= map_data;

	volume_= 0.0f;
	current_sound_= 0.0f;
	nearest_source_pos_= m_Vec3( 0.0f, 0.0f, 0.0f );
}

void ObjectsSoundsProcessor::Update( const MapState& map_state, const m_Vec3& head_pos )
{
	const float c_change_time_s= 0.5f;

	const Time current_tick_time= Time::CurrentTime();
	const float tick_delta_s= ( current_tick_time - prev_tick_time_ ).ToSeconds();
	prev_tick_time_= current_tick_time;

	const MapState::StaticModel* nearest_model= nullptr;
	float nearest_model_square_distance= Constants::max_float;
	m_Vec3 nearest_model_pos;

	// Search nearest sound.
	for( const MapState::StaticModel& model : map_state.GetStaticModels() )
	{
		if( model.model_id >= map_data_->models_description.size() )
			continue;

		const MapData::ModelDescription& description= map_data_->models_description[ model.model_id ];
		if( description.ambient_sfx_number == 0u )
			continue;

		const Model& model_geometry= map_data_->models[ model.model_id ];

		const float z= model.pos.z + ( model_geometry.z_min + model_geometry.z_max ) * 0.5f;
		const m_Vec3 sound_pos( model.pos.xy(), z );

		const float square_distance= ( head_pos - sound_pos ).SquareLength();
		if( square_distance < nearest_model_square_distance )
		{
			nearest_model= &model;
			nearest_model_square_distance= square_distance;
			nearest_model_pos= sound_pos;
		}
	}

	unsigned int sound_number= 0u;

	if( nearest_model != nullptr )
	{
		sound_number= map_data_->models_description[ nearest_model->model_id ].ambient_sfx_number;

		if( sound_number == current_sound_ )
			nearest_source_pos_= nearest_model_pos;
	}

	const float abs_volume_change= tick_delta_s / c_change_time_s;
	if( sound_number != current_sound_ )
	{
		volume_-= abs_volume_change;
		if( volume_ <= 0.0f )
		{
			current_sound_= sound_number;
			volume_= 0.0f;
		}
	}
	else
	{
		volume_+= abs_volume_change;
		if( volume_ >= 1.0f )
			volume_= 1.0f;
	}
}

unsigned int ObjectsSoundsProcessor::GetCurrentSoundNumber() const
{
	return current_sound_;
}

const m_Vec3& ObjectsSoundsProcessor::GetCurrentSoundPosition() const
{
	return nearest_source_pos_;
}

float ObjectsSoundsProcessor::GetCurrentSoundVolume() const
{
	return volume_;
}

} // namespace Sound

} // namespace PanzerChasm
