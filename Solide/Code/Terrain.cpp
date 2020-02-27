#include "Terrain.h"

#include <exception>

#include "Donya/Loader.h"
#include "Donya/Useful.h"

Terrain::Terrain( const std::string &drawMeshName, const std::string &collisionMeshName ) :
	pCollisionMesh( nullptr ), pDrawMesh( nullptr ),
	scale( 1.0f, 1.0f, 1.0f ), translation(),
	matWorld( Donya::Vector4x4::Identity() )
{
	bool succeeded = true;

	struct Bundle { std::string fileName; std::shared_ptr<Donya::StaticMesh> *pMesh; };
	const  Bundle bundles[]
	{
		Bundle{ collisionMeshName,	&pCollisionMesh	},
		Bundle{ drawMeshName,		&pDrawMesh		}
	};

	for ( auto &it : bundles )
	{
		Donya::Loader loader{};
		succeeded = loader.Load( it.fileName, nullptr );
		if ( !succeeded )
		{
			const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
			const std::wstring errMsg = errMsgBase + Donya::MultiToWide( it.fileName );
			_ASSERT_EXPR( 0, errMsg.c_str() );
			throw std::runtime_error{ "Loader's Error" };
		}
		// else

		*( it.pMesh ) = std::make_shared<Donya::StaticMesh>();
		succeeded = Donya::StaticMesh::Create( loader, *( it.pMesh->get() ) );
		if ( !succeeded )
		{
			const std::wstring errMsgBase{ L"Failed : Creating a model. That is : " };
			const std::wstring errMsg = errMsgBase + Donya::MultiToWide( it.fileName );
			_ASSERT_EXPR( 0, errMsg.c_str() );
			throw std::runtime_error{ "Mesh's Error" };
		}
	}
}

void Terrain::SetWorldConfig( const Donya::Vector3 &scaling, const Donya::Vector3 &translate )
{
	scale		= scaling;
	translation	= translate;
}
void Terrain::BuildWorldMatrix()
{
	matWorld =
	Donya::Vector4x4::MakeScaling( scale ) *
	Donya::Vector4x4::MakeTranslation( translation );
}

void Terrain::Render( const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	const Donya::Vector4x4 W = GetWorldMatrix();
	// pDrawMesh->Render
	pCollisionMesh->Render
	(
		nullptr,
		true, true,
		W * VP, W,
		lightDir, color
	);
}

#if USE_IMGUI
void Terrain::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"スケール", &scale.x,		0.01f );
	ImGui::DragFloat3( u8"平行移動", &translation.x,	0.01f );

	ImGui::TreePop();
}
#endif // USE_IMGUI
