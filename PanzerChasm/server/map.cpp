#include <matrix.hpp>

#include "../math_utils.hpp"

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

void Map::ProcessPlayerPosition(
	const Time current_time,
	Player& player,
	MessagesSender& messages_sender )
{
	const unsigned int x= player.MapPositionX();
	const unsigned int y= player.MapPositionY();
	if( x >= MapData::c_map_size ||
		y >= MapData::c_map_size )
		return;

	const MapData::Link& link= map_data_->links[ x + y * MapData::c_map_size ];

	// TODO - process other links types
	if( link.type == MapData::Link::Floor )
	{
		PC_ASSERT( link.proc_id < procedures_.size() );

		const MapData::Procedure& procedure= map_data_->procedures[ link.proc_id ];
		ProcedureState& procedure_state= procedures_[ link.proc_id ];

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
		if( player.MapPositionIsNew() &&
			procedure.first_message_number != 0u &&
			!procedure_state.first_message_printed )
		{
			procedure_state.first_message_printed= true;

			Messages::TextMessage text_message;
			text_message.message_id= MessageId::TextMessage;
			text_message.text_message_number= procedure.first_message_number;
			messages_sender.SendUnreliableMessage( text_message );
		}
		if( player.MapPositionIsNew() &&
			procedure.lock_message_number != 0u &&
			( procedure_state.locked || !have_necessary_keys ) )
		{
			Messages::TextMessage text_message;
			text_message.message_id= MessageId::TextMessage;
			text_message.text_message_number= procedure.lock_message_number;
			messages_sender.SendUnreliableMessage( text_message );
		}
		if( player.MapPositionIsNew() &&
			procedure.on_message_number != 0u )
		{
			Messages::TextMessage text_message;
			text_message.message_id= MessageId::TextMessage;
			text_message.text_message_number= procedure.on_message_number;
			messages_sender.SendUnreliableMessage( text_message );
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

			const unsigned int animation_frame_count= map_data_->models[ model.model_id ].frame_count;

			model.current_animation_frame=
				static_cast<unsigned int>( std::round(animation_frame) ) % animation_frame_count;
		}
		else
			model.current_animation_frame= model.animation_start_frame;
	} // for static models
}

void Map::SendUpdateMessages( MessagesSender& messages_sender ) const
{
	Messages::WallPosition wall_message;
	wall_message.message_id= MessageId::WallPosition;

	for( const DynamicWall& wall : dynamic_walls_ )
	{
		wall_message.wall_index= &wall - dynamic_walls_.data();

		wall_message.vertices_xy[0][0]= short( wall.vert_pos[0].x * 256.0f );
		wall_message.vertices_xy[0][1]= short( wall.vert_pos[0].y * 256.0f );
		wall_message.vertices_xy[1][0]= short( wall.vert_pos[1].x * 256.0f );
		wall_message.vertices_xy[1][1]= short( wall.vert_pos[1].y * 256.0f );

		wall_message.z= short( wall.z * 256.0f );

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
		if( model.destroyed )
			model_message.model_id++;

		for( unsigned int j= 0u; j < 3u; j++ )
			model_message.xyz[j]= short( model.pos.ToArr()[j] * 256.0f );

		model_message.angle= static_cast<unsigned short>( 65536.0f * model.angle / Constants::two_pi );

		messages_sender.SendUnreliableMessage( model_message );
	}
}

} // PanzerChasm
