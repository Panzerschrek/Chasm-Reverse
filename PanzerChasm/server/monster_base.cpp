#include "../assert.hpp"
#include "../game_resources.hpp"
#include "../math_utils.hpp"
#include "collisions.hpp"
#include "map.hpp"

#include "monster_base.hpp"

namespace PanzerChasm
{

MonsterBase::MonsterBase(
	const GameResourcesConstPtr& game_resources,
	const unsigned char monster_id,
	const m_Vec3& pos,
	const float angle )
	: game_resources_(game_resources)
	, monster_id_(monster_id)
	, pos_(pos)
	, angle_( NormalizeAngle(angle) )
	, health_( game_resources_->monsters_description[ monster_id ].life )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( monster_id_ < game_resources_->monsters_description.size() );
}

MonsterBase::~MonsterBase()
{}

unsigned char MonsterBase::MonsterId() const
{
	return monster_id_;
}

const m_Vec3& MonsterBase::Position() const
{
	return pos_;
}

void MonsterBase::SetPosition( const m_Vec3& pos )
{
	pos_= pos;
}

float MonsterBase::Angle() const
{
	return angle_;
}

m_Vec2 MonsterBase::GetZMinMax() const
{
	if( monster_id_ == 0u )
	{
		return m_Vec2( 0.0f, GameConstants::player_height );
	}

	const Model& model= game_resources_->monsters_models[ monster_id_ ];
	return m_Vec2( model.z_min, model.z_max );
}

int MonsterBase::Health() const
{
	return health_;
}

unsigned int MonsterBase::CurrentAnimation() const
{
	return current_animation_;
}

unsigned int MonsterBase::CurrentAnimationFrame() const
{
	return current_animation_frame_;
}

unsigned char MonsterBase::GetBodyPartsMask() const
{
	if( fragmented_ ) // Hide all body, if fragmented.
		return 0u;

	unsigned char mask= BodyPartsMask::Body;
	mask|= have_left_hand_  ? BodyPartsMask::LeftHand  : BodyPartsMask::LeftHandSeparated ;
	mask|= have_right_hand_ ? BodyPartsMask::RightHand : BodyPartsMask::RightHandSeparated;
	mask|= have_head_ ? BodyPartsMask::Head : BodyPartsMask::HeadSeparated;

	if( int(current_animation_) == GetAnimation( AnimationId::RemoteAttack ) )
	{
		PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );
		PC_ASSERT( current_animation_ < game_resources_->monsters_models[ monster_id_ ].animations.size() );

		const unsigned int frame_count= game_resources_->monsters_models[ monster_id_ ].animations[ current_animation_ ].frame_count;
		const unsigned int middle_frame= frame_count / 2u;

		// Draw weapon fire at middle of attack animation.
		if( std::abs( int(current_animation_frame_) - int(middle_frame) ) <= 2 )
			mask|= BodyPartsMask::WeaponFire;
	}

	return mask;
}

bool MonsterBase::TryShot( const m_Vec3& from, const m_Vec3& direction_normalized, m_Vec3& out_pos ) const
{
	if( health_ <= 0 )
		return false;

	const Model& model= game_resources_->monsters_models[ monster_id_ ];
	const GameResources::MonsterDescription& description= game_resources_->monsters_description[ monster_id_ ];

	return
		RayIntersectCylinder(
			pos_.xy(), description.w_radius,
			pos_.z + model.z_min, pos_.z + model.z_max,
			from, direction_normalized,
			out_pos );
}

void MonsterBase::SetMovementRestriction( const MovementRestriction& restriction )
{
	movement_restriction_= restriction;
}

const MovementRestriction& MonsterBase::GetMovementRestriction() const
{
	return movement_restriction_;
}

int MonsterBase::GetAnimation( const AnimationId id ) const
{
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	for( const Model::Animation& animation : model.animations )
	{
		if( animation.id == static_cast<unsigned int>(id) )
			return &animation - model.animations.data();
	}
	return -1;
}

int MonsterBase::GetAnyAnimation( const std::initializer_list<AnimationId>& ids ) const
{
	for( const AnimationId id : ids )
	{
		const int  animation= GetAnimation( id );
		if( animation >= 0 )
			return animation;
	}

	return -1;
}


} // namespace PanzerChasm
