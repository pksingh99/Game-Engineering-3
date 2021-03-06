// Header Files
//=============

#include "cMeshBuilder.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include "../../Engine/Windows/WindowsFunctions.h"

// Interface
//==========

// Build
//------

bool eae6320::cMeshBuilder::Build( const std::vector<std::string>& )
{
	//bool wereThereErrors = false;

	//// Copy the source to the target
	//{
	//	const bool dontFailIfTargetAlreadyExists = false;
	//	const bool updateTheTargetFileTime = true;
	//	std::string errorMessage;
	//	if ( !CopyFile( m_path_source, m_path_target, dontFailIfTargetAlreadyExists, updateTheTargetFileTime, &errorMessage ) )
	//	{
	//		wereThereErrors = true;
	//		std::stringstream decoratedErrorMessage;
	//		decoratedErrorMessage << "Windows failed to copy \"" << m_path_source << "\" to \"" << m_path_target << "\": " << errorMessage;
	//		eae6320::OutputErrorMessage( decoratedErrorMessage.str().c_str(), __FILE__ );
	//	}
	//}
	//
	//return !wereThereErrors;

	if (!LoadAsset(m_path_source))
	{
		return false;
	}
	if (!CreateBinaryFile())
	{
		return false;
	}

	{
		delete[] o_vertexData;
		o_vertexData = NULL;

		delete[] o_indexData;
		o_indexData = NULL;
	}
}

bool eae6320::cMeshBuilder::LoadAsset(const char* i_path)
{
	bool wereThereErrors = false;

	// Create a new Lua state
	lua_State* luaState = NULL;
	{
		luaState = luaL_newstate();
		if (!luaState)
		{
			wereThereErrors = true;
			eae6320::OutputErrorMessage("Failed to create a new Lua state");

			goto OnExit;
		}
	}

	// Load the asset file as a "chunk",
	// meaning there will be a callable function at the top of the stack
	{
		const int luaResult = luaL_loadfile(luaState, i_path);
		if (luaResult != LUA_OK)
		{
			wereThereErrors = true;
			eae6320::OutputErrorMessage(lua_tostring(luaState, -1));
			// Pop the error message
			lua_pop(luaState, 1);
			goto OnExit;
		}
	}
	// Execute the "chunk", which should load the asset
	// into a table at the top of the stack
	{
		const int argumentCount = 0;
		const int returnValueCount = LUA_MULTRET;	// Return _everything_ that the file returns
		const int noMessageHandler = 0;
		const int luaResult = lua_pcall(luaState, argumentCount, returnValueCount, noMessageHandler);
		if (luaResult == LUA_OK)
		{
			// A well-behaved asset file will only return a single value
			const int returnedValueCount = lua_gettop(luaState);
			if (returnedValueCount == 1)
			{
				// A correct asset file _must_ return a table
				if (!lua_istable(luaState, -1))
				{
					wereThereErrors = true;
					std::stringstream errMsg;
					//std::cerr << "Asset files must return a table (instead of a " <<
					//	luaL_typename(luaState, -1) << ")\n";
					// Pop the returned non-table value
					errMsg << "Asset files must return a table (instead of a " <<
							luaL_typename(luaState, -1) << ")\n";
					eae6320::OutputErrorMessage(errMsg.str().c_str());
					lua_pop(luaState, 1);
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				std::stringstream errMsg;
				//std::cerr << "Asset files must return a single table (instead of " <<
				//	returnedValueCount << " values)"
				//	"\n";
				// Pop every value that was returned
				errMsg << "Asset files must return a single table (instead of " <<
					returnedValueCount << " values)"
					"\n";
				eae6320::OutputErrorMessage(errMsg.str().c_str());
				lua_pop(luaState, returnedValueCount);
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			eae6320::OutputErrorMessage(lua_tostring(luaState, -1));
			// Pop the error message
			lua_pop(luaState, 1);
			goto OnExit;
		}
	}

	// If this code is reached the asset file was loaded successfully,
	// and its table is now at index -1
	if (!Table_Values(*luaState))
	{
		wereThereErrors = true;
	}

	// Pop the table
	lua_pop(luaState, 1);

OnExit:

	if (luaState)
	{
		// If I haven't made any mistakes
		// there shouldn't be anything on the stack,
		// regardless of any errors encountered while loading the file:
		assert(lua_gettop(luaState) == 0);

		lua_close(luaState);
		luaState = NULL;
	}

	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::Table_Values(lua_State& io_luaState)
{
	if (!Table_Vertices(io_luaState))
	{
		return false;
	}
	if (!Table_Indices(io_luaState))
	{
		return false;
	}

	return true;
}

bool eae6320::cMeshBuilder::Table_Vertices(lua_State& io_luaState)
{
	bool wereThereErrors = false;

	// Right now the asset table is at -1.
	// After the following table operation it will be at -2
	// and the "textures" table will be at -1:
	const char* const key = "vertices";
	lua_pushstring(&io_luaState, key);
	lua_gettable(&io_luaState, -2);
	// It can be hard to remember where the stack is at
	// and how many values to pop.
	// One strategy I would suggest is to always call a new function
	// When you are at a new level:
	// Right now we know that we have an original table at -2,
	// and a new one at -1,
	// and so we _know_ that we always have to pop at least _one_
	// value before leaving this function
	// (to make the original table be back to index -1).
	// If we don't do any further stack manipulation in this function
	// then it becomes easy to remember how many values to pop
	// because it will always be one.
	// This is the strategy I'll take in this example
	// (look at the "OnExit" label):
	if (lua_istable(&io_luaState, -1))
	{
		if (!Table_Vertices_Values(io_luaState))
		{
			wereThereErrors = true;
			goto OnExit;
		}
	}
	else
	{
		wereThereErrors = true;
		std::stringstream errMsg;
		//std::cerr << "The value at \"" << key << "\" must be a table "
		//	"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
		//using the UserOutput Print
		errMsg << "The value at \"" << key << "\" must be a table "
			"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
		eae6320::OutputErrorMessage(errMsg.str().c_str());
		goto OnExit;
	}

OnExit:

	// Pop the textures table
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::Table_Vertices_Values(lua_State& io_luaState)
{
	// Right now the asset table is at -2
	// and the textures table is at -1.
	// NOTE, however, that it doesn't matter to me in this function
	// that the asset table is at -2.
	// Because I've carefully called a new function for every "stack level"
	// The only thing I care about is that the textures table that I care about
	// is at the top of the stack.
	// As long as I make sure that when I leave this function it is _still_
	// at -1 then it doesn't matter to me at all what is on the stack below it.

	o_vertexCount = luaL_len(&io_luaState, -1);
	o_vertexData = new sVertex[o_vertexCount];
	for (unsigned int i = 1; i <= o_vertexCount; ++i)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		if (lua_istable(&io_luaState, -1))
		{
			if (!Table_Vertices_Pos(io_luaState, i - 1))
			{
				return false;
			}
			if (!Table_Vertices_Color(io_luaState, i - 1))
			{
				return false;
			}
			if (!Table_Vertives_Texture(io_luaState, i - 1))
			{
				return false;
			}
		}
		else
		{
			std::stringstream;
			eae6320::OutputErrorMessage("Error message - Error in Mesh.cpp in Table_Vertices_Values(lua_State& io_luaState) function");
			//Pop vertex value table
			lua_pop(&io_luaState, 1);
			return false;
		}
		lua_pop(&io_luaState, 1);
	}

	return true;
}

bool eae6320::cMeshBuilder::Table_Vertices_Pos(lua_State& io_luaState, int i_vertexIndex)
{
	bool wereThereErrors = false;

	// Right now the asset table is at -1.
	// After the following table operation it will be at -2
	// and the "parameters" table will be at -1:
	const char* const key = "pos";
	lua_pushstring(&io_luaState, key);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//for x position
		lua_pushinteger(&io_luaState, 1);
		lua_gettable(&io_luaState, -2);
		o_vertexData[i_vertexIndex].x = (float)lua_tonumber(&io_luaState, -1);
		lua_pop(&io_luaState, 1);

		//for y position
		lua_pushinteger(&io_luaState, 2);
		lua_gettable(&io_luaState, -2);
		o_vertexData[i_vertexIndex].y = (float)lua_tonumber(&io_luaState, -1);
		lua_pop(&io_luaState, 1);

		//for z position
		lua_pushinteger(&io_luaState, 3);
		lua_gettable(&io_luaState, -2);
		o_vertexData[i_vertexIndex].z = (float)lua_tonumber(&io_luaState, -1);
		lua_pop(&io_luaState, 1);

	}
	else
	{
		wereThereErrors = true;
		//std::cerr << "The value at \"" << key << "\" must be a table "
		//	"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";

		eae6320::OutputErrorMessage("Error Message - Error in Mesh.cpp in the function Table_Vertices_Pos(lua_State& io_luaState, int i_vertexIndex) ");
		goto OnExit;
	}

OnExit:

	// Pop the parameters table
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::Table_Vertices_Color(lua_State& io_luaState, int i_vertexIndex)
{
	bool wereThereErrors = false;

	// Right now the asset table is at -1.
	// After the following table operation it will be at -2
	// and the "parameters" table will be at -1:
	const char* const key = "color";
	lua_pushstring(&io_luaState, key);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//red
		lua_pushinteger(&io_luaState, 1);
		lua_gettable(&io_luaState, -2);

		o_vertexData[i_vertexIndex].r = (uint32_t)(lua_tonumber(&io_luaState, -1) * 255);
		lua_pop(&io_luaState, 1);

		//green
		lua_pushinteger(&io_luaState, 2);
		lua_gettable(&io_luaState, -2);

		o_vertexData[i_vertexIndex].g = (uint32_t)(lua_tonumber(&io_luaState, -1) * 255);
		lua_pop(&io_luaState, 1);

		//blue
		lua_pushinteger(&io_luaState, 3);
		lua_gettable(&io_luaState, -2);

		o_vertexData[i_vertexIndex].b = (uint32_t)(lua_tonumber(&io_luaState, -1) * 255);
		lua_pop(&io_luaState, 1);

		//alpha
		lua_pushinteger(&io_luaState, 4);
		lua_gettable(&io_luaState, -2);
		o_vertexData[i_vertexIndex].a = (uint32_t)(lua_tonumber(&io_luaState, -1) * 255);
		lua_pop(&io_luaState, 1);

	}


	else
	{
		wereThereErrors = true;
		//std::cerr << "The value at \"" << key << "\" must be a table "
		//	"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";

		eae6320::OutputErrorMessage("Error Message - Error in Mesh.cpp in the function LoadTableValues_vertice_values_color(lua_State& io_luaState, int i_vertexIndex) ");
		goto OnExit;
	}

OnExit:

	// Pop the parameters table
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::Table_Vertives_Texture(lua_State& io_luaState, int i_vertexIndex)
{
	bool wereThereErrors = false;

	const char* const key = "texture";
	lua_pushstring(&io_luaState, key);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		// u
		lua_pushinteger(&io_luaState, 1);
		lua_gettable(&io_luaState, -2);
		o_vertexData[i_vertexIndex].u = (float)lua_tonumber(&io_luaState, -1);
		lua_pop(&io_luaState, 1);

		// v
		lua_pushinteger(&io_luaState, 2);
		lua_gettable(&io_luaState, -2);
		// We will be using D3D convention for both platfomes for V coordinate
		o_vertexData[i_vertexIndex].v = (1.0f - (float)lua_tonumber(&io_luaState, -1));
		lua_pop(&io_luaState, 1);
	}
	else
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "The value at \"" << key << "\" must be a table "
			"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
		eae6320::OutputErrorMessage(errorMessage.str().c_str());
		goto OnExit;
	}

OnExit:

	// Pop the parameters table
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::Table_Indices(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	const char* const key = "indices";
	lua_pushstring(&io_luaState, key);
	lua_gettable(&io_luaState, -2);

	if (lua_istable(&io_luaState, -1))
	{
		if (!Table_Indices_Values(io_luaState))
		{
			wereThereErrors = true;
			goto OnExit;
		}
	}
	else
	{
		wereThereErrors = true;
		eae6320::OutputErrorMessage("Error Message - Error in Mesh.cpp in Table_Indices(lua_State& io_luaState) function ");
		goto OnExit;
	}

OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;

}

bool eae6320::cMeshBuilder::Table_Indices_Values(lua_State& io_luaState)
{
	const unsigned int triangleCount = luaL_len(&io_luaState, -1);
	const unsigned int  vertexCountPerTriangle = 3;

	o_indexCount = triangleCount * vertexCountPerTriangle;
	o_indexData = new uint32_t[o_indexCount];

	int k = 0;

	for (unsigned int i = 1; i <= triangleCount; ++i)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);

		if (lua_istable(&io_luaState, -1))
		{
			for (unsigned int j = 1; j <= vertexCountPerTriangle; ++j)
			{
				lua_pushinteger(&io_luaState, j);
				lua_gettable(&io_luaState, -2);
				o_indexData[k] = lua_tointeger(&io_luaState, -1);
				lua_pop(&io_luaState, 1);
				k++;
			}
		}
		else
		{
			eae6320::OutputErrorMessage("Error Message - Error in Mesh.cpp in the function Table_Indices_Values(lua_State& io_luaState) ");
			//Pop the vertice value table
			lua_pop(&io_luaState, 1);
			return false;
		}
		//Pop the vertice value table
		lua_pop(&io_luaState, 1);
	}
	return true;
}

bool eae6320::cMeshBuilder::CreateBinaryFile()
{
	std::ofstream outputBinFile(m_path_target, std::ofstream::binary);

	outputBinFile.write((char*)&o_vertexCount, sizeof(uint32_t));
	outputBinFile.write((char*)&o_indexCount, sizeof(uint32_t));

	outputBinFile.write((char*)o_vertexData, sizeof(sVertex) * o_vertexCount);
	outputBinFile.write((char*)o_indexData, sizeof(uint32_t) * o_indexCount);

	outputBinFile.close();

	return true;
}


