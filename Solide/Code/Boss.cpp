#include "Boss.h"

#include <algorithm>			// For std::max(), min()
#include <array>

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/Shader.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"		// For ZeroEqual().

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#include "Donya/Template.h"		// Use Clamp().
#endif // DEBUG_MODE

#include "Common.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"

#undef max
#undef min


namespace
{
	constexpr size_t TYPE_COUNT = scast<size_t>( BossType::BossCount );
	std::string GetBossName( BossType type )
	{
		switch ( type )
		{
		case BossType::Null:	return "Empty";
		case BossType::First:	return "First";
		default: break;
		}
		_ASSERT_EXPR( 0, L"Error : Unexpected type!" );
		return "ERROR";
	}
}


namespace BossModel
{
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Boss/";
	constexpr const char *MODEL_EXTENSION = ".bin";
	constexpr const char *MODEL_NAMES[TYPE_COUNT]
	{
		"First",
	};

	static std::vector<std::shared_ptr<BossBase::ModelResource>> modelPtrs{};

	bool LoadModels()
	{
		// Already has loaded.
		if ( !modelPtrs.empty() ) { return true; }
		// else

		modelPtrs.resize( TYPE_COUNT );

		Donya::Loader loader{};
		auto Load = [&loader]( const std::string &filePath, BossBase::ModelResource *pDest )->bool
		{
			loader.ClearData();

			bool  result  = loader.Load( filePath );
			if ( !result ) { return false; }
			// else

			const auto &source = loader.GetModelSource();
			pDest->model = Donya::Model::SkinningModel::Create( source, loader.GetFileDirectory() );
			pDest->motionHolder.AppendSource( source );

			return pDest->model.WasInitializeSucceeded();
		};

		bool result		= true;
		bool succeeded	= true;

		std::string filePath{};
		const std::string prefix = MODEL_DIRECTORY;
		for ( size_t i = 0; i < TYPE_COUNT; ++i )
		{
			filePath = prefix + MODEL_NAMES[i] + MODEL_EXTENSION;
			if ( !Donya::IsExistFile( filePath ) )
			{
				const std::string outputMsgBase{ "Error : The model file does not exist. That is : " };
				Donya::OutputDebugStr( ( outputMsgBase + "[" + filePath + "]" + "\n" ).c_str() );
				continue;
			}
			// else

			auto &pModel = modelPtrs[i];
			pModel = std::make_shared<BossBase::ModelResource>();
			result = Load( filePath, &( *pModel ) ); // std::shared_ptr<T> -> T -> T *
			if ( !result )
			{
				const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
				const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
				_ASSERT_EXPR( 0, errMsg.c_str() );

				succeeded = false;
			}
		}

		if ( !succeeded )
		{
			modelPtrs.clear();
			return false;
		}
		// else
		return true;
	}

	bool IsOutOfRange( BossType kind )
	{
		return ( scast<int>( kind ) < 0 || BossType::BossCount <= kind ) ? true : false;
	}
	std::shared_ptr<BossBase::ModelResource> GetModelPtr( BossType kind )
	{
		if ( modelPtrs.empty() )
		{
			_ASSERT_EXPR( 0, L"Error : The models were not initialized.!" );
			return nullptr;
		}
		// else
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Model subscript out of range!" );
			return nullptr;
		}
		// else
		return modelPtrs[scast<int>( kind )];
	}
}

namespace
{
	// The vector of parameters contain some value per kind of enemy.
	// "[i]" of these vectors represents a value of static_cast<enumKind>( i ). This size was guaranteed to: size() == BossModel::TYPE_COUNT

	struct DrawingParam
	{
		float				drawScale = 1.0f;
		Donya::Quaternion	drawRotation;
		Donya::Vector3		drawOffset;
		std::vector<float>	accelerations;

		Donya::Vector4		oilColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		RenderingHelper::AdjustColorConstant oilAdjustment;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( drawScale		),
				CEREAL_NVP( drawRotation	),
				CEREAL_NVP( drawOffset		),
				CEREAL_NVP( accelerations	),
				CEREAL_NVP( oilColor		),
				CEREAL_NVP( oilAdjustment	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct CollisionParam
	{
		struct PerKind
		{
			std::vector<Donya::AABB> hitBoxes;
			std::vector<Donya::AABB> hurtBoxes;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( hitBoxes	),
					CEREAL_NVP( hurtBoxes	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		std::vector<PerKind> collisions;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( collisions )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct FirstParam
	{
		struct Ready
		{
			int		preAimingFrame	= 1;
			int		aimingFrame		= 1;
			int		postAimingFrame	= 1;
			float	maxAimDegree	= 180.0f;	// Per frame.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( preAimingFrame	),
					CEREAL_NVP( aimingFrame		),
					CEREAL_NVP( postAimingFrame	),
					CEREAL_NVP( maxAimDegree	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct Rush
		{
			float	initialSpeed	= 1.0f;
			float	accel			= 1.0f;		// Per frame.
			float	maxSpeed		= 1.0f;
			float	movableRange	= 10.0f;
			std::vector<int> feintCountPerHP;	// [0:MAX - 0][1:MAX - 1]..., If a remains(e.g. if MAX is 3, but this contains only two values) are exist, please used as back() value.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( initialSpeed	),
					CEREAL_NVP( accel			),
					CEREAL_NVP( maxSpeed		),
					CEREAL_NVP( movableRange	),
					CEREAL_NVP( feintCountPerHP	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct Brake
		{
			float	normalDecel			= 1.0f;
			float	oiledDecel			= 1.0f;
			int		waitFrameAfterStop	= 1;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( normalDecel			),
					CEREAL_NVP( oiledDecel			),
					CEREAL_NVP( waitFrameAfterStop	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		Ready	ready;
		Rush	rush;
		Brake	brake;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( ready	),
				CEREAL_NVP( rush	),
				CEREAL_NVP( brake	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct Member
	{
		DrawingParam		drawer;
		CollisionParam		collider;
		std::vector<int>	initialHPs;
		FirstParam			forFirst;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( drawer		),
				CEREAL_NVP( collider	),
				CEREAL_NVP( initialHPs	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( forFirst ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				1 )
CEREAL_CLASS_VERSION( DrawingParam,			0 )
CEREAL_CLASS_VERSION( CollisionParam,		0 )
CEREAL_CLASS_VERSION( FirstParam,			0 )
CEREAL_CLASS_VERSION( FirstParam::Ready,	0 )
CEREAL_CLASS_VERSION( FirstParam::Rush,		0 )
CEREAL_CLASS_VERSION( FirstParam::Brake,	0 )

class ParamBoss : public ParameterBase<ParamBoss>
{
public:
	static constexpr const char *ID = "Boss";
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

		ResizeVectorIfNeeded();
	}
	Member Data() const { return m; }
private:
	void ResizeVectorIfNeeded()
	{
		auto ResizeIfNeeded = []( auto *pVector, auto initialValue )
		{
			if ( pVector->size() == TYPE_COUNT ) { return; };
			// else

			pVector->resize( TYPE_COUNT, initialValue );
		};

		ResizeIfNeeded( &m.drawer.accelerations, 1.0f );

		CollisionParam::PerKind defaultHitBox;
		defaultHitBox.hitBoxes.resize( 1 );
		defaultHitBox.hurtBoxes.resize( 1 );
		defaultHitBox.hitBoxes.back().size  = 1.0f;
		defaultHitBox.hurtBoxes.back().size = 1.0f;
		ResizeIfNeeded( &m.collider.collisions, defaultHitBox );
		
		ResizeIfNeeded( &m.initialHPs, 3 );
	}
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"ボスのパラメータ調整" ) )
		{
			ResizeVectorIfNeeded();

			if ( ImGui::TreeNode( u8"描画系" ) )
			{
				ImGui::DragFloat(		u8"描画スケール",		&m.drawer.drawScale,		0.01f );
				ImGui::DragFloat3(		u8"描画オフセット",	&m.drawer.drawOffset.x,		0.01f );
				ImGui::SliderFloat4(	u8"描画姿勢",		&m.drawer.drawRotation.x,	-1.0f, 1.0f );
				m.drawer.drawScale = std::max( 0.0f, m.drawer.drawScale );
				if ( ImGui::Button( u8"描画姿勢を正規化" ) ) { m.drawer.drawRotation.Normalize(); }

				if ( ImGui::TreeNode( u8"再生速度の倍率" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < TYPE_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + BossModel::MODEL_NAMES[i] + "]";
						ImGui::DragFloat( caption.c_str(), &m.drawer.accelerations[i], 0.001f );
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"状態毎の描画色" ) )
				{
					ImGui::ColorEdit4( u8"オイル時・描画色", &m.drawer.oilColor.x );
					ParameterHelper::ShowConstantNode( u8"オイル時・加算マテリアル色", &m.drawer.oilAdjustment );
					
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"当たり判定系" ) )
			{
				auto ShowPerKind = []( const std::string &nodeCaption, CollisionParam::PerKind *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					auto ShowHitBoxes = []( const std::string &nodeCaption, std::vector<Donya::AABB> *p )
					{
						if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
						// else
						
						if ( ImGui::Button( u8"追加" ) )
						{
							Donya::AABB tmp{};
							tmp.size = 1.0f;
							p->emplace_back( std::move( tmp ) );
						}
						if ( p->size() && ImGui::Button( u8"末尾を削除" ) )
						{
							p->pop_back();
						}

						const size_t count = p->size();
						size_t eraseIndex = count;
						std::string caption{};
						for ( size_t i = 0; i < count; ++i )
						{
							caption = Donya::MakeArraySuffix( i );

							if ( ImGui::Button( ( caption + u8"削除" ).c_str() ) )
							{
								eraseIndex = i;
							}

							ParameterHelper::ShowAABBNode( ( caption + u8"調整" ).c_str(), &p->at( i ) );
						}

						if ( eraseIndex != count )
						{
							p->erase( p->begin() + eraseIndex );
						}

						ImGui::TreePop();
					};

					ShowHitBoxes( u8"当たり判定", &p->hitBoxes	);
					ShowHitBoxes( u8"喰らい判定", &p->hurtBoxes	);

					ImGui::TreePop();
				};

				auto &vector = m.collider.collisions;

				std::string caption{};
				for ( size_t i = 0; i < TYPE_COUNT; ++i )
				{
					caption = "[" + std::to_string( i ) + ":" + BossModel::MODEL_NAMES[i] + "]";
				
					if ( ImGui::Button( u8"追加" ) )
					{
						CollisionParam::PerKind tmp;
						tmp.hitBoxes.resize( 1 );
						tmp.hurtBoxes.resize( 1 );
						tmp.hitBoxes.back().size = 1.0f;
						tmp.hurtBoxes.back().size = 1.0f;
						vector.emplace_back( std::move( tmp ) );
					}
					if ( vector.size() && ImGui::Button( u8"末尾を削除" ) )
					{
						vector.pop_back();
					}

					ShowPerKind( u8"調整", &vector[i] );
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"その他" ) )
			{
				if ( ImGui::TreeNode( u8"最大体力" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < TYPE_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + BossModel::MODEL_NAMES[i] + "]";
						ImGui::InputInt( caption.c_str(), &m.initialHPs[i] );
						m.initialHPs[i] = std::max( 1, m.initialHPs[i] );
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"１体目のパラメータ" ) )
			{
				auto &data = m.forFirst;
				
				auto ShowReady = [&]( const std::string &nodeCaption, FirstParam::Ready *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragInt( u8"狙い始めるまでの待ち時間（フレーム）",	&p->preAimingFrame	);
					ImGui::DragInt( u8"狙いつける時間（フレーム）",			&p->aimingFrame		);
					ImGui::DragInt( u8"狙いをつけた後の待ち時間（フレーム）",	&p->postAimingFrame	);
					Donya::Clamp( &p->preAimingFrame,	0, p->preAimingFrame	);
					Donya::Clamp( &p->aimingFrame,		0, p->aimingFrame		);
					Donya::Clamp( &p->postAimingFrame,	0, p->postAimingFrame	);
					ImGui::Text( u8"%d：合計所要時間（フレーム）", p->preAimingFrame + p->aimingFrame + p->postAimingFrame );

					ImGui::SliderFloat( u8"一度に曲がる最大角度（Degree）", &p->maxAimDegree, -180.0f, 180.0f );

					ImGui::TreePop();
				};
				auto ShowRush  = [&]( const std::string &nodeCaption, FirstParam::Rush *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragFloat( u8"初期速度",				&p->initialSpeed	);
					ImGui::DragFloat( u8"加速度",				&p->accel			);
					ImGui::DragFloat( u8"最高速度",				&p->maxSpeed		);
					ImGui::DragFloat( u8"移動可能範囲（半分）",	&p->movableRange	);
					p->initialSpeed	= std::max( 0.0f, p->initialSpeed	);
					p->accel		= std::max( 0.0f, p->accel			);
					p->maxSpeed		= std::max( 0.0f, p->maxSpeed		);
					p->movableRange	= std::max( 0.0f, p->movableRange	);

					if ( ImGui::TreeNode( u8"フェイント回数設定" ) )
					{
						if ( ImGui::TreeNode( u8"説明" ) )
						{
							ImGui::Text( u8"フェイント回数は，[最大HP - 現在HP]をインデックスに使われます。" );
							ImGui::Text( u8"また，要素数が最大HPより少ない場合は，切り詰めて末尾のものが使われます。" );
							ImGui::Text( u8"例：最大HP = 5; フェイント回数 = { 0, 2, 1, 5 };" );
							ImGui::Text( u8"現在HP: 5 -> フェイント回数: (5-5) -> [0番] -> 0" );
							ImGui::Text( u8"現在HP: 4 -> フェイント回数: (5-4) -> [1番] -> 2" );
							ImGui::Text( u8"現在HP: 3 -> フェイント回数: (5-3) -> [2番] -> 1" );
							ImGui::Text( u8"現在HP: 2 -> フェイント回数: (5-2) -> [3番] -> 5" );
							ImGui::Text( u8"現在HP: 1 -> フェイント回数: (5-1) -> [4番](範囲外) -> 5" );
							ImGui::TreePop();
						}

						auto &HPData = p->feintCountPerHP;
						ParameterHelper::ResizeByButton( &HPData, 0 );

						std::string caption{};
						const size_t count = HPData.size();
						size_t eraseIndex = count;
						for ( size_t i = 0; i < count; ++i )
						{
							caption = Donya::MakeArraySuffix( i );

							caption = caption + u8"を削除";
							if ( ImGui::Button( ( caption + u8"を削除" ).c_str() ) )
							{
								eraseIndex = i;
							}
							ImGui::SameLine();

							caption = u8"フェイントする回数##" + caption;
							ImGui::DragInt( caption.c_str(), &HPData[i] );
							HPData[i] = std::max( 0, HPData[i] );
						}

						if ( eraseIndex != count )
						{
							HPData.erase( HPData.begin() + eraseIndex );
						}

						ImGui::TreePop();
					}

					ImGui::TreePop();
				};
				auto ShowBrake = [&]( const std::string &nodeCaption, FirstParam::Brake *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					/*
					float	normalDecel			= 1.0f;
					float	oiledDecel			= 1.0f;
					int		waitFrameAfterStop	= 1;
					*/

					ImGui::DragFloat( u8"通常減速量",				&p->normalDecel			);
					ImGui::DragFloat( u8"オイル時減速量",				&p->oiledDecel			);
					ImGui::DragInt  ( u8"停止後の待ち時間",			&p->waitFrameAfterStop	);
					p->normalDecel			= std::max( 0.0001f,	p->normalDecel			);
					p->oiledDecel			= std::max( 0.0001f,	p->oiledDecel			);
					p->waitFrameAfterStop	= std::max( 1,			p->waitFrameAfterStop	);

					ImGui::TreePop();
				};

				ShowReady( u8"待機・軸あわせ",	&data.ready		);
				ShowRush( u8"突進",				&data.rush		);
				ShowBrake( u8"ブレーキ",			&data.brake		);

				ImGui::TreePop();
			}

			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};


// Internal utility.
namespace
{
	Member FetchMember()
	{
		return ParamBoss::Get().Data();
	}

	void ToWorldSpace( std::vector<Donya::AABB> *pTarget, const Donya::Vector3 &worldOrigin )
	{
		for ( auto &it : *pTarget )
		{
			it.pos += worldOrigin;
		}
	}
	Donya::Vector4x4 MakeHitBoxWorldMatrix( const Donya::Vector3 &wsPos, const Donya::AABB &localHitBox, const Donya::Quaternion &rotation = Donya::Quaternion::Identity() )
	{
		// The size is half.
		// But that will using as scale, so we should multiply to double.

		const Donya::Vector3 rotatedOffset = rotation.RotateVector( localHitBox.pos );
		
		Donya::Vector4x4 m{};
		m._11 = localHitBox.size.x * 2.0f;
		m._22 = localHitBox.size.y * 2.0f;
		m._33 = localHitBox.size.z * 2.0f;
		m._41 = rotatedOffset.x + wsPos.x;
		m._42 = rotatedOffset.y + wsPos.y;
		m._43 = rotatedOffset.z + wsPos.z;
		return m;
	}
	void DrawCube( RenderingHelper *pRenderer, const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		Donya::Model::Cube::Constant constant;
		constant.matWorld		= W;
		constant.matViewProj	= VP;
		constant.drawColor		= color;
		constant.lightDirection	= -Donya::Vector3::Up();
		pRenderer->ProcessDrawingCube( constant );
	}
}


bool				BossInitializer::ShouldGenerateBoss() const
{
	return ( type == BossType::Null || type == BossType::BossCount ) ? false : true;
}
BossType			BossInitializer::GetType() const { return type; }
Donya::Vector3		BossInitializer::GetInitialPos() const { return wsInitialPos; }
Donya::Quaternion	BossInitializer::GetInitialOrientation() const { return initialOrientation; }
void BossInitializer::LoadParameter( int stageNo )
{
#if DEBUG_MODE
	LoadJson( stageNo );
#else
	LoadBin( stageNo );
#endif // DEBUG_MODE
}
void BossInitializer::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void BossInitializer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void BossInitializer::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void BossInitializer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void BossInitializer::ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	int intType = scast<int>( type );
	ImGui::SliderInt( u8"ボスの種類", &intType, -1, TYPE_COUNT - 1 );
	type = scast<BossType>( intType );
	ImGui::Text( u8"現在の種類：%s", GetBossName( type ).c_str() );
	ImGui::Text( u8"%s だと出現しません", GetBossName( BossType::Null ).c_str() );

	ImGui::DragFloat3( u8"初期のワールド座標", &wsInitialPos.x, 0.01f );
	
	Donya::Vector3 lookDir = initialOrientation.LocalFront();
	ImGui::SliderFloat3( u8"初期の前方向", &lookDir.x, -1.0f, 1.0f );

	initialOrientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), lookDir.Unit(), Donya::Quaternion::Freeze::Up );

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		const std::string strIndex = u8"[" + std::to_string( stageNo ) + u8"]";

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード" + strIndex;
		loadStr += u8"(by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8")";

		if ( ImGui::Button( ( u8"セーブ" + strIndex ).c_str() ) )
		{
			SaveBin ( stageNo );
			SaveJson( stageNo );
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin( stageNo ) : LoadJson( stageNo );
		}

		ImGui::TreePop();
	};
	if ( allowShowIONode )
	{
		ShowIONode();
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI


bool BossBase::LoadModels()
{
	return BossModel::LoadModels();
}
bool BossBase::AssignDerivedClass( std::unique_ptr<BossBase> *pTarget, BossType assignType )
{
	switch ( assignType )
	{
	case BossType::First:	*pTarget = std::make_unique<BossFirst>(); return true;
	default: break;
	}
	_ASSERT_EXPR( 0, L"Error: Unexpected type!" );
	return false;
}
void BossBase::Init( const BossInitializer &param )
{
	ParamBoss::Get().Init();

	pos				= param.GetInitialPos();
	element			= Element::Type::Nil;
	velocity		= 0.0f;
	orientation		= param.GetInitialOrientation();

	const int intType = scast<int>( GetType() );
	const auto hpData = FetchMember().initialHPs;
	hp = ( intType < scast<int>( hpData.size() ) ) ? hpData[intType] : 1;

	model.pResource	= BossModel::GetModelPtr( GetType() );
	AssignSpecifyPose( 0 );
}
void BossBase::Update( float elapsedTime, const Donya::Vector3 &targetPos )
{
#if USE_IMGUI
	ParamBoss::Get().UseImGui();
#endif // USE_IMGUI
}
void BossBase::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMat )
{
	if ( IsDead() ) { return; }
	// else

	const auto data = FetchMember();
	
	const auto result = Actor::Move( velocity, {}, solids, pTerrain, pTerrainMat );
	const Donya::Vector3 standingNormal = result.lastNormal;
	
	// If now standing on some plane, that means corrected to vertically.
	bool wasCorrectedV = !standingNormal.IsZero();
	if ( wasCorrectedV )
	{
		velocity.y = 0.0f;
	}
}
void BossBase::Draw( RenderingHelper *pRenderer ) const
{
	if ( !model.pResource ) { return; }
	// else

	const auto drawData = FetchMember().drawer;
	Donya::Model::Constants::PerModel::Common modelConstant{};
	modelConstant.drawColor		= CalcDrawColor();
	modelConstant.worldMatrix	= CalcWorldMatrix( /* useForDrawing = */ true );
	RenderingHelper::AdjustColorConstant colorConstant =
		( element.Has( Element::Type::Oil ) )
		? drawData.oilAdjustment
		: RenderingHelper::AdjustColorConstant::MakeDefault();
	pRenderer->UpdateConstant( modelConstant );
	pRenderer->UpdateConstant( colorConstant );
	pRenderer->ActivateConstantModel();
	pRenderer->ActivateConstantAdjustColor();

	pRenderer->Render( model.pResource->model, model.pose );

	pRenderer->DeactivateConstantAdjustColor();
	pRenderer->DeactivateConstantModel();
}
void BossBase::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else
#if DEBUG_MODE

	const auto hitBoxes		= FetchOwnHitBoxes( /* wantHurtBoxes = */ false );
	const auto hurtBoxes	= FetchOwnHitBoxes( /* wantHurtBoxes = */ true  );

	constexpr Donya::Vector4	hitColor { 0.6f, 0.0f, 0.0f, 0.5f };
	constexpr Donya::Vector4	hurtColor{ 0.0f, 0.6f, 0.0f, 0.5f };

	Donya::Vector4x4 W;
	for ( const auto &it : hitBoxes )
	{
		W = MakeHitBoxWorldMatrix( pos, it, orientation );
		DrawCube( pRenderer, W, matVP, hitColor );
	}
	for ( const auto &it : hurtBoxes )
	{
		W = MakeHitBoxWorldMatrix( pos, it, orientation );
		DrawCube( pRenderer, W, matVP, hurtColor );
	}
#endif // DEBUG_MODE
}
void BossBase::MakeDamage( const Element &effect ) const
{
	element.Add( effect.Get() );
}
bool BossBase::IsDead() const
{
	return ( hp <= 0 );
}
std::vector<Donya::AABB>	BossBase::AcquireHitBoxes() const
{
	auto hitBoxes = FetchOwnHitBoxes( /* wantHurtBoxes = */ false );
	ToWorldSpace( &hitBoxes, GetPosition() );
	return hitBoxes;
}
std::vector<Donya::AABB>	BossBase::AcquireHurtBoxes() const
{
	auto hurtBoxes = FetchOwnHitBoxes( /* wantHurtBoxes = */ true );
	ToWorldSpace( &hurtBoxes, GetPosition() );
	return hurtBoxes;
}
void BossBase::AssignSpecifyPose( int motionIndex )
{
	if ( !model.pResource ) { return; }
	// else

	const int motionCount = scast<int>( model.pResource->motionHolder.GetMotionCount() );
	motionIndex = std::max( 0, std::min( motionCount - 1, motionIndex ) );

	const auto &applyMotion = model.pResource->motionHolder.GetMotion( motionIndex );
	model.animator.SetRepeatRange( applyMotion );
	model.pose.AssignSkeletal
	(
		model.animator.CalcCurrentPose
		(
			applyMotion
		)
	);
}
void BossBase::UpdateMotion( float elapsedTime, int motionIndex )
{
	if ( !model.pResource ) { return; }
	// else

	const auto data			= FetchMember();
	const auto &speedSource = data.drawer.accelerations;

	const size_t intType	= scast<size_t>( GetType() );
	const float  playSpeed	= ( intType < speedSource.size() ) ? speedSource[intType] : 1.0f;
	model.animator.Update( elapsedTime * playSpeed );

	if ( model.pResource )
	{
		AssignSpecifyPose( motionIndex );
	}
}
Donya::Vector4				BossBase::CalcDrawColor() const
{
	const auto data = FetchMember();
	Donya::Vector4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	if ( element.Has( Element::Type::Oil ) ) { baseColor.Product( data.drawer.oilColor ); }

	return baseColor;
}
Donya::Vector4x4			BossBase::CalcWorldMatrix( bool useForDrawing ) const
{
	const auto data = FetchMember();
	Donya::Vector4x4 W{};

	if ( useForDrawing )
	{
		W._11 = data.drawer.drawScale;
		W._22 = data.drawer.drawScale;
		W._33 = data.drawer.drawScale;
	}
	else
	{
		// Scales are 1.0f.
	}

	W *= orientation.MakeRotationMatrix();
	if ( useForDrawing )
	{
		W *= data.drawer.drawRotation.MakeRotationMatrix();
	}

	W._41 = pos.x;
	W._42 = pos.y;
	W._43 = pos.z;

	if ( useForDrawing )
	{
		W._41 += data.drawer.drawOffset.x;
		W._42 += data.drawer.drawOffset.y;
		W._43 += data.drawer.drawOffset.z;
	}

	return W;
}
std::vector<Donya::AABB>	BossBase::FetchOwnHitBoxes( bool wantHurtBoxes ) const
{
	const auto data			= FetchMember();
	const auto &collisions	= data.collider.collisions;
	const int  intType		= scast<int>( GetType() );

	if ( intType < 0 || scast<int>( collisions.size() ) <= intType ) { return {}; }
	// else

	const auto &perType		= data.collider.collisions[intType];
	return	( wantHurtBoxes )
			? perType.hurtBoxes
			: perType.hitBoxes;
}


#pragma region First
void BossFirst::MoverBase::Init( BossFirst *p )
{
	p->timer = 0;
}

void BossFirst::Ready::Init( BossFirst *p )
{
	MoverBase::Init( p );
}
void BossFirst::Ready::Uninit( BossFirst *p ) {}
void BossFirst::Ready::Update( BossFirst *p, float elapsedTime, const Donya::Vector3 &targetPos )
{

}
bool BossFirst::Ready::ShouldChangeMover( BossFirst *p ) const
{
	return false;
}
std::function<void()> BossFirst::Ready::GetChangeStateMethod( BossFirst *p ) const
{
	return [&]() { p->AssignMover<Rush>(); };
}
std::string BossFirst::Ready::GetStateName() const { return "Ready"; }

void BossFirst::Rush::Init( BossFirst *p )
{
	MoverBase::Init( p );
}
void BossFirst::Rush::Uninit( BossFirst *p ) {}
void BossFirst::Rush::Update( BossFirst *p, float elapsedTime, const Donya::Vector3 &targetPos )
{

}
bool BossFirst::Rush::ShouldChangeMover( BossFirst *p ) const
{
	return false;
}
std::function<void()> BossFirst::Rush::GetChangeStateMethod( BossFirst *p ) const
{
	return [&]() { p->AssignMover<Brake>(); };
}
std::string BossFirst::Rush::GetStateName() const { return "Rush"; }

void BossFirst::Brake::Init( BossFirst *p )
{
	MoverBase::Init( p );
}
void BossFirst::Brake::Uninit( BossFirst *p ) {}
void BossFirst::Brake::Update( BossFirst *p, float elapsedTime, const Donya::Vector3 &targetPos )
{

}
bool BossFirst::Brake::ShouldChangeMover( BossFirst *p ) const
{
	return false;
}
std::function<void()> BossFirst::Brake::GetChangeStateMethod( BossFirst *p ) const
{
	return [&]() { p->AssignMover<Ready>(); };
}
std::string BossFirst::Brake::GetStateName() const { return "Brake"; }

void BossFirst::Damage::Init( BossFirst *p )
{
	MoverBase::Init( p );
}
void BossFirst::Damage::Uninit( BossFirst *p ) {}
void BossFirst::Damage::Update( BossFirst *p, float elapsedTime, const Donya::Vector3 &targetPos )
{

}
bool BossFirst::Damage::ShouldChangeMover( BossFirst *p ) const
{
	return false;
}
std::function<void()> BossFirst::Damage::GetChangeStateMethod( BossFirst *p ) const
{
	return [&]() { p->AssignMover<Ready>(); };
}
std::string BossFirst::Damage::GetStateName() const { return "Damage"; }

void BossFirst::Die::Init( BossFirst *p )
{
	MoverBase::Init( p );
}
void BossFirst::Die::Uninit( BossFirst *p ) {}
void BossFirst::Die::Update( BossFirst *p, float elapsedTime, const Donya::Vector3 &targetPos )
{

}
bool BossFirst::Die::ShouldChangeMover( BossFirst *p ) const
{
	return false;
}
std::function<void()> BossFirst::Die::GetChangeStateMethod( BossFirst *p ) const
{
	return [&]() {}; // No op.
}
std::string BossFirst::Die::GetStateName() const { return "Die"; }

void BossFirst::Init( const BossInitializer &parameter )
{
	BossBase::Init( parameter );
	AssignMover<Ready>();
}
void BossFirst::Uninit()
{
	BossBase::Uninit();

	if ( pMover ) { pMover->Uninit( this ); }
	pMover.reset();
}
void BossFirst::Update( float elapsedTime, const Donya::Vector3 &targetPos )
{
	BossBase::Update( elapsedTime, targetPos );

	UpdateMotion( elapsedTime, 0 );

	if ( IsDead() ) { return; }
	// else

	UpdateByMover( elapsedTime, targetPos );
}
void BossFirst::UpdateByMover( float elapsedTime, const Donya::Vector3 &targetPos )
{
	if ( !pMover ) { return; }
	// else

	pMover->Update( this, elapsedTime, targetPos );
	if ( pMover->ShouldChangeMover( this ) )
	{
		auto ChangeState = pMover->GetChangeStateMethod( this );
		ChangeState();
	}
}
BossType BossFirst::GetType() const { return BossType::First; }
#if USE_IMGUI
void BossFirst::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::Text( u8"種類：%s", GetBossName( GetType() ).c_str() );

	ImGui::DragInt( u8"現在の体力", &hp ); hp = std::max( 0, hp );
	if ( pMover ) { ImGui::Text( u8"現在の状態：%s", pMover->GetStateName().c_str() ); }
	ImGui::DragInt( u8"内部タイマ", &timer );
	ImGui::DragFloat3( u8"現在の座標", &pos.x,		0.01f );
	ImGui::DragFloat3( u8"現在の速度", &velocity.x,	0.01f );

	Donya::Vector3 localFront = orientation.LocalFront();
	ImGui::SliderFloat3( u8"現在の前方向", &localFront.x,	 -1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront, Donya::Quaternion::Freeze::Up );

	element.ShowImGuiNode( /* useTreeNode = */ false, "" );

	ImGui::TreePop();
}
#endif // USE_IMGUI
// region First
#pragma endregion
