#include "ClearPerformance.h"

#include "Donya/Template.h"

#include "FilePath.h"
#include "Parameter.h"


namespace
{
	std::string GetTypeName( ClearPerformance::Type type )
	{
		switch ( type )
		{
		case ClearPerformance::Type::ShowFrame:			return u8"枠表示";
		case ClearPerformance::Type::ShowDescription:	return u8"説明表示";
		case ClearPerformance::Type::ShowTime:			return u8"時間表示";
		case ClearPerformance::Type::ShowRank:			return u8"ランク表示";
		case ClearPerformance::Type::Wait:				return u8"待機";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected Type!" );
		return "ERROR";
	};

	struct Member
	{
		struct Item
		{
			float			drawScale = 1.0f;
			Donya::Vector2	ssDrawPos{};	// Center
			Donya::Vector2	texPartPos{};	// Left-Top
			Donya::Vector2	texPartSize{};	// Whole size
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( drawScale	),
					CEREAL_NVP( ssDrawPos	),
					CEREAL_NVP( texPartPos	),
					CEREAL_NVP( texPartSize	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		struct ShowFrame
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
		struct ShowDesc
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
		struct ShowTime
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
		struct ShowRank
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
CEREAL_CLASS_VERSION( Member::ShowFrame,	0 )
CEREAL_CLASS_VERSION( Member::ShowDesc,		0 )
CEREAL_CLASS_VERSION( Member::ShowTime,		0 )
CEREAL_CLASS_VERSION( Member::ShowRank,		0 )
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

		if ( ImGui::TreeNode( u8"クリア演出のパラメータ調整" ) )
		{
			auto ShowFrame = [&]( const std::string &nodeCaption, Member::ShowFrame *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ImGui::TreePop();
			};
			auto ShowDesc = [&]( const std::string &nodeCaption, Member::ShowDesc *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ImGui::TreePop();
			};
			auto ShowTime = [&]( const std::string &nodeCaption, Member::ShowTime *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ImGui::TreePop();
			};
			auto ShowRank = [&]( const std::string &nodeCaption, Member::ShowRank *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ImGui::TreePop();
			};
			auto ShowWait = [&]( const std::string &nodeCaption, Member::Wait *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ImGui::TreePop();
			};

			ShowFrame( u8"枠",		&m.showFrame	);
			ShowDesc ( u8"説明",		&m.showDesc		);
			ShowTime ( u8"時間",		&m.showTime		);
			ShowRank ( u8"ランク",	&m.showRank		);
			ShowWait ( u8"待機",		&m.wait			);

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


#if USE_IMGUI
void ClearPerformance::LoadParameter()
{
	ParamClearPerformance::Get().Init();
}
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

void ClearPerformance::Init()
{
	AssignProcess<ShowFrame>( &processPtrs[scast<int>( Type::ShowFrame			)] );
	AssignProcess<ShowDesc>	( &processPtrs[scast<int>( Type::ShowDescription	)] );
	AssignProcess<ShowTime>	( &processPtrs[scast<int>( Type::ShowTime			)] );
	AssignProcess<ShowRank>	( &processPtrs[scast<int>( Type::ShowRank			)] );
	AssignProcess<Wait>		( &processPtrs[scast<int>( Type::Wait				)] );

	Timer zero{}; zero.Set( 0, 0, 0 );
	ResetProcess( zero );
}
void ClearPerformance::ResetProcess( const Timer &currentTime )
{
	nowType		= scast<Type>( 0 );
	timer		= 0;
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

	ImGui::DragInt( u8"内部タイマ", &timer );
	ImGui::Text( u8"状態：[%s]", GetTypeName( nowType ).c_str() );
	if ( ImGui::Button( u8"状態をリセット" ) )
	{
		ResetProcess( clearTime );
	}
	ImGui::Checkbox( u8"終了したか", &isFinished );

	ImGui::TreePop();
}
#endif // USE_IMGUI
