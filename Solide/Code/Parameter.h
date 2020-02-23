#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Donya/Template.h"
#include "Donya/UseImGui.h"

template<typename DerivedClass>
class ParameterBase : public Donya::Singleton<DerivedClass>
{
public:
	virtual void Init()     = 0;
	virtual void Uninit()   = 0;
	/* // For static polymorphism.
	DerivedClass &AcquireDerivedClass()
	{
		return static_cast<DerivedClass &>( *this );
	}
	*/
protected:
	virtual void LoadBin()  = 0;
	virtual void LoadJson() = 0;
	virtual void SaveBin()  = 0;
	virtual void SaveJson() = 0;
public:
#if USE_IMGUI
	virtual void UseImGui() = 0;

	void ShowIONode()
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
			SaveBin();
			SaveJson();
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin() : LoadJson();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
};

#if USE_IMGUI
#include "Donya/Collision.h"
namespace ParameterHelper
{
	void ShowAABBNode( const std::string &nodeCaption, Donya::AABB *pAABB );
}
#endif // USE_IMGUI
