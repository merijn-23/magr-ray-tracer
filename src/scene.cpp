#include "precomp.h"

#define TINYOBJLOADER_IMPLEMENTATION
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

namespace Tmpl8
{
Scene::Scene( )
{
	/*	primitives = new Primitive[9];
			_sizePrimitives = 9;
			spheres = new Sphere[2];
			_sizeSpheres = 2;
			planes = new Plane[6];
			_sizePlanes = 6;
			materials = new Material[8];
			_sizeMaterials = 8;
			triangles = new Triangle[1];
			_sizeTriangles = 1;
			lights = new Light[3];*/
			//Resize( primitives, 0, 9 );

	AddMaterial( Material{ float3( 1, 0, 0 ), 0, 0, 0, false }, "red" );
	AddMaterial( Material{ float3( 0, 1, 0 ), 0, 0, 0, false }, "green" );
	AddMaterial( Material{ float3( 0, 0, 1 ), 0, 0, 0, false }, "blue" );
	AddMaterial( Material{ float3( 1, 1, 1 ), 0, 0, 0, false }, "white" );
	AddMaterial( Material{ float3( 1, 1, 0 ), 0, 0, 0, false }, "yellow" );
	AddMaterial( Material{ float3( 1, 0, 1 ), 0, 0, 0, false }, "magenta" );
	AddMaterial( Material{ float3( 0, 1, 1 ), 0, 0, 0, false }, "cyan" );
	AddMaterial( Material{ float3( 1, 1, 1 ), 0, 1, 1.5f, true }, "glass" );
	//LoadTexture( "cash_money.png", "cash" );

	AddSphere( float4( 0, 0.f, -1.5f, 0.f ), 0.5f, "glass" );
	AddSphere( float4( 1.5f, -0.49f, 0.f, 0 ), 0.5f, "red" );

	AddPlane( float3( 1, 0, 0 ), 5.f, "yellow" );
	AddPlane( float3( -1, 0, 0 ), 2.99f, "green" );
	AddPlane( float3( 0, 1, 0 ), 1.f, "blue" );
	AddPlane( float3( 0, -1, 0 ), 2.f, "white" );
	AddPlane( float3( 0, 0, 1 ), 9.f, "magenta" );
	AddPlane( float3( 0, 0, -1 ), 3.99f, "cyan" );

	AddTriangle( float4( 0, 0.f, -1.5f, 0.f ), float4( 1, 0.f, -1.5f, 0.f ),
		float4( 0, 1.f, -1.5f, 0.f ), "cyan" );
	//LoadModel( "triangle.obj", "green" );

	lights.resize( 3 );
	lights[0] = Light{ float4( 2, 0, 3, 0 ), float4( 1, 1, .8f, 0 ), 2, -1 };
	lights[1] = Light{ float4( 1, 0, -5, 0 ), float4( 1, 1, .8f, 0 ), 2, -1 };
	lights[2] = Light{ float4( -2, 0, 0, 0 ), float4( 1, 1, .8f, 0 ), 1, -1 };

	SetTime( 0 );
	// Note: once we have triangle support we should get rid of the class
	// hierarchy: virtuals reduce performance somewhat.
}

Scene::~Scene( )
{

}

void Scene::SetTime( float t )
{
	// default time for the scene is simply 0. Updating/ the time per frame
	// enables animation. Updating it per ray can be used for motion blur.
	animTime = t * .1f;
	//lights[0].pos.x = sin(animTime) + 1;
	//lights[0].pos.y = sin(animTime + PI * 0.5) * 0.5f;

	// sphere animation: bounce
	float tm = 1 - sqrf( fmodf( animTime, 2.0f ) - 1 );
	//spheres[0].pos.y = -0.5f + tm;
}

void Scene::AddMaterial( Material material, std::string name )
{
	materials.push_back( material );
	matMap_[name] = matIdx_;
	matIdx_++;
}

void Scene::AddSphere( float3 pos, float radius, std::string material )
{
	float r2 = radius * radius;
	float invr = 1 / radius;

	// create sphere
	Sphere sphere;
	sphere.pos = pos;
	sphere.r2 = r2;
	sphere.invr = invr;
	spheres.push_back( sphere );

	// create primitive
	Primitive prim;
	prim.objType = SPHERE;
	prim.objIdx = spheres.size() - 1;
	prim.matIdx = matMap_[material];
	primitives.push_back( prim );
}

void Scene::AddPlane( float3 N, float d, std::string material )
{
	Plane plane;
	plane.N = N;
	plane.d = d;
	planes.push_back(plane);

	// create primitive
	Primitive prim;
	prim.objType = PLANE;
	prim.objIdx = planes.size() - 1;
	prim.matIdx = matMap_[material];
	primitives.push_back( prim );
}

void Scene::AddTriangle( float3 v0, float3 v1, float3 v2, std::string material )
{
	/*if ( _triIdx >= _sizeTriangles || _primIdx >= _sizePrimitives )
			{
				Resize( triangles, _sizeTriangles );
				Resize( primitives, _sizePrimitives );
			}*/
	float3 v0v1 = v1 - v0;
	float3 v0v2 = v2 - v0;
	float3 N = normalize( cross( v0v1, v0v2 ) );

	Triangle tri;
	tri.v0 = v0;
	tri.v1 = v1;
	tri.v2 = v2;
	tri.N = N;
	//triangles[triIdx_] = tri;
	triangles.push_back( tri );

	// create primitive
	Primitive prim;
	prim.objType = TRIANGLE;
	prim.objIdx = triangles.size() - 1;
	prim.matIdx = matMap_[material];
	primitives.push_back( prim );
}

void Scene::LoadModel( std::string filename, std::string material )
{
	tinyobj::ObjReaderConfig readerConfig;
	tinyobj::ObjReader reader;

	if ( !reader.ParseFromFile( filename, readerConfig ) )
	{
		if ( !reader.Error( ).empty( ) )
			std::cerr << "E/TinyObjReader: " << reader.Error( ) << std::endl;
		return;
	}

	if ( !reader.Warning( ).empty( ) )
		std::cout << "W/TinyObjReader: " << reader.Warning( ) << std::endl;

	auto& attrib = reader.GetAttrib( );
	auto& shapes = reader.GetShapes( );
	auto& materials = reader.GetMaterials( );

	// loop over shapes
	for ( size_t s = 0; s < shapes.size( ); s++ )
	{
		// loop over faces(polygon)
		size_t index_offset = 0;
		for ( size_t f = 0; f < shapes[s].mesh.num_face_vertices.size( ); f++ )
		{
			size_t fv = size_t( shapes[s].mesh.num_face_vertices[f] );

			// loop over vertices in the face.
			std::vector<float3> vertices;
			for ( size_t v = 0; v < fv; v++ )
			{
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				tinyobj::real_t vx = attrib.vertices[3 * size_t( idx.vertex_index ) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t( idx.vertex_index ) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t( idx.vertex_index ) + 2];

				vertices.push_back( float3( vx, vy, vz ) );
			}

			for ( size_t v = 0; v < vertices.size( ); )
				AddTriangle( vertices[v++], vertices[v++], vertices[v++], material );

			index_offset += fv;
		}
	}
}

void Scene::LoadTexture( std::string filename, std::string name )
{
	int width, height, n;
	float3* data = LoadImageF( filename.c_str( ), width, height, n );
	int size = width * height;

	textures.insert( textures.end( ), &data[0], &data[size] );

	Material mat;
	mat.texIdx = 0;
	mat.texSize = size;
	AddMaterial( mat, name );
}
} // namespace Tmpl8
