#include "precomp.h"
#define TINYOBJLOADER_IMPLEMENTATION
//Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust triangulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"
namespace Tmpl8
{
	Scene::Scene( )
	{
		bvh2 = new BVH2( primitives, blasNodes );
		// load skydome first
		LoadTexture( "assets/office.hdr", "skydome" );
		// materials
		auto& grey = AddMaterial( "grey" );
		grey.color = float4( 0.231f, 0.266f, 0.294f, 0 );
		auto& mirror = AddMaterial( "mirror" );
		mirror.color = float3( .1f, .1f, .9f );
		mirror.specular = .5f;
		// white glass
		auto& whiteglass = AddMaterial( "white-glass" );
		whiteglass.color = float3( 1 );
		whiteglass.isDieletric = true;
		whiteglass.n1 = 1.f;
		whiteglass.n2 = 1.1f;
		whiteglass.specular = 0.03f;
		whiteglass.absorption = float3( .01f );
		// white light
		auto& whiteLight = AddMaterial( "white-light" );
		whiteLight.isLight = true;
		whiteLight.color = float4( 1.f, .7f, 0.1f, 0 );
		whiteLight.emittance = float4( .9f, .9f, .9f, 0 ) * 100;
		auto& greenLight = AddMaterial( "green-light" );
		greenLight.isLight = true;
		greenLight.color = float4( .1f, 1.f, 0.1f, 0 );
		greenLight.emittance = float4( .1f, 1.f, 0.1f, 0 ) * 10;
		auto& redLight = AddMaterial( "red-light" );
		redLight.isLight = true;
		redLight.color = float4( 1.f, .1f, .1f, 0 );
		redLight.emittance = float4( 1.f, .1f, .1f, 0 ) * 100;
		auto& blueLight = AddMaterial( "blue-light" );
		blueLight.isLight = true;
		blueLight.color = float4( 1.f, .1f, .1f, 0 );
		blueLight.emittance = float4( 1.f, .1f, .1f, 0 ) * 100;

#if 1
		int startPrims = primitives.size( );
		//LoadModel( "assets/robo-orb/robo.obj", "white" );
		//LoadModel( "assets/robo-orb/robo-lights.obj", "yellow-light" );
		//bvh2->BuildBLAS( true, startPrims );
		
		startPrims = primitives.size( );
		LoadModel( "assets/terrarium_bot/bot.obj", "white" );
		LoadModel( "assets/terrarium_bot/bot-glass.obj", "white-glass" );
		bvh2->BuildBLAS( true, startPrims );
		startPrims = primitives.size( );
		LoadModel( "assets/hallway/hallway.obj", "grey", {}, true );
		LoadModel( "assets/hallway/hallway_lights_top.obj", "green-light" );
		LoadModel( "assets/hallway/hallway_lights_top_left.obj", "red-light" );
		LoadModel( "assets/hallway/hallway_lights_top_right.obj", "red-light" );
		LoadModel( "assets/hallway/hallway_lights_back.obj", "red-light" );
		LoadModel( "assets/hallway/hallway_lights_front.obj", "white-light" );
		bvh2->BuildBLAS( true, startPrims );
#else
		LoadModel( "assets/sponza/sponza.obj", "white" );
		// start of separate prims
		int startPrims = primitives.size( );
		AddQuad( float3( -2, 0, -7.5f ), float3( 2, 0, -7.5f ), float3( 2, 4, -7.5f ), float3( -2, 4, -7.5f ), 0, 0, 0, 0, "yellow-light" );
		bvh2->BuildBLAS( true, startPrims );
#endif
		// bvh4 as last
		bvh4 = new BVH4( *bvh2 );
		SetTime( 0 );
	}
	Scene::~Scene( )
	{
	}
	void Scene::SetTime( float t )
	{
		animTime = t * .1f;
		mat4 T;
		T = T.RotateY( animTime );
		//memcpy( blasNodes[0].invT, T.Inverted( ).cell, sizeof( float ) * 16 );
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
		default.isLight = false;
		materials.push_back( default );
		matMap_[name] = matIdx_;
		matIdx_++;
		return materials[matIdx_ - 1];
	}

	static float SphereArea( float r2 )
	{
		return 4 * PI * r2;
	}

	static float TriangleArea( float3 A, float3 B, float3 C )
	{
		//float3 AB = B - A;
		//float3 AC = C - A;

		//float ABL = length( AB );
		//float ACL = length( AC );
		//float theta = dot( AB, AC ) * (1 / (ABL * ACL));
		//return 0.5f * ABL * ACL * sin( theta );
		
		// Heron's Formula https://www.wikiwand.com/en/Heron%27s_formula
		float a = length( B - A );
		float b = length( B - C );
		float c = length( C - A );
		float s = 0.5f * (a + b + c);
		return sqrtf( s * (s - a) * (s - b) * (s - c) );
	}

	void Scene::AddSphere( float3 pos, float radius, std::string material )
	{
		Primitive prim;
		prim.objType = SPHERE;
		prim.objData.sphere.pos = pos;
		prim.objData.sphere.r = radius;
		prim.objData.sphere.r2 = radius * radius;
		prim.objData.sphere.invr = 1 / radius;
		prim.matIdx = matMap_[material];
		prim.area = SphereArea( prim.objData.sphere.r2 );
		primitives.push_back( prim );
		if ( materials[matMap_[material]].isLight )
			lights.push_back( primitives.size( ) - 1 );
	}

	void Scene::AddPlane( float3 N, float d, std::string material )
	{
		Primitive prim;
		prim.objType = PLANE;
		prim.objData.plane.N = N;
		prim.objData.plane.d = d;
		prim.matIdx = matMap_[material];
		primitives.push_back( prim );
		if ( materials[matMap_[material]].isLight )
			lights.push_back( primitives.size( ) - 1 );
	}

	void Scene::AddQuad( float3 v0, float3 v1, float3 v2, float3 v3, const std::string material,  bool _flipNormal, float2 uv0, float2 uv1, float2 uv2, float2 uv3 )
	{
		AddTriangle( v0, v1, v2, uv0, uv1, uv2, material, _flipNormal );
		AddTriangle( v2, v3, v0, uv2, uv3, uv1, material, _flipNormal );
	}

	void Scene::AddTriangle( float3 v0, float3 v1, float3 v2, float2 uv0, float2 uv1, float2 uv2, const std::string material, bool _flipNormal )
	{
		Primitive prim;
		prim.objType = TRIANGLE;
		prim.objData.triangle.v0 = v0;
		prim.objData.triangle.v1 = v1;
		prim.objData.triangle.v2 = v2;
		prim.objData.triangle.uv0 = uv0;
		prim.objData.triangle.uv1 = uv1;
		prim.objData.triangle.uv2 = uv2;
		prim.objData.triangle.N = normalize( cross( v1 - v0, v2 - v0 ) );
		if ( _flipNormal ) prim.objData.triangle.N *= -1;
		prim.objData.triangle.centroid = ( v0 + v1 + v2 ) * ( 1 / 3.f );
		prim.matIdx = matMap_[material];
		prim.area = TriangleArea( v0, v1, v2 );
		primitives.push_back( prim );
		if ( materials[matMap_[material]].isLight )
			lights.push_back( primitives.size( ) - 1 );
	}
	//https://pastebin.com/PZYVnJCd
	void Scene::LoadModel( std::string _filename, const std::string _defaultMat, float3 _pos, bool _forceDefaultMat )
	{
		cout << "Loading model: " << _filename << "..." << endl;
		tinyobj::ObjReaderConfig readerConfig;
		tinyobj::ObjReader reader;
		if ( !reader.ParseFromFile( _filename, readerConfig ) ) {
			if ( !reader.Error( ).empty( ) ) std::cerr << "E/TinyObjReader: " << reader.Error( ) << std::endl;
			return;
		}
		if ( !reader.Warning( ).empty( ) ) std::cout << "W/TinyObjReader: " << reader.Warning( ) << std::endl;
		auto& attrib = reader.GetAttrib( );
		auto& shapes = reader.GetShapes( );
		// load textures
		auto& materials = reader.GetMaterials( );
		for ( const auto& mat : materials ) {
			if ( !mat.diffuse_texname.empty( ) )
				LoadTexture( util::GetBaseDir( _filename ) + mat.diffuse_texname, mat.diffuse_texname );
		}
		for ( size_t s = 0; s < shapes.size( ); s++ ) {
			// start primitive index for bvh
			// loop over faces(polygon)
			size_t index_offset = 0;
			for ( size_t f = 0; f < shapes[s].mesh.num_face_vertices.size( ); f++ ) {
				size_t fv = size_t( shapes[s].mesh.num_face_vertices[f] );
				// loop over vertices, texcoords of the face.
				std::vector<float3> vertices;
				std::vector<float2> texcoords;
				std::vector<float4> colors;
				for ( size_t v = 0; v < fv; v++ ) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					tinyobj::real_t vx = attrib.vertices[3 * size_t( idx.vertex_index ) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t( idx.vertex_index ) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t( idx.vertex_index ) + 2];
					// add vertex
					vertices.push_back( float3( vx, vy, vz ) + _pos );
					// add texcoords
					tinyobj::real_t tx = 0, ty = 0;
					if ( idx.texcoord_index >= 0 ) {
						tx = attrib.texcoords[2 * size_t( idx.texcoord_index ) + 0];
						ty = 1.0 - attrib.texcoords[2 * size_t( idx.texcoord_index ) + 1];
					}
					texcoords.push_back( float2( tx, ty ) );
					// colors
					tinyobj::real_t r = attrib.colors[3 * idx.vertex_index + 0];
					tinyobj::real_t g = attrib.colors[3 * idx.vertex_index + 1];
					tinyobj::real_t b = attrib.colors[3 * idx.vertex_index + 2];
					colors.push_back( float4( r, g, b, 1 ) );
				}
				// reverse the vector to get the correct vertex order
				std::reverse( vertices.begin( ), vertices.end( ) );
				// get material
				int matIdx = shapes[s].mesh.material_ids[f];
				auto tex = _defaultMat; ;
				if ( matIdx >= 0 ) tex = materials[matIdx].diffuse_texname;
				if ( tex.empty( ) || _forceDefaultMat ) tex = _defaultMat;
				// add triangle
				for ( size_t v = 0, t = 0; v < vertices.size( ); )
					AddTriangle( vertices[v++], vertices[v++], vertices[v++],
						texcoords[t++], texcoords[t++], texcoords[t++], tex );
				index_offset += fv;
			}
			// build BLAS for this shape
		}
		printf( "...Finished loading model\n" );
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
