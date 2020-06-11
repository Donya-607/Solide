#include "Effect.h"

#include "EffectAdmin.h"
#include "EffectUtil.h"


namespace Fx = Effekseer;
namespace
{
	EffectAdmin &Admin() { return EffectAdmin::Get(); }
}


EffectHandle EffectHandle::Generate( EffectAttribute attr, const Donya::Vector3 &pos, int32_t startFrame )
{
	Fx::Manager	*pManager	= Admin().GetManagerOrNullptr();
	Fx::Effect	*pEffect	= Admin().GetEffectOrNullptr( attr );
	if ( !pManager || !pEffect )
	{
		assert( !"Error: Invalid manager or effect." );
		return EffectHandle{ -1 };
	}
	// else

	const Fx::Handle handle = pManager->Play( pEffect, ToFxVector( pos ), startFrame );
	if ( pManager->Exists( handle ) )
	{
		const float attrScale = Admin().GetEffectScale( attr );
		// We should update this handle before SetScale().
		// The SetScale() will throws an exception if we didn't call UpdateHandle() before that.
		// Why??? :(
		pManager->UpdateHandle( handle, 0.001f );
		pManager->SetScale( handle, attrScale, attrScale, attrScale );
	}
	return EffectHandle{ handle };
}


bool EffectHandle::IsValid() const
{
	auto pManager = Admin().GetManagerOrNullptr();
	return ( pManager ) ? pManager->Exists( handle ) : false;
}


namespace
{
	template<typename DoingMethod>
	void OperateIfManagerIsAvailable( DoingMethod method )
	{
		Fx::Manager *pManager = Admin().GetManagerOrNullptr();
		if ( !pManager ) { assert( !"Error: Manager is invalid." ); return; }
		// else

		method( pManager );
	}
}
void EffectHandle::SetScale( float scale )
{
	SetScale( scale, scale, scale );
}
void EffectHandle::SetScale( float scaleX, float scaleY, float scaleZ )
{
	OperateIfManagerIsAvailable
	(
		[&]( Fx::Manager *pManager )
		{
			pManager->SetScale( handle, scaleX, scaleY, scaleZ );
		}
	);
}
void EffectHandle::SetRotation( float pitch, float yaw, float roll )
{
	OperateIfManagerIsAvailable
	(
		[&]( Fx::Manager *pManager )
		{
			pManager->SetRotation( handle, pitch, yaw, roll );
		}
	);
}
void EffectHandle::SetPosition( const Donya::Vector3 &pos )
{
	OperateIfManagerIsAvailable
	(
		[&]( Fx::Manager *pManager )
		{
			pManager->SetLocation( handle, ToFxVector( pos ) );
		}
	);
}
void EffectHandle::Move( const Donya::Vector3 &velocity )
{
	OperateIfManagerIsAvailable
	(
		[&]( Fx::Manager *pManager )
		{
			pManager->AddLocation( handle, ToFxVector( velocity ) );
		}
	);
}
void EffectHandle::Stop()
{
	OperateIfManagerIsAvailable
	(
		[&]( Fx::Manager *pManager )
		{
			pManager->StopEffect( handle );
		}
	);
}
