#include "ClearPerformance.h"

#include "Donya/Easing.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"		// Use Lerp

#include "FilePath.h"
#include "Parameter.h"


namespace
{
	std::string GetTypeName( ClearPerformance::Type type )
	{
		switch ( type )
		{
		case ClearPerformance::Type::ShowFrame:			return u8"�g�\��";
		case ClearPerformance::Type::ShowDescription:	return u8"�����\��";
		case ClearPerformance::Type::ShowTime:			return u8"���ԕ\��";
		case ClearPerformance::Type::ShowRank:			return u8"�����N�\��";
		case ClearPerformance::Type::Wait:				return u8"�ҋ@";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected Type!" );
		return "ERROR";
	};

	struct Member
	{
		struct Item
		{
			struct Param
			{
				float			drawAlpha	= 0.0f;
				float			drawDegree	= 0.0f;
				float			drawScale	= 1.0f;
				Donya::Vector2	ssDrawPos{};	// Center
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( drawAlpha  ),
						CEREAL_NVP( drawDegree ),
						CEREAL_NVP( drawScale  ),
						CEREAL_NVP( ssDrawPos  )
					);

					if ( 1 <= version )
					{
						// archive( CEREAL_NVP( x ) );
					}
				}
			};

			Donya::Easing::Kind easeKind	= Donya::Easing::Kind::Linear;
			Donya::Easing::Type easeType	= Donya::Easing::Type::In;
			float	easeTakeSecond			= 1.0f;
			int		waitFrameAfterFinish	= 0;

			Param	paramStart;
			Param	paramDest;

			Donya::Vector2	texPartPos{};	// Left-Top
			Donya::Vector2	texPartSize{};	// Whole size
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( easeKind				),
					CEREAL_NVP( easeType				),
					CEREAL_NVP( easeTakeSecond			),
					CEREAL_NVP( waitFrameAfterFinish	),
					CEREAL_NVP( paramStart				),
					CEREAL_NVP( paramDest				),
					CEREAL_NVP( texPartPos				),
					CEREAL_NVP( texPartSize				)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		struct ShowFrame
		{
			int		wholeFrame = 1;
			Item	item;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive( CEREAL_NVP( wholeFrame ) );

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( item ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct ShowDesc
		{
			int		wholeFrame = 1;
			Item	item;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive( CEREAL_NVP( wholeFrame ) );

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( item ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct ShowTime
		{
			int		wholeFrame = 1;
			Item	item;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive( CEREAL_NVP( wholeFrame ) );

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( item ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct ShowRank
		{
			int		wholeFrame = 1;
			Item	item;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive( CEREAL_NVP( wholeFrame ) );

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( item ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct Wait
		{
			int wholeFrame = 1;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( wholeFrame )
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		ShowFrame	showFrame;
		ShowDesc	showDesc;
		ShowTime	showTime;
		ShowRank	showRank;
		Wait		wait;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( showFrame	),
				CEREAL_NVP( showDesc	),
				CEREAL_NVP( showTime	),
				CEREAL_NVP( showRank	),
				CEREAL_NVP( wait		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				0 )
CEREAL_CLASS_VERSION( Member::Item,			0 )
CEREAL_CLASS_VERSION( Member::Item::Param,	0 )
CEREAL_CLASS_VERSION( Member::ShowFrame,	1 )
CEREAL_CLASS_VERSION( Member::ShowDesc,		1 )
CEREAL_CLASS_VERSION( Member::ShowTime,		1 )
CEREAL_CLASS_VERSION( Member::ShowRank,		1 )
CEREAL_CLASS_VERSION( Member::Wait,			0 )

class ParamClearPerformance : public ParameterBase<ParamClearPerformance>
{
public:
	static constexpr const char *ID = "ClearPerformance";
private:
	Member m;
public:
	void Init() override
	{
	#if DEBUG_MODE
		constexpr bool fromBinary = false;
	#else
		constexpr bool fromBinary = true;
	#endif // DEBUG_MODE

		Load( m, fromBinary );
	}
	Member Data() const { return m; }
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"�N���A���o�̃p�����[�^����" ) )
		{
			auto ShowItem	= [&]( const std::string &nodeCaption, Member::Item *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				auto ShowParam = [&]( const std::string &nodeCaption, Member::Item::Param *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragFloat ( u8"�`��A���t�@",			&p->drawAlpha,  0.01f );
					ImGui::DragFloat ( u8"�`��X�P�[��",			&p->drawScale,  0.01f );
					ImGui::DragFloat ( u8"�`��p�x�iDegree�j",	&p->drawDegree  );
					ImGui::DragFloat2( u8"�`��ʒu",				&p->ssDrawPos.x );

					p->drawAlpha = std::max( 0.0f, std::min( 1.0f, p->drawAlpha ) );
					p->drawScale = std::max( 0.0f, p->drawScale );

					ImGui::TreePop();
				};

				ParameterHelper::ShowEaseParam( u8"�C�[�W���O�^�C�v", &p->easeKind, &p->easeType );
				ImGui::DragFloat( u8"�C�[�W���O�ɂ����鎞�ԁi�b�j", &p->easeTakeSecond, 0.1f );
				p->easeTakeSecond = std::max( 0.0001f, p->easeTakeSecond );

				ShowParam( u8"�n�_", &p->paramStart );
				ShowParam( u8"�I�_", &p->paramDest  );

				ImGui::DragInt( u8"�C�[�W���O�I����̑ҋ@���ԁi�t���[���j", &p->waitFrameAfterFinish );
				p->waitFrameAfterFinish = std::max( 0, p->waitFrameAfterFinish );

				ImGui::DragFloat2( u8"�e�N�X�`�����_�i����j",	&p->texPartPos.x  );
				ImGui::DragFloat2( u8"�e�N�X�`���T�C�Y�i�S�́j",	&p->texPartSize.x );

				ImGui::TreePop();
			};

			auto ShowFrame	= [&]( const std::string &nodeCaption, Member::ShowFrame *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"�S�̎��ԁi�t���[���j", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"�`��ݒ�", &p->item );

				ImGui::TreePop();
			};
			auto ShowDesc	= [&]( const std::string &nodeCaption, Member::ShowDesc *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"�S�̎��ԁi�t���[���j", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"�`��ݒ�", &p->item );

				ImGui::TreePop();
			};
			auto ShowTime	= [&]( const std::string &nodeCaption, Member::ShowTime *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"�S�̎��ԁi�t���[���j", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"�`��ݒ�", &p->item );

				ImGui::TreePop();
			};
			auto ShowRank	= [&]( const std::string &nodeCaption, Member::ShowRank *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"�S�̎��ԁi�t���[���j", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"�`��ݒ�", &p->item );

				ImGui::TreePop();
			};
			auto ShowWait	= [&]( const std::string &nodeCaption, Member::Wait *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"�S�̎��ԁi�t���[���j", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ImGui::TreePop();
			};

			ShowFrame( u8"�g",		&m.showFrame	);
			ShowDesc ( u8"����",		&m.showDesc		);
			ShowTime ( u8"����",		&m.showTime		);
			ShowRank ( u8"�����N",	&m.showRank		);
			ShowWait ( u8"�ҋ@",		&m.wait			);

			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};

namespace
{
	Member FetchMember()
	{
		return ParamClearPerformance::Get().Data();
	}
}


void ClearPerformance::LoadParameter()
{
	ParamClearPerformance::Get().Init();
}
#if USE_IMGUI
void ClearPerformance::UseImGui()
{
	ParamClearPerformance::Get().UseImGui();
}
#endif // USE_IMGUI
#pragma region States
void ClearPerformance::ProcessBase::Init( ClearPerformance &inst )
{
	timer	= 0;
	factor	= 0.0f;
}
void ClearPerformance::ProcessBase::Uninit( ClearPerformance &inst )
{
	timer	= 0;
	factor	= 0.0f;
}

ClearPerformance::Result ClearPerformance::ShowFrame::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showFrame;

	const float elapseTime = 1.0f / ( 60.0f * data.item.easeTakeSecond );
	factor += elapseTime;
	factor =  std::min( 1.0f, factor );

	timer++;
	if ( data.wholeFrame <= timer )
	{
		timer = data.wholeFrame;
		return Result::Finish;
	}
	// else
	return Result::Continue;
}
void ClearPerformance::ShowFrame::Draw( ClearPerformance &inst )
{
	const auto  data = FetchMember().showFrame.item;
	const float time = Donya::Easing::Ease( data.easeKind, data.easeType, factor );
	inst.sprFrame.degree = Donya::Lerp( data.paramStart.drawDegree, data.paramDest.drawDegree, time );
}

ClearPerformance::Result ClearPerformance::ShowDesc::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showDesc;

	timer++;
	if ( data.wholeFrame <= timer )
	{
		timer = data.wholeFrame;
		return Result::Finish;
	}
	// else
	return Result::Continue;
}
void ClearPerformance::ShowDesc::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::ShowTime::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showTime;

	timer++;
	if ( data.wholeFrame <= timer )
	{
		timer = data.wholeFrame;
		return Result::Finish;
	}
	// else
	return Result::Continue;
}
void ClearPerformance::ShowTime::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::ShowRank::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showRank;

	timer++;
	if ( data.wholeFrame <= timer )
	{
		timer = data.wholeFrame;
		return Result::Finish;
	}
	// else
	return Result::Continue;
}
void ClearPerformance::ShowRank::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::Wait::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().wait;

	timer++;
	if ( data.wholeFrame <= timer )
	{
		timer = data.wholeFrame;
		return Result::Finish;
	}
	// else
	return Result::Continue;
}
void ClearPerformance::Wait::Draw( ClearPerformance &inst )
{

}

// region States
#pragma endregion

bool ClearPerformance::Init( const std::wstring &frameSpritePath, const std::wstring &descSpritePath, const std::wstring &numberSpritePath, const std::wstring &rankSpritePath )
{
	bool succeeded = true;
	if ( !sprFrame.LoadSprite( frameSpritePath, 2U ) ) { succeeded = false; }
	if ( !sprDesc.LoadSprite ( descSpritePath,  8U ) ) { succeeded = false; }
	if ( !numberDrawer.Init( numberSpritePath ) ) { succeeded = false; }
	if ( !rankDrawer.Init  ( rankSpritePath   ) ) { succeeded = false; }

	AssignProcess<ShowFrame>( &processPtrs[scast<int>( Type::ShowFrame			)] );
	AssignProcess<ShowDesc>	( &processPtrs[scast<int>( Type::ShowDescription	)] );
	AssignProcess<ShowTime>	( &processPtrs[scast<int>( Type::ShowTime			)] );
	AssignProcess<ShowRank>	( &processPtrs[scast<int>( Type::ShowRank			)] );
	AssignProcess<Wait>		( &processPtrs[scast<int>( Type::Wait				)] );

	Timer zero{}; zero.Set( 0, 0, 0 );
	ResetProcess( zero, 0 );

	return succeeded;
}
void ClearPerformance::ResetProcess( const Timer &currentTime, int resultRank )
{
	nowType		= scast<Type>( 0 );
	timer		= 0;
	clearRank	= resultRank;
	clearTime	= currentTime;
	isFinished	= false;

	for ( auto &pIt : processPtrs )
	{
		if ( pIt ) { pIt->Init( *this ); }
	}
}
void ClearPerformance::Uninit()
{
	for ( auto &pIt : processPtrs )
	{
		if ( pIt ) { pIt->Uninit( *this ); }
	}
}
void ClearPerformance::Update()
{
	const size_t index = std::min( scast<size_t>( nowType ), processPtrs.size() );
	if ( processPtrs[index] )
	{
		Result result = processPtrs[index]->Update( *this );
		if ( result == Result::Finish )
		{
			const size_t limit = scast<size_t>( Type::TypeCount );
			if ( limit <= index + 1 )
			{
				isFinished = true;
			}
			else
			{
				nowType = scast<Type>( std::min( index + 1, limit - 1 ) );
			}
		}
	}
}
void ClearPerformance::Draw()
{
	const size_t drawLimit = std::min( scast<size_t>( nowType ), processPtrs.size() );
	for ( size_t i = 0; i < drawLimit; ++i )
	{
		if ( processPtrs[i] )
		{
			processPtrs[i]->Draw( *this );
		}
	}
}
#if USE_IMGUI
void ClearPerformance::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragInt( u8"�����^�C�}", &timer );
	ImGui::Text( u8"��ԁF[%s]", GetTypeName( nowType ).c_str() );
	if ( ImGui::Button( u8"��Ԃ����Z�b�g" ) )
	{
		ResetProcess( clearTime, clearRank );
	}
	ImGui::Checkbox( u8"�I��������", &isFinished );

	ImGui::TreePop();
}
#endif // USE_IMGUI
