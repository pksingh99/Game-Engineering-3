
#include "../Mesh.h"
#include <cassert>


void eae6320::Graphics::Mesh::DrawMesh()
{  
	// Bind a specific vertex buffer to the device as a data source
	{
		glBindVertexArray(s_vertexArrayId);
		assert(glGetError() == GL_NO_ERROR);
	}
	// Render objects from the current streams
	{
		// We are using triangles as the "primitive" type,
		// and we have defined the vertex buffer as a triangle list
		// (meaning that every triangle is defined by three vertices)
		const GLenum mode = GL_TRIANGLES;
		// We'll use 32-bit indices in this class to keep things simple
		// (i.e. every index will be a 32 bit unsigned integer)
		const GLenum indexType = GL_UNSIGNED_INT;
		// It is possible to start rendering in the middle of an index buffer
		const GLvoid* const offset = 0;
		// We are drawing a square
		const GLsizei primitiveCountToRender = 2;	// How many triangles will be drawn?
		const GLsizei vertexCountPerTriangle = 3;
		const GLsizei vertexCountToRender = primitiveCountToRender * vertexCountPerTriangle;
		glDrawElements(mode, vertexCountToRender, indexType, offset);
		assert(glGetError() == GL_NO_ERROR);
	}
}

		