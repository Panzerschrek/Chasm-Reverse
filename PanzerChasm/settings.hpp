#pragma once
#include <map>
#include <string>

namespace PanzerChasm
{

class Settings
{
public:
	explicit Settings( const char* file_name );
	~Settings();

	void SetSetting( const char* name, const char* value );
	void SetSetting( const char* name, int value );
	void SetSetting( const char* name, bool value );
	void SetSetting( const char* name, float value );

	bool IsValue( const char* name ) const;

	// Returns true, if can convert string to number
	bool IsNumber( const char* name ) const;

	const char* GetString( const char* name, const char* default_value = "" ) const;
	int GetInt( const char* name, int default_value = 0 ) const;
	float GetFloat( const char* name, float default_value = 0.0f ) const;
	bool GetBool( const char* name, bool default_value = false ) const;

private:
	/* Special class for settings keys.
	 * Designed for settings fetching without memory allocations.
	 */
	class SettingsStringContainer
	{
	public:
		explicit SettingsStringContainer( const char* str );
		explicit SettingsStringContainer( const std::string& str );
		SettingsStringContainer( const SettingsStringContainer& other );
		~SettingsStringContainer();

		explicit operator std::string() const;

		bool operator < ( const SettingsStringContainer& other ) const;

	private:
		void operator=(const SettingsStringContainer& other )= delete;

	private:
		const char* const c_str_;
		const std::string str_;
	};

	typedef std::map< SettingsStringContainer, std::string > MapType;

private:
	MapType map_;
	std::string file_name_;
};

} // namespace PanzerChasm
