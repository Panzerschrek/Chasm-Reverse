#include "../i_menu_drawer.hpp"
#include "../i_text_drawer.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "../sound/sound_engine.hpp"
#include "../shared_drawers.hpp"
#include "map_state.hpp"

#include "cutscene_player.hpp"

namespace PanzerChasm
{

static const float g_cam_pos_scale= 0.5f;
static const float g_cam_shift= 0.4f;
static const float g_characters_cam_shift= -0.0f;

CutscenePlayer::CutscenePlayer(
	const GameResourcesConstPtr& game_resoruces,
	MapLoader& map_loader,
	const Sound::SoundEnginePtr& sound_engine,
	const SharedDrawersPtr& shared_drawers,
	IMapDrawer& map_drawer,
	const unsigned int map_number )
	: sound_engine_(sound_engine)
	, shared_drawers_(shared_drawers)
	, map_drawer_(map_drawer)
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
	m_Mat4 translate_mat, room_mat;
	room_rotation_matrix_.RotateZ( room_angle_ );
	translate_mat.Translate( room_pos_ );
	room_mat= room_rotation_matrix_ * translate_mat;

	// Load characers.
	first_character_model_index_= map_data_patched->models.size();
	for( const CutsceneScript::Character& character : script_->characters )
	{
		map_data_patched->models.emplace_back();
		map_data_patched->models_description.emplace_back();
		Model& model= map_data_patched->models.back();

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
	}

	cutscene_map_data_= map_data_patched;
	const Time current_time= Time::CurrentTime();
	map_state_.reset( new MapState( cutscene_map_data_, game_resoruces, current_time ) );
	map_drawer.SetMap( cutscene_map_data_ );

	// Set characters.
	characters_.resize( script_->characters.size() );
	for( unsigned int c= 0u; c < characters_.size(); c++ )
	{
		const Model& model= cutscene_map_data_->models[ first_character_model_index_ + c ];

		characters_[c].current_animation_start_time= current_time;
		characters_[c].current_animation_number= model.animations.empty() ? 0u : model.animations.size() - 1u;
	}

	// Set initial camera position.
	camera_current_.pos= script_->camera_params.pos * room_rotation_matrix_;
	camera_current_.angle_z= script_->camera_params.angle;
	camera_next_= camera_previous_= camera_current_;
}

CutscenePlayer::~CutscenePlayer()
{
}

void CutscenePlayer::Process( const SystemEvents& events )
{
	if( script_ == nullptr )
		return;

	const Time current_time= Time::CurrentTime();
	map_state_->Tick( current_time );

	if( IsFinished() )
		return;

	using CommandType= CutsceneScript::ActionCommand::Type;

	// Count key presses.
	unsigned int key_press_events= 0u;
	for( const SystemEvent& event : events )
	{
		if( event.type == SystemEvent::Type::Key &&
			event.event.key.pressed )
		{
			if( event.event.key.key_code == SystemEvent::KeyEvent::KeyCode::Escape )
			{
				// Stop cutscene on escape press.
				next_action_index_= script_->commands.size();
				return;
			}
			else
				key_press_events++;
		}
	}

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
				const Model& model= cutscene_map_data_->models[ first_character_model_index_ + character_number ];
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
				{
					continue_action= true;

					const float mix= time_since_last_continuous_action.ToSeconds() / move_time.ToSeconds();
					camera_current_.pos    = camera_previous_.pos     * (1.0f - mix ) + camera_next_.pos     * mix;
					camera_current_.angle_z= camera_previous_.angle_z * (1.0f - mix ) + camera_next_.angle_z * mix;
				}
				else
					camera_current_= camera_next_;
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
			if( sound_engine_ != nullptr )
				sound_engine_->PlayOneTimeSound( command.params[1].c_str() );
			break;

		case CommandType::Setani:
			{
				const unsigned int character_number= std::atoi( command.params[0].c_str() );
				const unsigned int animation_number= std::atoi( command.params[1].c_str() );
				const Model& model= cutscene_map_data_->models[ first_character_model_index_ + character_number ];
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
			{
				command_time= Time::FromSeconds( std::atof( command.params[4].c_str() ) );

				camera_previous_= camera_next_;

				m_Vec3 pos;
				pos.x= std::atof( command.params[0].c_str() ) / 64.0f;
				pos.y= std::atof( command.params[1].c_str() ) / 64.0f;
				pos.z= std::atof( command.params[2].c_str() ) / 64.0f;
				camera_next_.pos= pos * room_rotation_matrix_;

				camera_next_.angle_z= std::atof( command.params[3].c_str() ) * Constants::to_rad;
			}
			break;
		};

		next_action_index_++;
		if( command_time.ToSeconds() > 0.0f )
		{
			current_countinuous_command_start_time_= current_time;
			break;
		}
	}

update_models:;
}

void CutscenePlayer::Draw()
{
	const float c_world_fov_deg= 90.0f;
	const float c_characters_fov_deg= 52.0f;
	const float c_fov_scaler= 1.05f;
	const float c_z_near= 1.0f / 4.0f;
	const float c_z_far= 128.0f;
	const unsigned int c_briefbar_height= 54;

	const float z_angle= Constants::pi - camera_current_.angle_z;

	const Size2 viewport_size= shared_drawers_->menu->GetViewportSize();
	const float aspect= float(viewport_size.Width()) / float(viewport_size.Height());

	const unsigned int menu_scale= shared_drawers_->menu->GetMenuScale();
	const float realtive_screen_y_shift= float(c_briefbar_height * menu_scale ) / float(viewport_size.Height());

	m_Mat4 screen_shift_mat;
	screen_shift_mat.Identity();
	screen_shift_mat[13]= realtive_screen_y_shift;

	{ // World.
		const m_Vec3 cam_pos=
			room_pos_ +
			m_Vec3( camera_current_.pos.xy() * g_cam_pos_scale, camera_current_.pos.z )
			+ g_cam_shift * m_Vec3( -std::sin(z_angle), std::cos(z_angle), 0.0f );

		m_Mat4  rot_z, inv_rot_z, perspective, basis_change;
		rot_z.RotateZ( -z_angle );
		inv_rot_z.RotateZ( z_angle );
		basis_change.Identity();
		basis_change[5]= 0.0f;
		basis_change[6]= 1.0f;
		basis_change[9]= 1.0f;
		basis_change[10]= 0.0f;

		perspective.PerspectiveProjection( aspect, c_world_fov_deg * Constants::to_rad, c_z_near, c_z_far );
		const m_Mat4 view_rotation_and_projection_matrix= rot_z * basis_change * perspective * screen_shift_mat;

		ViewClipPlanes view_clip_planes;

		const float half_fov_x= std::atan( aspect * std::tan( c_world_fov_deg * ( Constants::to_rad * 0.5f ) ) ) * c_fov_scaler;
		const float half_fov_y= c_world_fov_deg * ( c_fov_scaler * Constants::to_rad * 0.5f );

		// Near clip plane
		view_clip_planes[0].normal= m_Vec3( 0.0f, 1.0f, 0.0f ) * inv_rot_z;
		view_clip_planes[0].dist= -( cam_pos * view_clip_planes[0].normal + c_z_near );
		// Left
		view_clip_planes[1].normal= m_Vec3( +std::cos( half_fov_x ), +std::sin( half_fov_x ), 0.0f ) * inv_rot_z;
		view_clip_planes[1].dist= -( cam_pos * view_clip_planes[1].normal );
		// Right
		view_clip_planes[2].normal= m_Vec3( -std::cos( half_fov_x ), +std::sin( half_fov_x ), 0.0f ) * inv_rot_z;
		view_clip_planes[2].dist= -( cam_pos * view_clip_planes[2].normal );

		const float shift= 0.0f; // TODO - set shift for better culling.
		// Bottom
		view_clip_planes[3].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ) - shift, +std::cos( half_fov_y ) );
		view_clip_planes[3].normal.Normalize();
		view_clip_planes[3].normal= view_clip_planes[3].normal * inv_rot_z;
		view_clip_planes[3].dist= -( cam_pos * view_clip_planes[3].normal );
		// Top
		view_clip_planes[4].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ) + shift, -std::cos( half_fov_y ) );
		view_clip_planes[4].normal.Normalize();
		view_clip_planes[4].normal= view_clip_planes[4].normal * inv_rot_z;
		view_clip_planes[4].dist= -( cam_pos * view_clip_planes[4].normal );

		map_drawer_.Draw(
			*map_state_,
			view_rotation_and_projection_matrix,
			cam_pos,
			view_clip_planes,
			0u );
	}
	{ // Characters.

		const m_Vec3 cam_pos=
			room_pos_ +
			m_Vec3( camera_current_.pos.xy() * g_cam_pos_scale, camera_current_.pos.z )
			+ g_characters_cam_shift * m_Vec3( -std::sin(z_angle), std::cos(z_angle), 0.0f );

		m_Mat4 rot_x, rot_z, inv_rot_z, perspective, basis_change;
		rot_x.Identity();
		rot_x.value[6]= std::tan( 11.0f * Constants::to_rad );
		rot_z.RotateZ( -z_angle );
		inv_rot_z.RotateZ( z_angle );
		basis_change.Identity();
		basis_change[5]= 0.0f;
		basis_change[6]= 1.0f;
		basis_change[9]= 1.0f;
		basis_change[10]= 0.0f;

		perspective.PerspectiveProjection( aspect, c_characters_fov_deg * Constants::to_rad, c_z_near, c_z_far  );
		const m_Mat4 view_rotation_and_projection_matrix= rot_z * rot_x * basis_change * perspective * screen_shift_mat;

		ViewClipPlanes view_clip_planes;

		const float half_fov_x= std::atan( aspect * std::tan( c_characters_fov_deg * ( Constants::to_rad * 0.5f ) ) ) * c_fov_scaler;
		const float half_fov_y= c_characters_fov_deg * ( c_fov_scaler * Constants::to_rad * 0.5f );

		// Near clip plane
		view_clip_planes[0].normal= m_Vec3( 0.0f, 1.0f, 0.0f ) * inv_rot_z;
		view_clip_planes[0].dist= -( cam_pos * view_clip_planes[0].normal + c_z_near );
		// Left
		view_clip_planes[1].normal= m_Vec3( +std::cos( half_fov_x ), +std::sin( half_fov_x ), 0.0f ) * inv_rot_z;
		view_clip_planes[1].dist= -( cam_pos * view_clip_planes[1].normal );
		// Right
		view_clip_planes[2].normal= m_Vec3( -std::cos( half_fov_x ), +std::sin( half_fov_x ), 0.0f ) * inv_rot_z;
		view_clip_planes[2].dist= -( cam_pos * view_clip_planes[2].normal );

		const float shift= -std::cos( half_fov_y ) * rot_x.value[6];
		// Bottom
		view_clip_planes[3].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ) - shift, +std::cos( half_fov_y ) );
		view_clip_planes[3].normal.Normalize();
		view_clip_planes[3].normal= view_clip_planes[3].normal * inv_rot_z;
		view_clip_planes[3].dist= -( cam_pos * view_clip_planes[3].normal );
		// Top
		view_clip_planes[4].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ) + shift, -std::cos( half_fov_y ) );
		view_clip_planes[4].normal.Normalize();
		view_clip_planes[4].normal= view_clip_planes[4].normal * inv_rot_z;
		view_clip_planes[4].dist= -( cam_pos * view_clip_planes[4].normal );

		const Time current_time= Time::CurrentTime();

		characters_map_models_.resize( characters_.size() );
		for( unsigned int c= 0u; c < characters_.size(); c++ )
		{
			const CutsceneScript::Character& character= script_->characters[c];
			const CharacterState& character_state= characters_[c];
			IMapDrawer::MapRelatedModel& out_model= characters_map_models_[c];

			const unsigned int model_id= first_character_model_index_ + c;
			const Model& model= cutscene_map_data_->models[ model_id ];

			out_model.pos= room_pos_ + ( character.pos * room_rotation_matrix_ );
			out_model.pos.z+= cam_pos.z - 0.125f;
			out_model.angle_z= character.angle + room_angle_;
			out_model.model_id= model_id;

			if( !model.animations.empty() )
			{
				const Time time_delta= current_time - character_state.current_animation_start_time;
				const unsigned int animation_frame=
					static_cast<unsigned int>( time_delta.ToSeconds() * GameConstants::animations_frames_per_second ) %
					model.animations[ character_state.current_animation_number ].frame_count;

				out_model.frame= model.animations[ character_state.current_animation_number ].first_frame + animation_frame;
			}
			else
				out_model.frame= 0u;
		}

		map_drawer_.DrawMapRelatedModels(
			characters_map_models_.data(), characters_map_models_.size(),
			view_rotation_and_projection_matrix,
			cam_pos,
			view_clip_planes );
	}

	// Draw briefbar and text.

	shared_drawers_->menu->DrawBriefBar();

	ITextDrawer& text_drawer= *shared_drawers_->text;
	const unsigned int text_scale= menu_scale;
	const unsigned int line_height= text_drawer.GetLineHeight();
	const unsigned int start_x= ( 5u * menu_scale + viewport_size.Width() - GameConstants::min_screen_width * menu_scale ) >> 1u;
	const unsigned int start_y= viewport_size.Height() - ( c_say_lines * line_height + 5u ) * text_scale;
	for( unsigned int s= 0u; s < c_say_lines; s++ )
	{
		text_drawer.Print(
			start_x,
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
