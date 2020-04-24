#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Donya/Serializer.h"
#include "Donya/Template.h"
#include "Donya/UseImGui.h"

template<typename DerivedClass>
class ParameterBase : public Donya::Singleton<DerivedClass>
{
public:
	virtual void Init()     = 0;
	virtual void Uninit() {}
protected:
	virtual std::string GetSerializeIdentifier() = 0;
	virtual std::string GetSerializePath( bool isBinary ) = 0;
protected:
	template<class SerializeObject>
	void Load( SerializeObject &object, bool fromBinary )
	{
		Donya::Serializer::Load
		(
			object,
			GetSerializePath( fromBinary ).c_str(),
			GetSerializeIdentifier().c_str(),
			fromBinary
		);
	}
	template<class SerializeObject>
	void Save( SerializeObject &object, bool toBinary )
	{
		Donya::Serializer::Save
		(
			object,
			GetSerializePath( toBinary ).c_str(),
			GetSerializeIdentifier().c_str(),
			toBinary
		);
	}
public:
#if USE_IMGUI
	virtual void UseImGui() = 0;

	template<class SerializeObject>
	void ShowIONode( SerializeObject &IOTarget )
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード（by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8"）";

		if ( ImGui::Button( u8"セーブ" ) )
		{
			Save( IOTarget, true  );
			Save( IOTarget, false );
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			Load( IOTarget, isBinary );
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
};

#if USE_IMGUI
#include "Donya/Collision.h"
#include "Renderer.h"
namespace ParameterHelper
{
	void ShowAABBNode( const std::string &nodeCaption, Donya::AABB *pAABB );
	void ShowSphereNode( const std::string &nodeCaption, Donya::Sphere *pSphere );

	void ShowConstantNode( const std::string &nodeCaption, RenderingHelper::TransConstant *pConstant );
	void ShowConstantNode( const std::string &nodeCaption, RenderingHelper::AdjustColorConstant *pConstant );
}
#endif // USE_IMGUI
