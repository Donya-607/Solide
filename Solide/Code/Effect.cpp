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

	const Fx::Handle handle	= pManager->Play( pEffect, ToFxVector( pos ), startFrame );
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
