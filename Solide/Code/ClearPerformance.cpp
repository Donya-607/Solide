#include "ClearPerformance.h"

#include "Donya/Easing.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"		// Use Lerp

#include "FilePath.h"
#include "Parameter.h"


namespace
{
#if USE_IMGUI
	bool wantPauseUpdate = false;
#endif // USE_IMGUI
}
namespace
{
	std::string GetTypeName( ClearPerformance::Type type )
	{
		switch ( type )
		{
		case ClearPerformance::Type::Hidden:			return u8"隠蔽";
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
			struct Param
			{
				float			drawAlpha	= 1.0f;
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
			Item	itemTime;
			Item	itemRank;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive( CEREAL_NVP( wholeFrame ) );

				if ( 1 <= version )
				{
					archive
					(
						CEREAL_NVP( itemTime ),
						CEREAL_NVP( itemRank )
					);
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

		if ( ImGui::TreeNode( u8"クリア演出のパラメータ調整" ) )
		{
			ImGui::Checkbox( u8"演出の更新を止める", &wantPauseUpdate );

			auto ShowTexPart= [&]( Donya::Vector2 *pPos, Donya::Vector2 *pSize )
			{
				ImGui::DragFloat2( u8"テクスチャ原点（左上）",	&pPos->x  );
				ImGui::DragFloat2( u8"テクスチャサイズ（全体）",	&pSize->x );
			};
			auto ShowItem	= [&]( const std::string &nodeCaption, Member::Item *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				auto ShowParam = [&]( const std::string &nodeCaption, Member::Item::Param *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragFloat ( u8"描画アルファ",			&p->drawAlpha,  0.01f );
					ImGui::DragFloat ( u8"描画スケール",			&p->drawScale,  0.01f );
					ImGui::DragFloat ( u8"描画角度（Degree）",	&p->drawDegree  );
					ImGui::DragFloat2( u8"描画位置",				&p->ssDrawPos.x );

					p->drawAlpha = std::max( 0.0f, std::min( 1.0f, p->drawAlpha ) );
					p->drawScale = std::max( 0.0f, p->drawScale );

					ImGui::TreePop();
				};

				ParameterHelper::ShowEaseParam( u8"イージングタイプ", &p->easeKind, &p->easeType );
				ImGui::DragFloat( u8"イージングにかける時間（秒）", &p->easeTakeSecond, 0.1f );
				p->easeTakeSecond = std::max( 0.0001f, p->easeTakeSecond );

				ShowParam( u8"始点", &p->paramStart );
				ShowParam( u8"終点", &p->paramDest  );

				ShowTexPart( &p->texPartPos, &p->texPartSize );

				ImGui::TreePop();
			};

			auto ShowFrame	= [&]( const std::string &nodeCaption, Member::ShowFrame *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"描画設定", &p->item );

				ImGui::TreePop();
			};
			auto ShowDesc	= [&]( const std::string &nodeCaption, Member::ShowDesc *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"描画設定・時間",		&p->itemTime );
				ShowItem( u8"描画設定・ランク",	&p->itemRank );

				ImGui::TreePop();
			};
			auto ShowTime	= [&]( const std::string &nodeCaption, Member::ShowTime *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"描画設定", &p->item );

				// Disable unused parameters.
				p->item.paramStart.drawDegree = 0.0f;
				p->item.paramDest.drawDegree  = 0.0f;
				p->item.texPartPos  = 0.0f;
				p->item.texPartSize = 0.0f;

				ImGui::TreePop();
			};
			auto ShowRank	= [&]( const std::string &nodeCaption, Member::ShowRank *p )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else

				ImGui::DragInt( u8"全体時間（フレーム）", &p->wholeFrame );
				p->wholeFrame = std::max( 0, p->wholeFrame );

				ShowItem( u8"描画設定", &p->item );

				// Disable unused parameters.
				p->item.texPartPos  = 0.0f;
				p->item.texPartSize = 0.0f;

				ImGui::TreePop();
			};
			auto ShowWait	= [&]( const std::string &nodeCaption, Member::Wait *p )
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

	Member::Item::Param Lerp( const Member::Item &source, float time )
	{
		const auto	&start	= source.paramStart;
		const auto	&dest	= source.paramDest;
		
		Member::Item::Param param{};
		param.drawAlpha		= Donya::Lerp( start.drawAlpha,  dest.drawAlpha,  time );
		param.drawScale		= Donya::Lerp( start.drawScale,  dest.drawScale,  time );
		param.drawDegree	= Donya::Lerp( start.drawDegree, dest.drawDegree, time );
		param.ssDrawPos		= Donya::Lerp( start.ssDrawPos,  dest.ssDrawPos,  time );
		return param;
	}
	void AssignLerpedItem( UIObject *pDest, const Member::Item &source, float lerpFactor )
	{
		const float	time	= Donya::Easing::Ease( source.easeKind, source.easeType, lerpFactor );
		const auto	param	= Lerp( source, time );

		pDest->alpha		= param.drawAlpha;
		pDest->drawScale	= param.drawScale;
		pDest->degree		= param.drawDegree;
		pDest->pos			= param.ssDrawPos;
		pDest->texPos		= source.texPartPos;
		pDest->texSize		= source.texPartSize;
	}

	constexpr float depthFrame	= 0.1f;
	constexpr float depthDesc	= 0.08f;
	constexpr float depthTime	= 0.04f;
	constexpr float depthRank	= 0.04f;
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
void ClearPerformance::ProcessBase::UpdateEaseFactor( float wholeEaseSecond )
{
	wholeEaseSecond = std::max( 0.0001f, wholeEaseSecond );

	const float elapseTime = 1.0f / ( 60.0f * wholeEaseSecond );
	factor += elapseTime;
	factor = std::min( 1.0f, factor );
}

ClearPerformance::Result ClearPerformance::ShowFrame::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showFrame;

	UpdateEaseFactor( data.item.easeTakeSecond );

	timer++;
	inst.timer = timer; // For visualize
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
	AssignDrawData( inst );
	inst.sprFrame.DrawPart( depthFrame );
}
void ClearPerformance::ShowFrame::AssignDrawData( ClearPerformance &inst )
{
	const auto data = FetchMember().showFrame;
	AssignLerpedItem( &inst.sprFrame, data.item, factor );
}

ClearPerformance::Result ClearPerformance::ShowDesc::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showDesc;

	UpdateEaseFactor( data.itemTime.easeTakeSecond );

	timer++;
	inst.timer = timer; // For visualize
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
	AssignDrawData( inst );

	const auto before = inst.sprDesc;

	inst.sprDesc = paramTime;
	inst.sprDesc.DrawPart( depthDesc );

	inst.sprDesc = paramRank;
	inst.sprDesc.DrawPart( depthDesc );

	inst.sprDesc = before;
}
void ClearPerformance::ShowDesc::AssignDrawData( ClearPerformance &inst )
{
	const auto data = FetchMember().showDesc;

	AssignLerpedItem( &inst.sprDesc, data.itemTime, factor );
	paramTime = inst.sprDesc;

	AssignLerpedItem( &inst.sprDesc, data.itemRank, factor );
	paramRank = inst.sprDesc;
}

ClearPerformance::Result ClearPerformance::ShowTime::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showTime;

	UpdateEaseFactor( data.item.easeTakeSecond );
	
	timer++;
	inst.timer = timer; // For visualize
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
	AssignDrawData( inst );

	inst.numberDrawer.DrawTime
	(
		inst.clearTime,
		parameter.pos,
		parameter.drawScale.x, // x == y.
		parameter.alpha,
		Donya::Vector2{ 0.5f, 0.5f },
		depthTime
	);
}
void ClearPerformance::ShowTime::AssignDrawData( ClearPerformance &inst )
{
	const auto data = FetchMember().showTime;
	AssignLerpedItem( &parameter, data.item, factor );
}

ClearPerformance::Result ClearPerformance::ShowRank::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().showRank;

	UpdateEaseFactor( data.item.easeTakeSecond );

	timer++;
	inst.timer = timer; // For visualize
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
	AssignDrawData( inst );

	inst.rankDrawer.Draw
	(
		inst.clearRank,
		parameter.pos,
		parameter.drawScale.x, // x == y.
		parameter.degree,
		parameter.alpha,
		Donya::Vector2{ 0.5f, 0.5f },
		depthRank
	);
}
void ClearPerformance::ShowRank::AssignDrawData( ClearPerformance &inst )
{
	const auto data = FetchMember().showRank;
	AssignLerpedItem( &parameter, data.item, factor );
}

ClearPerformance::Result ClearPerformance::Wait::Update( ClearPerformance &inst )
{
	const auto data = FetchMember().wait;

	timer++;
	inst.timer = timer; // For visualize
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
	// No op.
}
void ClearPerformance::Wait::AssignDrawData( ClearPerformance &inst )
{
	// No op.
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
	nowType		= Type::Hidden;
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
#if USE_IMGUI
	if ( wantPauseUpdate ) { return; }
#endif // USE_IMGUI

	if ( IsHidden() ) { return; }
	// else

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
	if ( IsHidden() ) { return; }
	// else

	const size_t drawLimit = std::min( scast<size_t>( nowType ), processPtrs.size() - 1 );
	for ( size_t i = 0; i <= drawLimit; ++i )
	{
		if ( processPtrs[i] )
		{
			processPtrs[i]->Draw( *this );
		}
	}
}
void ClearPerformance::Appear()
{
	nowType = scast<Type>( 0 ); // Set to first process
}
bool ClearPerformance::IsHidden() const
{
	return ( nowType == Type::Hidden );
}
#if USE_IMGUI
void ClearPerformance::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragInt( u8"内部タイマ", &timer );
	ImGui::Text( u8"状態：[%s]", GetTypeName( nowType ).c_str() );
	if ( ImGui::Button( u8"演出を再スタート" ) )
	{
		ResetProcess( clearTime, clearRank );
		Appear();
	}
	ImGui::Checkbox( u8"終了したか", &isFinished );

	ImGui::TreePop();
}
#endif // USE_IMGUI
