#include "../game_constants.hpp"

#include "monster.hpp"

namespace PanzerChasm
{

Monster::Monster(
	const MapData::Monster& map_monster,
	const GameResourcesConstPtr& game_resources,
	const Time spawn_time )
	: monster_id_( map_monster.monster_id )
	, game_resources_(game_resources)
	, pos_( map_monster.pos, 0.0f )
	, angle_( map_monster.angle )
	, current_animation_start_time_(spawn_time)
{
	PC_ASSERT( game_resources_ != nullptr );

	current_animation_= GetAnimation( AnimationId::Idle );
}

Monster::~Monster()
{}

unsigned char Monster::MonsterId() const
{
	return monster_id_;
}

const m_Vec3& Monster::Position() const
{
	return pos_;
}

float Monster::Angle() const
{
	return angle_;
}

unsigned int Monster::CurrentAnimation() const
{
	return current_animation_;
}

unsigned int Monster::CurrentAnimationFrame() const
{
	return current_animation_frame_;
}

void Monster::Tick( const Time current_time )
{
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	const float time_delta_s= ( current_time - current_animation_start_time_ ).ToSeconds();

	const float frame= time_delta_s * GameConstants::animations_frames_per_second;

	PC_ASSERT( current_animation_ < model.animations.size() );
	PC_ASSERT( model.animations[ current_animation_ ].frame_count > 0u );

	current_animation_frame_=
		static_cast<unsigned int>( std::round(frame) ) % model.animations[ current_animation_ ].frame_count;
}

unsigned int Monster::GetAnimation( const AnimationId id ) const
{
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	for( const Model::Animation& animation : model.animations )
	{
		if( animation.id == static_cast<unsigned int>(id) )
			return &animation - model.animations.data();
	}

	// TODO - what do, if ne not found animation?
	return 0u;
}

} // namespace PanzerChasm
