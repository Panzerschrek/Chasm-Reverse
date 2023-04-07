#include "ogl_state_manager.hpp"

const float r_OGLState::default_clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
const constexpr float r_OGLState::default_clear_depth;
const constexpr GLenum r_OGLState::default_cull_face_mode;
const constexpr bool r_OGLState::default_depth_mask;

r_OGLState r_OGLStateManager::state_;

static inline void SetGLFlag( GLenum flag, bool enabled )
{
	( enabled ? glEnable : glDisable )(flag);
}

r_OGLState::r_OGLState()
	: blend_(false), cull_face_(false), depth_test_(false), program_point_size_(false)
	, clear_depth_(1.0f)
	, cull_face_mode_(GL_BACK)
	, depth_mask_(true)
	, depth_func_(GL_LESS)
{
	blend_func_[0]= GL_ONE;
	blend_func_[1]= GL_ZERO;
	clear_color_[0]= clear_color_[1]= clear_color_[2]= clear_color_[3]= 0.0f;
}

r_OGLState::r_OGLState(
	bool blend, bool cull_face, bool depth_test, bool program_point_size,
	const GLenum* blend_func,
	const float* clear_color,
	float clear_depth,
	GLenum cull_face_mode,
	bool depth_mask )
	: blend_(blend), cull_face_(cull_face), depth_test_(depth_test), program_point_size_(program_point_size)
	, clear_depth_(clear_depth)
	, cull_face_mode_(cull_face_mode)
	, depth_mask_(depth_mask)
	, depth_func_(GL_LEQUAL)
{
	blend_func_[0]= blend_func[0];
	blend_func_[1]= blend_func[1];
	clear_color_[0]= clear_color[0];
	clear_color_[1]= clear_color[1];
	clear_color_[2]= clear_color[2];
	clear_color_[3]= clear_color[3];
}

void r_OGLStateManager::UpdateState( const r_OGLState& state )
{
	if( state_.blend_ != state.blend_ )
		SetGLFlag( GL_BLEND, state.blend_ );

	if( state_.cull_face_ != state.cull_face_ )
		SetGLFlag( GL_CULL_FACE, state.cull_face_ );

	if( state_.depth_test_ != state.depth_test_ )
		SetGLFlag( GL_DEPTH_TEST, state.depth_test_ );

	if( state_.program_point_size_ != state.program_point_size_ )
		SetGLFlag( GL_PROGRAM_POINT_SIZE, state.program_point_size_ );

	if( state_.blend_func_[0] != state.blend_func_[0] || state_.blend_func_[1] != state.blend_func_[1] )
		glBlendFunc( state.blend_func_[0], state.blend_func_[1] );

	if( state_.clear_color_[0] != state.clear_color_[0] || state_.clear_color_[1] != state.clear_color_[1] ||
		state_.clear_color_[2] != state.clear_color_[2] || state_.clear_color_[3] != state.clear_color_[3] )
		glClearColor( state.clear_color_[0], state.clear_color_[1], state.clear_color_[2], state.clear_color_[3] );

	if( state_.clear_depth_ != state.clear_depth_ )
		glClearDepth( state.clear_depth_ );

	if( state_.cull_face_mode_ != state.cull_face_mode_ )
		glCullFace( state.cull_face_mode_ );

	if( state_.depth_func_ != state.depth_func_ )
		glDepthFunc( state.depth_func_ );
	if( state_.depth_mask_ != state.depth_mask_ )
		glDepthMask( state.depth_mask_ );

	state_= state;
}

void r_OGLStateManager::ResetState()
{
	state_= r_OGLState();
}

const r_OGLState& r_OGLStateManager::GetCurrentState()
{
	return state_;
}
