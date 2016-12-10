#pragma once
#include "../fwd.hpp"
#include "../messages.hpp"

namespace PanzerChasm
{

class WeaponState final
{
public:
	WeaponState( const GameResourcesConstPtr& game_resources );
	~WeaponState();

	void ProcessMessage( const Messages::PlayerWeapon& message );

	unsigned int CurrentWeaponIndex() const;

	unsigned int CurrentAnimation() const;
	unsigned int CurrentAnimationFrame() const;

	float GetSwitchStage() const;

private:
	const GameResourcesConstPtr game_resources_;

	unsigned int current_weapon_index_= 0u;
	unsigned int current_animation_= 0u;
	unsigned int current_animation_frame_= 0u;
	float switch_stage_= 1.0f;
};

} // namespace PanzerChasm
