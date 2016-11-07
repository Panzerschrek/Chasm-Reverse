#include <matrix.hpp>

#include "../math_utils.hpp"
#include "a_code.hpp"
#include "collisions.hpp"

#include "map.hpp"

namespace PanzerChasm
{

static const float g_commands_coords_scale= 1.0f / 256.0f;

static const float g_animations_frames_per_second= 20.0f;

Map::Map(
	const MapDataConstPtr& map_data,
	const Time map_start_time )
	: map_data_(map_data)
{
	PC_ASSERT( map_data_ );

	procedures_.resize( map_data_->procedures.size() );
	for( unsigned int p= 0u; p < procedures_.size(); p++ )
	{
		if( map_data_->procedures[p].locked )
			procedures_[p].locked= true;
	}

	dynamic_walls_.resize( map_data_->dynamic_walls.size() );

	static_models_.resize( map_data_->static_models.size() );
	for( unsigned int m= 0u; m < static_models_.size(); m++ )
	{
		const MapData::StaticModel& in_model= map_data_->static_models[m];
		StaticModel& out_model= static_models_[m];

		out_model.model_id= in_model.model_id;

		out_model.pos= m_Vec3( in_model.pos, 0.0f );
		out_model.angle= in_model.angle;

		out_model.animation_state= StaticModel::AnimationState::Animation;
		out_model.animation_start_time= map_start_time;
		out_model.animation_start_frame= 0u;
	}
}

Map::~Map()
{}

void Map::Shoot( const m_Vec3& from, const m_Vec3& normalized_direction )
{
	shots_.emplace_back();
	Shot& shot= shots_.back();
	shot.from= from;
	shot.normalized_direction= normalized_direction;
}

void Map::ProcessPlayerPosition(
	const Time current_time,
	Player& player,
	MessagesSender& messages_sender )
{
	// TODO - read from config
	const float c_player_radius= 0.4f;
	const float c_player_height= 1.0f;
	const float c_wall_height= 2.0f;

	const unsigned int player_x= player.MapPositionX();
	const unsigned int player_y= player.MapPositionY();
	if( player_x >= MapData::c_map_size ||
		player_y >= MapData::c_map_size )
		return;

	// Process floors
	for( int x= std::max( 0, int(player_x) - 2); x < std::min( int(MapData::c_map_size), int(player_x) + 2 ); x++ )
	for( int y= std::max( 0, int(player_y) - 2); y < std::min( int(MapData::c_map_size), int(player_y) + 2 ); y++ )
	{
		// TODO - select correct player radius for floor collisions.
		if( !CircleIntersectsWithSquare(
				player.Position().xy(), c_player_radius * 0.5f, x, y ) )
			continue;

		const MapData::Link& link= map_data_->links[ x + y * MapData::c_map_size ];

		// TODO - process other links types
		if( link.type == MapData::Link::Floor )
		{
			TryActivateProcedure( link.proc_id, current_time, player, messages_sender );
		}
	}

	// Collide
	m_Vec2 pos= player.Position().xy();

	const float z_bottom= player.Position().z;
	const float z_top= z_bottom + c_player_height;

	// Static walls
	for( const MapData::Wall& wall : map_data_->static_walls )
	{
		if( wall.vert_pos[0] == wall.vert_pos[1] )
			continue;

		const MapData::WallTextureDescription& tex= map_data_->walls_textures[ wall.texture_id ];
		if( tex.file_name[0] == '\0' )
			continue;

		if( tex.gso[0] || tex.gso[2] )
			continue;

		m_Vec2 new_pos;
		if( CollideCircleWithLineSegment(
				wall.vert_pos[0], wall.vert_pos[1],
				pos, c_player_radius,
				new_pos ) )
		{
			pos= new_pos;
			if( wall.link.type == MapData::Link::Link_ )
				TryActivateProcedure( wall.link.proc_id, current_time, player, messages_sender );
		}
	}

	// Dynamic walls
	for( unsigned int w= 0u; w < dynamic_walls_.size(); w++ )
	{
		const DynamicWall& wall= dynamic_walls_[w];
		const MapData::Wall& map_wall= map_data_->dynamic_walls[w];

		if( wall.vert_pos[0] == wall.vert_pos[1] )
			continue;

		if( map_data_->walls_textures[ map_wall.texture_id ].file_name[0] == '\0' )
			continue;

		if( z_top < wall.z || z_bottom > wall.z + c_wall_height )
			continue;

		m_Vec2 new_pos;
		if( CollideCircleWithLineSegment(
				wall.vert_pos[0], wall.vert_pos[1],
				pos, c_player_radius,
				new_pos ) )
			pos= new_pos;
	}

	// Models
	for( unsigned int m= 0u; m < static_models_.size(); m++ )
	{
		const StaticModel& model= static_models_[m];
		if( model.model_id >= map_data_->models_description.size() )
			continue;

		const MapData::ModelDescription& model_description= map_data_->models_description[ model.model_id ];

		if( model_description.radius <= 0.0f )
			continue;

		const MapData::StaticModel& map_model= map_data_->static_models[m];
		const Model& model_geometry= map_data_->models[ model.model_id ];

		if( z_top < model_geometry.z_min || z_bottom > model_geometry.z_max )
			continue;

		const float min_distance= c_player_radius + model_description.radius;

		const m_Vec2 vec_to_player_pos= pos - model.pos.xy();
		const float square_distance= vec_to_player_pos.SquareLength();

		if( square_distance < min_distance * min_distance )
		{
			pos= model.pos.xy() + vec_to_player_pos * ( min_distance / std::sqrt( square_distance ) );

			if( map_model.link.type == MapData::Link::Link_ ||
				map_model.link.type == MapData::Link::OnOffLink )
				TryActivateProcedure( map_model.link.proc_id, current_time, player, messages_sender );
		}
	}

	const float new_player_z= std::max( 0.0f, player.Position().z );

	// Set position after collisions
	player.SetPosition( m_Vec3( pos, new_player_z ) );
	player.ResetNewPositionFlag();

	// Process "special" models.
	// Pick-up keys.
	for( unsigned int m= 0u; m < static_models_.size(); m++ )
	{
		StaticModel& model= static_models_[m];
		const MapData::StaticModel& map_model= map_data_->static_models[m];

		if( model.pos.z < 0.0f ||
			map_model.model_id >= map_data_->models_description.size() )
			continue;

		const MapData::ModelDescription& model_description= map_data_->models_description[ map_model.model_id ];

		const ACode a_code= static_cast<ACode>( model_description.ac );
		if( a_code >= ACode::RedKey && a_code <= ACode::BlueKey )
		{
			const m_Vec2 vec_to_player_pos= pos - model.pos.xy();
			const float square_distance= vec_to_player_pos.SquareLength();
			if( square_distance <= c_player_radius * c_player_radius )
			{
				model.pos.z= -2.0f; // HACK. TODO - hide models
				if( a_code == ACode::RedKey )
					player.GiveRedKey();
				if( a_code == ACode::GreenKey )
					player.GiveGreenKey();
				if( a_code == ACode::BlueKey )
					player.GiveBlueKey();
			}
		}
	}
}

void Map::Tick( const Time current_time )
{
	// Update state of procedures
	for( unsigned int p= 0u; p < procedures_.size(); p++ )
	{
		const MapData::Procedure& procedure= map_data_->procedures[p];
		ProcedureState& procedure_state= procedures_[p];

		if( procedure.speed <= 0.0f )
		{
			// Reset procedures without speed
			procedure_state.movement_state= ProcedureState::MovementState::None;
			procedure_state.last_state_change_time= current_time;
			procedure_state.movement_stage= 0.0f;
			continue;
		}

		const Time time_since_last_state_change= current_time - procedure_state.last_state_change_time;
		const float new_stage= time_since_last_state_change.ToSeconds() * procedure.speed / 10.0f;

		switch( procedure_state.movement_state )
		{
		case ProcedureState::MovementState::None:
			break;

		case ProcedureState::MovementState::Movement:
			if( new_stage >= 1.0f )
			{
				procedure_state.movement_state= ProcedureState::MovementState::BackWait;
				procedure_state.movement_stage= 0.0f;
				procedure_state.last_state_change_time= current_time;
			}
			else
				procedure_state.movement_stage= new_stage;
			break;

		case ProcedureState::MovementState::BackWait:
		{
			const Time wait_time= current_time - procedure_state.last_state_change_time;
			if(
				procedure.back_wait_s > 0.0f &&
				wait_time.ToSeconds() >= procedure.back_wait_s )
			{
				procedure_state.movement_state= ProcedureState::MovementState::ReverseMovement;
				procedure_state.movement_stage= 0.0f;
				procedure_state.last_state_change_time= current_time;
			}
		}
			break;

		case ProcedureState::MovementState::ReverseMovement:
			if( new_stage >= 1.0f )
			{
				procedure_state.movement_state= ProcedureState::MovementState::None;
				procedure_state.movement_stage= 0.0f;
				procedure_state.last_state_change_time= current_time;
			}
			else
				procedure_state.movement_stage= new_stage;
			break;
		}; // switch state


		// Select positions, using movement state.

		float absolute_action_stage;
		if( procedure_state.movement_state == ProcedureState::MovementState::Movement )
			absolute_action_stage= procedure_state.movement_stage;
		else if( procedure_state.movement_state == ProcedureState::MovementState::BackWait )
			absolute_action_stage= 1.0f;
		else if( procedure_state.movement_state == ProcedureState::MovementState::ReverseMovement )
			absolute_action_stage= 1.0f - procedure_state.movement_stage;
		else
			absolute_action_stage= 0.0f;

		for( const MapData::Procedure::ActionCommand& command : procedure.action_commands )
		{
			using Action= MapData::Procedure::ActionCommandId;
			switch( command.id )
			{
			case Action::Move:

			case Action::XMove: // Xmove and YMove commands looks just like move command alias.
			case Action::YMove:
			{
				const unsigned char x= static_cast<unsigned char>(command.args[0]);
				const unsigned char y= static_cast<unsigned char>(command.args[1]);
				const float dx= command.args[2] * g_commands_coords_scale;
				const float dy= command.args[3] * g_commands_coords_scale;
				const float sound_number= command.args[4];
				PC_UNUSED(sound_number);

				PC_ASSERT( x < MapData::c_map_size && y < MapData::c_map_size );
				const MapData::IndexElement& index_element= map_data_->map_index[ x + y * MapData::c_map_size ];

				if( index_element.type == MapData::IndexElement::DynamicWall )
				{
					PC_ASSERT( index_element.index < map_data_->dynamic_walls.size() );

					const MapData::Wall& map_wall= map_data_->dynamic_walls[ index_element.index ];
					DynamicWall& wall= dynamic_walls_[ index_element.index ];

					for( unsigned int v= 0u; v < 2u; v++ )
					{
						wall.vert_pos[v]= map_wall.vert_pos[v];
						wall.vert_pos[v].x+= dx * absolute_action_stage;
						wall.vert_pos[v].y+= dy * absolute_action_stage;
					}
					wall.z= 0.0f;
				}
				else if( index_element.type == MapData::IndexElement::StaticModel )
				{
					PC_ASSERT( index_element.index < static_models_.size() );
					const MapData::StaticModel& map_model= map_data_->static_models[ index_element.index ];
					StaticModel& model= static_models_[ index_element.index ];

					model.pos=
						m_Vec3(
							map_model.pos + m_Vec2( dx, dy ) * absolute_action_stage,
							0.0f );
				}
			}
				break;

			case Action::Rotate:
			{
				const unsigned char x= static_cast<unsigned char>(command.args[0]);
				const unsigned char y= static_cast<unsigned char>(command.args[1]);
				const float center_x= command.args[2] * g_commands_coords_scale;
				const float center_y= command.args[3] * g_commands_coords_scale;
				const float angle= command.args[4] * Constants::to_rad;
				const float sound_number= command.args[5];
				PC_UNUSED(sound_number);

				PC_ASSERT( x < MapData::c_map_size && y < MapData::c_map_size );

				const MapData::IndexElement& index_element= map_data_->map_index[ x + y * MapData::c_map_size ];

				if( index_element.type == MapData::IndexElement::DynamicWall )
				{
					PC_ASSERT( index_element.index < map_data_->dynamic_walls.size() );

					const MapData::Wall& map_wall= map_data_->dynamic_walls[ index_element.index ];
					DynamicWall& wall= dynamic_walls_[ index_element.index ];

					m_Mat3 rot_mat;
					rot_mat.RotateZ( angle * absolute_action_stage );
					const m_Vec2 center( center_x, center_y );

					for( unsigned int v= 0u; v < 2u; v++ )
					{
						const m_Vec2 vec= map_wall.vert_pos[v] - center;
						const m_Vec2 vec_rotated= ( m_Vec3( vec, 0.0f ) * rot_mat ).xy();
						wall.vert_pos[v]= center + vec_rotated;
					}
					wall.z= 0.0f;
				}
				else if( index_element.type == MapData::IndexElement::StaticModel )
				{
					PC_ASSERT( index_element.index < static_models_.size() );
					const MapData::StaticModel& map_model= map_data_->static_models[ index_element.index ];
					StaticModel& model= static_models_[ index_element.index ];

					model.angle= map_model.angle + angle * absolute_action_stage;
				}
			}
				break;

			case Action::Up:
			{
				const unsigned char x= static_cast<unsigned char>(command.args[0]);
				const unsigned char y= static_cast<unsigned char>(command.args[1]);
				const float height= command.args[2] * g_commands_coords_scale * 4.0f;
				const float sound_number= command.args[3];
				PC_UNUSED(sound_number);

				PC_ASSERT( x < MapData::c_map_size && y < MapData::c_map_size );
				const MapData::IndexElement& index_element= map_data_->map_index[ x + y * MapData::c_map_size ];

				if( index_element.type == MapData::IndexElement::DynamicWall )
				{
					PC_ASSERT( index_element.index < map_data_->dynamic_walls.size() );

					const MapData::Wall& map_wall= map_data_->dynamic_walls[ index_element.index ];
					DynamicWall& wall= dynamic_walls_[ index_element.index ];

					for( unsigned int v= 0u; v < 2u; v++ )
						wall.vert_pos[v]= map_wall.vert_pos[v];
					wall.z= height * absolute_action_stage;
				}
				else if( index_element.type == MapData::IndexElement::StaticModel )
				{
					PC_ASSERT( index_element.index < static_models_.size() );
					const MapData::StaticModel& map_model= map_data_->static_models[ index_element.index ];
					StaticModel& model= static_models_[ index_element.index ];

					model.pos= m_Vec3( map_model.pos, height );
				}
			}
				break;

			default:
				// TODO
				break;
			}
		} // for action commands

	} // for procedures

	// Process static models
	for( StaticModel& model : static_models_ )
	{
		if( model.animation_state == StaticModel::AnimationState::Animation )
		{
			const float time_delta_s= ( current_time - model.animation_start_time ).ToSeconds();
			const float animation_frame= time_delta_s * g_animations_frames_per_second;

			if( model.model_id < map_data_->models.size() )
			{
				const unsigned int animation_frame_count= map_data_->models[ model.model_id ].frame_count;

				model.current_animation_frame=
					static_cast<unsigned int>( std::round(animation_frame) ) % animation_frame_count;
			}
			else
				model.current_animation_frame= 0u;
		}
		else
			model.current_animation_frame= model.animation_start_frame;
	} // for static models

	// Sprite effects
	sprite_effects_.clear();

	// Process shots
	for( const Shot& shot : shots_ )
	{
		enum class ObjectType
		{
			None, StaticWall, DynamicWall, Model, Floor,
		};

		float nearest_shot_point_square_distance= Constants::max_float;
		m_Vec3 nearest_shot_pos;
		ObjectType hited_object_type= ObjectType::None;
		unsigned int hited_object_index= ~0u;

		const auto process_candidate_shot_pos=
		[&]( const m_Vec3& candidate_pos, const ObjectType object_type, const unsigned object_index )
		{
			const float square_distance= ( candidate_pos - shot.from ).SquareLength();
			if( square_distance < nearest_shot_point_square_distance )
			{
				nearest_shot_pos= candidate_pos;
				nearest_shot_point_square_distance= square_distance;

				hited_object_type= object_type;
				hited_object_index= object_index;
			}
		};

		for( const MapData::Wall& wall : map_data_->static_walls )
		{
			if( map_data_->walls_textures[ wall.texture_id ].file_name[0] == '\0' )
				continue;

			m_Vec3 candidate_pos;
			if( RayIntersectWall(
					wall.vert_pos[0], wall.vert_pos[1],
					0.0f, 2.0f,
					shot.from, shot.normalized_direction,
					candidate_pos ) )
			{
				process_candidate_shot_pos( candidate_pos, ObjectType::StaticWall, &wall - map_data_->static_walls.data() );
			}
		}

		for( unsigned int w= 0u; w < dynamic_walls_.size(); w++ )
		{
			if( map_data_->walls_textures[ map_data_->dynamic_walls[w].texture_id ].file_name[0] == '\0' )
				continue;

			const DynamicWall& wall= dynamic_walls_[w];

			m_Vec3 candidate_pos;
			if( RayIntersectWall(
					wall.vert_pos[0], wall.vert_pos[1],
					wall.z, wall.z + 2.0f,
					shot.from, shot.normalized_direction,
					candidate_pos ) )
			{
				process_candidate_shot_pos( candidate_pos, ObjectType::DynamicWall, w );
			}
		}

		for( const StaticModel& model : static_models_ )
		{
			if( model.model_id >= map_data_->models_description.size() )
				continue;

			const MapData::ModelDescription& model_description= map_data_->models_description[ model.model_id ];
			if( model_description.radius <= 0.0f )
				continue;

			const Model& model_data= map_data_->models[ model.model_id ];

			m_Vec3 candidate_pos;
			if( RayIntersectCylinder(
					model.pos.xy(), model_description.radius,
					model_data.z_min + model.pos.z,
					model_data.z_max + model.pos.z,
					shot.from, shot.normalized_direction,
					candidate_pos ) )
			{
				process_candidate_shot_pos( candidate_pos, ObjectType::Model, &model - static_models_.data() );
			}
		}

		for( unsigned int z= 0u; z <= 2u; z+= 2u )
		{
			m_Vec3 candidate_pos;
			if( RayIntersectXYPlane(
					float(z),
					shot.from, shot.normalized_direction,
					candidate_pos ) )
			{
				const int x= static_cast<int>( std::floor(candidate_pos.x) );
				const int y= static_cast<int>( std::floor(candidate_pos.y) );
				if( x < 0 || x >= int(MapData::c_map_size) ||
					y < 0 || y >= int(MapData::c_map_size) )
					continue;

				const int coord= x + y * int(MapData::c_map_size);
				const unsigned char texture_id=
					( z == 0 ? map_data_->floor_textures : map_data_->ceiling_textures )[ coord ];

				if( texture_id == MapData::c_empty_floor_texture_id ||
					texture_id == MapData::c_sky_floor_texture_id )
					continue;

				process_candidate_shot_pos( candidate_pos, ObjectType::Floor, z >> 1u );
			}
		}

		if( hited_object_type != ObjectType::None )
		{
			sprite_effects_.emplace_back();
			SpriteEffect& effect= sprite_effects_.back();

			effect.pos= nearest_shot_pos;
			effect.effect_id= 8u;
		}

		// Try break breakable models.
		// TODO - process break event, check break limit.
		if( hited_object_type == ObjectType::Model )
		{
			StaticModel& model= static_models_[ hited_object_index ];

			if( model.model_id >= map_data_->models_description.size() )
				continue;

			const MapData::ModelDescription& model_description= map_data_->models_description[ model.model_id ];
			if( model_description.break_limit <= 0 )
				continue;

			model.model_id++;
		}
	}
	shots_.clear();
}

void Map::SendUpdateMessages( MessagesSender& messages_sender ) const
{
	Messages::WallPosition wall_message;
	wall_message.message_id= MessageId::WallPosition;

	for( const DynamicWall& wall : dynamic_walls_ )
	{
		wall_message.wall_index= &wall - dynamic_walls_.data();

		PositionToMessagePosition( wall.vert_pos[0], wall_message.vertices_xy[0] );
		PositionToMessagePosition( wall.vert_pos[1], wall_message.vertices_xy[1] );
		wall_message.z= CoordToMessageCoord( wall.z );

		messages_sender.SendUnreliableMessage( wall_message );
	}

	Messages::StaticModelState model_message;
	model_message.message_id= MessageId::StaticModelState;

	for( unsigned int m= 0u; m < static_models_.size(); m++ )
	{
		const StaticModel& model= static_models_[m];

		model_message.static_model_index= m;
		model_message.animation_frame= model.current_animation_frame;
		model_message.animation_playing= model.animation_state == StaticModel::AnimationState::Animation;
		model_message.model_id= model.model_id;

		PositionToMessagePosition( model.pos, model_message.xyz );
		model_message.angle= AngleToMessageAngle( model.angle );

		messages_sender.SendUnreliableMessage( model_message );
	}

	Messages::SpriteEffectBirth sprite_message;
	sprite_message.message_id= MessageId::SpriteEffectBirth;

	for( const SpriteEffect& effect : sprite_effects_ )
	{
		sprite_message.effect_id= effect.effect_id;
		PositionToMessagePosition( effect.pos, sprite_message.xyz );

		messages_sender.SendUnreliableMessage( sprite_message );
	}
}

void Map::TryActivateProcedure(
	unsigned int procedure_number,
	const Time current_time,
	Player& player,
	MessagesSender& messages_sender )
{
	if( procedure_number == 0u )
		return;

	PC_ASSERT( procedure_number < procedures_.size() );

	const MapData::Procedure& procedure= map_data_->procedures[ procedure_number ];
	ProcedureState& procedure_state= procedures_[ procedure_number ];

	const bool have_necessary_keys=
		( !procedure.  red_key_required || player.HaveRedKey() ) &&
		( !procedure.green_key_required || player.HaveGreenKey() ) &&
		( !procedure. blue_key_required || player.HaveBlueKey() );

	if(
		have_necessary_keys &&
		!procedure_state.locked &&
		procedure_state.movement_state == ProcedureState::MovementState::None )
	{
		procedure_state.movement_stage= 0.0f;
		procedure_state.movement_state= ProcedureState::MovementState::Movement;
		procedure_state.last_state_change_time= current_time;

		// Do immediate commands
		for( const MapData::Procedure::ActionCommand& command : procedure.action_commands )
		{
			using Command= MapData::Procedure::ActionCommandId;
			if( command.id == Command::Lock )
			{
				const unsigned short proc_number= static_cast<unsigned short>( command.args[0] );
				PC_ASSERT( proc_number < procedures_.size() );

				procedures_[ proc_number ].locked= true;
			}
			else if( command.id == Command::Unlock )
			{
				const unsigned short proc_number= static_cast<unsigned short>( command.args[0] );
				PC_ASSERT( proc_number < procedures_.size() );

				procedures_[ proc_number ].locked= false;
			}
			// TODO - know, how animation commands works
			else if( command.id == Command::PlayAnimation )
			{}
			else if( command.id == Command::StopAnimation )
			{}
			// TODO - process other commands
			else
			{}
		}
	} // if activated

	// Activation messages.
	if( procedure.first_message_number != 0u &&
		!procedure_state.first_message_printed )
	{
		procedure_state.first_message_printed= true;

		Messages::TextMessage text_message;
		text_message.message_id= MessageId::TextMessage;
		text_message.text_message_number= procedure.first_message_number;
		messages_sender.SendUnreliableMessage( text_message );
	}
	if( procedure.lock_message_number != 0u &&
		( procedure_state.locked || !have_necessary_keys ) )
	{
		Messages::TextMessage text_message;
		text_message.message_id= MessageId::TextMessage;
		text_message.text_message_number= procedure.lock_message_number;
		messages_sender.SendUnreliableMessage( text_message );
	}
	if( procedure.on_message_number != 0u )
	{
		Messages::TextMessage text_message;
		text_message.message_id= MessageId::TextMessage;
		text_message.text_message_number= procedure.on_message_number;
		messages_sender.SendUnreliableMessage( text_message );
	}
}

} // PanzerChasm
