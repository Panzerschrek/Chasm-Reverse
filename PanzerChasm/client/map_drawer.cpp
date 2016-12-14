#include <algorithm>
#include <cstring>

#include <ogl_state_manager.hpp>

#include "../assert.hpp"
#include "../game_resources.hpp"
#include "../log.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "weapon_state.hpp"

#include "map_drawer.hpp"

namespace PanzerChasm
{

namespace
{

constexpr unsigned int g_wall_texture_height= 128u;
constexpr unsigned int g_max_wall_texture_width= 128u;

constexpr float g_walls_coords_scale= 256.0f;

const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
const r_OGLState g_walls_gl_state(
	false, false, true, false,
	g_gl_state_blend_func );

const r_OGLState g_floors_gl_state= g_walls_gl_state;

const r_OGLState g_models_gl_state(
	false, true, true, false,
	g_gl_state_blend_func );

const r_OGLState g_transparent_models_gl_state(
	true, true, true, false,
	g_gl_state_blend_func );

const r_OGLState g_sprites_gl_state(
	true, false, true, false,
	g_gl_state_blend_func );

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
	short tex_coord[2]; // 8.8 fixed
	unsigned char texture_id;
	char normal[2];
	unsigned char reserved[3];
};

SIZE_ASSERT( WallVertex, 16u );

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

static void CreateFullbrightLightmapDummy( r_Texture& texture )
{
	constexpr unsigned int c_size= 4u;
	unsigned char data[ c_size * c_size ];
	std::memset( data, 255u, sizeof(data) );

	texture=
		r_Texture(
			r_Texture::PixelFormat::R8,
			c_size, c_size,
			data );

	texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
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

MapDrawer::MapDrawer(
	GameResourcesConstPtr game_resources,
	const RenderingContext& rendering_context )
	: game_resources_(std::move(game_resources))
	, rendering_context_(rendering_context)
{
	PC_ASSERT( game_resources_ != nullptr );

	// Textures
	glGenTextures( 1, &floor_textures_array_id_ );
	glGenTextures( 1, &wall_textures_array_id_ );
	glGenTextures( 1, &models_textures_array_id_ );
	glGenTextures( 1, &items_textures_array_id_ );
	glGenTextures( 1, &rockets_textures_array_id_ );
	glGenTextures( 1, &weapons_textures_array_id_ );

	CreateFullbrightLightmapDummy( fullbright_lightmap_dummy_ );

	LoadSprites();

	// Items
	LoadModels(
		game_resources_->items_models,
		items_geometry_,
		items_geometry_data_,
		items_textures_array_id_ );

	// Monsters
	LoadMonstersModels();

	// Rockets
	LoadModels(
		game_resources_->rockets_models,
		rockets_geometry_,
		rockets_geometry_data_,
		rockets_textures_array_id_ );

	// Weapons
	LoadModels(
		game_resources_->weapons_models,
		weapons_geometry_,
		weapons_geometry_data_,
		weapons_textures_array_id_ );

	// Shaders
	char lightmap_scale[32];
	std::snprintf( lightmap_scale, sizeof(lightmap_scale), "LIGHTMAP_SCALE %f", 1.0f / float(MapData::c_map_size) );

	const std::vector<std::string> defines{ lightmap_scale };

	floors_shader_.ShaderSource(
		rLoadShader( "floors_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "floors_v.glsl", rendering_context. glsl_version, defines ) );
	floors_shader_.SetAttribLocation( "pos", 0u );
	floors_shader_.SetAttribLocation( "tex_id", 1u );
	floors_shader_.Create();

	walls_shader_.ShaderSource(
		rLoadShader( "walls_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "walls_v.glsl", rendering_context.glsl_version, defines ) );
	walls_shader_.SetAttribLocation( "pos", 0u );
	walls_shader_.SetAttribLocation( "tex_coord", 1u );
	walls_shader_.SetAttribLocation( "tex_id", 2u );
	walls_shader_.SetAttribLocation( "normal", 3u );
	walls_shader_.Create();

	models_shader_.ShaderSource(
		rLoadShader( "models_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "models_v.glsl", rendering_context.glsl_version ) );
	models_shader_.SetAttribLocation( "pos", 0u );
	models_shader_.SetAttribLocation( "tex_coord", 1u );
	models_shader_.SetAttribLocation( "tex_id", 2u );
	models_shader_.SetAttribLocation( "alpha_test_mask", 3u );
	models_shader_.Create();

	sprites_shader_.ShaderSource(
		rLoadShader( "sprites_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "sprites_v.glsl", rendering_context.glsl_version ) );
	sprites_shader_.SetAttribLocation( "pos", 0u );
	sprites_shader_.Create();

	monsters_shader_.ShaderSource(
		rLoadShader( "monsters_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "monsters_v.glsl", rendering_context.glsl_version ) );
	monsters_shader_.SetAttribLocation( "pos", 0u );
	monsters_shader_.SetAttribLocation( "tex_coord", 1u );
	monsters_shader_.SetAttribLocation( "tex_id", 2u );
	monsters_shader_.SetAttribLocation( "alpha_test_mask", 3u );
	monsters_shader_.Create();

}

MapDrawer::~MapDrawer()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
	glDeleteTextures( 1, &wall_textures_array_id_ );
	glDeleteTextures( 1, &models_textures_array_id_ );
	glDeleteTextures( 1, &items_textures_array_id_ );
	glDeleteTextures( 1, &rockets_textures_array_id_ );
	glDeleteTextures( 1, &weapons_textures_array_id_ );

	glDeleteTextures( sprites_textures_arrays_.size(), sprites_textures_arrays_.data() );
}

void MapDrawer::SetMap( const MapDataConstPtr& map_data )
{
	PC_ASSERT( map_data != nullptr );

	if( map_data == current_map_data_ )
		return;

	current_map_data_= map_data;

	LoadFloorsTextures( *map_data );
	LoadWallsTextures( *map_data );
	LoadFloors( *map_data );
	LoadWalls( *map_data );

	LoadModels(
		map_data->models,
		models_geometry_,
		models_geometry_data_,
		models_textures_array_id_ );

	lightmap_=
		r_Texture(
			r_Texture::PixelFormat::R8,
			MapData::c_lightmap_size, MapData::c_lightmap_size,
			map_data->lightmap );
	lightmap_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
}

void MapDrawer::Draw(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position )
{
	if( current_map_data_ == nullptr )
		return;

	UpdateDynamicWalls( map_state.GetDynamicWalls() );

	r_OGLStateManager::UpdateState( g_walls_gl_state );
	DrawWalls( view_matrix );

	r_OGLStateManager::UpdateState( g_floors_gl_state );
	DrawFloors( view_matrix );

	r_OGLStateManager::UpdateState( g_models_gl_state );
	DrawModels( map_state, view_matrix, false );
	DrawItems( map_state, view_matrix, false );
	DrawMonsters( map_state, view_matrix, false );
	DrawRockets( map_state, view_matrix, false );

	/*
	TRANSPARENT SECTION
	*/
	r_OGLStateManager::UpdateState( g_transparent_models_gl_state );
	DrawModels( map_state, view_matrix, true );
	DrawItems( map_state, view_matrix, true );
	DrawMonsters( map_state, view_matrix, true );
	DrawRockets( map_state, view_matrix, true );

	r_OGLStateManager::UpdateState( g_sprites_gl_state );
	DrawSprites( map_state, view_matrix, camera_position );
}

void MapDrawer::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& view_matrix,
	const m_Vec3& position,
	const m_Vec3& angle )
{
	// TODO - maybe this points differnet for differnet weapons?
	// Crossbow: m_Vec3( 0.2f, 0.7f, -0.45f )
	const m_Vec3 c_weapon_shift= m_Vec3( 0.2f, 0.7f, -0.45f );
	const m_Vec3 c_weapon_change_shift= m_Vec3( 0.0f, -0.9f, 0.0f );

	weapons_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, weapons_textures_array_id_ );
	lightmap_.Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	m_Mat4 shift_mat, rotate_x_mat, rotate_z_mat, rotate_mat;
	rotate_x_mat.RotateX( angle.x );
	rotate_z_mat.RotateZ( angle.z );
	rotate_mat= rotate_x_mat * rotate_z_mat;

	const m_Vec3 additional_shift=
		( c_weapon_shift + c_weapon_change_shift * ( 1.0f - weapon_state.GetSwitchStage() ) ) *
		rotate_mat;
	shift_mat.Translate( position + additional_shift );

	const m_Mat4 model_mat= rotate_mat * shift_mat;

	m_Mat3 scale_in_lightmap_mat, lightmap_shift_mat, lightmap_scale_mat;
	scale_in_lightmap_mat.Scale( 0.0f ); // Make model so small, that it will have uniform light.
	lightmap_shift_mat.Translate( position.xy() );
	lightmap_scale_mat.Scale( 1.0f / float(MapData::c_map_size) );
	const m_Mat3 lightmap_mat= scale_in_lightmap_mat * lightmap_shift_mat * lightmap_scale_mat;

	models_shader_.Uniform( "view_matrix", model_mat * view_matrix );
	models_shader_.Uniform( "lightmap_matrix", lightmap_mat );

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
		const unsigned int first_vertex=
			model_geometry.first_vertex_index +
			model_geometry.vertex_count * frame;

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			first_vertex );
	};

	glDepthRange( 0.0f, 1.0f / 8.0f );

	r_OGLStateManager::UpdateState( g_models_gl_state );
	draw_model_polygons(false);

	r_OGLStateManager::UpdateState( g_transparent_models_gl_state );
	draw_model_polygons(true);

	glDepthRange( 0.0f, 1.0f );
}

void MapDrawer::LoadSprites()
{
	const Palette& palette= game_resources_->palette;

	std::vector<unsigned char> data_rgba;

	sprites_textures_arrays_.resize( game_resources_->effects_sprites.size() );
	glGenTextures( sprites_textures_arrays_.size(), sprites_textures_arrays_.data() );
	for( unsigned int i= 0u; i < sprites_textures_arrays_.size(); i++ )
	{
		const ObjSprite& sprite= game_resources_->effects_sprites[i];

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

		glBindTexture( GL_TEXTURE_2D_ARRAY, sprites_textures_arrays_[i] );
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
			sprite.size[0], sprite.size[1], sprite.frame_count,
			0, GL_RGBA, GL_UNSIGNED_BYTE, data_rgba.data() );

		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	}
}

void MapDrawer::LoadFloorsTextures( const MapData& map_data )
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

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapDrawer::LoadWallsTextures( const MapData& map_data )
{
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
			src_height != g_wall_texture_height )
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

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, t,
			g_max_wall_texture_width, g_wall_texture_height, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for wall textures

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapDrawer::LoadFloors( const MapData& map_data )
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

			if( texture_number == MapData::c_empty_floor_texture_id )
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

void MapDrawer::LoadWalls( const MapData& map_data )
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
		ind[0]= first_vertex_index + 0u;
		ind[1]= first_vertex_index + 1u;
		ind[2]= first_vertex_index + 3u;
		ind[3]= first_vertex_index + 0u;
		ind[4]= first_vertex_index + 3u;
		ind[5]= first_vertex_index + 2u;
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

	// Try reuse indeces from regular walls
	if( walls_indeces.size() < map_data.dynamic_walls.size() * 6u )
	{
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

void MapDrawer::LoadModels(
	const std::vector<Model>& models,
	std::vector<ModelGeometry>& out_geometry,
	r_PolygonBuffer& out_geometry_data,
	GLuint& out_textures_array ) const
{
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

		// TODO - fill free texture space

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
		model_geometry.vertex_count= model.frame_count == 0u ? 0u : ( model.vertices.size() / model.frame_count );
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

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

	PrepareModelsPolygonBuffer( vertices, indeces, out_geometry_data );
}

void MapDrawer::LoadMonstersModels()
{
	const std::vector<Model>& in_models= game_resources_->monsters_models;
	const Palette& palette= game_resources_->palette;

	monsters_models_.resize( in_models.size() );

	std::vector<unsigned char> texture_data_rgba;

	std::vector<unsigned  short> indeces;
	std::vector<Model::Vertex> vertices;

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

		out_model.texture=
			r_Texture(
				r_Texture::PixelFormat::RGBA8,
				in_model.texture_size[0], in_model.texture_size[1],
				texture_data_rgba.data() );
		out_model.texture.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );
		out_model.texture.BuildMips();

		// Copy vertices.
		const unsigned int first_vertex_index= vertices.size();
		vertices.resize( vertices.size() + in_model.vertices.size() );
		std::memcpy(
			vertices.data() + first_vertex_index,
			in_model.vertices.data(),
			in_model.vertices.size() * sizeof(Model::Vertex) );

		// Copy indeces.
		const unsigned int first_index= indeces.size();
		indeces.resize( indeces.size() + in_model.regular_triangles_indeces.size() + in_model.transparent_triangles_indeces.size() );
		unsigned short* const ind= indeces.data() + first_index;

		std::memcpy(
			ind,
			in_model.regular_triangles_indeces.data(),
			in_model.regular_triangles_indeces.size() * sizeof(unsigned short) );

		std::memcpy(
			ind + in_model.regular_triangles_indeces.size(),
			in_model.transparent_triangles_indeces.data(),
			in_model.transparent_triangles_indeces.size() * sizeof(unsigned short) );

		// Setup geometry info.
		ModelGeometry& model_geometry= out_model.geometry_description;
		model_geometry.frame_count= in_model.frame_count;
		model_geometry.vertex_count= in_model.vertices.size() / in_model.frame_count;
		model_geometry.first_vertex_index= first_vertex_index;

		model_geometry.first_index= first_index;
		model_geometry.index_count= in_model.regular_triangles_indeces.size();

		model_geometry.first_transparent_index= first_index + in_model.regular_triangles_indeces.size();
		model_geometry.transparent_index_count= in_model.transparent_triangles_indeces.size();
	}

	PrepareModelsPolygonBuffer( vertices, indeces, monsters_geometry_data_ );
}

void MapDrawer::UpdateDynamicWalls( const MapState::DynamicWalls& dynamic_walls )
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

void MapDrawer::PrepareModelsPolygonBuffer(
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
	buffer.VertexAttribPointer(
		0, 3, GL_FLOAT, false,
		((char*)v.pos) - ((char*)&v) );

	buffer.VertexAttribPointer(
		1, 3, GL_FLOAT, false,
		((char*)v.tex_coord) - ((char*)&v) );

	buffer.VertexAttribPointerInt(
		2, 1, GL_UNSIGNED_BYTE,
		((char*)&v.texture_id) - ((char*)&v) );

	buffer.VertexAttribPointer(
		3, 1, GL_UNSIGNED_BYTE, true,
		((char*)&v.alpha_test_mask) - ((char*)&v) );
}

void MapDrawer::DrawWalls( const m_Mat4& view_matrix )
{
	walls_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, wall_textures_array_id_ );
	lightmap_.Bind(1);

	walls_shader_.Uniform( "tex", int(0) );
	walls_shader_.Uniform( "lightmap", int(1) );

	m_Mat4 scale_mat;
	scale_mat.Scale( 1.0f / 256.0f );
	walls_shader_.Uniform( "view_matrix", scale_mat * view_matrix );

	walls_geometry_.Bind();
	walls_geometry_.Draw();
	dynamic_walls_geometry_.Bind();
	dynamic_walls_geometry_.Draw();
}

void MapDrawer::DrawFloors( const m_Mat4& view_matrix )
{
	floors_geometry_.Bind();
	floors_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	lightmap_.Bind(1);

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

void MapDrawer::DrawModels(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const bool transparent )
{
	models_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, models_textures_array_id_ );
	lightmap_.Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= models_geometry_.size() )
			continue;

		const ModelGeometry& model_geometry= models_geometry_[ static_model.model_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_vertex=
			model_geometry.first_vertex_index +
			static_model.animation_frame * model_geometry.vertex_count;

		m_Mat4 model_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( static_model.pos, static_model.angle, model_matrix, lightmap_matrix );

		models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			first_vertex );
	}
}

void MapDrawer::DrawItems(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const bool transparent )
{
	items_geometry_data_.Bind();
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, items_textures_array_id_ );
	lightmap_.Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	for( const MapState::Item& item : map_state.GetItems() )
	{
		if( item.picked_up || item.item_id >= items_geometry_.size() )
			continue;

		const ModelGeometry& model_geometry= items_geometry_[ item.item_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_vertex=
			model_geometry.first_vertex_index +
			item.animation_frame * model_geometry.vertex_count;

		m_Mat4 model_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( item.pos, item.angle, model_matrix, lightmap_matrix );

		models_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		models_shader_.Uniform( "lightmap_matrix", lightmap_matrix );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			first_vertex );
	}
}

void MapDrawer::DrawMonsters(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const bool transparent )
{
	monsters_shader_.Bind();
	monsters_geometry_data_.Bind();

	lightmap_.Bind(1);
	monsters_shader_.Uniform( "lightmap", int(1) );

	for( const MapState::MonstersContainer::value_type& monster_value : map_state.GetMonsters() )
	{
		const MapState::Monster& monster= monster_value.second;

		// HACK -REMOVE THIS. Skip players.
		// TODO - skip only this player
		if( monster.monster_id == 0u )
			continue;

		if( monster.monster_id >= monsters_models_.size() )
			continue;

		const MonsterModel& monster_model= monsters_models_[ monster.monster_id ];
		const ModelGeometry& model_geometry= monster_model.geometry_description;
		const Model& model= game_resources_->monsters_models[ monster.monster_id ];

		const unsigned int frame= model.animations[ monster.animation ].first_frame + monster.animation_frame;

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_vertex=
			model_geometry.first_vertex_index +
			frame * model_geometry.vertex_count;

		m_Mat4 model_matrix;
		m_Mat3 lightmap_matrix;
		CreateModelMatrices( monster.pos, monster.angle + Constants::half_pi, model_matrix, lightmap_matrix );

		monsters_shader_.Uniform( "view_matrix", model_matrix * view_matrix );
		monsters_shader_.Uniform( "lightmap_matrix", lightmap_matrix );

		monster_model.texture.Bind(0);
		monsters_shader_.Uniform( "tex", int(0) );

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			first_vertex );
	}
}

void MapDrawer::DrawRockets(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const bool transparent )
{
	models_shader_.Bind();
	rockets_geometry_data_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, rockets_textures_array_id_ );
	lightmap_.Bind(1);
	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "lightmap", int(1) );

	for( const MapState::RocketsContainer::value_type& rocket_value : map_state.GetRockets() )
	{
		const MapState::Rocket& rocket= rocket_value.second;
		PC_ASSERT( rocket.rocket_id < rockets_geometry_.size() );

		const ModelGeometry& model_geometry= rockets_geometry_[ rocket.rocket_id ];

		const unsigned int index_count= transparent ? model_geometry.transparent_index_count : model_geometry.index_count;
		if( index_count == 0u )
			continue;

		const unsigned int first_index= transparent ? model_geometry.first_transparent_index : model_geometry.first_index;
		const unsigned int first_vertex=
			model_geometry.first_vertex_index +
			rocket.frame * model_geometry.vertex_count;

		m_Mat4 rotate_max_x, rotate_mat_z, shift_mat, scale_mat;
		rotate_max_x.RotateX( rocket.angle[1] );
		rotate_mat_z.RotateZ( rocket.angle[0] - Constants::half_pi );
		shift_mat.Translate( rocket.pos );
		scale_mat.Scale( 1.0f / float(MapData::c_map_size) );

		const m_Mat4 model_mat= rotate_max_x * rotate_mat_z * shift_mat;

		monsters_shader_.Uniform( "view_matrix", model_mat * view_matrix );
		monsters_shader_.Uniform( "lightmap_matrix", model_mat * scale_mat );

		if( game_resources_->rockets_description[ rocket.rocket_id ].fullbright )
			fullbright_lightmap_dummy_.Bind(1);
		else
			lightmap_.Bind(1);

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_index * sizeof(unsigned short) ),
			first_vertex );
	}
}

void MapDrawer::DrawSprites(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position )
{

	// Sort from far to near
	const MapState::SpriteEffects& sprite_effects= map_state.GetSpriteEffects();
	sorted_sprites_.resize( sprite_effects.size() );
	for( unsigned int i= 0u; i < sprite_effects.size(); i++ )
		sorted_sprites_[i]= & sprite_effects[i];

	std::sort(
		sorted_sprites_.begin(),
		sorted_sprites_.end(),
		[&]( const MapState::SpriteEffect* const a, const MapState::SpriteEffect* const b )
		{
			const m_Vec3 dir_a= camera_position - a->pos;
			const m_Vec3 dir_b= camera_position - b->pos;
			return dir_a.SquareLength() >= dir_b.SquareLength();
		});

	sprites_shader_.Bind();
	glActiveTexture( GL_TEXTURE0 + 0 );

	for( const MapState::SpriteEffect* const sprite_ptr : sorted_sprites_ )
	{
		const MapState::SpriteEffect& sprite= *sprite_ptr;

		const GameResources::SpriteEffectDescription& sprite_description= game_resources_->sprites_effects_description[ sprite.effect_id ];
		const ObjSprite& sprite_picture= game_resources_->effects_sprites[ sprite.effect_id ];

		glBindTexture( GL_TEXTURE_2D_ARRAY, sprites_textures_arrays_[ sprite.effect_id ] );

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
		sprites_shader_.Uniform( "frame", sprite.frame );

		float light= 1.0f;
		// TODO - check fullbright criteria
		if( !sprite_description.light_on )
		{
			const int sx= static_cast<int>( sprite.pos.x * float(MapData::c_lightmap_scale) );
			const int sy= static_cast<int>( sprite.pos.y * float(MapData::c_lightmap_scale) );
			if( sx >= 0 && sx < int( MapData::c_lightmap_size ) &&
				sy >= 0 && sy < int( MapData::c_lightmap_size ) )
				light= float( current_map_data_->lightmap[ sx + sy * int( MapData::c_lightmap_size ) ] ) / 255.0f;
		}

		sprites_shader_.Uniform( "light", light );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}
}

} // PanzerChasm
