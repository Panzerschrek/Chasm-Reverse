#include <cctype>
#include <cstring>

#include "common/files.hpp"
using namespace ChasmReverse;

#include "log.hpp"

#include "settings.hpp"

namespace PanzerChasm
{

static bool StrToInt( const char* str, int* i )
{
	int sign = 1;
	if( str[0] == '-' )
	{
		sign = -1;
		str++;
	}

	int v = 0;
	while( *str != 0 )
	{
		if( str[0] < '0' || str[0] > '9' )
			return false;
		v*= 10;
		v+= str[0] - '0';
		str++;
	}
	*i= v * sign;
	return true;
}

static bool StrToFloat( const char* str, float* f )
{
	float sign = 1.0f;
	if( str[0] == '-' )
	{
		sign = -1.0f;
		str++;
	}

	float v = 0;
	while( *str != 0 )
	{
		if( str[0] == ',' || str[0] == '.' )
		{
			str++;
			break;
		}
		if( str[0] < '0' || str[0] > '9' )
			return false;
		v*= 10.0f;
		v+= float(str[0] - '0');
		str++;
	}
	float m = 0.1f;
	while( *str != 0 )
	{
		if( str[0] < '0' || str[0] > '9' )
			return false;

		v+= float(str[0] - '0') * m;
		m*= 0.1f;
		str++;
	}

	*f= v * sign;
	return true;
}

std::string FloatToStr( const float f )
{
	// HACK - replace ',' to '.' for bad locale
	std::string result= std::to_string( f );
	size_t pos = result.find(",");
	if ( pos != std::string::npos ) result[pos] = '.';

	return result;
}

static std::string MakeQuotedString( const std::string& str )
{
	std::string result;
	result.reserve( str.size() + 3u );
	result+= "\"";

	const char* s= str.c_str();
	while( *s != '\0' )
	{
		if( *s == '"' || *s == '\\' )
			result+= '\\';
		result+= *s;
		s++;
	}

	result += "\"";
	return result;
}

Settings::Settings( const char* file_name )
	: file_name_(file_name)
{
	FILE* const file= std::fopen( file_name, "rb" );
	if( file == nullptr )
	{
		Log::Warning( "Can not open settins file \"", file_name, "\"" );
		return;
	}

	std::fseek( file, 0, SEEK_END );
	const unsigned int file_size= std::ftell( file );
	std::fseek( file, 0, SEEK_SET );

	std::vector<char> file_data;
	file_data.resize( file_size + 1u );
	file_data[ file_size ]= '\0';

	FileRead( file, file_data.data(), file_size );
	std::fclose( file );

	const char* s= file_data.data();
	while( *s != '\0' )
	{
		std::string str[2]; // key-value pair

		for( unsigned int i= 0u; i < 2u; i++ )
		{
			while( std::isspace( *s ) && *s != '\0' ) s++;
			if( *s == '\0' ) break;

			if( *s == '"' ) // string in quotes
			{
				s++;
				while( *s != '\0' && *s != '"' )
				{
					if( *s == '\\' && ( s[1] == '"' || s[1] == '\\' ) ) s++; // Escaped symbol
					str[i].push_back( *s );
					s++;
				}
				if( *s == '"' )
					s++;
				else
					break;
			}
			else
			{
				while( *s != '\0' && !std::isspace( *s ) )
				{
					str[i].push_back( *s );
					s++;
				}
			}
		}

		if( ! str[0].empty() )
			map_[ SettingsStringContainer(str[0]) ]= str[1];
	}
}

Settings::~Settings()
{
	FILE* const file= std::fopen( file_name_.c_str(), "wb" );
	if( file == nullptr )
	{
		Log::Warning( "Can not open settins file \"", file_name_, "\"" );
		return;
	}

	for( const MapType::value_type& map_value : map_ )
	{
		const std::string key= MakeQuotedString( std::string(map_value.first) );
		const std::string value= MakeQuotedString( map_value.second );

		std::fprintf( file, "%s %s\r\n", key.c_str(), value.c_str() );
	}

	std::fclose( file );
}

void Settings::SetSetting( const char* const name, const char* const value )
{
	map_[ SettingsStringContainer(name) ]= std::string(value);
}

void Settings::SetSetting( const char* const name, const int value )
{
	map_[ SettingsStringContainer(name) ]= std::to_string(value);
}

void Settings::SetSetting( const char* const name, const bool value )
{
	SetSetting( name, value ? 1 : 0 );
}

void Settings::SetSetting( const char* const name, const float value )
{
	map_[ SettingsStringContainer(name) ]= FloatToStr( value );
}

bool Settings::IsValue( const char* const name ) const
{
	return map_.find( SettingsStringContainer(name) ) != map_.cend();
}

bool Settings::IsNumber( const char* const name ) const
{
	const auto it = map_.find( SettingsStringContainer(name) );
	if( it == map_.cend() )
		return false;

	float f;
	return StrToFloat( (*it).second.data(), &f );
}

const char* Settings::GetString( const char* const name, const char* const default_value ) const
{
	const auto it= map_.find( SettingsStringContainer(name) );
	if ( it == map_.cend() )
		return default_value;

	return it->second.data();
}

int Settings::GetInt( const char* const name, const int default_value ) const
{
	const auto it= map_.find( SettingsStringContainer(name) );
	if( it == map_.cend() )
		return default_value;

	int val;
	if( StrToInt( (*it).second.data(), &val ) )
		return val;
	return default_value;
}

float Settings::GetFloat( const char* const name, const float default_value ) const
{
	const auto it = map_.find( SettingsStringContainer(name) );
	if( it == map_.cend() )
		return default_value;

	float val;
	if( StrToFloat( (*it).second.data(), &val ) )
		return val;
	return default_value;
}

bool Settings::GetBool( const char* const name, const bool default_value ) const
{
	return GetInt( name, default_value ? 1 : 0 ) != 0;
}

const char* Settings::GetOrSetString( const char* const name, const char* const default_value )
{
	const auto it = map_.find( SettingsStringContainer(name) );
	if ( it == map_.cend() )
	{
		map_[ SettingsStringContainer(name) ]= std::string( default_value );
		return default_value;
	}

	return it->second.data();
}

int Settings::GetOrSetInt( const char* const name, const int default_value )
{
	const auto it= map_.find( SettingsStringContainer(name) );
	if ( it != map_.cend() )
	{
		int val;
		if( StrToInt( it->second.data(), &val ) )
			return val;
	}

	map_[ SettingsStringContainer(name) ]= std::to_string( default_value );
	return default_value;
}

float Settings::GetOrSetFloat( const char* const name, const float default_value )
{
	const auto it= map_.find( SettingsStringContainer(name) );
	if ( it != map_.cend() )
	{
		float val;
		if( StrToFloat( it->second.data(), &val ) )
			return val;
	}

	map_[ SettingsStringContainer(name) ]= FloatToStr( default_value );
	return default_value;
}

bool Settings::GetOrSetBool( const char* const name, const bool default_value )
{
	return GetOrSetInt( name, default_value ? 1 : 0 ) != 0;
}

void Settings::GetSettingsKeysStartsWith(
	const char* const settings_key_start,
	std::vector<std::string>& out_seettings_keys ) const
{
	for( const MapType::value_type& map_value : map_ )
	{
		const char* const str= map_value.first.CStr();
		if( std::strstr( str, settings_key_start ) == str )
			out_seettings_keys.push_back( str );
	}
}

/*
------------------SettingsStringContainer-----------------------
*/

Settings::SettingsStringContainer::SettingsStringContainer( const char* const str )
	: c_str_(str), str_()
{}

Settings::SettingsStringContainer::SettingsStringContainer( const std::string& str )
	: c_str_(nullptr), str_(str)
{}

Settings::SettingsStringContainer::SettingsStringContainer( const SettingsStringContainer& other )
	: c_str_(nullptr)
	, str_( other.c_str_ ? other.c_str_ : other.str_ )
{}

Settings::SettingsStringContainer::operator std::string() const
{
	return c_str_ ? std::string(c_str_) : str_;
}

const char* Settings::SettingsStringContainer::CStr() const
{
	return c_str_ != nullptr ? c_str_ : str_.c_str();
}

Settings::SettingsStringContainer::~SettingsStringContainer()
{}

bool Settings::SettingsStringContainer::operator < ( const SettingsStringContainer& other ) const
{
	const char* const this_c_str= c_str_ ? c_str_ : str_.c_str();
	const char* const other_c_str= other.c_str_ ? other.c_str_ : other.str_.c_str();
	return std::strcmp( this_c_str, other_c_str ) < 0;
}

} // namespace PanzerChasm
