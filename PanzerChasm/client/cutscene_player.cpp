#include "../i_menu_drawer.hpp"
#include "../i_text_drawer.hpp"
#include "../map_loader.hpp"
#include "../shared_drawers.hpp"
#include "i_map_drawer.hpp"
#include "map_state.hpp"
#include "movement_controller.hpp"

#include "cutscene_player.hpp"

namespace PanzerChasm
{


CutscenePlayer::CutscenePlayer(
	const GameResourcesConstPtr& game_resoruces,
	MapLoader& map_loader,
	const SharedDrawersPtr& shared_drawers,
	IMapDrawer& map_drawer,
	MovementController& movement_controller,
	const unsigned int map_number )
	: shared_drawers_(shared_drawers)
	, map_drawer_(map_drawer)
	, camera_controller_(movement_controller)
{
	const Vfs& vfs= *game_resoruces->vfs;

	MapDataConstPtr map_data= map_loader.LoadMap( 99u );
	if( map_data == nullptr )
		return;

	script_= LoadCutsceneScript( vfs, map_number );
	if( script_ == nullptr )
		return;

	// Create old map data from new map data.
	// Add to map characters as map models.
	const MapDataPtr map_data_patched= std::make_shared<MapData>( *map_data );
	map_data= nullptr; // destroy old map data

	// Search room.
	for( const MapData::Monster& monster : map_data_patched->monsters )
	{
		if( monster.monster_id == 0u )
		{
			if( monster.difficulty_flags == script_->room_number )
			{
				room_pos_= m_Vec3( monster.pos, 0.0f );
				room_angle_= monster.angle - Constants::half_pi;
				break;
			}
		}
	}
	m_Mat4 rotation_mat, translate_mat, room_mat;
	rotation_mat.RotateZ( room_angle_ );
	translate_mat.Translate( room_pos_ );
	room_mat= rotation_mat * translate_mat;

	// Load characers.
	first_character_static_model_index_= map_data_patched->static_models.size();
	for( const CutsceneScript::Character& character : script_->characters )
	{
		const unsigned int model_id= map_data_patched->models.size();
		map_data_patched->models.emplace_back();
		map_data_patched->models_description.emplace_back();
		Model& model= map_data_patched->models.back();
		// MapData::ModelDescription& description= map_data_patched->models_description.back();

		{ // Load model and animations.

			const Vfs::FileContent model_content= vfs.ReadFile( character.model_file_name );
			if( !model_content.empty() )
			{
				std::vector<Vfs::FileContent> animations_content;
				// Animations.
				for( unsigned int a= 0u; a < CutsceneScript::c_max_character_animations; a++ )
				{
					if( character.animations_file_name[a][0] == '\0' )
						continue;
					animations_content.emplace_back();
					vfs.ReadFile( character.animations_file_name[a], animations_content.back() );
				}
				// Idle animation.
				animations_content.emplace_back();
				vfs.ReadFile( character.idle_animation_file_name, animations_content.back() );

				LoadModel_o3(
					model_content,
					animations_content.data(), animations_content.size(),
					model );

				const int c_inv_models_scale= 4;
				for( Model::AnimationVertex& v : model.animations_vertices )
				{
					v.pos[0] /= c_inv_models_scale;
					v.pos[1] /= c_inv_models_scale;
					v.pos[2] /= c_inv_models_scale;
				}
				for( m_BBox3& bbox : model.animations_bboxes )
				{
					bbox.min/= float(c_inv_models_scale);
					bbox.max/= float(c_inv_models_scale);
				}
			}
		}

		map_data_patched->static_models.emplace_back();
		MapData::StaticModel& static_model= map_data_patched->static_models.back();

		static_model.pos= ( character.pos * room_mat ).xy();
		static_model.angle= character.angle + room_angle_;
		static_model.difficulty_flags= 0u;
		static_model.is_dynamic= false;
		static_model.model_id= model_id;
	}

	cutscene_map_data_= map_data_patched;
	map_state_.reset( new MapState( cutscene_map_data_, game_resoruces, Time::CurrentTime() ) );
	map_drawer.SetMap( cutscene_map_data_ );

	// Set characters z.
	for( const CutsceneScript::Character& character : script_->characters )
	{
		const unsigned int index= first_character_static_model_index_ + ( &character - script_->characters.data() );
		const MapData::StaticModel& static_model= cutscene_map_data_->static_models[ index ];
		Messages::StaticModelState message;

		PositionToMessagePosition( static_model.pos, message.xyz );
		message.xyz[2]= CoordToMessageCoord( character.pos.z );
		message.angle= AngleToMessageAngle( static_model.angle );
		message.model_id= static_model.model_id;
		message.static_model_index= index;
		message.animation_playing= true;
		message.visible= true;

		const Model& model= cutscene_map_data_->models[ static_model.model_id ];
		if( !model.animations.empty() )
			message.animation_frame= model.animations.back().first_frame;
		else
			message.animation_frame= 0u;

		map_state_->ProcessMessage( message );
	}

	// Set characters.
	const Time current_time= Time::CurrentTime();
	characters_.resize( script_->characters.size() );
	for( unsigned int c= 0u; c < characters_.size(); c++ )
	{
		const MapData::StaticModel& static_model= cutscene_map_data_->static_models[ first_character_static_model_index_ + c ];
		const Model& model= cutscene_map_data_->models[ static_model.model_id ];

		characters_[c].current_animation_start_time= current_time;
		characters_[c].current_animation_number= model.animations.empty() ? 0u : model.animations.size() - 1u;
	}

	// Set initial camera position.
	camera_pos_= script_->camera_params.pos * rotation_mat;
	camara_angles_.z= script_->camera_params.angle - Constants::half_pi - 0.5f;
}

CutscenePlayer::~CutscenePlayer()
{
}

void CutscenePlayer::Process( const SystemEvents& events )
{
	if( script_ == nullptr )
		return;
	if( IsFinished() )
		return;

	using CommandType= CutsceneScript::ActionCommand::Type;

	// Count key presses.
	unsigned int key_press_events= 0u;
	for( const SystemEvent& event : events )
	{
		if( event.type == SystemEvent::Type::Key &&
			event.event.key.pressed &&
			event.event.key.key_code == SystemEvent::KeyEvent::KeyCode::Space )
			key_press_events++;
	}

	const Time current_time= Time::CurrentTime();

	bool continue_action= false;
	if( current_countinuous_command_start_time_.ToSeconds() != 0.0f )
	{
		const Time time_since_last_continuous_action= current_time - current_countinuous_command_start_time_;

		PC_ASSERT( next_action_index_ > 0u );
		const CutsceneScript::ActionCommand& command= script_->commands[ next_action_index_ - 1u ];

		switch( command.type )
		{
		case CommandType::None:
		case CommandType::Say:
		case CommandType::Voice:
		case CommandType::WaitKey:
			PC_ASSERT(false);
			break;

		case CommandType::Delay:
			{
				const float delay_time= std::atof( command.params[0].c_str() );
				if( delay_time < time_since_last_continuous_action.ToSeconds() )
					continue_action= true;
			}
			break;

		case CommandType::Setani:
			{
				const unsigned int character_number= std::atoi( command.params[0].c_str() );
				const unsigned int animation_number= std::atoi( command.params[1].c_str() );
				const MapData::StaticModel& static_model= cutscene_map_data_->static_models[ first_character_static_model_index_ + character_number ];
				const Model& model= cutscene_map_data_->models[ static_model.model_id ];
				PC_ASSERT( !model.animations.empty() );

				const unsigned int frame= time_since_last_continuous_action.ToSeconds() * GameConstants::animations_frames_per_second;
				if( frame < model.animations[ animation_number ].frame_count )
					continue_action= true;
				else
				{
					// Switch to idle.
					characters_[ character_number ].current_animation_number= model.animations.size() - 1u;
					characters_[ character_number ].current_animation_start_time= current_time;
				}
			}
			break;

		case CommandType::MoveCamTo:
			{
				const Time move_time= Time::FromSeconds( std::atof( command.params[4].c_str() ) );
				if( time_since_last_continuous_action < move_time )
					continue_action= true;
			}
			break;
		};
	}

	if( !continue_action )
		current_countinuous_command_start_time_= Time::FromSeconds(0);

	while( !continue_action && next_action_index_ < script_->commands.size() )
	{
		const CutsceneScript::ActionCommand& command= script_->commands[ next_action_index_ ];

		Time command_time= Time::FromSeconds(0);

		switch( command.type )
		{
		case CommandType::None:
			break;
		case CommandType::Delay:
			command_time= Time::FromSeconds( std::atof( command.params[0].c_str() ) );
			break;
		case CommandType::Say:
			{
				say_lines_[ next_say_line_ ]= command.params[0];
				next_say_line_= ( next_say_line_ + 1u ) % c_say_lines;
			}
			break;
		case CommandType::Voice:
			break;

		case CommandType::Setani:
			{
				const unsigned int character_number= std::atoi( command.params[0].c_str() );
				const unsigned int animation_number= std::atoi( command.params[1].c_str() );
				const MapData::StaticModel& static_model= cutscene_map_data_->static_models[ first_character_static_model_index_ + character_number ];
				const Model& model= cutscene_map_data_->models[ static_model.model_id ];
				if( !model.animations.empty() )
				{
					command_time=
						Time::FromSeconds(
							float(model.animations[ animation_number ].frame_count) /
							GameConstants::animations_frames_per_second );

					characters_[ character_number ].current_animation_number= animation_number;
					characters_[ character_number ].current_animation_start_time= current_time;
				}
			}
			break;

		case CommandType::WaitKey:
			{
				if( key_press_events == 0u )
					goto update_models;
				else
					--key_press_events;
			}
			break;

		case CommandType::MoveCamTo:
			command_time= Time::FromSeconds( std::atof( command.params[4].c_str() ) );
			break;
		};

		next_action_index_++;
		if( command_time.ToSeconds() > 0.0f )
		{
			current_countinuous_command_start_time_= current_time;
			break;
		}
	}

update_models:
	// Update characters
	for( CharacterState& character : characters_ )
	{
		const unsigned int i= &character - characters_.data();
		const unsigned int index= first_character_static_model_index_ + i;
		const MapData::StaticModel& static_model= cutscene_map_data_->static_models[ index ];
		const Model& model= cutscene_map_data_->models[ static_model.model_id ];
		Messages::StaticModelState message;

		PositionToMessagePosition( static_model.pos, message.xyz );
		message.xyz[2]= CoordToMessageCoord( script_->characters[i].pos.z );
		message.angle= AngleToMessageAngle( static_model.angle );
		message.model_id= static_model.model_id;
		message.static_model_index= index;
		message.animation_playing= true;
		message.visible= true;

		if( !model.animations.empty() )
		{
			const Time time_delta= current_time - character.current_animation_start_time;
			const unsigned int animation_frame=
				static_cast<unsigned int>( time_delta.ToSeconds() * GameConstants::animations_frames_per_second ) %
				model.animations[ character.current_animation_number ].frame_count;

			message.animation_frame= model.animations[ character.current_animation_number ].first_frame + animation_frame;
		}
		else
			message.animation_frame= 0u;

		map_state_->ProcessMessage( message );
	}
}

void CutscenePlayer::Draw()
{
	const m_Vec3 cam_pos=
		room_pos_ +
		camera_pos_
		+ 0.3f * m_Vec3( -std::sin( camera_controller_.GetViewAngleZ()), std::cos(camera_controller_.GetViewAngleZ()), 0.0f );

//	camera_controller_.SetAngles( camara_angles_.z, 0.0f );

	m_Mat4 view_rotation_and_projection_matrix;
	camera_controller_.GetViewRotationAndProjectionMatrix( view_rotation_and_projection_matrix );

	ViewClipPlanes view_clip_planes;
	camera_controller_.GetViewClipPlanes( cam_pos, view_clip_planes );

	map_drawer_.Draw(
		*map_state_,
		view_rotation_and_projection_matrix,
		cam_pos,
		view_clip_planes,
		0u );

	// Draw text.
	// TODO - draw brifbar.cel.
	ITextDrawer& text_drawer= *shared_drawers_->text;
	const unsigned int text_scale= shared_drawers_->menu->GetMenuScale();
	const Size2 viewport_size= shared_drawers_->menu->GetViewportSize();
	const unsigned int line_height= text_drawer.GetLineHeight();
	const unsigned int start_y= viewport_size.Height() - c_say_lines * line_height * text_scale;
	for( unsigned int s= 0u; s < c_say_lines; s++ )
	{
		text_drawer.Print(
			0,
			start_y + s * ( text_scale * line_height ),
			say_lines_[s].c_str(),
			text_scale );
	}
}

bool CutscenePlayer::IsFinished() const
{
	if( script_ == nullptr )
		return true;

	return next_action_index_ >= script_->commands.size();
}

} // namespace PanzerChasm
