
#include <cstring>

#include <ogl_state_manager.hpp>

#include "../assert.hpp"
#include "../game_resources.hpp"
#include "../images.hpp"
#include "../log.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "../settings.hpp"
#include "../shared_settings_keys.hpp"
#include  "opengl_renderer/models_textures_corrector.hpp"
#include "map_drawers_common.hpp"
#include "weapon_state.hpp"

#include "map_drawer_gl.hpp"

namespace PanzerChasm
{

namespace
{

constexpr float g_walls_coords_scale= 256.0f;

const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };

// Draw static walls with back-faces culling.
// Draw dynamic walls without back-faces culling.
const r_OGLState g_static_walls_gl_state(
	false, true, true, false,
	g_gl_state_blend_func );
const r_OGLState g_dynamic_walls_gl_state(
	false, false, true, false,
	g_gl_state_blend_func );

const r_OGLState g_floors_gl_state(
	false, false, true, false,
	g_gl_state_blend_func );

const r_OGLState g_models_gl_state(
	false, true, true, false,
	g_gl_state_blend_func );

// Draw transparent models without depth-write.
// This looks good on static light, but not so good in dynamic light.
const r_OGLState g_transparent_models_gl_state(
	true, true, true, false,
	g_gl_state_blend_func,
	r_OGLState::default_clear_color,
	r_OGLState::default_clear_depth,
	r_OGLState::default_cull_face_mode,
	false );

const r_OGLState g_sprites_gl_state(
	true, false, true, false,
	g_gl_state_blend_func );

const r_OGLState g_sky_gl_state(
	false, false, true, false,
	g_gl_state_blend_func );

const r_OGLState g_shadows_gl_state(
	true, true, true, false,
	g_gl_state_blend_func,
	r_OGLState::default_clear_color,
	r_OGLState::default_clear_depth,
	r_OGLState::default_cull_face_mode,
	false );

const r_OGLState g_fullscreen_blend_state(
	true, false, false, false,
	g_gl_state_blend_func,
	r_OGLState::default_clear_color,
	r_OGLState::default_clear_depth,
	r_OGLState::default_cull_face_mode,
	false );

} // namespace

struct FloorVertex
{
	unsigned char xy[2];
	unsigned char texture_id;
	unsigned char reserved;
};

SIZE_ASSERT( FloorVertex, 4u );

struct WallVertex
{
	unsigned short xyz[3]; // 8.8 fixed
	unsigned short pad1;
	short tex_coord[2]; // 8.8 fixed
	unsigned char texture_id;
	unsigned char pad2[3];
	char normal[2];
	char pad3[2];
	unsigned char lightmap_coord[2];
	unsigned char pad4[2];
	//unsigned char reserved[1];
};

//SIZE_ASSERT( WallVertex, 16u );
SIZE_ASSERT( WallVertex, 24u );

struct ModelsTexturesPlacement
{
	struct ModelTexturePlacement
	{
		unsigned int y;
		unsigned int layer;
	};

	static constexpr unsigned int c_max_textures= 128u;

	ModelTexturePlacement textures_placement[ c_max_textures ];
	unsigned int layer_count;
};

/* Models textures has fixed width (64) and variative height.
 * We place all models textures in array texture.
 * We can not just place single model texture in single layer, because we left useless a lot of texture space.
 * Instead, we place as many as possible textures in each layer of texture array.
 *
 * In this function we try solve "bin packing problem", using "best-fit" algorithm.
 */
static void CalculateModelsTexturesPlacement(
	const std::vector<Model>& models,
	const unsigned int texture_height,
	ModelsTexturesPlacement& out_placement )
{
	PC_ASSERT( models.size() <= ModelsTexturesPlacement::c_max_textures );

	unsigned int textures_placed= 0u;
	bool texture_placed[ ModelsTexturesPlacement::c_max_textures ];
	for( bool& placed : texture_placed )
		placed= false;

	const unsigned int c_border= 16u;

	unsigned int current_y= c_border;
	unsigned int current_layer= 0u;
	const unsigned int no_texture= models.size();

	while( textures_placed < models.size() )
	{
		const unsigned int place_left= texture_height - current_y;

		unsigned int best_texture_number= no_texture;
		unsigned int best_texture_delta= texture_height;
		for( unsigned int i= 0u; i < models.size(); i++ )
		{
			if( texture_placed[i] )
				continue;

			int delta= int(place_left) - int( models[i].texture_size[1] );
			if( delta >= 0 && delta < int(best_texture_delta) )
			{
				best_texture_delta= delta;
				best_texture_number= i;
			}
		}

		// Best texture not found - use first unplaced.
		if( best_texture_number == no_texture )
		{
			for( unsigned int i= 0u; i < models.size(); i++ )
				if( !texture_placed[i] )
				{
					best_texture_number= i;
					break;
				}
		}

		// TODO - what do, if texture height iz zero?
		const unsigned int best_texture_height= models[ best_texture_number ].texture_size[1];
		if( current_y + best_texture_height > texture_height )
		{
			current_y= c_border;
			current_layer++;
		}

		out_placement.textures_placement[ best_texture_number ].y= current_y;
		out_placement.textures_placement[ best_texture_number ].layer= current_layer;

		current_y+= best_texture_height + c_border;
		texture_placed[ best_texture_number ]= true;
		textures_placed++;
	}

	out_placement.layer_count= current_layer + 1u;
}

static void CreateModelMatrices(
	const m_Vec3& pos, const float angle,
	m_Mat4& out_model_matrix, m_Mat3& out_lightmap_matrix )
{
	m_Mat4 shift, rotate;
	shift.Translate( pos );
	rotate.RotateZ( angle );

	out_model_matrix= rotate * shift;

	m_Mat3 shift_xy, rotate_z( rotate ), scale;
	shift_xy.Translate( pos.xy() );
	scale.Scale( 1.0f / float(MapData::c_map_size) );

	out_lightmap_matrix= rotate_z * shift_xy * scale;
}

MapDrawerGL::MapDrawerGL(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextGL& rendering_context )
	: settings_(settings)
	, game_resources_(game_resources)
	, rendering_context_(rendering_context)
	, filter_textures_( settings.GetOrSetBool( SettingsKeys::opengl_textures_filtering, false ) )
	, use_hd_dynamic_lightmap_( settings.GetOrSetBool( SettingsKeys::opengl_dynamic_lighting, false ) )
	, map_light_( game_resources, rendering_context, use_hd_dynamic_lightmap_ )
{
	PC_ASSERT( game_resources_ != nullptr );

	current_sky_texture_file_name_[0]= '\0';

	// Textures
	glGenTextures( 1, &floor_textures_array_id_ );
	glGenTextures( 1, &wall_textures_array_id_ );
	glGenTextures( 1, &models_textures_array_id_ );
	glGenTextures( 1, &items_textures_array_id_ );
	glGenTextures( 1, &rockets_textures_array_id_ );
	glGenTextures( 1, &gibs_textures_array_id_ );
	glGenTextures( 1, &weapons_textures_array_id_ );

	LoadSprites( game_resources_->effects_sprites, sprites_textures_arrays_ );
	LoadSprites( game_resources_->bmp_objects_sprites, bmp_objects_sprites_textures_arrays_ );
	PrepareSkyGeometry();

	// Check buffer textures limitations.
	// Buffer textures used for models animations vertices.
	{
		GLint max_texture_buffer_size;
		glGetIntegerv( GL_MAX_TEXTURE_BUFFER_SIZE, &max_texture_buffer_size );
		Log::Info( "GL_MAX_TEXTURE_BUFFER_SIZE is ", max_texture_buffer_size );

		const int c_min_required_texture_buffer_size= 2 * 1024 * 1024;

		// 0 - auto, 1 - force buffer texture, 2 - force 2d texture.
		const int animations_storage= settings.GetOrSetInt( "r_animations_storage", 0 );

		if( animations_storage == 1 )
			use_2d_textures_for_animations_= false;
		else if( animations_storage == 2 )
			use_2d_textures_for_animations_= true;
		else
		{
			if( animations_storage != 0 )
				settings.SetSetting( "r_animations_storage", int(0) );

			use_2d_textures_for_animations_= max_texture_buffer_size < c_min_required_texture_buffer_size;
		}

		if( use_2d_textures_for_animations_ )
			Log::Info( "Use 2d textures as animations storage" );
		else
		{
			if( max_texture_buffer_size < c_min_required_texture_buffer_size )
				Log::Warning( "Warning, GL_MAX_TEXTURE_BUFFER_SIZE is too small, required at least ", c_min_required_texture_buffer_size );
			Log::Info( "Use buffer textures as animations storage" );
		}
	}

	// Items
	LoadModels(
		game_resources_->items_models,
		items_geometry_,
		items_geometry_data_,
		items_animations_,
		items_textures_array_id_ );

	// Monsters
	LoadMonstersModels();

	// Rockets
	LoadModels(
		game_resources_->rockets_models,
		rockets_geometry_,
		rockets_geometry_data_,
		rockets_animations_,
		rockets_textures_array_id_ );

	// Rockets
	LoadModels(
		game_resources_->gibs_models,
		gibs_geometry_,
		gibs_geometry_data_,
		gibs_animations_,
		gibs_textures_array_id_ );

	// Weapons
	LoadModels(
		game_resources_->weapons_models,
		weapons_geometry_,
		weapons_geometry_data_,
		weapons_animations_,
		weapons_textures_array_id_ );

	// Shaders
	char lightmap_scale[32];
	std::snprintf( lightmap_scale, sizeof(lightmap_scale), "LIGHTMAP_SCALE %f", 1.0f / float(MapData::c_map_size) );

	const std::vector<std::string> defines{ lightmap_scale };

	std::vector<std::string> models_defines;
	if( use_2d_textures_for_animations_ )
	{
		models_defines.emplace_back( "USE_2D_TEXTURES_FOR_ANIMATIONS" );
		models_defines.emplace_back( "ANIMATION_TEXTURE_WIDTH " + std::to_string( AnimationsBuffer::c_2d_texture_width ) );
	}

	if( use_hd_dynamic_lightmap_ )
		floors_shader_.ShaderSource(
			rLoadShader( "floors_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "floors_v.glsl", rendering_context. glsl_version, defines ) );
	else
		floors_shader_.ShaderSource(
			rLoadShader( "static_light/floors_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "floors_v.glsl", rendering_context. glsl_version, defines ) );
	floors_shader_.SetAttribLocation( "pos", 0u );
	floors_shader_.SetAttribLocation( "tex_id", 1u );
	floors_shader_.Create();

	if( use_hd_dynamic_lightmap_ )
		walls_shader_.ShaderSource(
			rLoadShader( "walls_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "walls_v.glsl", rendering_context.glsl_version, defines ) );
	else
		walls_shader_.ShaderSource(
			rLoadShader( "static_light/walls_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "static_light/walls_v.glsl", rendering_context.glsl_version, defines ) );
	walls_shader_.SetAttribLocation( "pos", 0u );
	walls_shader_.SetAttribLocation( "tex_coord", 1u );
	walls_shader_.SetAttribLocation( "tex_id", 2u );
	walls_shader_.SetAttribLocation( "normal", 3u );
	walls_shader_.SetAttribLocation( "lightmap_coord", 4u );
	walls_shader_.Create();

	if( use_hd_dynamic_lightmap_ )
		models_shader_.ShaderSource(
			rLoadShader( "models_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "models_v.glsl", rendering_context.glsl_version, models_defines ),
			rLoadShader( "models_g.glsl", rendering_context.glsl_version ) );
	else
		models_shader_.ShaderSource(
			rLoadShader( "static_light/models_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "static_light/models_v.glsl", rendering_context.glsl_version, models_defines ));
	models_shader_.SetAttribLocation( "vertex_id", 0u );
	models_shader_.SetAttribLocation( "tex_coord", 1u );
	models_shader_.SetAttribLocation( "tex_id", 2u );
	models_shader_.SetAttribLocation( "alpha_test_mask", 3u );
	models_shader_.Create();

	models_shadow_shader_.ShaderSource(
		rLoadShader( "models_shadow_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "models_shadow_v.glsl", rendering_context.glsl_version, models_defines ) );
	models_shadow_shader_.SetAttribLocation( "vertex_id", 0u );
	models_shadow_shader_.SetAttribLocation( "groups_mask", 4u );
	models_shadow_shader_.Create();

	if( use_hd_dynamic_lightmap_ )
		sprites_shader_.ShaderSource(
			rLoadShader( "sprites_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "sprites_v.glsl", rendering_context.glsl_version ) );
	else
		sprites_shader_.ShaderSource(
			rLoadShader( "sprites_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "static_light/sprites_v.glsl", rendering_context.glsl_version ) );
	sprites_shader_.SetAttribLocation( "pos", 0u );
	sprites_shader_.Create();

	if( use_hd_dynamic_lightmap_ )
		monsters_shader_.ShaderSource(
			rLoadShader( "monsters_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "monsters_v.glsl", rendering_context.glsl_version, models_defines ),
			rLoadShader( "monsters_g.glsl", rendering_context.glsl_version ) );
	else
		monsters_shader_.ShaderSource(
			rLoadShader( "static_light/monsters_f.glsl", rendering_context.glsl_version ),
			rLoadShader( "static_light/monsters_v.glsl", rendering_context.glsl_version, models_defines ) );
	monsters_shader_.SetAttribLocation( "vertex_id", 0u );
	monsters_shader_.SetAttribLocation( "tex_coord", 1u );
	monsters_shader_.SetAttribLocation( "tex_id", 2u );
	monsters_shader_.SetAttribLocation( "alpha_test_mask", 3u );
	monsters_shader_.SetAttribLocation( "groups_mask", 4u );
	monsters_shader_.Create();

	sky_shader_.ShaderSource(
		rLoadShader( "sky_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "sky_v.glsl", rendering_context.glsl_version ) );
	sky_shader_.SetAttribLocation( "pos", 0u );
	sky_shader_.Create();

	fullscreen_blend_shader_.ShaderSource(
		rLoadShader( "fullscreen_blend_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "fullscreen_blend_v.glsl", rendering_context.glsl_version ) );
	fullscreen_blend_shader_.Create();
}

MapDrawerGL::~MapDrawerGL()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
	glDeleteTextures( 1, &wall_textures_array_id_ );
	glDeleteTextures( 1, &models_textures_array_id_ );
	glDeleteTextures( 1, &items_textures_array_id_ );
	glDeleteTextures( 1, &rockets_textures_array_id_ );
	glDeleteTextures( 1, &gibs_textures_array_id_ );
	glDeleteTextures( 1, &weapons_textures_array_id_ );

	glDeleteTextures( sprites_textures_arrays_.size(), sprites_textures_arrays_.data() );
	glDeleteTextures( bmp_objects_sprites_textures_arrays_.size(), bmp_objects_sprites_textures_arrays_.data() );
}

void MapDrawerGL::SetMap( const MapDataConstPtr& map_data )
{
	PC_ASSERT( map_data != nullptr );

	if( map_data == current_map_data_ )
		return;

	map_light_.SetMap( map_data );

	current_map_data_= map_data;

	LoadFloorsTextures( *map_data );
	LoadWallsTextures( *map_data );
	LoadFloors( *map_data );
	LoadWalls( *map_data );

	LoadModels(
		map_data->models,
		models_geometry_,
		models_geometry_data_,
		models_animations_,
		models_textures_array_id_ );

	// Sky
	if( std::strcmp( current_sky_texture_file_name_, current_map_data_->sky_texture_name ) != 0 )
	{
		std::strncpy( current_sky_texture_file_name_, current_map_data_->sky_texture_name, sizeof(current_sky_texture_file_name_) );

		char sky_texture_file_path[ MapData::c_max_file_path_size ];
		std::snprintf( sky_texture_file_path, sizeof(sky_texture_file_path), "COMMON/%s", current_map_data_->sky_texture_name );

		const Vfs::FileContent sky_texture_data= game_resources_->vfs->ReadFile( sky_texture_file_path );
		const CelTextureHeader& cel_header= *reinterpret_cast<const CelTextureHeader*>( sky_texture_data.data() );

		const unsigned int sky_pixel_count= cel_header.size[0] * cel_header.size[1];
		std::vector<unsigned char> sky_texture_data_rgba( sky_pixel_count * 4u );
		ConvertToRGBA(
			sky_pixel_count,
			sky_texture_data.data() + sizeof(CelTextureHeader),
			game_resources_->palette,
			sky_texture_data_rgba.data() );

		sky_texture_=
			r_Texture(
				r_Texture::PixelFormat::RGBA8,
				cel_header.size[0], cel_header.size[1],
				sky_texture_data_rgba.data() );

		if( filter_textures_ )
			sky_texture_.SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
		else
			sky_texture_.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );
		sky_texture_.BuildMips();
	}

	active_lightmap_= &map_light_.GetFloorLightmap();
}

void MapDrawerGL::Draw(
	const MapState& map_state,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const ViewClipPlanes& view_clip_planes,
	const EntityId player_monster_id )
{
	if( current_map_data_ == nullptr )
		return;

	UpdateDynamicWalls( map_state.GetDynamicWalls() );
	map_light_.Update( map_state );

	m_Mat4 translate;
	translate.Translate( -camera_position );

	const m_Mat4 view_matrix= translate * view_rotation_and_projection_matrix;

	glClear( GL_DEPTH_BUFFER_BIT );

	DrawWalls( view_matrix );

	r_OGLStateManager::UpdateState( g_floors_gl_state );
	DrawFloors( view_matrix );

	r_OGLStateManager::UpdateState( g_models_gl_state );
	DrawModels( map_state, view_matrix, view_clip_planes, false );
	DrawItems( map_state, view_matrix, view_clip_planes, false );
	DrawDynamicItems( map_state, view_matrix, view_clip_planes, false );
	DrawMonsters( map_state, view_matrix, view_clip_planes, player_monster_id, false, true );
	DrawMonstersBodyParts( map_state, view_matrix, view_clip_planes, false );
	DrawRockets( map_state, view_matrix, view_clip_planes, false );
	DrawGibs( map_state, view_matrix, view_clip_planes, false );

	r_OGLStateManager::UpdateState( g_sky_gl_state );
	DrawSky( view_rotation_and_projection_matrix );

	if( settings_.GetOrSetBool( SettingsKeys::shadows, true ) )
	{
		r_OGLStateManager::UpdateState( g_shadows_gl_state );
		DrawMapModelsShadows( map_state, view_matrix, view_clip_planes );
		DrawItemsShadows( map_state, view_matrix, view_clip_planes );
		DrawMonstersShadows( map_state, view_matrix, view_clip_planes, player_monster_id );
	}

	/*
	TRANSPARENT SECTION
	*/
	r_OGLStateManager::UpdateState( g_transparent_models_gl_state );
	DrawModels( map_state, view_matrix, view_clip_planes, true );
	DrawItems( map_state, view_matrix, view_clip_planes, true );
	DrawDynamicItems( map_state, view_matrix, view_clip_planes, true );
	DrawMonsters( map_state, view_matrix, view_clip_planes, player_monster_id, true, false );
	DrawMonsters( map_state, view_matrix, view_clip_planes, player_monster_id, false, false );
	DrawMonstersBodyParts( map_state, view_matrix, view_clip_planes, true );
	DrawRockets( map_state, view_matrix, view_clip_planes, true );
	DrawGibs( map_state, view_matrix, view_clip_planes, true );

	r_OGLStateManager::UpdateState( g_sprites_gl_state );
	DrawBMPObjectsSprites( map_state, view_matrix, camera_position );
	DrawEffectsSprites( map_state, view_matrix, camera_position );
}

void MapDrawerGL::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& projection_matrix,
	const m_Vec3& camera_position,
	const float x_angle, const float z_angle,
	const bool invisible )
{
	weapons_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, weapons_textures_array_id_ );
	active_lightmap_->Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	weapons_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	m_Mat4 shift_mat;
	const m_Vec3 additional_shift=
		g_weapon_shift + g_weapon_change_shift * ( 1.0f - weapon_state.GetSwitchStage() );
	shift_mat.Translate( additional_shift );

	m_Mat3 scale_in_lightmap_mat, lightmap_shift_mat, lightmap_scale_mat;
	scale_in_lightmap_mat.Scale( 0.0f ); // Make model so small, that it will have uniform light.
	lightmap_shift_mat.Translate( camera_position.xy() );
	lightmap_scale_mat.Scale( 1.0f / float(MapData::c_map_size) );
	const m_Mat3 lightmap_mat= scale_in_lightmap_mat * lightmap_shift_mat * lightmap_scale_mat;

	// Rotate model for anisothropic lighting.
	m_Mat4 rotation_mat_x, rotation_mat_z;
	rotation_mat_x.RotateX( x_angle );
	rotation_mat_z.RotateZ( z_angle );

	models_shader_.Uniform( "view_matrix", shift_mat * projection_matrix );
	models_shader_.Uniform( "lightmap_matrix", lightmap_mat );
	models_shader_.Uniform( "rotation_matrix", rotation_mat_x * rotation_mat_z );

	const Model& model= game_resources_->weapons_models[ weapon_state.CurrentWeaponIndex() ];
	const unsigned int frame= model.animations[ weapon_state.CurrentAnimation() ].first_frame + weapon_state.CurrentAnimationFrame();

	const auto draw_model_polygons=
	[&]( const bool transparent )
	{
		const ModelGeometry& model_geometry= weapons_geometry_[ weapon_state.CurrentWeaponIndex() ];
		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			return;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;

		models_shader_.Uniform(
			"first_animation_vertex_number",
			int( model_geometry.first_animations_vertex + model_geometry.animations_vertex_count * frame ) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	};

	glDepthRange( 0.0f, 1.0f / 8.0f );

	r_OGLStateManager::UpdateState( invisible ? g_transparent_models_gl_state : g_models_gl_state );
	draw_model_polygons( false );

	r_OGLStateManager::UpdateState( g_transparent_models_gl_state );
	draw_model_polygons( true );

	glDepthRange( 0.0f, 1.0f );
}

void MapDrawerGL::DrawActiveItemIcon(
	const MapState& map_state,
	const unsigned int icon_item_id,
	const unsigned int slot_number )
{
	const float scale= ItemIconsParams::scale;
	const float aspect=
		float(rendering_context_.viewport_size.Width()) / float(rendering_context_.viewport_size.Height());

	const m_Vec3 pos(
		( aspect + ItemIconsParams::x_shift + float(slot_number) * ItemIconsParams::x_slot_shift ) * scale,
		1.0f,
		ItemIconsParams::y_shift * scale );

	m_Mat4 view_matrix, view_rotate_matrix;
	view_rotate_matrix.RotateX( Constants::half_pi );
	view_matrix.Identity();
	view_matrix[0]= 1.0f / ( aspect * scale );
	view_matrix[5]= -1.0f / scale;

	view_matrix[10]= 1.0f / 64.0f;
	view_matrix[14]= -0.95f; // z hack

	view_matrix= view_rotate_matrix * view_matrix;

	items_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, items_textures_array_id_ );
	map_light_.GetFullbrightLightmapDummy().Bind(1);
	items_animations_.Bind( 2 );
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	m_Mat4 model_matrix, rotation_matrix;
	m_Mat3 lightmap_matrix;
	CreateModelMatrices( pos, 0.0f, model_matrix, lightmap_matrix );
	rotation_matrix.Identity();

	const ModelGeometry& model_geometry= items_geometry_[ icon_item_id ];
	const Model& model= game_resources_->items_models[ icon_item_id ];

	const unsigned int frame_number=
		static_cast<unsigned int>(map_state.GetSpritesFrame()) % model.frame_count;
	const unsigned int first_animation_vertex=
		model_geometry.first_animations_vertex +
		model_geometry.animations_vertex_count * frame_number;

	models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
	models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
	models_shader_.Uniform( "rotation_matrix", rotation_matrix );
	models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

	for( unsigned int t= 0u; t < 2u; ++t )
	{
		const bool transparent= t == 1u;
		r_OGLStateManager::UpdateState( transparent ? g_transparent_models_gl_state : g_models_gl_state );

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DoFullscreenPostprocess( const MapState& map_state )
{
	m_Vec3 blend_color;
	float blend_alpha;
	map_state.GetFullscreenBlend( blend_color, blend_alpha );
	if( blend_alpha > 0.001f )
	{
		r_OGLStateManager::UpdateState( g_fullscreen_blend_state );
		fullscreen_blend_shader_.Bind();
		fullscreen_blend_shader_.Uniform( "blend_color", blend_color.x, blend_color.y, blend_color.z, blend_alpha );
		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}

}

void MapDrawerGL::DrawMapRelatedModels(
	const MapRelatedModel* const models, const unsigned int model_count,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const ViewClipPlanes& view_clip_planes )
{
	if( current_map_data_ == nullptr )
		return;

	glClear( GL_DEPTH_BUFFER_BIT );

	m_Mat4 translate;
	translate.Translate( -camera_position );

	const m_Mat4 view_matrix= translate * view_rotation_and_projection_matrix;

	models_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, models_textures_array_id_ );
	active_lightmap_->Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	models_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( unsigned int t= 0u; t < 2u; t++ )
	{
		const bool transparent= t > 0u;

		r_OGLStateManager::UpdateState( transparent ? g_transparent_models_gl_state : g_models_gl_state );

		for( unsigned int m= 0u; m < model_count; m++ )
		{
			const MapRelatedModel& map_model= models[m];

			if( map_model.model_id >= models_geometry_.size() )
				continue;

			const ModelGeometry& model_geometry= models_geometry_[ map_model.model_id ];
			const Model& model= current_map_data_->models[ map_model.model_id ];

			const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
			if( index_count == 0u )
				continue;

			const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
			const unsigned int first_animation_vertex=
				model_geometry.first_animations_vertex +
				model_geometry.animations_vertex_count * map_model.frame;

			m_Mat4 model_matrix, rotation_matrix;
			m_Mat3 lightmap_matrix;
			CreateModelMatrices( map_model.pos, map_model.angle_z, model_matrix, lightmap_matrix );
			rotation_matrix.RotateZ( map_model.angle_z );

			PC_ASSERT( map_model.frame < model.frame_count );

			const m_BBox3& bbox= model.animations_bboxes[ map_model.frame ];
			if( BBoxIsOutsideView( view_clip_planes, bbox, model_matrix ) )
				continue;

			models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
			models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
			models_shader_.Uniform( "rotation_matrix", rotation_matrix );
			models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

			glDrawElementsBaseVertex(
				GL_TRIANGLES,
				index_count,
				GL_UNSIGNED_SHORT,
				reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
				model_geometry.first_vertex_index );
		}
	} // for transparent/untransparent
}

void MapDrawerGL::LoadSprites( const std::vector<ObjSprite>& sprites, std::vector<GLuint>& out_textures )
{
	const Palette& palette= game_resources_->palette;

	std::vector<unsigned char> data_rgba;

	out_textures.resize( sprites.size() );
	glGenTextures( out_textures.size(), out_textures.data() );
	for( unsigned int i= 0u; i < out_textures.size(); i++ )
	{
		const ObjSprite& sprite= sprites[i];

		data_rgba.clear();
		data_rgba.resize( sprite.data.size() * 4u, 0u );

		for( unsigned int j= 0u; j < sprite.data.size(); j++ )
		{
			const unsigned char color_index= sprite.data[j];

			unsigned char* const dst= data_rgba.data() + 4u * j;

			dst[0]= palette[ 3u * color_index + 0u ];
			dst[1]= palette[ 3u * color_index + 1u ];
			dst[2]= palette[ 3u * color_index + 2u ];
			dst[3]= color_index == 255u ? 0u : 255u;
		}

		if( filter_textures_ )
		{
			for( unsigned int j= 0u; j < sprite.frame_count; j++ )
				FillAlphaTexelsColorRGBA(
					sprite.size[0], sprite.size[1],
					data_rgba.data() + 4u * sprite.size[0] * sprite.size[1] * j );
		}

		glBindTexture( GL_TEXTURE_2D_ARRAY, out_textures[i] );
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
			sprite.size[0], sprite.size[1], sprite.frame_count,
			0, GL_RGBA, GL_UNSIGNED_BYTE, data_rgba.data() );

		if( filter_textures_ )
		{
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		}
		else
		{
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
		}
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	}
}

void MapDrawerGL::PrepareSkyGeometry()
{
	static const float c_scene_radius= 64.0f;
	static const float c_bottom_z_level= -c_scene_radius * 0.5f;
	static const float c_sky_vertices[]=
	{
		c_scene_radius,  c_scene_radius,   c_scene_radius,  -c_scene_radius,  c_scene_radius,   c_scene_radius,
		c_scene_radius, -c_scene_radius,   c_scene_radius,  -c_scene_radius, -c_scene_radius,   c_scene_radius,
		c_scene_radius,  c_scene_radius, c_bottom_z_level,  -c_scene_radius,  c_scene_radius, c_bottom_z_level,
		c_scene_radius, -c_scene_radius, c_bottom_z_level,  -c_scene_radius, -c_scene_radius, c_bottom_z_level,
	};
	static const unsigned short c_sky_indeces[]=
	{
		0, 1, 5,  0, 5, 4,
		0, 4, 6,  0, 6, 2,
		//4, 5, 7,  4, 7, 6, // bottom
		0, 3, 1,  0, 2, 3, // top
		2, 7, 3,  2, 6, 7,
		1, 3, 7,  1, 7, 5,
	};

	sky_geometry_data_.VertexData( c_sky_vertices, sizeof(c_sky_vertices), sizeof(float) * 3u );
	sky_geometry_data_.IndexData( c_sky_indeces, sizeof(c_sky_indeces), GL_UNSIGNED_SHORT, GL_TRIANGLES );
	sky_geometry_data_.VertexAttribPointer( 0u, 3u, GL_FLOAT, false, 0u );
}

const r_Texture& MapDrawerGL::GetPlayerTexture( const unsigned char color )
{
	// Should be done after monsters loading.
	PC_ASSERT( !monsters_models_.empty() );

	const unsigned char color_corrected= color % GameConstants::player_colors_count;

	// Default texture (unshifted).
	if( color_corrected == 0u )
		return monsters_models_.front().texture;

	if( player_textures_.size() <= color_corrected )
		player_textures_.resize( color_corrected + 1u );

	r_Texture& texture= player_textures_[ color_corrected ];
	if( !texture.IsEmpty() ) // Texture already generated.
		return texture;

	// Generate new texture and save it.

	const Model& model= game_resources_->monsters_models.front();
	const unsigned int pixel_count= model.texture_data.size();

	// TODO - maybe cache buffers?
	std::vector<unsigned char> data_shifted( pixel_count );
	std::vector<unsigned char> data_rgba( pixel_count * 4u );

	ColorShift(
		14 * 16u, 14 * 16u + 16u,
		GameConstants::player_colors_shifts[ color_corrected ],
		pixel_count,
		model.texture_data.data(),
		data_shifted.data() );
	ConvertToRGBA(
		pixel_count,
		data_shifted.data(),
		game_resources_->palette,
		data_rgba.data() );

	texture=
		r_Texture(
			r_Texture::PixelFormat::RGBA8,
			model.texture_size[0], model.texture_size[1],
			data_rgba.data() );

	if( filter_textures_ )
		texture.SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
	else
		texture.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );

	texture.BuildMips();

	return texture;
}

void MapDrawerGL::LoadFloorsTextures( const MapData& map_data )
{
	const Palette& palette= game_resources_->palette;

	const unsigned int texture_texels= MapData::c_floor_texture_size * MapData::c_floor_texture_size;

	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		MapData::c_floor_texture_size, MapData::c_floor_texture_size, MapData::c_floors_textures_count,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	for( unsigned int t= 0u; t < MapData::c_floors_textures_count; t++ )
	{
		const unsigned char* const in_data= map_data.floor_textures_data[t];

		unsigned char texture_data[ 4u * texture_texels ];

		for( unsigned int i= 0u; i < texture_texels; i++ )
		{
			const unsigned char color_index= in_data[i];
			for( unsigned int j= 0u; j < 3u; j++ )
				texture_data[ (i << 2u) + j ]= palette[ color_index * 3u + j ];
			texture_data[ (i << 2u) + 3u ]= 255u;
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, t,
			MapData::c_floor_texture_size, MapData::c_floor_texture_size, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for textures

	if( filter_textures_ )
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	}

	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapDrawerGL::LoadWallsTextures( const MapData& map_data )
{
	Log::Info( "Loading walls textures for map" );

	const Palette& palette= game_resources_->palette;

	Vfs::FileContent texture_file;

	glBindTexture( GL_TEXTURE_2D_ARRAY, wall_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		g_max_wall_texture_width, g_wall_texture_height, MapData::c_max_walls_textures,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	for( unsigned int t= 0u; t < MapData::c_max_walls_textures; t++ )
	{
		if( map_data.walls_textures[t].file_path[0] == '\0' )
			continue;

		game_resources_->vfs->ReadFile( map_data.walls_textures[t].file_path, texture_file );
		if( texture_file.empty() )
			continue;

		unsigned char texture_data[ g_max_wall_texture_width * g_wall_texture_height * 4u ];

		unsigned short src_width, src_height;
		std::memcpy( &src_width , texture_file.data() + 0x2u, sizeof(unsigned short) );
		std::memcpy( &src_height, texture_file.data() + 0x4u, sizeof(unsigned short) );

		if( g_max_wall_texture_width / src_width * src_width != g_max_wall_texture_width ||
			src_height < g_wall_texture_height )
		{
			Log::Warning( "Invalid wall texture size: ", src_width, "x", src_height );
			continue;
		}

		const unsigned char* const src= texture_file.data() + 0x320u;

		for( unsigned int y= 0u; y < g_wall_texture_height; y++ )
		{
			const unsigned int y_flipped= g_wall_texture_height - 1u - y;

			for( unsigned int x= 0u; x < src_width; x++ )
			{
				const unsigned int color_index= src[ x + y_flipped * src_width ];
				const unsigned int i= ( x + y * g_max_wall_texture_width ) << 2;

				for( unsigned int j= 0u; j < 3u; j++ )
					texture_data[ i + j ]= palette[ color_index * 3u + j ];

				texture_data[ i + 3u ]= color_index == 255u ? 0u : 255u;
			}

			const unsigned int repeats= g_max_wall_texture_width / src_width;
			unsigned char* const line= texture_data + g_max_wall_texture_width * 4u * y;
			for( unsigned int r= 1u; r < repeats; r++ )
				std::memcpy(
					line + r * src_width * 4u,
					line,
					src_width * 4u );
		}

		// Fill alpha-texels with color of neighbor texels.
		// This needs only if textures filtering is enabled, for prevention of ugly dark outlines around nonalpha texels.
		if( filter_textures_ )
			FillAlphaTexelsColorRGBA( g_max_wall_texture_width, g_wall_texture_height, texture_data );

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, t,
			g_max_wall_texture_width, g_wall_texture_height, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for wall textures

	if( filter_textures_ )
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	}

	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapDrawerGL::LoadFloors( const MapData& map_data )
{
	std::vector<FloorVertex> floors_vertices;
	for( unsigned int floor_or_ceiling= 0u; floor_or_ceiling < 2u; floor_or_ceiling++ )
	{
		floors_geometry_info[ floor_or_ceiling ].first_vertex_number= floors_vertices.size();

		const unsigned char* const in_data= floor_or_ceiling == 0u ? map_data.floor_textures : map_data.ceiling_textures;

		for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
		for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
		{
			const unsigned char texture_number= in_data[ x + y * MapData::c_map_size ];

			if( texture_number == MapData::c_empty_floor_texture_id ||
				texture_number == MapData::c_sky_floor_texture_id )
				continue;

			floors_vertices.resize( floors_vertices.size() + 6u );
			FloorVertex* v= floors_vertices.data() + floors_vertices.size() - 6u;

			v[0].xy[0]= x   ;  v[0].xy[1]= y;
			v[1].xy[0]= x+1u;  v[1].xy[1]= y;
			v[2].xy[0]= x+1u;  v[2].xy[1]= y+1u;
			v[3].xy[0]= x   ;  v[3].xy[1]= y+1u;

			v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= texture_number;
			v[4]= v[0];
			v[5]= v[2];
		} // for xy

		floors_geometry_info[ floor_or_ceiling ].vertex_count=
			floors_vertices.size() - floors_geometry_info[ floor_or_ceiling ].first_vertex_number;
	} // for floor and ceiling

	floors_geometry_.VertexData(
		floors_vertices.data(),
		floors_vertices.size() * sizeof(FloorVertex),
		sizeof(FloorVertex) );

	FloorVertex v;

	floors_geometry_.VertexAttribPointer(
		0, 2, GL_UNSIGNED_BYTE, false,
		((char*)v.xy) - ((char*)&v) );

	floors_geometry_.VertexAttribPointerInt(
		1, 1, GL_UNSIGNED_BYTE,
		((char*)&v.texture_id) - ((char*)&v) );
}

void MapDrawerGL::LoadWalls( const MapData& map_data )
{
	std::vector<WallVertex> walls_vertices;
	std::vector<unsigned short> walls_indeces;

	walls_vertices.reserve( map_data.static_walls.size() * 4u );
	walls_indeces.reserve( map_data.static_walls.size() * 6u );

	for( const MapData::Wall& wall : map_data.static_walls )
	{
		if( map_data.walls_textures[ wall.texture_id ].file_path[0] == '\0' )
			continue; // Wall has no texture - do not draw it.

		const unsigned int first_vertex_index= walls_vertices.size();
		walls_vertices.resize( walls_vertices.size() + 4u );
		WallVertex* const v= walls_vertices.data() + first_vertex_index;

		v[0].xyz[0]= v[2].xyz[0]= static_cast<unsigned short>( wall.vert_pos[0].x * g_walls_coords_scale );
		v[0].xyz[1]= v[2].xyz[1]= static_cast<unsigned short>( wall.vert_pos[0].y * g_walls_coords_scale );
		v[1].xyz[0]= v[3].xyz[0]= static_cast<unsigned short>( wall.vert_pos[1].x * g_walls_coords_scale );
		v[1].xyz[1]= v[3].xyz[1]= static_cast<unsigned short>( wall.vert_pos[1].y * g_walls_coords_scale );

		v[0].xyz[2]= v[1].xyz[2]= 0u;
		v[2].xyz[2]= v[3].xyz[2]= 2u << 8u;

		v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= wall.texture_id;

		v[2].tex_coord[0]= v[0].tex_coord[0]= static_cast<short>( wall.vert_tex_coord[0] * 256.0f );
		v[1].tex_coord[0]= v[3].tex_coord[0]= static_cast<short>( wall.vert_tex_coord[1] * 256.0f );

		v[0].tex_coord[1]= v[1].tex_coord[1]= 0;
		v[2].tex_coord[1]= v[3].tex_coord[1]= 256;

		map_light_.GetStaticWallLightmapCoord(
			&wall - map_data.static_walls.data(),
			v[0].lightmap_coord );

		v[2].lightmap_coord[0]= v[0].lightmap_coord[0];
		v[2].lightmap_coord[1]= v[0].lightmap_coord[1];
		v[3].lightmap_coord[0]= v[1].lightmap_coord[0]= v[0].lightmap_coord[0] + 1u;
		v[3].lightmap_coord[1]= v[1].lightmap_coord[1]= v[0].lightmap_coord[1];

		m_Vec2 wall_vec=
			m_Vec2(float(v[0].xyz[0]), float(v[0].xyz[1])) -
			m_Vec2(float(v[1].xyz[0]), float(v[1].xyz[1]));
		wall_vec.Normalize();

		const int normal[2]= {
			int( +126.5f * wall_vec.y ),
			int( -126.5f * wall_vec.x ) };
		for( unsigned int j= 0u; j < 4u; j++ )
		{
			v[j].normal[0]= normal[0];
			v[j].normal[1]= normal[1];
		}

		const unsigned int first_index= walls_indeces.size();
		walls_indeces.resize( walls_indeces.size() + 6u );
		unsigned short* const ind= walls_indeces.data() + first_index;
		ind[0]= first_vertex_index + 3u;
		ind[1]= first_vertex_index + 1u;
		ind[2]= first_vertex_index + 0u;
		ind[3]= first_vertex_index + 2u;
		ind[4]= first_vertex_index + 3u;
		ind[5]= first_vertex_index + 0u;

		// Duplicate transparent walls for pervent of back face culling.
		// TODO - maybe duplicate wertices and set reverse normal?
		if( wall.texture_id >= MapData::c_first_transparent_texture_id )
		{
			const unsigned int first_reverse_index= walls_indeces.size();
			walls_indeces.resize( walls_indeces.size() + 6u );
			unsigned short* const ind= walls_indeces.data() + first_reverse_index;
			ind[0]= first_vertex_index + 0u;
			ind[1]= first_vertex_index + 1u;
			ind[2]= first_vertex_index + 3u;
			ind[3]= first_vertex_index + 0u;
			ind[4]= first_vertex_index + 3u;
			ind[5]= first_vertex_index + 2u;
		}
	} // for walls

	const auto setup_attribs=
	[]( r_PolygonBuffer& polygon_buffer )
	{
		WallVertex v;

		polygon_buffer.VertexAttribPointer(
			0, 3, GL_UNSIGNED_SHORT, false,
			((char*)v.xyz) - ((char*)&v) );

		polygon_buffer.VertexAttribPointer(
			1, 2, GL_SHORT, false,
			((char*)v.tex_coord) - ((char*)&v) );

		polygon_buffer.VertexAttribPointerInt(
			2, 1, GL_UNSIGNED_BYTE,
			((char*)&v.texture_id) - ((char*)&v) );

		polygon_buffer.VertexAttribPointer(
			3, 2, GL_BYTE, true,
			((char*)v.normal) - ((char*)&v) );

		polygon_buffer.VertexAttribPointer(
			4, 2, GL_UNSIGNED_BYTE, false,
			((char*)v.lightmap_coord) - ((char*)&v) );
	};

	walls_geometry_.VertexData(
		walls_vertices.data(),
		walls_vertices.size() * sizeof(WallVertex),
		sizeof(WallVertex) );

	walls_geometry_.IndexData(
		walls_indeces.data(),
		walls_indeces.size() * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	setup_attribs( walls_geometry_ );

	// Reserve place for dynamic walls geometry
	dynamc_walls_vertices_.resize( map_data.dynamic_walls.size() * 4u );

	// Prepare indeces for dynamic walls.
	walls_indeces.clear();
	walls_indeces.resize( map_data.dynamic_walls.size() * 6u );
	for( unsigned int w= 0u; w < map_data.dynamic_walls.size(); w++ )
	{
		unsigned short* const ind= walls_indeces.data() + w * 6u;
		const unsigned int first_vertex_index= w * 4u;
		ind[0]= first_vertex_index + 0u;
		ind[1]= first_vertex_index + 1u;
		ind[2]= first_vertex_index + 3u;
		ind[3]= first_vertex_index + 0u;
		ind[4]= first_vertex_index + 3u;
		ind[5]= first_vertex_index + 2u;
	}

	dynamic_walls_geometry_.VertexData(
		nullptr,
		sizeof(WallVertex) * map_data.dynamic_walls.size() * 4u,
		sizeof(WallVertex) );

	dynamic_walls_geometry_.IndexData(
		walls_indeces.data(),
		map_data.dynamic_walls.size() * 6u * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	setup_attribs( dynamic_walls_geometry_ );
}

void MapDrawerGL::LoadModels(
	const std::vector<Model>& models,
	std::vector<ModelGeometry>& out_geometry,
	r_PolygonBuffer& out_geometry_data,
	AnimationsBuffer& out_animations_buffer,
	GLuint& out_textures_array ) const
{
	ModelsTexturesCorrector textures_corrector;

	const Palette& palette= game_resources_->palette;

	const unsigned int model_count= models.size();
	out_geometry.resize( model_count );

	const unsigned int c_texture_size[2]= { 64u, 2048u };
	ModelsTexturesPlacement textures_placement;
	CalculateModelsTexturesPlacement( models, c_texture_size[1], textures_placement );

	const unsigned int c_texels_in_layer= c_texture_size[0u] * c_texture_size[1u];
	std::vector<unsigned char> textures_data_rgba( 4u * c_texels_in_layer * textures_placement.layer_count, 0u );

	std::vector<unsigned  short> indeces;
	std::vector<Model::Vertex> vertices;
	std::vector<Model::AnimationVertex> animations_vertices;

	for( unsigned int m= 0u; m < model_count; m++ )
	{
		const Model& model= models[m];
		ModelGeometry& model_geometry= out_geometry[m];

		unsigned int model_texture_height= model.texture_size[1];
		if( model.texture_size[1u] > c_texture_size[1u] )
		{
			Log::Warning( "Model texture height is too big: ", model.texture_size[1u] );
			model_texture_height= c_texture_size[1u];
		}

		// Copy texture into atlas.
		unsigned char* const texture_dst=
			textures_data_rgba.data() +
			4u * textures_placement.textures_placement[m].layer * c_texels_in_layer +
			4u * textures_placement.textures_placement[m].y * c_texture_size[0];
		for( unsigned int y= 0u; y < model_texture_height; y++ )
		for( unsigned int x= 0u; x < model.texture_size[0u]; x++ )
		{
			const unsigned int i= ( x + y * c_texture_size[0u] ) << 2u;
			const unsigned char color_index= model.texture_data[ x + y * model.texture_size[0u] ];
			for( unsigned int j= 0u; j < 3u; j++ )
				texture_dst[ i + j ]= palette[ color_index * 3u + j ];
			texture_dst[ i + 3u ]= color_index == 0u ? 0u : 255u;
		}

		if( filter_textures_ )
			textures_corrector.CorrectModelTexture( model, texture_dst );

		if( model_texture_height > 0u )
		{
			// Fill lower border
			for( unsigned int dy= 1u; dy < std::min( textures_placement.textures_placement[m].y, 4u ); dy++ )
			{
				const unsigned char* const src= texture_dst;
				unsigned char* const dst= texture_dst - 4u * dy * c_texture_size[0];
				std::memcpy( dst, src, model.texture_size[0u] * 4u );
			}
			// Fill upper border
			for( unsigned int dy= 0u; dy < 4u && textures_placement.textures_placement[m].y + model_texture_height + dy < c_texture_size[1]; dy++ )
			{
				const unsigned char* const src= texture_dst + 4u * c_texture_size[0] * ( model_texture_height - 1u );
				unsigned char* const dst= texture_dst +  4u * c_texture_size[0] * ( model_texture_height + dy );
				std::memcpy( dst, src, model.texture_size[0u] * 4u );
			}
		}

		// Copy vertices, transform textures coordinates, set texture layer.
		const unsigned int first_vertex_index= vertices.size();
		vertices.resize( vertices.size() + model.vertices.size() );
		Model::Vertex* const vertex= vertices.data() + first_vertex_index;

		const float tex_coord_scaler[2u]=
		{
			float(model.texture_size[0u]) / float(c_texture_size[0u]),
			float(model.texture_size[1u]) / float(c_texture_size[1u]),
		};
		for( unsigned int v= 0u; v < model.vertices.size(); v++ )
		{
			vertex[v]= model.vertices[v];
			vertex[v].texture_id= textures_placement.textures_placement[m].layer;
			for( unsigned int j= 0u; j < 2u; j++ )
				vertex[v].tex_coord[j]*= tex_coord_scaler[j];

			vertex[v].tex_coord[1]+=
				float(textures_placement.textures_placement[m].y) / float(c_texture_size[1]);
		}

		// Copy animations vertices.
		const unsigned int first_animation_vertex_index= animations_vertices.size();
		animations_vertices.resize( animations_vertices.size() + model.animations_vertices.size() );
		std::memcpy(
			animations_vertices.data() + first_animation_vertex_index,
			model.animations_vertices.data(),
			model.animations_vertices.size() * sizeof( Model::AnimationVertex ) );

		// Copy indeces.
		const unsigned int first_index= indeces.size();
		indeces.resize( indeces.size() + model.regular_triangles_indeces.size() + model.transparent_triangles_indeces.size() );
		unsigned short* const ind= indeces.data() + first_index;

		std::memcpy(
			ind,
			model.regular_triangles_indeces.data(),
			model.regular_triangles_indeces.size() * sizeof(unsigned short) );

		std::memcpy(
			ind + model.regular_triangles_indeces.size(),
			model.transparent_triangles_indeces.data(),
			model.transparent_triangles_indeces.size() * sizeof(unsigned short) );

		// Setup geometry info.
		model_geometry.frame_count= model.frame_count;
		model_geometry.animations_vertex_count= model.frame_count == 0u ? 0u : model.animations_vertices.size() / model.frame_count;
		model_geometry.first_animations_vertex= first_animation_vertex_index;

		model_geometry.vertex_count= model.vertices.size();
		model_geometry.first_vertex_index= first_vertex_index;

		model_geometry.first_index= first_index;
		model_geometry.index_count= model.regular_triangles_indeces.size();

		model_geometry.first_transparent_index= first_index + model.regular_triangles_indeces.size();
		model_geometry.transparent_index_count= model.transparent_triangles_indeces.size();

	} // for models

	// Prepare texture.
	glBindTexture( GL_TEXTURE_2D_ARRAY, out_textures_array );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		c_texture_size[0u], c_texture_size[1u], textures_placement.layer_count,
		0, GL_RGBA, GL_UNSIGNED_BYTE, textures_data_rgba.data() );

	if( filter_textures_ )
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	}
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LOD, 1 );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

	// Prepare animations buffer
	if( use_2d_textures_for_animations_ )
		out_animations_buffer= AnimationsBuffer::As2dTexture( animations_vertices );
	else
		out_animations_buffer= AnimationsBuffer::AsTextureBuffer( animations_vertices );

	PrepareModelsPolygonBuffer( vertices, indeces, out_geometry_data );
}

void MapDrawerGL::LoadMonstersModels()
{
	ModelsTexturesCorrector textures_corrector;

	const std::vector<Model>& in_models= game_resources_->monsters_models;
	const Palette& palette= game_resources_->palette;

	monsters_models_.resize( in_models.size() );

	std::vector<unsigned char> texture_data_rgba;

	std::vector<unsigned  short> indeces;
	std::vector<Model::Vertex> vertices;
	std::vector<Model::AnimationVertex> animations_vertices;

	// TODO - load gibs from models.
	for( unsigned int m= 0u; m < monsters_models_.size(); m++ )
	{
		const Model& in_model= in_models[m];
		MonsterModel& out_model= monsters_models_[m];

		// Prepare texture.
		texture_data_rgba.clear();
		texture_data_rgba.resize( in_model.texture_data.size() * 4u );
		for( unsigned int i= 0u; i < in_model.texture_data.size(); i++ )
		{
			const unsigned char color_index= in_model.texture_data[i];
			unsigned char* const out_pixel= texture_data_rgba.data() + i * 4u;
			for( unsigned int j= 0u; j < 3u; j++ )
				out_pixel[ j ]= palette[ color_index * 3u + j ];
			out_pixel[ 3u ]= color_index == 0u ? 0u : 255u;
		}

		if( filter_textures_ )
			textures_corrector.CorrectModelTexture( in_model, texture_data_rgba.data() );

		out_model.texture=
			r_Texture(
				r_Texture::PixelFormat::RGBA8,
				in_model.texture_size[0], in_model.texture_size[1],
				texture_data_rgba.data() );

		if( filter_textures_ )
			out_model.texture.SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
		else
			out_model.texture.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );
		out_model.texture.BuildMips();

		const auto prepare_geometry=
		[&]( const Submodel& submodel, ModelGeometry& model_geometry )
		{
			// Copy vertices.
			const unsigned int first_vertex_index= vertices.size();
			vertices.resize( vertices.size() + submodel.vertices.size() );
			std::memcpy(
				vertices.data() + first_vertex_index,
				submodel.vertices.data(),
				submodel.vertices.size() * sizeof(Model::Vertex) );

			// Copy animations vertices.
			const unsigned int first_animation_vertex_index= animations_vertices.size();
			animations_vertices.resize( animations_vertices.size() + submodel.animations_vertices.size() );
			std::memcpy(
				animations_vertices.data() + first_animation_vertex_index,
				submodel.animations_vertices.data(),
				submodel.animations_vertices.size() * sizeof( Model::AnimationVertex ) );

			// Copy indeces.
			const unsigned int first_index= indeces.size();
			indeces.resize( indeces.size() + submodel.regular_triangles_indeces.size() + submodel.transparent_triangles_indeces.size() );
			unsigned short* const ind= indeces.data() + first_index;

			std::memcpy(
				ind,
				submodel.regular_triangles_indeces.data(),
				submodel.regular_triangles_indeces.size() * sizeof(unsigned short) );

			std::memcpy(
				ind + submodel.regular_triangles_indeces.size(),
				submodel.transparent_triangles_indeces.data(),
				submodel.transparent_triangles_indeces.size() * sizeof(unsigned short) );

			// Setup geometry info.
			model_geometry.frame_count= submodel.frame_count;
			model_geometry.animations_vertex_count= submodel.frame_count == 0u ? 0u : submodel.animations_vertices.size() / submodel.frame_count;
			model_geometry.first_animations_vertex= first_animation_vertex_index;

			model_geometry.vertex_count= submodel.vertices.size();
			model_geometry.first_vertex_index= first_vertex_index;

			model_geometry.first_index= first_index;
			model_geometry.index_count= submodel.regular_triangles_indeces.size();

			model_geometry.first_transparent_index= first_index + submodel.regular_triangles_indeces.size();
			model_geometry.transparent_index_count= submodel.transparent_triangles_indeces.size();
		};

		prepare_geometry( in_model, out_model.geometry_description );

		PC_ASSERT( in_model.submodels.size() == 3u );
		for( unsigned int s= 0u; s < 3u; s++ )
		{
			prepare_geometry( in_model.submodels[s], out_model.submodels_geometry_description[s] );
		}
	}

	// Prepare animations buffer
	if( use_2d_textures_for_animations_ )
		monsters_animations_= AnimationsBuffer::As2dTexture( animations_vertices );
	else
		monsters_animations_= AnimationsBuffer::AsTextureBuffer( animations_vertices );

	PrepareModelsPolygonBuffer( vertices, indeces, monsters_geometry_data_ );
}

void MapDrawerGL::UpdateDynamicWalls( const MapState::DynamicWalls& dynamic_walls )
{
	PC_ASSERT( current_map_data_->dynamic_walls.size() == dynamic_walls.size() );

	for( unsigned int w= 0u; w < dynamic_walls.size(); w++ )
	{
		const MapData::Wall& map_wall= current_map_data_->dynamic_walls[w];
		const MapState::DynamicWall wall= dynamic_walls[w];
		// TODO - discard walls without textures.

		WallVertex* const v= dynamc_walls_vertices_.data() + w * 4u;

		v[0].xyz[0]= v[2].xyz[0]= short( wall.vert_pos[0].x * 256.0f );
		v[0].xyz[1]= v[2].xyz[1]= short( wall.vert_pos[0].y * 256.0f );
		v[1].xyz[0]= v[3].xyz[0]= short( wall.vert_pos[1].x * 256.0f );
		v[1].xyz[1]= v[3].xyz[1]= short( wall.vert_pos[1].y * 256.0f );

		v[0].xyz[2]= v[1].xyz[2]= short( wall.z * 256.0f );
		v[2].xyz[2]= v[3].xyz[2]=v[0].xyz[2] + (2u << 8u);

		v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= wall.texture_id;

		v[2].tex_coord[0]= v[0].tex_coord[0]= static_cast<short>( map_wall.vert_tex_coord[0] * 256.0f );
		v[1].tex_coord[0]= v[3].tex_coord[0]= static_cast<short>( map_wall.vert_tex_coord[1] * 256.0f );

		v[0].tex_coord[1]= v[1].tex_coord[1]= 0;
		v[2].tex_coord[1]= v[3].tex_coord[1]= 256;

		map_light_.GetDynamicWallLightmapCoord( w, v[0].lightmap_coord );

		v[2].lightmap_coord[0]= v[0].lightmap_coord[0];
		v[2].lightmap_coord[1]= v[0].lightmap_coord[1];
		v[3].lightmap_coord[0]= v[1].lightmap_coord[0]= v[0].lightmap_coord[0] + 1u;
		v[3].lightmap_coord[1]= v[1].lightmap_coord[1]= v[0].lightmap_coord[1];

		m_Vec2 wall_vec=
			m_Vec2(float(v[0].xyz[0]), float(v[0].xyz[1])) -
			m_Vec2(float(v[1].xyz[0]), float(v[1].xyz[1]));
		wall_vec.Normalize();

		const int normal[2]= {
			int( +126.5f * wall_vec.y ),
			int( -126.5f * wall_vec.x ) };
		for( unsigned int j= 0u; j < 4u; j++ )
		{
			v[j].normal[0]= normal[0];
			v[j].normal[1]= normal[1];
		}
	}

	dynamic_walls_geometry_.VertexSubData(
		dynamc_walls_vertices_.data(),
		dynamc_walls_vertices_.size() * sizeof(WallVertex),
		0u );
}

void MapDrawerGL::PrepareModelsPolygonBuffer(
	std::vector<Model::Vertex>& vertices,
	std::vector<unsigned short>& indeces,
	r_PolygonBuffer& buffer )
{
	buffer.VertexData(
		vertices.data(),
		vertices.size() * sizeof(Model::Vertex),
		sizeof(Model::Vertex) );

	buffer.IndexData(
		indeces.data(),
		indeces.size() * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	Model::Vertex v;
	buffer.VertexAttribPointerInt(
		0, 1, GL_UNSIGNED_SHORT,
		((char*)&v.vertex_id) - ((char*)&v) );

	buffer.VertexAttribPointer(
		1, 3, GL_FLOAT, false,
		((char*)v.tex_coord) - ((char*)&v) );

	buffer.VertexAttribPointerInt(
		2, 1, GL_UNSIGNED_BYTE,
		((char*)&v.texture_id) - ((char*)&v) );

	buffer.VertexAttribPointer(
		3, 1, GL_UNSIGNED_BYTE, true,
		((char*)&v.alpha_test_mask) - ((char*)&v) );

	buffer.VertexAttribPointerInt(
		4, 1, GL_UNSIGNED_BYTE,
		((char*)&v.groups_mask) - ((char*)&v) );
}

void MapDrawerGL::DrawWalls( const m_Mat4& view_matrix )
{
	walls_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, wall_textures_array_id_ );
	//active_lightmap_->Bind(1);
	map_light_.GetWallsLightmap().Bind(1);

	walls_shader_.Uniform( "tex", int(0) );
	walls_shader_.Uniform( "lightmap", int(1) );

	m_Mat4 scale_mat;
	scale_mat.Scale( 1.0f / 256.0f );
	walls_shader_.Uniform( "view_matrix", scale_mat * view_matrix );

	r_OGLStateManager::UpdateState( g_static_walls_gl_state );
	walls_geometry_.Bind();
	walls_geometry_.Draw();

	r_OGLStateManager::UpdateState( g_dynamic_walls_gl_state );
	dynamic_walls_geometry_.Bind();
	dynamic_walls_geometry_.Draw();
}

void MapDrawerGL::DrawFloors( const m_Mat4& view_matrix )
{
	floors_geometry_.Bind();
	floors_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	active_lightmap_->Bind(1);

	floors_shader_.Uniform( "tex", int(0) );
	floors_shader_.Uniform( "lightmap", int(1) );

	floors_shader_.Uniform( "view_matrix", view_matrix );

	for( unsigned int z= 0u; z < 2u; z++ )
	{
		floors_shader_.Uniform( "pos_z", float(z) * 2.0f );

		glDrawArrays(
			GL_TRIANGLES,
			floors_geometry_info[z].first_vertex_number,
			floors_geometry_info[z].vertex_count );
	}
}

void MapDrawerGL::DrawModels(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const bool transparent )
{
	models_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, models_textures_array_id_ );
	active_lightmap_->Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	models_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= models_geometry_.size() ||
			!static_model.visible )
			continue;

		const ModelGeometry& model_geometry= models_geometry_[ static_model.model_id ];
		const Model& model= current_map_data_->models[ static_model.model_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			model_geometry.animations_vertex_count * static_model.animation_frame;

		m_Mat4 model_matrix, rotation_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( static_model.pos, static_model.angle, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( static_model.angle );

		PC_ASSERT( static_model.animation_frame < model.frame_count );

		const m_BBox3& bbox= model.animations_bboxes[ static_model.animation_frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_matrix ) )
			continue;

		models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
		models_shader_.Uniform( "rotation_matrix", rotation_matrix );
		models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawItems(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const bool transparent )
{
	items_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, items_textures_array_id_ );
	active_lightmap_->Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	items_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::Item& item : map_state.GetItems() )
	{
		if( item.picked_up || item.item_id >= items_geometry_.size() )
			continue;

		const ModelGeometry& model_geometry= items_geometry_[ item.item_id ];
		const Model& model= game_resources_->items_models[ item.item_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			model_geometry.animations_vertex_count * item.animation_frame;

		m_Mat4 model_matrix, rotation_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( item.pos, item.angle, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( item.angle );

		PC_ASSERT( item.animation_frame < model.frame_count );

		const m_BBox3& bbox= model.animations_bboxes[ item.animation_frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_matrix ) )
			continue;

		models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
		models_shader_.Uniform( "rotation_matrix", rotation_matrix );
		models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawDynamicItems(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const bool transparent )
{
	items_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, items_textures_array_id_ );
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	items_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::DynamicItemsContainer::value_type& item_value : map_state.GetDynamicItems() )
	{
		const MapState::DynamicItem& item= item_value.second;

		const ModelGeometry& model_geometry= items_geometry_[ item.item_type_id ];
		const Model& model= game_resources_->items_models[ item.item_type_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			model_geometry.animations_vertex_count * item.frame;

		m_Mat4 model_matrix, rotation_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( item.pos, item.angle, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( item.angle );

		PC_ASSERT( item.frame < model.frame_count );

		const m_BBox3& bbox= model.animations_bboxes[ item.frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_matrix ) )
			continue;

		models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
		models_shader_.Uniform( "rotation_matrix", rotation_matrix );
		models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

		( item.fullbright ? map_light_.GetFullbrightLightmapDummy() : *active_lightmap_ ).Bind(1);

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawMonsters(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const EntityId player_monster_id,
	const bool transparent,
	const bool skip_invisible_monsters )
{
	monsters_shader_.Bind();
	monsters_geometry_data_.Bind();

	active_lightmap_->Bind(1);
	monsters_shader_.Uniform( "lightmap", int(1) );

	monsters_animations_.Bind(2);
	monsters_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::MonstersContainer::value_type& monster_value : map_state.GetMonsters() )
	{
		if( monster_value.first == player_monster_id )
			continue;

		const MapState::Monster& monster= monster_value.second;

		if( monster.monster_id >= monsters_models_.size() || // Unknown monster
			monster.body_parts_mask == 0u ) // Monster is invisible.
			continue;

		if( skip_invisible_monsters && monster.is_invisible )
			continue;

		const MonsterModel& monster_model= monsters_models_[ monster.monster_id ];
		const ModelGeometry& model_geometry= monster_model.geometry_description;
		const Model& model= game_resources_->monsters_models[ monster.monster_id ];

		PC_ASSERT( monster.animation < model.animations.size() );
		PC_ASSERT( monster.animation_frame < model.animations[ monster.animation ].frame_count );
		const unsigned int frame= model.animations[ monster.animation ].first_frame + monster.animation_frame;

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animations_vertex=
			model_geometry.first_animations_vertex +
			frame * model_geometry.animations_vertex_count;

		m_Mat4 model_matrix, rotation_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( monster.pos, monster.angle + Constants::half_pi, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( monster.angle + Constants::half_pi );

		const m_BBox3& bbox= model.animations_bboxes[ frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_matrix ) )
			continue;

		monsters_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		monsters_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
		monsters_shader_.Uniform( "enabled_groups_mask", int(monster.body_parts_mask) );
		monsters_shader_.Uniform( "rotation_matrix", rotation_matrix );
		monsters_shader_.Uniform( "first_animation_vertex_number", int(first_animations_vertex) );

		if( monster.monster_id == 0u )
			GetPlayerTexture( monster.color ).Bind(0);
		else
			monster_model.texture.Bind(0);
		monsters_shader_.Uniform( "tex", int(0) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawMonstersBodyParts(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const bool transparent )
{
	monsters_shader_.Bind();
	monsters_geometry_data_.Bind();

	active_lightmap_->Bind(1);
	monsters_shader_.Uniform( "lightmap", int(1) );

	monsters_animations_.Bind(2);
	monsters_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::MonsterBodyPart& part : map_state.GetMonstersBodyParts() )
	{
		if( part.monster_type >= monsters_models_.size() || part.body_part_id >= 3u )
			continue;

		const MonsterModel& monster_model= monsters_models_[ part.monster_type ];
		const ModelGeometry& model_geometry= monster_model.submodels_geometry_description[ part.body_part_id ];
		const Submodel& model= game_resources_->monsters_models[ part.monster_type ].submodels[ part.body_part_id ];

		PC_ASSERT( part.animation < model.animations.size() );
		PC_ASSERT( part.animation_frame < model.animations[ part.animation ].frame_count );
		const unsigned int frame= model.animations[ part.animation ].first_frame + part.animation_frame;

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animations_vertex=
			model_geometry.first_animations_vertex +
			frame * model_geometry.animations_vertex_count;

		m_Mat4 model_matrix, rotation_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( part.pos, part.angle + Constants::half_pi, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( part.angle + Constants::half_pi );

		const m_BBox3& bbox= model.animations_bboxes[ frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_matrix ) )
			continue;

		monsters_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		monsters_shader_.Uniform( "lightmap_matrix", lightmap_matrix );
		monsters_shader_.Uniform( "enabled_groups_mask", int(255) );
		monsters_shader_.Uniform( "rotation_matrix", rotation_matrix );
		monsters_shader_.Uniform( "first_animation_vertex_number", int(first_animations_vertex) );

		monster_model.texture.Bind(0);
		monsters_shader_.Uniform( "tex", int(0) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawRockets(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const bool transparent )
{
	models_shader_.Bind();
	rockets_geometry_data_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, rockets_textures_array_id_ );
	active_lightmap_->Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	rockets_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::RocketsContainer::value_type& rocket_value : map_state.GetRockets() )
	{
		const MapState::Rocket& rocket= rocket_value.second;
		PC_ASSERT( rocket.rocket_id < rockets_geometry_.size() );

		const ModelGeometry& model_geometry= rockets_geometry_[ rocket.rocket_id ];
		const Model& model= game_resources_->rockets_models[ rocket.rocket_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			rocket.frame * model_geometry.animations_vertex_count;

		m_Mat4 rotate_max_x, rotate_mat_z, shift_mat, scale_mat;
		rotate_max_x.RotateX( rocket.angle[1] );
		rotate_mat_z.RotateZ( rocket.angle[0] - Constants::half_pi );
		shift_mat.Translate( rocket.pos );
		scale_mat.Scale( 1.0f / float(MapData::c_map_size) );

		const m_Mat4 rotate_mat= rotate_max_x * rotate_mat_z;
		const m_Mat4 model_mat= rotate_mat * shift_mat;

		m_Mat3 lightmap_fetch_mat, lightmap_shift_mat, lightmap_scale_mat, lightmap_mat;
		lightmap_fetch_mat.Scale( 0.0f ); // Convert coordinates to zero - fetch lightmap from one point for whole model.
		lightmap_shift_mat.Translate( rocket.pos.xy() );
		lightmap_scale_mat.Scale( 1.0f / float(MapData::c_map_size) );
		lightmap_mat= lightmap_fetch_mat * lightmap_shift_mat * lightmap_scale_mat;

		PC_ASSERT( rocket.frame < model.frame_count );

		const m_BBox3& bbox= model.animations_bboxes[ rocket.frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_mat ) )
			continue;

		models_shader_.Uniform( "view_matrix", model_mat * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_mat );
		models_shader_.Uniform( "rotation_matrix", rotate_mat );
		models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

		if( game_resources_->rockets_description[ rocket.rocket_id ].fullbright )
			map_light_.GetFullbrightLightmapDummy().Bind(1);
		else
			active_lightmap_->Bind(1);

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawGibs(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	bool transparent )
{
	models_shader_.Bind();
	gibs_geometry_data_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, gibs_textures_array_id_ );
	active_lightmap_->Bind(1);
	models_shader_.Uniform( "tex", int(0) );

	active_lightmap_->Bind(1);
	models_shader_.Uniform( "lightmap", int(1) );

	gibs_animations_.Bind( 2 );
	models_shader_.Uniform( "animations_vertices_buffer", int(2) );

	for( const MapState::Gib& gib : map_state.GetGibs() )
	{
		if( gib.gib_id >= game_resources_->gibs_models.size() )
			continue;

		const ModelGeometry& model_geometry= gibs_geometry_[ gib.gib_id  ];
		const Model& model= game_resources_->gibs_models[ gib.gib_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int frame= 0u;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			frame * model_geometry.animations_vertex_count;

		m_Mat4 rotate_max_x, rotate_mat_z, shift_mat, scale_mat;
		rotate_max_x.RotateX( gib.angle_x );
		rotate_mat_z.RotateZ( gib.angle_z );
		shift_mat.Translate( gib.pos );
		scale_mat.Scale( 1.0f / float(MapData::c_map_size) );

		const m_Mat4 rotate_mat= rotate_max_x * rotate_mat_z;
		const m_Mat4 model_mat= rotate_mat * shift_mat;

		m_Mat3 lightmap_fetch_mat, lightmap_shift_mat, lightmap_scale_mat, lightmap_mat;
		lightmap_fetch_mat.Scale( 0.0f ); // Convert coordinates to zero - fetch lightmap from one point for whole model.
		lightmap_shift_mat.Translate( gib.pos.xy() );
		lightmap_scale_mat.Scale( 1.0f / float(MapData::c_map_size) );
		lightmap_mat= lightmap_fetch_mat * lightmap_shift_mat * lightmap_scale_mat;

		PC_ASSERT( frame< model.frame_count );

		const m_BBox3& bbox= model.animations_bboxes[ frame ];
		if( BBoxIsOutsideView( view_clip_planes, bbox, model_mat ) )
			continue;

		models_shader_.Uniform( "view_matrix", model_mat * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_mat );
		models_shader_.Uniform( "rotation_matrix", rotate_mat );
		models_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawBMPObjectsSprites(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position )
{
	const float sprites_frame= map_state.GetSpritesFrame();

	sprites_shader_.Bind();

	for( const MapState::StaticModel& model : map_state.GetStaticModels() )
	{
		if( model.model_id >= current_map_data_->models_description.size() )
			continue;

		const MapData::ModelDescription& model_description= current_map_data_->models_description[ model.model_id ];
		const int bmp_obj_id= model_description.bobj - 1u;
		if( bmp_obj_id < 0 || bmp_obj_id > static_cast<int>( game_resources_->bmp_objects_description.size() ) )
			continue;

		const GameResources::BMPObjectDescription& bmp_description= game_resources_->bmp_objects_description[ bmp_obj_id ];
		const ObjSprite& sprite_picture= game_resources_->bmp_objects_sprites[ bmp_obj_id ];

		glActiveTexture( GL_TEXTURE0 + 0 );
		glBindTexture( GL_TEXTURE_2D_ARRAY, bmp_objects_sprites_textures_arrays_[ bmp_obj_id ] );

		// Force fullbright. Fetch light, as with outher sprites, if this needed.
		map_light_.GetFullbrightLightmapDummy().Bind(1);

		const float additional_scale= ( bmp_description.half_size ? 0.5f : 1.0f ) / 128.0f;
		const m_Vec3 scale_vec(
			float(sprite_picture.size[0]) * additional_scale,
			1.0f,
			float(sprite_picture.size[1]) * additional_scale );

		m_Vec3 pos= model.pos;
		pos.z+= float( model_description.bmpz ) / 64.0f + scale_vec.z;

		const m_Vec3 vec_to_sprite= pos - camera_position;
		float sprite_angles[2];
		VecToAngles( vec_to_sprite, sprite_angles );

		m_Mat4 rotate_z, shift_mat, scale_mat;
		rotate_z.RotateZ( sprite_angles[0] - Constants::half_pi );
		shift_mat.Translate( pos );
		scale_mat.Scale( scale_vec );

		sprites_shader_.Uniform( "view_matrix", scale_mat * rotate_z * shift_mat * view_matrix );
		sprites_shader_.Uniform( "tex", int(0) );
		sprites_shader_.Uniform( "lightmap", int(1) );

		const unsigned int phase= GetModelBMPSpritePhase( model );
		const unsigned int frame= static_cast<unsigned int>( sprites_frame + phase ) % sprite_picture.frame_count;
		sprites_shader_.Uniform( "frame", float(frame) );
		sprites_shader_.Uniform( "lightmap_coord", pos.xy() / float(MapData::c_map_size) );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}
}

void MapDrawerGL::DrawEffectsSprites(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position )
{
	SortEffectsSprites( map_state.GetSpriteEffects(), camera_position, sorted_sprites_ );

	sprites_shader_.Bind();

	for( const MapState::SpriteEffect* const sprite_ptr : sorted_sprites_ )
	{
		const MapState::SpriteEffect& sprite= *sprite_ptr;

		const GameResources::SpriteEffectDescription& sprite_description= game_resources_->sprites_effects_description[ sprite.effect_id ];
		const ObjSprite& sprite_picture= game_resources_->effects_sprites[ sprite.effect_id ];

		glActiveTexture( GL_TEXTURE0 + 0 );
		glBindTexture( GL_TEXTURE_2D_ARRAY, sprites_textures_arrays_[ sprite.effect_id ] );

		if( !sprite_description.light_on )
			map_light_.GetFloorLightmap().Bind(1);
		else
			map_light_.GetFullbrightLightmapDummy().Bind(1);

		const m_Vec3 vec_to_sprite= sprite.pos - camera_position;
		float sprite_angles[2];
		VecToAngles( vec_to_sprite, sprite_angles );

		m_Mat4 rotate_z, rotate_x, shift_mat, scale_mat;
		rotate_z.RotateZ( sprite_angles[0] - Constants::half_pi );
		rotate_x.RotateX( sprite_angles[1] );
		shift_mat.Translate( sprite.pos );

		const float additional_scale= ( sprite_description.half_size ? 0.5f : 1.0f ) / 128.0f;
		scale_mat.Scale(
			m_Vec3(
				float(sprite_picture.size[0]) * additional_scale,
				1.0f,
				float(sprite_picture.size[1]) * additional_scale ) );

		sprites_shader_.Uniform( "view_matrix", scale_mat * rotate_x * rotate_z * shift_mat * view_matrix );
		sprites_shader_.Uniform( "tex", int(0) );
		sprites_shader_.Uniform( "lightmap", int(1) );
		sprites_shader_.Uniform( "frame", sprite.frame );
		sprites_shader_.Uniform( "lightmap_coord", sprite.pos.xy() / float(MapData::c_map_size) );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}
}

void MapDrawerGL::DrawSky( const m_Mat4& view_rotation_matrix )
{
	sky_shader_.Bind();

	sky_texture_.Bind(0);

	sky_shader_.Uniform( "tex", int(0) );
	sky_shader_.Uniform( "view_matrix", view_rotation_matrix );

	sky_geometry_data_.Draw();
}


void MapDrawerGL::DrawMapModelsShadows(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes )
{
	PC_UNUSED(view_clip_planes);

	models_geometry_data_.Bind();
	models_shadow_shader_.Bind();

	models_animations_.Bind( 0 );
	models_shadow_shader_.Uniform( "animations_vertices_buffer", int(0) );

	models_shadow_shader_.Uniform( "enabled_groups_mask", int(255) );

	const bool transparent= false;
	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= models_geometry_.size() ||
			!static_model.visible )
			continue;

		const MapData::ModelDescription& description= current_map_data_->models_description[ static_model.model_id ];
		if( !description.cast_shadow )
			continue;

		m_Vec3 light_pos;
		if( !GetNearestLightSourcePos( static_model.pos, *current_map_data_, map_state, false, light_pos ) )
			continue;

		const ModelGeometry& model_geometry= models_geometry_[ static_model.model_id ];
		// const Model& model= current_map_data_->models[ static_model.model_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			model_geometry.animations_vertex_count * static_model.animation_frame;

		m_Mat4 model_matrix;
		m_Mat3 lightmap_matrix, rotation_matrix;
		CreateModelMatrices( static_model.pos, static_model.angle, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( -static_model.angle );

		models_shadow_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shadow_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );
		models_shadow_shader_.Uniform( "light_pos", ( light_pos - static_model.pos ) * rotation_matrix );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawItemsShadows(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes )
{
	PC_UNUSED(view_clip_planes);

	items_geometry_data_.Bind();
	models_shadow_shader_.Bind();

	items_animations_.Bind( 0 );
	models_shadow_shader_.Uniform( "animations_vertices_buffer", int(0) );

	models_shadow_shader_.Uniform( "enabled_groups_mask", int(255) );

	const bool transparent= false;
	for( const MapState::Item& item : map_state.GetItems() )
	{
		if( item.picked_up || item.item_id >= items_geometry_.size() )
			continue;

		const GameResources::ItemDescription& description= game_resources_->items_description[ item.item_id ];
		if( !description.cast_shadow )
			continue;

		m_Vec3 light_pos;
		if( !GetNearestLightSourcePos( item.pos, *current_map_data_, map_state, true, light_pos ) )
			continue;

		const ModelGeometry& model_geometry= items_geometry_[ item.item_id ];
		// const Model& model= game_resources_->items_models[ item.item_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animation_vertex=
			model_geometry.first_animations_vertex +
			model_geometry.animations_vertex_count * item.animation_frame;

		m_Mat4 model_matrix;
		m_Mat3 lightmap_matrix, rotation_matrix;
		CreateModelMatrices( item.pos, item.angle, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( - item.angle );

		models_shadow_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shadow_shader_.Uniform( "first_animation_vertex_number", int(first_animation_vertex) );
		models_shadow_shader_.Uniform( "light_pos", ( light_pos - item.pos ) * rotation_matrix );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

void MapDrawerGL::DrawMonstersShadows(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const ViewClipPlanes& view_clip_planes,
	const EntityId player_monster_id )
{
	PC_UNUSED(view_clip_planes);

	monsters_geometry_data_.Bind();
	models_shadow_shader_.Bind();

	monsters_animations_.Bind( 0 );
	models_shadow_shader_.Uniform( "animations_vertices_buffer", int(0) );

	const bool transparent= false;
	for( const MapState::MonstersContainer::value_type& monster_value : map_state.GetMonsters() )
	{
		if( monster_value.first == player_monster_id ) // TODO - maybe cast player shadow?
			continue;

		const MapState::Monster& monster= monster_value.second;
		if( monster.monster_id >= monsters_models_.size() || // Unknown monster
			monster.body_parts_mask == 0u || // Monster is invisible.
			monster.is_fully_dead ) // Dead monsters does not cast shadows.
			continue;

		m_Vec3 light_pos;
		if( !GetNearestLightSourcePos( monster.pos, *current_map_data_, map_state, true, light_pos ) )
			continue;

		// TODO - monsters cast shadows allways?
		// const GameResources::MonsterDescription& description= game_resources_->monsters_description[ monster.monster_id ];

		const MonsterModel& monster_model= monsters_models_[ monster.monster_id ];
		const ModelGeometry& model_geometry= monster_model.geometry_description;
		const Model& model= game_resources_->monsters_models[ monster.monster_id ];

		PC_ASSERT( monster.animation < model.animations.size() );
		PC_ASSERT( monster.animation_frame < model.animations[ monster.animation ].frame_count );
		const unsigned int frame= model.animations[ monster.animation ].first_frame + monster.animation_frame;

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_animations_vertex=
			model_geometry.first_animations_vertex +
			frame * model_geometry.animations_vertex_count;

		m_Mat4 model_matrix;
		m_Mat3 lightmap_matrix, rotation_matrix;
		CreateModelMatrices( monster.pos, monster.angle + Constants::half_pi, model_matrix, lightmap_matrix );
		rotation_matrix.RotateZ( -( monster.angle + Constants::half_pi ) );

		models_shadow_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shadow_shader_.Uniform( "enabled_groups_mask", int(monster.body_parts_mask) );
		models_shadow_shader_.Uniform( "first_animation_vertex_number", int(first_animations_vertex) );
		models_shadow_shader_.Uniform( "light_pos", ( light_pos - monster.pos ) * rotation_matrix );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			model_geometry.first_vertex_index );
	}
}

} // PanzerChasm
