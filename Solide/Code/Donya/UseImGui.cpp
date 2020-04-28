#include "UseImGui.h"

#include "Donya.h"		// Use for GetWindowCaption().

namespace Donya
{
	static bool isAllowShowingImGui = true;
	void SetShowStateOfImGui( bool isAllow )
	{
		isAllowShowingImGui = isAllow;
	}
	void ToggleShowStateOfImGui()
	{
		isAllowShowingImGui = !isAllowShowingImGui;
	}

	bool IsAllowShowImGui()
	{
		return isAllowShowingImGui;
	}

	bool IsMouseHoveringImGuiWindow()
	{
	#if USE_IMGUI
		return ImGui::IsMouseHoveringAnyWindow();
	#else
		return false;
	#endif // !USE_IMGUI
	}
}

#if USE_IMGUI

namespace Donya
{
	Donya::Vector2 FromImVec( const ImVec2 &v ) { return Donya::Vector2{ v.x, v.y }; }
	Donya::Vector4 FromImVec( const ImVec4 &v ) { return Donya::Vector4{ v.x, v.y, v.z, v.w }; }
	ImVec2 ToImVec( const Donya::Vector2 &v ) { return ImVec2{ v.x, v.y }; }
	ImVec4 ToImVec( const Donya::Vector4 &v ) { return ImVec4{ v.x, v.y, v.z, v.w }; }
}

namespace ImGui
{
	bool BeginIfAllowed( const char* name, bool* p_open, ImGuiWindowFlags flags )
	{
	#if !USE_IMGUI

		return false;

	#endif // !USE_IMGUI

		if ( !Donya::IsAllowShowImGui() ) { return false; }
		// else

		const char *windowName	=	( name == nullptr )
									? Donya::GetWindowCaption()
									: name;

		if ( !ImGui::Begin( windowName, p_open, flags ) )
		{
			ImGui::End();
			return false;
		}
		// else
		return true;
	}
}

#endif // USE_IMGUI
