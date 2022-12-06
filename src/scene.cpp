#include "precomp.h"

#define TINYOBJLOADER_IMPLEMENTATION
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

namespace Tmpl8
{
	Scene::Scene( )
	{
		// load skydome first
		LoadTexture( "assets/office.png", "skydome" );

		auto& red = AddMaterial( "red" );
		red.color = float3( 1, 0, 0 );
		auto& green = AddMaterial( "green" );
		green.color = float3( 0, 1, 0 );
		auto& blue = AddMaterial( "blue" );
		blue.color = float3( 0, 0, 1 );
		auto& white = AddMaterial( "white" );
		white.color = float3( 1, 1, 1 );	

		auto& yellow = AddMaterial( "yellow" );
		yellow.color = float3( 1, 1, 0 );

		auto& magenta = AddMaterial( "magenta" );
		magenta.color = float3( 1, 0, 1 );
		magenta.isLight = true;
		magenta.emittance = float3( 1, .1f, 1 );

		auto& mirror = AddMaterial( "mirror" );
		mirror.color = float3( .1f, .1f, .9f );
		mirror.specular = .5f;

		auto& cyan = AddMaterial( "cyan" );
		cyan.color = float3( 0, 1, 1 );

		auto& redglass = AddMaterial( "red-glass" );
		redglass.color = float3( 1 );
		redglass.isDieletric = true;
		redglass.n1 = 1.f;
		redglass.n2 = 1.5f;
		redglass.specular = 0.03f;
		redglass.absorption = float3(0, 1, 1);

		auto& greenglass = AddMaterial("green-glass");
		greenglass = redglass;
		greenglass.absorption = float3(0, 1, 0);

		auto& whiteLight = AddMaterial("white-light");
		whiteLight.isLight = true;
		whiteLight.color = float4(1);
		whiteLight.emittance = float4(2);
		auto& greenLight = AddMaterial("green-light");
		greenLight.isLight = true;
		greenLight.color = float4(.1f, 1, .1f, 0);
		greenLight.emittance = float4(.1f, 1, .1f, 0) * 5;

		LoadTexture( "assets/cash_money.png", "cash" );
		LoadTexture( "assets/suprised_pikachu.png", "pika" );
		LoadTexture( "assets/ufo/ufo_diffuse.png", "ufo");

		// Ceiling lamp
		int lamp_y = 4;
		int size = 50;
		AddTriangle(
			float3( -size, lamp_y, 0 ), float3( size, lamp_y, 0 ), float3( -size, lamp_y, 1 ),
			float2( 1, 0 ), float2( 0, 0 ), float2( 0.5, 1 ), "white-light" );
		AddTriangle(
			float3( -size, lamp_y, 1 ), float3( size, lamp_y, 1 ), float3( size, lamp_y, 0 ),
			float2( 1, 0 ), float2( 0, 0 ), float2( 0.5, 1 ), "white-light" );

		// Back wall
		AddTriangle(
			float3( -size, 2, -1 ), float3( -size, 0, -1 ), float3(size, 2, -1 ),
			float2( 1, 0 ), float2( 0, 0 ), float2( 0.5, 1 ), "white" );	
		AddTriangle(
			float3( -size, 0, -1 ), float3(size, 0, -1 ), float3(size, 2, -1 ),
			float2( 1, 0 ), float2( 0, 0 ), float2( 0.5, 1 ), "white" );	

		// Floor
		AddTriangle(
			float3( -size, 0, -1 ), float3( -size, 0, 3 ), float3(size, 0, -1 ),
			float2( 1, 0 ), float2( 0, 0 ), float2( 0.5, 1 ), "white" );	
		AddTriangle(
			float3( -size, 0, 3 ), float3(size, 0, 3 ), float3(size, 0, -1 ),
			float2( 1, 0 ), float2( 0, 0 ), float2( 0.5, 1 ), "white" );	


		float step = -1.5f;
		AddSphere( float3( 1 * step, -0.75f, 0), 1, "white" );
		AddSphere( float3( 1 * step, .75f, 0), .5f, "green" );

		AddSphere(float3(2 * step, -0.75f, 0), 1, "white");
		AddSphere(float3(2 * step, .75f, 0), .5f, "mirror");

		AddSphere(float3(3 * step, -0.75f, 0), 1, "white");
		AddSphere(float3(3 * step, .75f, 0), .5f, "red-glass");

		AddSphere(float3(4 * step, -0.75f, 0), 1, "white");
		AddSphere(float3(4 * step, .75f, 0), .5f, "pika");

		LoadModel("assets/cube.obj", "green-glass", float3(5 * step, .75f, 0));
		
		//LoadModel( "assets/ufo/Low_poly_UFO.obj", "glass", float3(0, 2, -2));
		//AddSphere( float3( 0, 1.5f, -2.f ), 0.5f, "green-light" );



		//AddPlane( float3( -1, 0, 0 ), 2.99f, "green" );
		//AddPlane( float3( 1, 0, 0 ), 5.f, "yellow" );
		//AddPlane( float3( 0, 1, 0 ), 0, "white" );
		//AddPlane( float3( 0, -1, 0 ), 2.f, "white" );
		//AddPlane( float3( 0, 0, 1 ), 9.f, "magenta" );
		//AddPlane( float3( 0, 0, -1 ), 3.99f, "cyan" );

		

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

	Material& Scene::AddMaterial( std::string name )
	{
		Material default;
		default.color = float4( 0 );
		default.absorption = float4( 0 );
		default.isDieletric = false;
		default.n1 = default.n2 = 0;
		default.specular = 0;
		default.texIdx = -1;
		default.texH = 0;
		default.texW = 0;

		materials.push_back( default );
		matMap_[name] = matIdx_;
		matIdx_++;
		return materials[matIdx_ - 1];
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
		prim.objIdx = spheres.size( ) - 1;
		prim.matIdx = matMap_[material];
		primitives.push_back( prim );
	}

	void Scene::AddPlane( float3 N, float d, std::string material )
	{
		Plane plane;
		plane.N = N;
		plane.d = d;
		planes.push_back( plane );

		// create primitive
		Primitive prim;
		prim.objType = PLANE;
		prim.objIdx = planes.size( ) - 1;
		prim.matIdx = matMap_[material];
		primitives.push_back( prim );
	}

	void Scene::AddTriangle( float3 v0, float3 v1, float3 v2, float2 uv0, float2 uv1, float2 uv2, std::string material )
	{
		float3 v0v1 = v1 - v0;
		float3 v0v2 = v2 - v0;
		float3 N = normalize( cross( v0v1, v0v2 ) );

		Triangle tri;
		tri.v0 = v0;
		tri.v1 = v1;
		tri.v2 = v2;
		tri.uv0 = uv0;
		tri.uv1 = uv1;
		tri.uv2 = uv2;

		tri.N = N;
		triangles.push_back( tri );

		// create primitive
		Primitive prim;
		prim.objType = TRIANGLE;
		prim.objIdx = triangles.size( ) - 1;
		prim.matIdx = matMap_[material];
		primitives.push_back( prim );
	}

	void Scene::LoadModel( std::string filename, std::string material, float3 pos )
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
				std::vector<float2> texcoords;
				for ( size_t v = 0; v < fv; v++ )
				{
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

					tinyobj::real_t vx = attrib.vertices[3 * size_t( idx.vertex_index ) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t( idx.vertex_index ) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t( idx.vertex_index ) + 2];

					vertices.push_back( float3( vx, vy, vz ) + pos );

					tinyobj::real_t tx = 0, ty = 0;
					if ( idx.texcoord_index >= 0 )
					{
						tx = attrib.texcoords[2 * size_t( idx.texcoord_index ) + 0];
						ty = attrib.texcoords[2 * size_t( idx.texcoord_index ) + 1];
					}
					texcoords.push_back( float2( tx, ty ) );
				}

				// reverse the vector to get the correct vertex order
				std::reverse(vertices.begin(), vertices.end());

				for ( size_t v = 0, t = 0; v < vertices.size(); )
					AddTriangle(
						vertices[v++], vertices[v++], vertices[v++],
						texcoords[t++], texcoords[t++], texcoords[t++], material );

				
				index_offset += fv;
			}
		}
	}

	void Scene::LoadTexture( std::string filename, std::string name )
	{
		int width, height, n;
		float3* data = LoadImageF( filename.c_str( ), width, height, n );
		int size = width * height;

		int texIdx = textures.size( );
		textures.insert( textures.end( ), &data[0], &data[size] );

		auto& mat = AddMaterial( name );
		mat.texIdx = texIdx;
		mat.isDieletric = false;
		mat.texW = width;
		mat.texH = height;
	}

}; // namespace Tmpl8
