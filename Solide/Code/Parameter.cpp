#include "Parameter.h"

#if USE_IMGUI
void ParameterBase::ShowIONode( ParameterBase *pParam )
{
	if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
	// else

	static bool isBinary = true;
	if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
	if ( ImGui::RadioButton( "JSON",  !isBinary ) ) { isBinary = false; }

	std::string loadStr = ( isBinary ) ? u8"Binary" : u8"JSON";
	loadStr += u8"ファイルから読み込む";

	if ( ImGui::Button( u8"保存" ) )
	{
		pParam->SaveBin();
		pParam->SaveJson();
	}
	if ( ImGui::Button( loadStr.c_str() ) )
	{
		( isBinary ) ? pParam->LoadBin() : pParam->LoadJson();
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI

void ParameterStorage::Reset()
{
	storage.clear();
}

std::unique_ptr<ParameterBase> *ParameterStorage::Find( const std::string &keyName )
{
	auto find = storage.find( keyName );
	return ( find == storage.end() ) ? nullptr : &( find->second );
}
