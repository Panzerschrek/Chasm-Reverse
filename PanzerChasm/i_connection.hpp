#pragma once
#include <memory>

namespace PanzerChasm
{

class IConnection
{
public:
	virtual ~IConnection(){}

	virtual void SendReliablePacket( const void* data, unsigned int data_size )= 0;
	virtual void SendUnreliablePacket( const void* data, unsigned int data_size )= 0;

	virtual unsigned int ReadRealiableData( void* out_data, unsigned int buffer_size )= 0;
	virtual unsigned int ReadUnrealiableData( void* out_data, unsigned int buffer_size )= 0;

	virtual void Disconnect()= 0;
	virtual bool Disconnected()= 0;
};

} // namespace PanzerChasm
