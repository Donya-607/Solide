#include "Framework.h"

#include <array>

#include "Donya/Blend.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Resource.h"
#include "Donya/ScreenShake.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"
#include "Donya/UseImgui.h"

#include "Common.h"
#include "EffectAdmin.h"
#include "EffectAttribute.h"
#include "Music.h"

using namespace DirectX;

Framework::Framework() :
	pSceneMng( nullptr )
{}
Framework::~Framework() = default;

bool Framework::Init()
{
	pSceneMng = std::make_unique<SceneMng>();

#if DEBUG_MODE
	pSceneMng->Init( Scene::Type::Load );
	// pSceneMng->Init( Scene::Type::Logo );
#else
	pSceneMng->Init( Scene::Type::Logo );
#endif // DEBUG_MODE

	return true;
}

void Framework::Uninit()
{
	pSceneMng->Uninit();
}

void Framework::Update( float elapsedTime/*Elapsed seconds from last frame*/ )
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'C' ) )
		{
			char debugstopper = 0;
		}
		if ( Donya::Keyboard::Trigger( 'T' ) )
		{
			Donya::ToggleShowStateOfImGui();
		}
		if ( Donya::Keyboard::Trigger( 'H' ) )
		{
			Common::ToggleShowCollision();
		}
	}
#endif // DEBUG_MODE
	
#if USE_IMGUI
	DebugShowInformation();
#endif // USE_IMGUI

	pSceneMng->Update( elapsedTime );

	EffectAdmin::Get().Update();
}

void Framework::Draw( float elapsedTime/*Elapsed seconds from last frame*/ )
{
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

	pSceneMng->Draw( elapsedTime );

	EffectAdmin::Get().Draw();
}

#if USE_IMGUI
#include "Donya/Easing.h"
#include "Donya/Mouse.h"
#include "Donya/ScreenShake.h"
#include "Effect.h"
void Framework::DebugShowInformation()
{
	constexpr Donya::Vector2 windowSize{ 360.0f, 540.0f };
	const     Donya::Vector2 windowPosLT
	{
		Common::ScreenWidthF()  - windowSize.x,
		Common::ScreenHeightF() - windowSize.y,
	};
	ImGui::SetNextWindowPos ( Donya::ToImVec( windowPosLT ), ImGuiCond_Once );
	ImGui::SetNextWindowSize( Donya::ToImVec( windowSize  ), ImGuiCond_Once );

	if ( !ImGui::BeginIfAllowed( u8"�f�o�b�O���" ) ) { return; }
	// else

	ImGui::Text( u8"�u�e�Q�L�[�v�������ƁC�V�[���J�ڂ��s���܂��B" );
	ImGui::Text( u8"�u�`�k�s�L�[�v�������Ȃ���C" );
	ImGui::Text( u8"�@�u�g�L�[�v�ŁC�����蔻������̗L�����C" );
	ImGui::Text( u8"�@�u�s�L�[�v�ŁCImGui�̕\���̗L�����C�؂�ւ��܂��B" );
	ImGui::Text( "" );

	if ( ImGui::TreeNode( u8"�G�t�F�N�g�����e�X�g" ) )
	{
		static std::shared_ptr<EffectHandle> pHandle = nullptr;
		static Donya::Vector3 genPos{};
		static Donya::Vector3 setPos{};
		static Donya::Vector3 velocity{};
		ImGui::DragFloat3( u8"�����ʒu",	&genPos.x,		0.1f );
		ImGui::DragFloat3( u8"�ݒ�ʒu",	&setPos.x,		0.1f );
		ImGui::DragFloat3( u8"�ړ����x",	&velocity.x,	0.1f );

		if ( ImGui::Button( u8"����" ) )
		{
			if ( pHandle ) { pHandle->Stop(); }
			pHandle.reset();

			pHandle = std::make_shared<EffectHandle>( EffectHandle::Generate( EffectAttribute::Fire, genPos ) );
			if ( !pHandle->IsValid() ) { pHandle.reset(); }
		}
		if ( ImGui::Button( u8"�ݒ�ʒu����" ) && pHandle )
		{
			pHandle->SetPosition( setPos );
		}
		if ( ImGui::Button( u8"Scale = 1.0f" ) && pHandle )
		{
			pHandle->SetScale( 1.0f );
		}
		if ( ImGui::Button( u8"Scale = 0.0f" ) && pHandle )
		{
			pHandle->SetScale( 0.0f );
		}
		if ( ImGui::Button( u8"�X�g�b�v" ) && pHandle )
		{
			pHandle->Stop();
			pHandle.reset();
		}

		if ( pHandle )
		{
			pHandle->Move( velocity );
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"�}�E�X���" ) )
	{
		int x = 0, y = 0;
		Donya::Mouse::Coordinate( &x, &y );
		ImGui::Text( u8"�}�E�X���W[X:%d][Y:%d]", x, y );
		ImGui::Text( u8"�}�E�X�z�C�[��[%d]", Donya::Mouse::WheelRot() );

		int LB = 0, MB = 0, RB = 0;
		LB = Donya::Mouse::Press( Donya::Mouse::LEFT	);
		MB = Donya::Mouse::Press( Donya::Mouse::MIDDLE	);
		RB = Donya::Mouse::Press( Donya::Mouse::RIGHT	);
		ImGui::Text( "LB : %d, MB : %d, RB : %d", LB, MB, RB );

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"�C�[�W���O�T���v��" ) )
	{
		using namespace Donya;

		static float time = 0.0f;
		ImGui::SliderFloat( u8"����", &time, 0.0f, 1.0f );
		ImGui::Text( "" );

		static Easing::Type type = Easing::Type::In;
		{
			int iType = scast<int>( type );

			std::string caption = "Type : ";
			caption += Easing::TypeName( iType );

			ImGui::SliderInt( caption.c_str(), &iType, 0, Easing::GetTypeCount() - 1 );

			type = scast<Easing::Type>( iType );
		}

		constexpr int kindCount = Easing::GetKindCount();
		std::array<float, kindCount> easeResults{};
		for ( int i = 0; i < kindCount; ++i )
		{
			easeResults[i] = Easing::Ease( scast<Easing::Kind>( i ), type, time );
		}

		for ( int i = 0; i < kindCount; ++i )
		{
			float tmp = easeResults[i]; // Don't change the source.
			ImGui::SliderFloat( Easing::KindName( i ), &tmp, -1.0f, 2.0f );
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"��ʃV�F�C�N�̊m�F" ) )
	{
		static float power		= 20.0f;
		static float decel		= 5.0f;
		static float time		= 1.0f;
		static float interval	= 0.05f;
		static Donya::ScreenShake::Kind kind = Donya::ScreenShake::Kind::MOMENT;

		ImGui::Text( "now X : %f\n", Donya::ScreenShake::GetX() );
		ImGui::Text( "now Y : %f\n", Donya::ScreenShake::GetY() );

		// ImGui::SliderFloat( "Power",			&power,		6.0f, 128.0f );
		// ImGui::SliderFloat( "Deceleration",	&decel,		0.2f, 64.0f );
		// ImGui::SliderFloat( "ShakeTime",		&time,		0.1f, 10.0f );
		// ImGui::SliderFloat( "Interval",		&interval,	0.1f, 3.0f );
		ImGui::DragFloat( "Power",			&power,		0.1f  );
		ImGui::DragFloat( "Deceleration",	&decel,		0.01f );
		ImGui::DragFloat( "ShakeTime",		&time,		0.01f );
		ImGui::DragFloat( "Interval",		&interval,	0.01f );

		if ( ImGui::Button( "Toggle the kind" ) )
		{
			kind =	( kind == Donya::ScreenShake::MOMENT )
					? Donya::ScreenShake::PERMANENCE
					: Donya::ScreenShake::MOMENT;
		}
		ImGui::Text( "Now Kind : %s", ( kind == Donya::ScreenShake::MOMENT ) ? "Moment" : "Permanence" );

		if ( ImGui::Button( "Activate Shake X" ) )
		{
			if ( Donya::Keyboard::Shifts() )
			{
				Donya::ScreenShake::SetX( kind, power );
			}
			else
			{
				Donya::ScreenShake::SetX( kind, power, decel, time, interval );
			}
		}
		if ( ImGui::Button( "Activate Shake Y" ) )
		{
			if ( Donya::Keyboard::Shifts() )
			{
				Donya::ScreenShake::SetY( kind, power );
			}
			else
			{
				Donya::ScreenShake::SetY( kind, power, decel, time, interval );
			}
		}
		if ( ImGui::Button( "Stop Shake" ) )
		{
			Donya::ScreenShake::StopX();
			Donya::ScreenShake::StopY();
		}

		ImGui::TreePop();
	}

	ImGui::End();

}
#endif // USE_IMGUI
