return
{
	fragmentShaderPath = "data/transparent_fragment.shader";
	vertexShaderPath = "data/vertex.shader";
	
	render_states =
	{
		alpha_transparency = true;
		depth_testing = true;
		depth_writing = false;
		face_culling = true;
	}
}