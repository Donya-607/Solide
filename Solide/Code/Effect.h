#pragma once

#include <string>

#include "Effekseer.h"

#include "Donya/Vector.h"

#include "EffectAttribute.h"

class EffectHandle
{
public:
	/// <summary>
	/// Returns invalid object if specified effect was not loaded.
	/// </summary>
	static EffectHandle Generate( EffectAttribute effectAttribute, const Donya::Vector3 &position, int32_t startFrame = 0 );
private:
	Effekseer::Handle handle = -1;
public:
	EffectHandle( const Effekseer::Handle &handle ) : handle( handle ) {}
public:
	/// <summary>
	/// Returns false if my handle is not valid, or the EffectAdmin does not initialized.
	/// </summary>
	bool IsValid() const;
public:
	void SetScale( float scale );
	void SetScale( float scaleX, float scaleY, float scaleZ );
	void SetRotation( float pitch, float yaw, float roll );
	void SetPosition( const Donya::Vector3 &position );
	void Move( const Donya::Vector3 &velocity );
	void Stop();
};
