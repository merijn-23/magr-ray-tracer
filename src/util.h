#pragma	once

namespace util
{
	inline std::string GetBaseDir( std::string filename )
	{
		using std::filesystem::path;
		path p( filename );
		return p.parent_path( ).string( ) + '/';
	}
}