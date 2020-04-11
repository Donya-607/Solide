#include "Terrain.h"

#include <exception>

#include "Donya/Loader.h"
#include "Donya/Useful.h"

#include "FilePath.h"

Terrain::Terrain( int stageNo ) :
	scale( 1.0f, 1.0f, 1.0f ), translation(),
	matWorld( Donya::Vector4x4::Identity() ),
	pDrawModel( nullptr ), pPose( nullptr ), pPolygons( nullptr )
{
	const std::string drawModelPath			= MakeTerrainModelPath( "Display",   stageNo );
	const std::string collisionModelPath	= MakeTerrainModelPath( "Collision", stageNo );
	if ( !Donya::IsExistFile( drawModelPath ) || !Donya::IsExistFile( collisionModelPath ) )
	{
		return;
	}
	// else

	auto Load = []( Donya::Loader *pLoader, const std::string &filePath )
	{
		pLoader->ClearData();
		if ( !pLoader->Load( filePath ) )
		{
			const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
			const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
			_ASSERT_EXPR( 0, errMsg.c_str() );

			throw std::runtime_error{ "Loading Error" };
		}
	};

	Donya::Loader loader{};
	Load( &loader, drawModelPath );

	const auto drawModel = Donya::Model::StaticModel::Create( loader.GetModelSource(), loader.GetFileDirectory() );
	pDrawModel = std::make_shared<Donya::Model::StaticModel>( drawModel );
	_ASSERT_EXPR( pDrawModel->WasInitializeSucceeded(), L"Failed : The model creation of Terrain." );

	pPose = std::make_shared<Donya::Model::Pose>();
	pPose->AssignSkeletal( loader.GetModelSource().skeletal );
	pPose->UpdateTransformMatrices();

	Load( &loader, collisionModelPath );

#if DEBUG_MODE
	const auto collisionModel = Donya::Model::StaticModel::Create( loader.GetModelSource(), loader.GetFileDirectory() );
	pCollisionModel = std::make_shared<Donya::Model::StaticModel>( collisionModel );
	_ASSERT_EXPR( pCollisionModel->WasInitializeSucceeded(), L"Debug.Failed : The collision model creation of Terrain." );
	pCollisionPose = std::make_shared<Donya::Model::Pose>();
	pCollisionPose->AssignSkeletal( loader.GetModelSource().skeletal );
	pCollisionPose->UpdateTransformMatrices();
#endif // DEBUG_MODE

	pPolygons = std::make_shared<Donya::Model::PolygonGroup>
	(
		loader.GetPolygonGroup()
	);
}

void Terrain::SetWorldConfig( const Donya::Vector3 &scaling, const Donya::Vector3 &translate )
{
	scale		= scaling;
	translation	= translate;
}
void Terrain::BuildWorldMatrix()
{
	// matWorld =
	// Donya::Vector4x4::MakeScaling( scale ) *
	// Donya::Vector4x4::MakeTranslation( translation );
	
	matWorld._11 = scale.x;
	matWorld._22 = scale.y;
	matWorld._33 = scale.z;
	matWorld._41 = translation.x;
	matWorld._42 = translation.y;
	matWorld._43 = translation.z;
}

void Terrain::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	if ( !pDrawModel || !pPose ) { return; }
	// else

	Donya::Model::Constants::PerModel::Common constant{};
	constant.drawColor		= color;
	constant.worldMatrix	= GetWorldMatrix();
	pRenderer->UpdateConstant( constant );
	pRenderer->ActivateConstantModel();

#if DEBUG_MODE && 0
	if ( !pCollisionModel || !pCollisionPose ) { return; }
	// else

	pRenderer->Render( *pCollisionModel, *pCollisionPose );
	// pRenderer->Render( *pCollisionModel, *pPose );
#endif // DEBUG_MODE
	pRenderer->Render( *pDrawModel, *pPose );

	pRenderer->DeactivateConstantModel();
}

#if USE_IMGUI
void Terrain::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"スケール", &scale.x,		0.01f );
	ImGui::DragFloat3( u8"平行移動", &translation.x,	0.1f  );

	ImGui::TreePop();
}
#endif // USE_IMGUI
