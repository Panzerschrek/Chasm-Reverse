#pragma once

#include <framebuffer.hpp>
#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../fwd.hpp"
#include "../rendering_context.hpp"
#include "fwd.hpp"

namespace PanzerChasm
{

class MapLight final
{
public:
	MapLight( const RenderingContext& rendering_context );
	~MapLight();

	void SetMap( const MapDataConstPtr& map_data );

	void Update( const MapState& map_state );

	const r_Texture& GetFloorLightmap() const;
	const r_Texture& GetWallsLightmap() const;

private:
	// Floor
	r_Framebuffer base_floor_lightmap_;
	r_Framebuffer final_floor_lightmap_;

	// Walls
	r_Framebuffer base_walls_lightmap_;
	r_Framebuffer final_walls_lightmap_;

	// Shadowmap
	r_Framebuffer shadowmap_;

	// Shaders
	r_GLSLProgram floor_light_pass_shader_;
	r_GLSLProgram floor_ambient_light_pass_shader_;

	r_GLSLProgram shadowmap_shader_;

	MapDataConstPtr map_data_;
};

} // namespace PanzerChasm
