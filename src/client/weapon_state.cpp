#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../game_resources.hpp"
#include "../math_utils.hpp"

#include "weapon_state.hpp"

namespace PanzerChasm
{

WeaponState::WeaponState( const GameResourcesConstPtr& game_resources )
	: game_resources_(game_resources)
{
	PC_ASSERT( game_resources_ != nullptr );
}

WeaponState::~WeaponState()
{
}

void WeaponState::ProcessMessage( const Messages::PlayerWeapon& message )
{
	current_weapon_index_= message.current_weapon_index_;
	current_animation_= message.animation;
	current_animation_frame_= message.animation_frame;

	switch_stage_= static_cast<float>( message.switch_stage ) / 255.0f;
}

unsigned int WeaponState::CurrentWeaponIndex() const
{
	return current_weapon_index_;
}

unsigned int WeaponState::CurrentAnimation() const
{
	return current_animation_;
}

unsigned int WeaponState::CurrentAnimationFrame() const
{
	return current_animation_frame_;
}

float WeaponState::GetSwitchStage() const
{
	return switch_stage_;
}

} // namespace PanzerChasm
