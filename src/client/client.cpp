#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../i_drawers_factory.hpp"
#include "../i_menu_drawer.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"
#include "../save_load_streams.hpp"
#include "../settings.hpp"
#include "../shared_drawers.hpp"
#include "../shared_settings_keys.hpp"
#include "../server/a_code.hpp"
#include "../sound/sound_engine.hpp"
#include "../sound/sound_id.hpp"
#include "cutscene_player.hpp"
#include "i_hud_drawer.hpp"
#include "i_map_drawer.hpp"
#include "i_minimap_drawer.hpp"
#include "../key_checker.hpp"
#include "client.hpp"

namespace PanzerChasm
{

static const char g_small_hud_mode[]= "cl_small_hud_mode";

struct Client::LoadedMinimapState
{
	unsigned int map_number;
	MinimapState::WallsVisibility static_walls_visibility ;
	MinimapState::WallsVisibility dynamic_walls_visibility;
};

Client::Client(
	Settings& settings,
	CommandsProcessor& commands_processor,
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	IDrawersFactory& drawers_factory,
	const SharedDrawersPtr& shared_drawers,
	const Sound::SoundEnginePtr& sound_engine,
	const DrawLoadingCallback& draw_loading_callback )
	: settings_(settings)
	, game_resources_(game_resources)
	, map_loader_(map_loader)
	, sound_engine_(sound_engine)
	, draw_loading_callback_(draw_loading_callback)
	, current_tick_time_( Time::CurrentTime() )
	, camera_controller_(
		settings,
		m_Vec3( 0.0f, 0.0f, 0.0f ),
		shared_drawers->menu->GetViewportSize().GetWidthToHeightRatio() )
	, shared_drawers_(shared_drawers)
	, map_drawer_( drawers_factory.CreateMapDrawer() )
	, minimap_drawer_( drawers_factory.CreateMinimapDrawer() )
	, weapon_state_( game_resources )
	, hud_drawer_( drawers_factory.CreateHUDDrawer( shared_drawers ) )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( map_loader_ != nullptr );
	PC_ASSERT( map_drawer_ != nullptr );
	PC_ASSERT( minimap_drawer_ != nullptr );
	PC_ASSERT( hud_drawer_ != nullptr );

	CommandsMapPtr commands= std::make_shared<CommandsMap>();
	commands->emplace( "fullmap", std::bind( &Client::FullMap, this ) );
	commands->emplace( "pos", std::bind( &Client::PrintPlayerPos, this ) );
	commands_= std::move( commands );
	commands_processor.RegisterCommands(commands_);

	std::memset( &player_state_, 0, sizeof(player_state_) );
	std::memset( &server_state_, 0, sizeof(server_state_) );

	CorrectPlayerName();
	player_name_= settings_.GetString( SettingsKeys::player_name );
}

Client::~Client()
{}

void Client::VidClear()
{
	map_drawer_= nullptr;
	minimap_drawer_= nullptr;
	hud_drawer_= nullptr;

	// Kill cutscene, because it doesn`t supports vid_restart.
	if( cutscene_player_ != nullptr )
	{
		cutscene_player_= nullptr;
		// Set map in sound engine too, because sound engine have null map in cutscene.
		if( sound_engine_ != nullptr )
			sound_engine_->SetMap( current_map_data_ );
	}
}

void Client::VidRestart( IDrawersFactory& drawers_factory )
{
	PC_ASSERT( map_drawer_ == nullptr );
	PC_ASSERT( minimap_drawer_ == nullptr );
	PC_ASSERT( hud_drawer_ == nullptr );

	map_drawer_= drawers_factory.CreateMapDrawer();
	minimap_drawer_= drawers_factory.CreateMinimapDrawer();
	hud_drawer_= drawers_factory.CreateHUDDrawer( shared_drawers_ );

	if( current_map_data_ != nullptr )
	{
		map_drawer_->SetMap( current_map_data_ );
		minimap_drawer_->SetMap( current_map_data_ );
	}

	camera_controller_.SetAspect( shared_drawers_->menu->GetViewportSize().GetWidthToHeightRatio() );
}

void Client::Save( SaveLoadBuffer& buffer, SaveComment& out_save_comment )
{
	PC_ASSERT( current_map_data_ != nullptr );
	PC_ASSERT( minimap_state_ != nullptr );
	if( minimap_state_ == nullptr ) return;

	SaveStream save_stream( buffer );

	save_stream.WriteUInt32( requested_weapon_index_ );
	save_stream.WriteBool( minimap_mode_ );
	save_stream.WriteUInt32( current_map_data_->number );

	const MinimapState::WallsVisibility& static_walls_visibility = minimap_state_->GetStaticWallsVisibility ();
	const MinimapState::WallsVisibility& dynamic_walls_visibility= minimap_state_->GetDynamicWallsVisibility();

	// TODO - write bools in more compact form

	save_stream.WriteUInt32( static_cast<uint32_t>(static_walls_visibility .size()) );
	for( const bool& wall_visibility : static_walls_visibility  )
		save_stream.WriteBool( wall_visibility );

	save_stream.WriteUInt32( static_cast<uint32_t>(dynamic_walls_visibility.size()) );
	for( const bool& wall_visibility : dynamic_walls_visibility )
		save_stream.WriteBool( wall_visibility );

	// Write comment
	std::snprintf( out_save_comment.data(), sizeof(SaveComment), "Level%2d  health %03d", current_map_data_->number, player_state_.health );
}

void Client::Load( const SaveLoadBuffer& buffer, unsigned int& buffer_pos )
{
	LoadStream load_stream( buffer, buffer_pos );

	loaded_minimap_state_.reset( new LoadedMinimapState );

	load_stream.ReadUInt32( requested_weapon_index_ );
	load_stream.ReadBool( minimap_mode_ );
	load_stream.ReadUInt32( loaded_minimap_state_->map_number );

	unsigned int static_walls_count ;
	load_stream.ReadUInt32( static_walls_count  );
	loaded_minimap_state_->static_walls_visibility .resize( static_walls_count  );
	for( unsigned int i= 0u; i < static_walls_count ; i++ )
	{
		bool b;
		load_stream.ReadBool( b );
		loaded_minimap_state_->static_walls_visibility [i]= b;
	}

	unsigned int dynamic_walls_count;
	load_stream.ReadUInt32( dynamic_walls_count );
	loaded_minimap_state_->dynamic_walls_visibility .resize( dynamic_walls_count );
	for( unsigned int i= 0u; i < dynamic_walls_count; i++ )
	{
		bool b;
		load_stream.ReadBool( b );
		loaded_minimap_state_->dynamic_walls_visibility[i]= b;
	}

	buffer_pos= load_stream.GetBufferPos();
}

void Client::SetConnection( IConnectionPtr connection )
{
	if( connection == nullptr )
	{
		connection_info_= nullptr;
		StopMap();
	}
	else
	{
		connection_info_.reset( new ConnectionInfo( connection ) );
		TransmitPlayerName();
	}
}

bool Client::Disconnected() const
{
	if( connection_info_ == nullptr )
		return true;

	return connection_info_->connection->Disconnected();
}

bool Client::PlayingCutscene() const
{
	if( cutscene_player_ == nullptr )
		return false;
	return !cutscene_player_->IsFinished();
}

void Client::ProcessEvents( const SystemEvents& events )
{
	if( cutscene_player_ != nullptr )
	{
		cutscene_player_->Process( events );
	}

	using KeyCode= SystemEvent::KeyEvent::KeyCode;

	for( const SystemEvent& event : events )
	{
		if( event.type == SystemEvent::Type::MouseMove )
		{
			camera_controller_.ControllerRotate(
				event.event.mouse_move.dy,
				event.event.mouse_move.dx );
		}

		if( event.type == SystemEvent::Type::Key || event.type == SystemEvent::Type::MouseKey || event.type == SystemEvent::Type::Wheel)
		{
			const KeyChecker key_pressed( settings_, event );

			if( key_pressed( SettingsKeys::key_weapon_next, KeyCode::MouseWheelDown ) ) TrySwitchWeaponNext();
			if( key_pressed( SettingsKeys::key_weapon_prev, KeyCode::MouseWheelUp ) ) TrySwitchWeaponPrevious();
			if( key_pressed( SettingsKeys::key_weapon_change, KeyCode::Mouse3 ) ) TrySwitchWeaponNext();
			if( key_pressed( SettingsKeys::key_weapon_1, KeyCode::K1 ) ||
			    key_pressed( SettingsKeys::key_weapon_2, KeyCode::K2 ) ||
			    key_pressed( SettingsKeys::key_weapon_3, KeyCode::K3 ) ||
			    key_pressed( SettingsKeys::key_weapon_4, KeyCode::K4 ) ||
			    key_pressed( SettingsKeys::key_weapon_5, KeyCode::K5 ) ||
			    key_pressed( SettingsKeys::key_weapon_6, KeyCode::K6 ) ||
			    key_pressed( SettingsKeys::key_weapon_7, KeyCode::K7 ) ||
			    key_pressed( SettingsKeys::key_weapon_8, KeyCode::K8 ) ||
			    key_pressed( SettingsKeys::key_weapon_9, KeyCode::K9 ) )
			{
				unsigned int weapon_index=static_cast<unsigned int>( event.event.key.key_code ) - static_cast<unsigned int>( KeyCode::K1 );
				if( player_state_.ammo[ weapon_index ] > 0u && ( player_state_.weapons_mask & (1u << weapon_index) ) != 0u )
					requested_weapon_index_= weapon_index;
			}
			if( key_pressed( SettingsKeys::key_minimap, KeyCode::Tab ) ) minimap_mode_= !minimap_mode_;
			if( key_pressed( SettingsKeys::key_small_hud_off, KeyCode::Minus ) ) settings_.SetSetting( g_small_hud_mode, false );
			if( key_pressed( SettingsKeys::key_small_hud_on, KeyCode::Equals ) ) settings_.SetSetting( g_small_hud_mode, true );
		}
	} // for events
}

void Client::Loop( const InputState& input_state, const bool paused )
{
	const Time current_real_time= Time::CurrentTime();
	const KeyChecker key_pressed( settings_, input_state );

	// Calculate time, which we spend in pause.
	// Subtract time, spended in pauses, from real time.
	if( paused && !paused_ )
	{
		paused_= paused;
		pause_start_time_= current_real_time;
	}
	if( paused )
		return;
	if( !paused && paused_ )
	{
		paused_= paused;
		accumulated_pauses_time_+= current_real_time - pause_start_time_;
		pause_start_time_= Time::FromSeconds(0);
	}

	const Time prev_tick_time= current_tick_time_;
	current_tick_time_= current_real_time - accumulated_pauses_time_;
	const float tick_dt_s= ( current_tick_time_ - prev_tick_time ).ToSeconds();

	if( connection_info_ != nullptr )
	{
		if( connection_info_->connection->Disconnected() )
			StopMap();
		else
			connection_info_->messages_extractor.ProcessMessages( *this );
	}

	if( cutscene_player_ != nullptr )
	{
		if( cutscene_player_->IsFinished() )
		{
			cutscene_player_= nullptr;

			PC_ASSERT( current_map_data_ != nullptr );
			if( sound_engine_ != nullptr )
				sound_engine_->SetMap( current_map_data_ );
			map_drawer_->SetMap( current_map_data_ );
			minimap_drawer_->SetMap( current_map_data_ );
		}
		else
			return;
	}

	{ // Scale minimap.
		const float log2_delta= 2.0f * tick_dt_s;
		
		if( key_pressed( SettingsKeys::key_minimap_scale_dec, KeyCode::SquareBracketLeft ) )
			minimap_scale_log2_-= log2_delta;
		if( key_pressed( SettingsKeys::key_minimap_scale_inc, KeyCode::SquareBracketRight ) )
			minimap_scale_log2_+= log2_delta;

		minimap_scale_log2_= std::max( -2.0f, std::min( minimap_scale_log2_, 1.0f ) );
	}

	camera_controller_.Tick( input_state );

	if( sound_engine_ != nullptr )
	{
		sound_engine_->SetHeadPosition(
			player_position_ + m_Vec3( 0.0f, 0.0f, GameConstants::player_eyes_level ), // TODO - use exact camera position here
			camera_controller_.GetViewAngleZ(),
			camera_controller_.GetViewAngleX() );

		if( map_state_ != nullptr )
			sound_engine_->UpdateMapState( *map_state_ );
	}

	hud_drawer_->SetPlayerState( player_state_, weapon_state_.CurrentWeaponIndex() );

	if( player_state_.ammo[ requested_weapon_index_ ] == 0u )
		TrySwitchWeaponOnOutOfAmmo();

	if( map_state_ != nullptr )
	{
		map_state_->Tick( current_tick_time_ );

		if( minimap_state_ != nullptr )
			minimap_state_->Update(
				*map_state_,
				player_position_.xy(),
				camera_controller_.GetViewAngleZ() );
	}

	if( connection_info_ != nullptr )
	{
		{ // Send to server new player name, if it changed.
			CorrectPlayerName();
			const char* const cur_player_name= settings_.GetString( SettingsKeys::player_name );
			if( player_name_ != cur_player_name )
			{
				player_name_= cur_player_name;
				TransmitPlayerName();
			}
		}
		{ // Send move message
			float move_direction, move_acceleration;
			camera_controller_.GetAcceleration( input_state, move_direction, move_acceleration );

			Messages::PlayerMove message;
			message.view_direction   = AngleToMessageAngle( camera_controller_.GetViewAngleZ() + Constants::half_pi );
			message.move_direction   = AngleToMessageAngle( move_direction );
			message.acceleration     = static_cast<unsigned char>( move_acceleration * 254.5f );
			message.jump_pressed     = key_pressed( SettingsKeys::key_jump, KeyCode::Mouse2 ) || camera_controller_.JumpPressed();
			message.weapon_index     = requested_weapon_index_;
			message.view_dir_angle_x = AngleToMessageAngle( camera_controller_.GetViewAngleX() );
			message.view_dir_angle_z = AngleToMessageAngle( camera_controller_.GetViewAngleZ() );
			message.shoot_pressed    = key_pressed( SettingsKeys::key_fire, KeyCode::Mouse1 );
			message.color            = settings_.GetOrSetInt( SettingsKeys::player_color );
			connection_info_->messages_sender.SendUnreliableMessage( message );
		}

		connection_info_->messages_sender.Flush();
	}
}

void Client::Draw()
{
	if( cutscene_player_ != nullptr )
	{
		cutscene_player_->Draw();
		return;
	}

	if( map_state_ != nullptr )
	{
		PC_ASSERT( current_map_data_ != nullptr );

		camera_controller_.UpdateParams();

		m_Vec3 pos= player_position_;
		const float z_shift= camera_controller_.GetEyeZShift();

		if( player_state_.health > 0u )
			pos.z+= GameConstants::player_eyes_level + z_shift;
		else
			pos.z= std::min( pos.z + GameConstants::player_deathcam_level, GameConstants::walls_height );

		m_Mat4 view_rotation_and_projection_matrix, projection_matrix;
		camera_controller_.GetViewRotationAndProjectionMatrix( view_rotation_and_projection_matrix );
		camera_controller_.GetViewProjectionMatrix( projection_matrix );

		ViewClipPlanes view_clip_planes;
		camera_controller_.GetViewClipPlanes( pos, view_clip_planes );

		map_drawer_->Draw(
			*map_state_,
			view_rotation_and_projection_matrix,
			pos,
			view_clip_planes,
			player_state_.health > 0u ? player_monster_id_ : 0u /* draw body, if death */ );

		// Draw weapon, if alive.
		if( player_state_.health > 0u )
		{
			const float c_weapon_shift_scale= 0.25f;
			const float c_weapon_angle_scale= 0.75f;
			const float weapon_angle_for_shift= camera_controller_.GetViewAngleX() *  c_weapon_angle_scale;
			const float weapon_shift_z= c_weapon_shift_scale * z_shift * std::cos( weapon_angle_for_shift );
			const float weapon_shift_y= c_weapon_shift_scale * z_shift * std::sin( weapon_angle_for_shift );

			m_Mat4 weapon_shift_matrix;
			weapon_shift_matrix.Translate( m_Vec3( 0.0f, weapon_shift_y, weapon_shift_z ) );

			map_drawer_->DrawWeapon(
				weapon_state_,
				weapon_shift_matrix * projection_matrix,
				pos,
				camera_controller_.GetViewAngleX(),
				camera_controller_.GetViewAngleZ(),
				player_state_.is_invisible );

			if( player_state_.show_shield )
				map_drawer_->DrawActiveItemIcon( *map_state_, GameConstants::shield_item_id, 0u );
			if( player_state_.show_chojin )
				map_drawer_->DrawActiveItemIcon( *map_state_, GameConstants::chojin_item_id, 1u );
		}

		map_drawer_->DoFullscreenPostprocess( *map_state_ );

		if( minimap_mode_ && map_state_ != nullptr && minimap_state_ != nullptr )
		{
			minimap_drawer_->Draw(
				*map_state_, *minimap_state_,
				full_map_,
				player_position_.xy(),
				std::exp2( minimap_scale_log2_ ),
				camera_controller_.GetViewAngleZ() );
		}

		if( settings_.GetOrSetBool( SettingsKeys::crosshair, true ) )
			hud_drawer_->DrawCrosshair();
		hud_drawer_->DrawCurrentMessage( current_tick_time_ );
		if( settings_.GetOrSetBool( g_small_hud_mode, false ) )
			hud_drawer_->DrawSmallHud();
		else
		{
			if( server_state_.game_rules != GameRules::SinglePlayer )
			{
				IHudDrawer::NetgameScores netgame_scores;
				netgame_scores.score_count= server_state_.player_count;
				netgame_scores.active_score_number= player_state_.index;
				for( unsigned int i= 0u; i < netgame_scores.score_count; i++ )
					netgame_scores.scores[i]= server_state_.frags[i];

				hud_drawer_->DrawHud(
					minimap_mode_,
					current_map_data_->map_name,
					&netgame_scores );
			}
			else
			{
				hud_drawer_->DrawHud(
					minimap_mode_,
					current_map_data_->map_name,
					nullptr );
			}
		}
	}
}

void Client::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Client::operator()( const Messages::ServerState& message )
{
	server_state_= message;
}

void Client::operator()( const Messages::DynamicTextMessage& message )
{
	// TODO - check if message text is not null-terminated
	Log::User( message.text );
}

void Client::operator()( const Messages::PlayerSpawn& message )
{
	MessagePositionToPosition( message.xyz, player_position_ );
	camera_controller_.SetAngles( MessageAngleToAngle( message.direction ) - Constants::half_pi, 0.0f );
	player_monster_id_= message.player_monster_id;
}

void Client::operator()( const Messages::PlayerPosition& message )
{
	MessagePositionToPosition( message.xyz, player_position_ );
	camera_controller_.SetSpeed( MessageCoordToCoord( message.speed ) );
}

void Client::operator()( const Messages::PlayerState& message )
{
	player_state_= message;
}

void Client::operator()( const Messages::PlayerWeapon& message )
{
	weapon_state_.ProcessMessage( message );
}

void Client::operator()( const Messages::PlayerItemPickup& message )
{
	if( message.item_id >= game_resources_->items_description.size() )
		return;

	const unsigned char a_code= game_resources_->items_description[ message.item_id ].a_code;
	if(
		a_code >= static_cast<unsigned char>(ACode::Weapon_First) &&
		a_code <= static_cast<unsigned char>(ACode::Weapon_Last) )
	{
		requested_weapon_index_= a_code - static_cast<unsigned char>(ACode::Weapon_First);

		// Mark weapon as recieved, because TrySwitchWeaponOnOutOfAmmo can switch weapon back,
		// if PlayerState message has not yet arrived.
		player_state_.weapons_mask|= 1u << requested_weapon_index_;
		if( player_state_.ammo[ requested_weapon_index_ ] == 0u )
			player_state_.ammo[ requested_weapon_index_ ]= 1u;
	}
}

void Client::operator()( const Messages::MapEventSound& message )
{
	if( sound_engine_ != nullptr )
	{
		m_Vec3 pos;
		MessagePositionToPosition( message.xyz, pos );
		sound_engine_->PlayWorldSound( message.sound_id, pos );
	}
}

void Client::operator()( const Messages::MonsterLinkedSound& message )
{
	if( sound_engine_ == nullptr )
		return;

	if( message.monster_id == player_monster_id_ )
	{
		sound_engine_->PlayHeadSound( message.sound_id );
	}
	else if( map_state_ != nullptr )
	{
		const MapState::MonstersContainer& monsters= map_state_->GetMonsters();
		const auto it= monsters.find( message.monster_id );
		if( it != monsters.end() )
			sound_engine_->PlayMonsterLinkedSound( *it, message.sound_id );
	}
}

void Client::operator()( const Messages::MonsterSound& message )
{
	if( sound_engine_ == nullptr )
		return;

	if( map_state_ != nullptr )
	{
		const MapState::MonstersContainer& monsters= map_state_->GetMonsters();
		const auto it= monsters.find( message.monster_id );
		if( it != monsters.end() )
			sound_engine_->PlayMonsterSound( *it, message.monster_sound_id );
	}
}

void Client::operator()( const Messages::MapChange& message )
{
	Log::Info( "Changing client map to ", message.map_number );

	const auto show_progress=
	[&]( const float progress )
	{
		if( draw_loading_callback_ != nullptr )
			draw_loading_callback_( progress, "Client" );
	};

	show_progress( 0.0f );

	const MapDataConstPtr map_data= map_loader_->LoadMap( message.map_number );
	if( map_data == nullptr )
	{
		Log::Warning( "Error, server requested map, which does not exist on client" );
		return;
	}

	show_progress( 0.5 );
	map_state_.reset( new MapState( map_data, game_resources_, Time::CurrentTime() ) );
	minimap_state_.reset( new MinimapState( map_data ) );

	if( loaded_minimap_state_ != nullptr &&
		loaded_minimap_state_->map_number == message.map_number )
	{
		minimap_state_->SetState(
			loaded_minimap_state_->static_walls_visibility ,
			loaded_minimap_state_->dynamic_walls_visibility );
	}
	loaded_minimap_state_= nullptr;

	show_progress( 0.8f );

	hud_drawer_->ResetMessage();

	current_map_data_= map_data;

	show_progress( 1.0f );

	// Try load cutscene.
	if( message.need_play_cutscene )
	{
		cutscene_player_.reset(
			new CutscenePlayer(
				game_resources_,
				*map_loader_,
				sound_engine_,
				shared_drawers_,
				*map_drawer_,
				message.map_number ) );

		if( cutscene_player_->IsFinished() ) // No cutscene for this map.
			cutscene_player_= nullptr;

		if( cutscene_player_ != nullptr && sound_engine_ != nullptr ) // Stop sound at cutscene start.
			sound_engine_->SetMap( nullptr );
	}

	// If no cutscene - set map for MapDrawer, MinimapDrawer, SoundEngine.
	// Else - set map only after cutscene end.
	if( cutscene_player_ == nullptr )
	{
		if( sound_engine_ != nullptr )
			sound_engine_->SetMap( map_data );

		map_drawer_->SetMap( map_data );
		minimap_drawer_->SetMap( map_data );
	}
}

void Client::operator()( const Messages::TextMessage& message )
{
	if( current_map_data_ != nullptr )
	{
		if( message.text_message_number < current_map_data_->messages.size() )
		{
			hud_drawer_->AddMessage( current_map_data_->messages[ message.text_message_number ], current_tick_time_ );

			// TODO - start playing text message sound from server, as monster-linked sound.
			if( sound_engine_ != nullptr )
				sound_engine_->PlayHeadSound( Sound::SoundId::Message );
		}
	}
}

void Client::StopMap()
{
	if( current_map_data_ != nullptr && sound_engine_ != nullptr )
		sound_engine_->SetMap( nullptr );

	current_map_data_= nullptr;
	map_state_= nullptr;
	minimap_state_= nullptr;

	cutscene_player_= nullptr;
}

MapDataConstPtr Client::CurrentMap()
{
	return current_map_data_;
}

void Client::TrySwitchWeaponNext()
{
	for(uint8_t i = weapon_state_.CurrentWeaponIndex() + 1; i < GameConstants::weapon_count; i++ )
	{
		if( ( player_state_.weapons_mask & ( 1u << i ) ) != 0u && player_state_.ammo[i] != 0u )
		{
			requested_weapon_index_= i;
			return;
		}
	}

	requested_weapon_index_= 0u;
}

void Client::TrySwitchWeaponPrevious()
{
	for(uint8_t i = weapon_state_.CurrentWeaponIndex() == 0 ? GameConstants::weapon_count - 1 : weapon_state_.CurrentWeaponIndex() - 1; i >= 0; i--)
	{
		if( (player_state_.weapons_mask & ( 1u << i ) ) != 0u && player_state_.ammo[i] != 0u )
		{
			requested_weapon_index_= i;
			return;
		}
	}

	requested_weapon_index_= 0u;
}

void Client::TrySwitchWeaponOnOutOfAmmo()
{
	for(uint8_t i = 1u; i < GameConstants::weapon_count; i++ )
	{
		if( ( player_state_.weapons_mask & ( 1u << i ) ) != 0u && player_state_.ammo[i] != 0u )
		{
			requested_weapon_index_= i;
			return;
		}
	}

	requested_weapon_index_= 0u;
}

void Client::TransmitPlayerName()
{
	PC_ASSERT( connection_info_ != nullptr );

	Messages::PlayerName message;
	std::snprintf( message.name, sizeof(message.name), "%s", player_name_.c_str() );
	connection_info_->messages_sender.SendReliableMessage( message );
}

void Client::CorrectPlayerName()
{
	// Set player name to "unnamed" if it is empty or not set.

	const char* const name= settings_.GetString( SettingsKeys::player_name );
	if( std::strlen( name ) == 0 )
		settings_.SetSetting( SettingsKeys::player_name, "unnamed" );
}

void Client::FullMap()
{
	if( full_map_ )
	{
		full_map_= false;
		Log::Info( "FULL MAP IS OFF" );
	}
	else
	{
		full_map_= true;
		Log::Info( "FULL MAP IS ON" );
	}
}

void Client::PrintPlayerPos()
{
	Log::Info( "Pos: ", player_position_.x, ", ", player_position_.y, ", ", player_position_.z );
}

} // namespace PanzerChasm
