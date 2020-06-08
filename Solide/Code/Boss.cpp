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

	std::string ToString( BossFirst::ActionType type )
	{
		switch ( type )
		{
		case BossFirst::ActionType::Rush:		return u8"突進";
		case BossFirst::ActionType::Breath:		return u8"ブレス";
		case BossFirst::ActionType::Wait_Long:	return u8"待機・長";
		case BossFirst::ActionType::Wait_Short:	return u8"待機・短";
		case BossFirst::ActionType::Walk_Long:	return u8"歩行・長";
		case BossFirst::ActionType::Walk_Short:	return u8"歩行・短";
		default: break;
		}
		_ASSERT_EXPR( 0, L"Error: Unexpected type!" );
		return "ERROR_ACTION";
	}
#if USE_IMGUI
	void ShowActionGuiNode( const std::string &caption, BossFirst::ActionType *p )
	{
		ImGui::Text( ( caption + u8"：%-20s" ).c_str(), ToString( *p ).c_str() );
		ImGui::SameLine();

		constexpr int count = scast<int>( BossFirst::ActionType::ActionCount );

		int intType = scast<int>( *p );
		ImGui::SliderInt( ( "##" + caption ).c_str(), &intType, 0, count - 1 );
		*p = scast<BossFirst::ActionType>( intType );
	}

	bool wantPauseUpdates = false;
#endif // USE_IMGUI
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
		struct Breath
		{
			struct PerHP
			{
				int		preFireFrame	= 1;
				int		fireFrame		= 1;
				int		fireInterval	= 5;
				int		postFireFrame	= 1;
				float	maxAimDegree	= 180.0f;	// Per frame.
				Bullet::BulletAdmin::FireDesc fireDesc;
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( preFireFrame	),
						CEREAL_NVP( fireFrame		),
						CEREAL_NVP( fireInterval	),
						CEREAL_NVP( postFireFrame	),
						CEREAL_NVP( maxAimDegree	),
						CEREAL_NVP( fireDesc		)
					);

					if ( 1 <= version )
					{
						// archive( CEREAL_NVP( x ) );
					}
				}
			};
			
			std::vector<PerHP> paramPerHP;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				// OLD version
				// archive
				// (
				// 	CEREAL_NVP( aimingFrame		),
				// 	CEREAL_NVP( postAimingFrame	),
				// 	CEREAL_NVP( afterWaitFrame	),
				// 	CEREAL_NVP( maxAimDegree	),
				// 	CEREAL_NVP( fireDesc		)
				// );

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( paramPerHP ) );
				}
			}
		};
		struct Wait
		{
			int		longFrame		= 1;
			int		shortFrame		= 1;
			float	maxAimDegree	= 180.0f;	// Per frame.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( longFrame		),
					CEREAL_NVP( shortFrame		),
					CEREAL_NVP( maxAimDegree	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct Walk
		{
			int		longFrame		= 1;
			int		shortFrame		= 1;
			float	maxAimDegree	= 180.0f;	// Per frame.
			float	walkSpeed		= 1.0f;		// Per frame.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( longFrame		),
					CEREAL_NVP( shortFrame		),
					CEREAL_NVP( maxAimDegree	),
					CEREAL_NVP( walkSpeed		)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct Damage
		{
			int reactFrame		= 1;
			int reactFrameFinal	= 1; // Use for critical damage(hp will be 0) frame.
			int flushInterval	= 1;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( reactFrame		),
					CEREAL_NVP( reactFrameFinal	),
					CEREAL_NVP( flushInterval	)
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
		Breath	breath;

		std::vector<std::vector<BossFirst::ActionType>> actionPatterns; // Per HPs. The index is calced by: maxHP - nowHP(e.g. nowHP == maxHP -> 0, nowHP = maxHP-1 -> 1, ...)
		BossFirst::ActionType initialAction = BossFirst::ActionType::Wait_Long;

		Wait	wait;
		Walk	walk;
		Damage	damage;

		int		releaseOilFrame = 1;
		Ready	readyInFeint;
		Brake	brakeInFeint;

		std::vector<float> motionSpeeds; // size() == MotionKind::MotionCount
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
				archive
				(
					CEREAL_NVP( breath			),
					CEREAL_NVP( actionPatterns	)
				);
			}
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( initialAction	),
					CEREAL_NVP( wait			),
					CEREAL_NVP( walk			)
				);
			}
			if ( 3 <= version )
			{
				archive
				(
					CEREAL_NVP( damage			),
					CEREAL_NVP( releaseOilFrame	)
				);
			}
			// if ( 4 <= version ) // It's lost
			if ( 5 <= version )
			{
				archive
				(
					CEREAL_NVP( readyInFeint ),
					CEREAL_NVP( brakeInFeint )
				);
			}
			if ( 6 <= version )
			{
				archive( CEREAL_NVP( motionSpeeds ) );
			}
			if ( 7 <= version )
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
	public:
		/// <summary>
		/// Returns zero if the type outs of range.
		/// </summary>
		int FetchInitialHP( BossType type ) const
		{
			const int intType = scast<int>( type );
			if ( intType < 0 || scast<int>( BossType::BossCount ) <= intType )
			{
				_ASSERT_EXPR( 0, L"Error: Out of range!" );
				return 0;
			}
			// else
			return ( scast<int>( initialHPs.size() ) <= intType ) ? 0 : initialHPs[intType];
		}
	};
}
CEREAL_CLASS_VERSION( Member,					1 )
CEREAL_CLASS_VERSION( DrawingParam,				0 )
CEREAL_CLASS_VERSION( CollisionParam,			0 )
CEREAL_CLASS_VERSION( FirstParam,				6 )
CEREAL_CLASS_VERSION( FirstParam::Ready,		0 )
CEREAL_CLASS_VERSION( FirstParam::Rush,			0 )
CEREAL_CLASS_VERSION( FirstParam::Brake,		0 )
CEREAL_CLASS_VERSION( FirstParam::Breath,		1 )
CEREAL_CLASS_VERSION( FirstParam::Breath::PerHP,0 )
CEREAL_CLASS_VERSION( FirstParam::Wait,			0 )
CEREAL_CLASS_VERSION( FirstParam::Walk,			0 )
CEREAL_CLASS_VERSION( FirstParam::Damage,		0 )

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
		auto ResizeIfNeeded = []( auto *pVector, auto initialValue, size_t size = TYPE_COUNT )
		{
			if ( pVector->size() == size ) { return; };
			// else

			pVector->resize( size, initialValue );
		};

		ResizeIfNeeded( &m.drawer.accelerations, 1.0f );

		CollisionParam::PerKind defaultHitBox;
		defaultHitBox.hitBoxes.resize( 1 );
		defaultHitBox.hurtBoxes.resize( 1 );
		defaultHitBox.hitBoxes.back().size  = 1.0f;
		defaultHitBox.hurtBoxes.back().size = 1.0f;
		ResizeIfNeeded( &m.collider.collisions, defaultHitBox );
		
		ResizeIfNeeded( &m.initialHPs, 3 );

		const int firstSize = m.FetchInitialHP( BossType::First );
		ResizeIfNeeded
		(
			&m.forFirst.actionPatterns, std::vector<BossFirst::ActionType>(),
			( firstSize < 0 ) ? 0U : scast<size_t>( firstSize )
		);
		ResizeIfNeeded
		(
			&m.forFirst.breath.paramPerHP, FirstParam::Breath::PerHP{},
			( firstSize < 0 ) ? 0U : scast<size_t>( firstSize )
		);
		ResizeIfNeeded
		(
			&m.forFirst.motionSpeeds, 1.0f,
			scast<size_t>( BossFirst::MotionKind::MotionCount )
		);
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

			ImGui::Checkbox( u8"ボスの更新を止める", &wantPauseUpdates );

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

				ImGui::DragInt( u8"オイルが剥がれる時間（フレーム）", &data.releaseOilFrame );
				Donya::Clamp( &data.releaseOilFrame, 0, data.releaseOilFrame );
				
				auto ShowReady	= [&]( const std::string &nodeCaption, FirstParam::Ready *p )
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
				auto ShowRush	= [&]( const std::string &nodeCaption, FirstParam::Rush *p )
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
							if ( ImGui::Button( caption.c_str() ) )
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
				auto ShowBrake	= [&]( const std::string &nodeCaption, FirstParam::Brake *p )
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
				auto ShowBreath	= [&]( const std::string &nodeCaption, FirstParam::Breath *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					auto &data = p->paramPerHP;

					ParameterHelper::ResizeByButton( &data );

					auto ShowPerHP = [&]( const std::string &caption, FirstParam::Breath::PerHP *p )
					{
						if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
						// else

						ImGui::DragInt( u8"発射前の時間（フレーム）",	&p->preFireFrame	);
						ImGui::DragInt( u8"発射時間（フレーム）",		&p->fireFrame		);
						ImGui::DragInt( u8"発射間隔（フレーム）",		&p->fireInterval	);
						ImGui::DragInt( u8"発射後の時間（フレーム）",	&p->postFireFrame	);
						Donya::Clamp( &p->preFireFrame,		0, p->preFireFrame	);
						Donya::Clamp( &p->fireFrame,		0, p->fireFrame		);
						Donya::Clamp( &p->fireInterval,		0, p->fireInterval	);
						Donya::Clamp( &p->postFireFrame,	0, p->postFireFrame	);
						ImGui::Text( u8"%d：合計所要時間（フレーム）", p->preFireFrame + p->fireFrame + p->postFireFrame );
						ImGui::Text( u8"この間，常に自機を向き続けます" );

						ImGui::SliderFloat( u8"一度に曲がる最大角度（Degree）", &p->maxAimDegree, -180.0f, 180.0f );

						p->fireDesc.ShowImGuiNode( u8"発射弾の設定" );
						ImGui::Text( u8"発射方向は無視されます。" );
						ImGui::Text( u8"発射弾の属性は[Flame]になります。" );

						ImGui::TreePop();
					};

					std::string  caption{};
					const size_t hpCount = data.size();
					for ( size_t i = 0; i < hpCount; ++i )
					{
						caption = u8"[残りＨＰ：" + std::to_string( hpCount - i ) + u8"]";
						ShowPerHP( caption, &data[i] );
					}

					ImGui::TreePop();
				};
				auto ShowWait	= [&]( const std::string &nodeCaption, FirstParam::Wait *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragInt( u8"待機時間・長（フレーム）", &p->longFrame  );
					ImGui::DragInt( u8"待機時間・短（フレーム）", &p->shortFrame );
					Donya::Clamp( &p->longFrame,  0, p->longFrame  );
					Donya::Clamp( &p->shortFrame, 0, p->shortFrame );
					
					ImGui::SliderFloat( u8"一度に曲がる最大角度（Degree）", &p->maxAimDegree, -180.0f, 180.0f );

					ImGui::TreePop();
				};
				auto ShowWalk	= [&]( const std::string &nodeCaption, FirstParam::Walk *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragInt( u8"歩行時間・長（フレーム）", &p->longFrame  );
					ImGui::DragInt( u8"歩行時間・短（フレーム）", &p->shortFrame );
					Donya::Clamp( &p->longFrame,  0, p->longFrame  );
					Donya::Clamp( &p->shortFrame, 0, p->shortFrame );
					
					ImGui::SliderFloat( u8"一度に曲がる最大角度（Degree）", &p->maxAimDegree, -180.0f, 180.0f );
					ImGui::DragFloat( u8"歩行速度", &p->walkSpeed, 0.01f );
					Donya::Clamp( &p->walkSpeed, 0.0f, p->walkSpeed );

					ImGui::TreePop();
				};
				auto ShowDamage	= [&]( const std::string &nodeCaption, FirstParam::Damage *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragInt( u8"リアクション時間（フレーム）",			&p->reactFrame		);
					ImGui::DragInt( u8"トドメのリアクション時間（フレーム）",	&p->reactFrameFinal	);
					ImGui::DragInt( u8"点滅間隔（フレーム）",					&p->flushInterval	);
					ImGui::Text( u8"点滅間隔＝０で点滅無効" );
					Donya::Clamp( &p->reactFrame,		0, p->reactFrame		);
					Donya::Clamp( &p->reactFrameFinal,	0, p->reactFrameFinal	);
					Donya::Clamp( &p->flushInterval,	0, p->flushInterval		);

					ImGui::TreePop();
				};

				ShowReady	( u8"待機・軸あわせ",	&data.ready		);
				ShowReady	( u8"待機・軸あわせ（フェイント）",	&data.readyInFeint );
				ShowRush	( u8"突進",			&data.rush		);
				ShowBrake	( u8"ブレーキ",		&data.brake		);
				ShowBrake	( u8"ブレーキ（フェイント）",			&data.brakeInFeint );
				ShowBreath	( u8"ブレス",		&data.breath	);
				ShowWait	( u8"待機",			&data.wait		);
				ShowWalk	( u8"歩行",			&data.walk		);
				ShowDamage	( u8"ダメージ",		&data.damage	);

				if ( ImGui::TreeNode( u8"行動パターン設定" ) )
				{
					ShowActionGuiNode( u8"初期行動", &data.initialAction );

					std::string caption{};
					const int hpCount = m.FetchInitialHP( BossType::First ); // == data.actionPatterns.size()
					for ( int i = 0; i < hpCount; ++i )
					{
						caption = u8"[残ＨＰ:" + std::to_string( hpCount - i ) + u8"]";
						if ( !ImGui::TreeNode( caption.c_str() ) ) { continue; }
						// else

						auto &patterns = data.actionPatterns[i];
						ParameterHelper::ResizeByButton( &patterns, BossFirst::ActionType::Rush );

						const size_t patternCount = patterns.size();
						for ( size_t j = 0; j < patternCount; ++j )
						{
							caption = Donya::MakeArraySuffix( j );
							ShowActionGuiNode( caption, &patterns[j] );
						}

						auto IsContainBreath = [&]( const std::vector<BossFirst::ActionType> &pattern )
						{
							for ( const auto &it : pattern )
							{
								if ( it == BossFirst::ActionType::Breath )
								{
									return true;
								}
							}
							return false;
						};
						if ( !IsContainBreath( patterns ) )
						{
							ImGui::TextColored
							(
								ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f },
								u8"！ブレスがありません！"
							 );
						}

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"モーション再生速度" ) )
				{
					constexpr size_t motionCount = scast<size_t>( BossFirst::MotionKind::MotionCount );
					auto &speeds = data.motionSpeeds;
					if ( speeds.size() != motionCount ) { speeds.resize( motionCount, 1.0f ); }

					std::string caption;
					for ( size_t i = 0; i < motionCount; ++i )
					{
						caption =  Donya::MakeArraySuffix( i );
						caption += BossFirst::GetMotionName( scast<BossFirst::MotionKind>( i ) );
						ImGui::DragFloat( caption.c_str(), &speeds[i], 0.01f );
					}

					ImGui::TreePop();
				}

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

	if ( wantPauseUpdates ) { return; }
#endif // USE_IMGUI
}
void BossBase::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMat )
{
#if USE_IMGUI
	if ( wantPauseUpdates ) { return; }
#endif // USE_IMGUI

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
void BossBase::MakeDamage( const Element &effect, const Donya::Vector3 &othersVelocity ) const
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
std::string BossFirst::GetMotionName( MotionKind kind )
{
	switch ( kind )
	{
	case BossFirst::MotionKind::Wait:			return "Wait";
	case BossFirst::MotionKind::Walk:			return "Walk";
	case BossFirst::MotionKind::RushReady:		return "Rush-Ready";
	case BossFirst::MotionKind::RushProcess:	return "Rush-Process";
	case BossFirst::MotionKind::RushBrake:		return "Rush-Brake";
	case BossFirst::MotionKind::BreathReady:	return "Breath-Ready";
	case BossFirst::MotionKind::BreathProcess:	return "Breath-Process";
	case BossFirst::MotionKind::Damage:			return "Damage";
	case BossFirst::MotionKind::Die:			return "Die";
	default: break;
	}
	_ASSERT_EXPR( 0, L"Error : Unexpected kind!" );
	return "ERROR";
}
void BossFirst::MotionManager::Init( BossFirst &inst )
{
	inst.model.animator.ResetTimer();
}
void BossFirst::MotionManager::Update( BossFirst &inst, float elapsedTime )
{
	const auto data = FetchMember().forFirst;
	const auto &motionSpeeds = data.motionSpeeds;

	const int intKind = scast<int>( currentKind );
	if ( scast<int>( motionSpeeds.size() ) <= intKind )
	{
		_ASSERT_EXPR( 0, L"Error: Motion index out of range!" );
		return;
	}
	// else

	const float acceleration = motionSpeeds[intKind];
	inst.model.animator.Update( elapsedTime * acceleration );
	ApplyMotion( inst, currentKind );
}
void BossFirst::MotionManager::ChangeMotion( MotionKind kind )
{
	if ( kind == MotionKind::MotionCount ) { return; }
	// else
	currentKind = kind;
}
BossFirst::MotionKind BossFirst::MotionManager::GetCurrentKind() const
{
	return currentKind;
}
void BossFirst::MotionManager::ApplyMotion( BossFirst &inst, MotionKind kind )
{
	if ( !inst.model.pResource ) { return; }
	// else

	const int intKind		= scast<int>( kind );
	const int motionCount	= scast<int>( inst.model.pResource->motionHolder.GetMotionCount() );
	if ( motionCount <= intKind )
	{
		_ASSERT_EXPR( 0, L"Error: Specified motion is out of range!" );
		return;
	}
	// else

	currentKind = kind;

	inst.AssignSpecifyPose( intKind );

	ApplyLoopFlag( inst, kind );
}
void BossFirst::MotionManager::ApplyLoopFlag( BossFirst &inst, MotionKind kind )
{
	auto Apply = [&]( bool enableLoop )
	{
		( enableLoop )
		? inst.model.animator.EnableLoop()
		: inst.model.animator.DisableLoop();
	};

	switch ( kind )
	{
	case BossFirst::MotionKind::Wait:			Apply( true  );	return;
	case BossFirst::MotionKind::Walk:			Apply( true  );	return;
	case BossFirst::MotionKind::RushReady:		Apply( true  );	return;
	case BossFirst::MotionKind::RushProcess:	Apply( true  );	return;
	case BossFirst::MotionKind::RushBrake:		Apply( true  );	return;
	case BossFirst::MotionKind::BreathReady:	Apply( false );	return;
	case BossFirst::MotionKind::BreathProcess:	Apply( true  );	return;
	case BossFirst::MotionKind::Damage:			Apply( true  );	return;
	case BossFirst::MotionKind::Die:			Apply( true  );	return;
	default: _ASSERT_EXPR( 0, L"Error: Unexpected kind!" );		return;
	}
}

void BossFirst::MoverBase::Init( BossFirst &inst )
{
	inst.timer = 0;
}
void BossFirst::MoverBase::PhysicUpdate( BossFirst &inst, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix )
{
	inst.BossBase::PhysicUpdate( solids, pTerrain, pTerrainWorldMatrix );
}
bool BossFirst::MoverBase::IsDead( const BossFirst &inst ) const
{
	return false;
}
bool BossFirst::MoverBase::AcceptDamage( const BossFirst &inst ) const { return true; }
bool BossFirst::MoverBase::AcceptDraw( const BossFirst &inst ) const { return true; }

void BossFirst::Ready::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	inst.velocity.x = 0.0f;
	inst.velocity.z = 0.0f;

	gotoNext = false;

	inst.motionManager.ChangeMotion( MotionKind::RushReady );
}
void BossFirst::Ready::Uninit( BossFirst &inst ) {}
void BossFirst::Ready::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	const auto data		= ( 0 < inst.remainFeintCount )
						? FetchMember().forFirst.readyInFeint
						: FetchMember().forFirst.ready;
	const int preFrame	= data.preAimingFrame;
	const int midFrame	= preFrame + data.aimingFrame;
	const int postFrame	= midFrame + data.postAimingFrame;

	inst.timer++;

	if ( preFrame <= inst.timer && inst.timer < postFrame )
	{
		const Donya::Vector3 aimingVector = inst.CalcAimingVector( targetPos, data.maxAimDegree );
		inst.orientation	= Donya::Quaternion::LookAt( Donya::Vector3::Front(), aimingVector.Unit(), Donya::Quaternion::Freeze::Up );
		inst.aimingPos		= targetPos;
	}
	else
	if ( postFrame < inst.timer )
	{
		gotoNext = true;
	}
}
bool BossFirst::Ready::ShouldChangeMover( BossFirst &inst ) const
{
	return gotoNext;
}
std::function<void()> BossFirst::Ready::GetChangeStateMethod( BossFirst &inst ) const
{
	return [&]() { inst.AssignMover<Rush>(); };
}
std::string BossFirst::Ready::GetStateName() const { return "Ready"; }

void BossFirst::Rush::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	const Donya::Vector3 initialVelocity = inst.orientation.LocalFront() * FetchMember().forFirst.rush.initialSpeed;
	inst.velocity.x = initialVelocity.x;
	inst.velocity.z = initialVelocity.z;

	constexpr int dontUseFeintSign = 0;
	if ( inst.remainFeintCount == dontUseFeintSign )
	{
		// Set the count if first time.
		const auto data		= FetchMember();
		const int  maxHP	= data.FetchInitialHP( inst.GetType() );
		const int  index	= maxHP - inst.hp;
		const auto &source	= data.forFirst.rush.feintCountPerHP;
		if ( index < 0 || scast<int>( source.size() ) <= index )
		{
			inst.remainFeintCount = dontUseFeintSign;
		}
		else
		{
			inst.remainFeintCount =	source[index];
		}
	}
	else
	{
		inst.remainFeintCount--;
	}

	shouldStop = false;

	inst.motionManager.ChangeMotion( MotionKind::RushProcess );
}
void BossFirst::Rush::Uninit( BossFirst &inst ) {}
void BossFirst::Rush::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	if ( shouldStop ) { return; }
	// else

	const auto data = FetchMember().forFirst.rush;

	float currentSpeed = inst.velocity.XZ().Length();
	currentSpeed += data.accel;
	currentSpeed = std::min( data.maxSpeed, currentSpeed );

	const Donya::Vector3 updatedVelocity = inst.orientation.LocalFront() * currentSpeed;
	inst.velocity.x = updatedVelocity.x;
	inst.velocity.z = updatedVelocity.z;

	if ( inst.element.Has( Element::Type::Oil ) )
	{
		inst.element.Subtract( Element::Type::Oil );
	}
}
void BossFirst::Rush::PhysicUpdate( BossFirst &inst, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix )
{
	MoverBase::PhysicUpdate( inst, solids, pTerrain, pTerrainWorldMatrix );

	const Donya::Vector2 movedPos = inst.GetPosition().XZ();
	const float movableRange = FetchMember().forFirst.rush.movableRange;
	auto OnOutSide = []( const Donya::Vector2 &xzPos, float movableRange )
	{
		return ( movableRange < fabsf( xzPos.x ) || movableRange < fabsf( xzPos.y ) );
	};
	if ( OnOutSide( movedPos, movableRange ) )
	{
		shouldStop = true;

		constexpr float ERROR_MARGIN = 0.99f;
		const float insideRange = movableRange * ERROR_MARGIN;
		Donya::Clamp( &inst.pos.x, -insideRange, insideRange );
		Donya::Clamp( &inst.pos.z, -insideRange, insideRange );
	}

	auto IsCloseToTarget = [&]()
	{
		const float closeRange = std::max( 0.1f, inst.velocity.XZ().Length() );
		const Donya::Vector2 diffXZ = inst.aimingPos.XZ() - inst.GetPosition().XZ();

		return ( diffXZ.Length() <= closeRange );
	};
	if ( 0 < inst.remainFeintCount && IsCloseToTarget() )
	{
		shouldStop = true;
		inst.pos.x = inst.aimingPos.x;
		inst.pos.z = inst.aimingPos.z;
	}
}
bool BossFirst::Rush::ShouldChangeMover( BossFirst &inst ) const
{
	return shouldStop;
}
std::function<void()> BossFirst::Rush::GetChangeStateMethod( BossFirst &inst ) const
{
	return [&]() { inst.AssignMover<Brake>(); };
}
std::string BossFirst::Rush::GetStateName() const { return "Rush"; }

void BossFirst::Brake::Init( BossFirst &inst )
{
	MoverBase::Init( inst );

	isStopping	= false;
	gotoNext	= false;

	inst.motionManager.ChangeMotion( MotionKind::RushBrake );
}
void BossFirst::Brake::Uninit( BossFirst &inst ) {}
void BossFirst::Brake::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	const auto data	= ( 0 < inst.remainFeintCount )
					? FetchMember().forFirst.brakeInFeint
					: FetchMember().forFirst.brake;

	if ( isStopping )
	{
		inst.timer++;

		if ( data.waitFrameAfterStop <= inst.timer )
		{
			gotoNext = true;
		}
		return;
	}
	// else

	float currentSpeed = inst.velocity.XZ().Length();
	currentSpeed -= ( inst.element.Has( Element::Type::Oil ) ) ? data.oiledDecel : data.normalDecel;
	currentSpeed = std::max( 0.0f, currentSpeed );

	const Donya::Vector3 updatedVelocity = inst.orientation.LocalFront() * currentSpeed;
	inst.velocity.x = updatedVelocity.x;
	inst.velocity.z = updatedVelocity.z;

	if ( ZeroEqual( currentSpeed ) )
	{
		isStopping = true;
	}
}
void BossFirst::Brake::PhysicUpdate( BossFirst &inst, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix )
{
	MoverBase::PhysicUpdate( inst, solids, pTerrain, pTerrainWorldMatrix );

	// Prevent to starting the rush from outside.

	const Donya::Vector2 movedPos = inst.GetPosition().XZ();
	const float movableRange = FetchMember().forFirst.rush.movableRange;
	auto OnOutSide = []( const Donya::Vector2 &xzPos, float movableRange )
	{
		return ( movableRange < fabsf( xzPos.x ) || movableRange < fabsf( xzPos.y ) );
	};
	if ( OnOutSide( movedPos, movableRange ) && !inst.element.Has( Element::Type::Oil ) )
	{
		constexpr float ERROR_MARGIN = 0.99f;
		const float insideRange = movableRange * ERROR_MARGIN;
		Donya::Clamp( &inst.pos.x, -insideRange, insideRange );
		Donya::Clamp( &inst.pos.z, -insideRange, insideRange );
	}
}
bool BossFirst::Brake::ShouldChangeMover( BossFirst &inst ) const
{
	return gotoNext;
}
std::function<void()> BossFirst::Brake::GetChangeStateMethod( BossFirst &inst ) const
{
	if ( 0 < inst.remainFeintCount )
	{
		return [&]() { inst.AssignMover<Ready>(); };
	}
	// else
	return [&]() { inst.AdvanceAction(); };
}
std::string BossFirst::Brake::GetStateName() const { return "Brake"; }

void BossFirst::Breath::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	inst.velocity.x = 0.0f;
	inst.velocity.z = 0.0f;

	gotoNext = false;

	inst.motionManager.ChangeMotion( MotionKind::BreathReady );
}
void BossFirst::Breath::Uninit( BossFirst &inst ) {}
void BossFirst::Breath::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	inst.timer++;

	const int	maxHP	= FetchMember().FetchInitialHP( inst.GetType() );
	const int	index	= maxHP - inst.hp;
	const auto	data	= FetchMember().forFirst.breath.paramPerHP;
	if ( data.empty() ) { gotoNext = true; return; }
	// else
	const auto	&source	= ( scast<int>( data.size() ) <= index )
				? data.back()
				:	( index < 0 )
					? data.front()
					: data[index];
	const int preFrame	= source.preFireFrame;
	const int fireFrame	= preFrame + source.fireFrame;
	const int postFrame	= fireFrame + source.postFireFrame;

	const Donya::Vector3 aimingVector = inst.CalcAimingVector( targetPos, source.maxAimDegree );
	inst.orientation	= Donya::Quaternion::LookAt( Donya::Vector3::Front(), aimingVector.Unit(), Donya::Quaternion::Freeze::Up );
	inst.aimingPos		= targetPos;

	if ( preFrame <= inst.timer && inst.timer < fireFrame )
	{
		if ( inst.motionManager.GetCurrentKind() != MotionKind::BreathProcess )
		{
			inst.motionManager.ChangeMotion( MotionKind::BreathProcess );
		}

		auto ShouldFire = [&]()
		{
			const int &interval = source.fireInterval;
			if ( interval <= 1 ) { return true; }
			// else

			const int rem = inst.timer % interval;
			return ( rem == 0 );
		};
		if ( ShouldFire() )
		{
			Fire( inst, targetPos, source.fireDesc );
		}
	}

	if ( postFrame <= inst.timer )
	{
		gotoNext = true;
	}
}
bool BossFirst::Breath::ShouldChangeMover( BossFirst &inst ) const
{
	return gotoNext;
}
std::function<void()> BossFirst::Breath::GetChangeStateMethod( BossFirst &inst ) const
{
	return [&]() { inst.AdvanceAction(); };
}
std::string BossFirst::Breath::GetStateName() const { return "Breath"; }
void BossFirst::Breath::Fire( BossFirst &inst, const Donya::Vector3 &targetPos, const Bullet::BulletAdmin::FireDesc &desc )
{
	Bullet::BulletAdmin::FireDesc tmp = desc;
	tmp.generatePos	=  inst.orientation.RotateVector( tmp.generatePos );
	tmp.generatePos	+= inst.GetPosition();
	tmp.addElement	=  Element::Type::Flame;

	// I want vector that looking my front and Y of "to target vector".
	// That is made by rotate front vector by proper radian.
	const Donya::Vector3 targetVec = targetPos - tmp.generatePos;
	const Donya::Vector3 exceptY{ targetVec.x, 0.0f, targetVec.z };
	float cosTheta = Donya::Dot( targetVec.Unit(), exceptY.Unit() );
	Donya::Clamp( &cosTheta, -1.0f, 1.0f );
	const float radian   = acosf( cosTheta );
	const auto  rotation = Donya::Quaternion::Make( inst.orientation.LocalRight(), radian );
	tmp.direction = rotation.RotateVector( inst.orientation.LocalFront() );

	Bullet::BulletAdmin::Get().Append( tmp );
}

BossFirst::Wait::Wait( int waitFrame )
	: BossFirst::MoverBase(), waitFrame( waitFrame )
{}
void BossFirst::Wait::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	inst.velocity.x = 0.0f;
	inst.velocity.z = 0.0f;

	inst.motionManager.ChangeMotion( MotionKind::Wait );
}
void BossFirst::Wait::Uninit( BossFirst &inst ) {}
void BossFirst::Wait::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	inst.timer++;

	const auto data = FetchMember().forFirst.wait;
	const Donya::Vector3 aimingVector = inst.CalcAimingVector( targetPos, data.maxAimDegree );
	inst.orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), aimingVector.Unit(), Donya::Quaternion::Freeze::Up );
}
bool BossFirst::Wait::ShouldChangeMover( BossFirst &inst ) const
{
	return ( waitFrame <= inst.timer );
}
std::function<void()> BossFirst::Wait::GetChangeStateMethod( BossFirst &inst ) const
{
	return [&]() { inst.AdvanceAction(); };
}
std::string BossFirst::Wait::GetStateName() const { return "Wait"; }

BossFirst::Walk::Walk( int walkFrame )
	: BossFirst::MoverBase(), walkFrame( walkFrame )
{}
void BossFirst::Walk::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	inst.velocity.x = 0.0f;
	inst.velocity.z = 0.0f;

	inst.motionManager.ChangeMotion( MotionKind::Walk );
}
void BossFirst::Walk::Uninit( BossFirst &inst ) {}
void BossFirst::Walk::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	inst.timer++;

	const auto data = FetchMember().forFirst.walk;
	const Donya::Vector3 aimingVector = inst.CalcAimingVector( targetPos, data.maxAimDegree );
	inst.orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), aimingVector.Unit(), Donya::Quaternion::Freeze::Up );

	const float speed = data.walkSpeed * elapsedTime;
	const Donya::Vector3 updatedVelocity = inst.orientation.LocalFront() * speed;
	inst.velocity.x = updatedVelocity.x;
	inst.velocity.z = updatedVelocity.z;
}
bool BossFirst::Walk::ShouldChangeMover( BossFirst &inst ) const
{
	return ( walkFrame <= inst.timer );
}
std::function<void()> BossFirst::Walk::GetChangeStateMethod( BossFirst &inst ) const
{
	return [&]() { inst.AdvanceAction(); };
}
std::string BossFirst::Walk::GetStateName() const { return "Walk"; }

void BossFirst::Damage::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	inst.remainFeintCount	= 0;
	inst.actionIndex		= 0;
	inst.hp--;
	inst.velocity.x			= 0.0f;
	inst.velocity.z			= 0.0f;

	gotoNext = false;

	inst.motionManager.ChangeMotion( MotionKind::Damage );
}
void BossFirst::Damage::Uninit( BossFirst &inst ) {}
void BossFirst::Damage::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{
	inst.timer++;

	const auto data = FetchMember().forFirst.damage;

	if ( inst.hp <= 0 )
	{
		if ( data.reactFrameFinal <= inst.timer )
		{
			gotoNext = true;
		}
	}
	else
	{
		if ( data.reactFrame <= inst.timer )
		{
			gotoNext = true;
		}
	}
}
bool BossFirst::Damage::AcceptDamage( const BossFirst &inst ) const
{
	return false;
}
bool BossFirst::Damage::AcceptDraw( const BossFirst &inst ) const
{
	const auto data = FetchMember().forFirst.damage;
	const int &interval = data.flushInterval;
	if ( interval <= 0 ) { return true; }
	// else

	const int rem = inst.timer % ( interval << 1 );
	return ( rem < interval );
}
bool BossFirst::Damage::ShouldChangeMover( BossFirst &inst ) const
{
	return gotoNext;
}
std::function<void()> BossFirst::Damage::GetChangeStateMethod( BossFirst &inst ) const
{
	if ( inst.hp <= 0 )
	{
		return [&]() { inst.AssignMover<Die>(); };
	}
	// else
	return [&]()
	{
		inst.actionIndex = -1; // I want assign the zero action. This expects will be increment.
		inst.AdvanceAction();
	};
}
std::string BossFirst::Damage::GetStateName() const { return "Damage"; }

void BossFirst::Die::Init( BossFirst &inst )
{
	MoverBase::Init( inst );
	inst.velocity.x = 0.0f;
	inst.velocity.z = 0.0f;

	inst.motionManager.ChangeMotion( MotionKind::Die );
}
void BossFirst::Die::Uninit( BossFirst &inst ) {}
void BossFirst::Die::Update( BossFirst &inst, float elapsedTime, const Donya::Vector3 &targetPos )
{

}
bool BossFirst::Die::IsDead( const BossFirst &inst ) const
{
	return true;
}
bool BossFirst::Die::ShouldChangeMover( BossFirst &inst ) const
{
	return false; // Don't revive.
}
std::function<void()> BossFirst::Die::GetChangeStateMethod( BossFirst &inst ) const
{
	return [&]() {}; // No op.
}
std::string BossFirst::Die::GetStateName() const { return "Die"; }

void BossFirst::Init( const BossInitializer &parameter )
{
	BossBase::Init( parameter );
	actionIndex = 0;

	motionManager.Init( *this );

	AssignMoverByAction( FetchMember().forFirst.initialAction );
}
void BossFirst::Uninit()
{
	BossBase::Uninit();

	if ( pMover ) { pMover->Uninit( *this ); }
	pMover.reset();
}
void BossFirst::Update( float elapsedTime, const Donya::Vector3 &targetPos )
{
	BossBase::Update( elapsedTime, targetPos );

#if USE_IMGUI
	if ( wantPauseUpdates ) { return; }
#endif // USE_IMGUI

	if ( IsDead() ) { return; }
	// else

	if ( receiveDamage )
	{
		AssignMover<Damage>();
		receiveDamage = false;
	}

	UpdateByMover( elapsedTime, targetPos );

	if ( element.Has( Element::Type::Oil ) )
	{
		oilTimer++;
		if ( FetchMember().forFirst.releaseOilFrame <= oilTimer )
		{
			element.Subtract( Element::Type::Oil );
		}
	}
	else
	{
		oilTimer = 0;
	}

	motionManager.Update( *this, elapsedTime );
}
void BossFirst::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix )
{
#if USE_IMGUI
	if ( wantPauseUpdates ) { return; }
#endif // USE_IMGUI

	if ( !pMover ) { return; }
	// else

	pMover->PhysicUpdate( *this, solids, pTerrain, pTerrainWorldMatrix );
}
void BossFirst::Draw( RenderingHelper *pRenderer ) const
{
	if ( pMover && !pMover->AcceptDraw( *this ) ) { return; }
	// else
	BossBase::Draw( pRenderer );
}
void BossFirst::MakeDamage( const Element &effect, const Donya::Vector3 &othersVelocity ) const
{
	if ( pMover && !pMover->AcceptDamage( *this ) ) { return; }
	// else

	auto IsImpactableType = []( const Element &element )
	{
		if ( !element.Has( Element::Type::Oil	) ) { return false; }
		if ( !element.Has( Element::Type::Flame	) ) { return false; }
		// else
		return true;
	};
	if ( IsImpactableType( effect ) && !othersVelocity.IsZero() )
	{
		receiveDamage = true;
		return;
	}
	// else

	// Flame check is only do when already has oil.
	if ( effect.Has( Element::Type::Flame ) && element.Has( Element::Type::Oil ) )
	{
		receiveDamage = true;
		return;
	}
	// else

	if ( effect.Has( Element::Type::Oil ) )
	{
		element.Add( Element::Type::Oil );
	}
}
void BossFirst::AssignMoverByAction( ActionType type )
{
	const auto data = FetchMember().forFirst;

	switch ( type )
	{
	case ActionType::Rush:			AssignMover<Ready>();		return;
	case ActionType::Breath:		AssignMover<Breath>();		return;
	case ActionType::Wait_Long:		AssignMover<Wait>( data.wait.longFrame  );	return;
	case ActionType::Wait_Short:	AssignMover<Wait>( data.wait.shortFrame );	return;
	case ActionType::Walk_Long:		AssignMover<Walk>( data.walk.longFrame  );	return;
	case ActionType::Walk_Short:	AssignMover<Walk>( data.walk.shortFrame );	return;
	default: _ASSERT_EXPR( 0, L"Error: Unexpected Type!" );		return;
	}
}
void BossFirst::AssignMoverByAction( int actionIndex )
{
	AssignMoverByAction( FetchAction( actionIndex ) );
}
void BossFirst::AdvanceAction()
{
	const auto patterns		= FetchActionPatterns();
	const int  patternCount	= scast<int>( patterns.size() );

	actionIndex++;
	if ( 0 < patternCount )
	{
		actionIndex %= patternCount;
	}

	AssignMoverByAction( actionIndex );
}
void BossFirst::UpdateByMover( float elapsedTime, const Donya::Vector3 &targetPos )
{
	if ( !pMover ) { return; }
	// else

	pMover->Update( *this, elapsedTime, targetPos );
	if ( pMover->ShouldChangeMover( *this ) )
	{
		auto ChangeState = pMover->GetChangeStateMethod( *this );
		ChangeState();
	}
}
Donya::Vector3 BossFirst::CalcAimingVector( const Donya::Vector3 &targetPos, float maxRotDegree ) const
{
	const Donya::Vector3 front = orientation.LocalFront();
	Donya::Vector3 targetVec = ( targetPos - GetPosition() ).Unit();
	targetVec.y = 0.0f;

	const float rotDeg = Donya::Dot( front, targetVec );
	if ( fabsf( rotDeg ) <= maxRotDegree ) { return targetVec; }
	// else

	const auto rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( maxRotDegree ) );
	return rotation.RotateVector( front );
}
std::vector<BossFirst::ActionType> BossFirst::FetchActionPatterns() const
{
	const auto data = FetchMember();
	const auto &patterns = data.forFirst.actionPatterns;
	if ( patterns.empty() ) { return {}; }
	// else

	const int  maxHP = data.FetchInitialHP( GetType() );
	const int  index = maxHP - hp;
	return	( scast<int>( patterns.size() ) <= index )
			? patterns.back()
			:	( index < 0 )
				? patterns.front()
				: patterns[index];
}
BossFirst::ActionType BossFirst::FetchAction( int actionIndex ) const
{
	const auto patterns		= FetchActionPatterns();
	const int  patternCount	= scast<int>( patterns.size() );
	return	( !patternCount ) 
			? ActionType::Rush // Fail safe
			: patterns[actionIndex % patternCount];
}
bool BossFirst::IsDead() const
{
	return ( pMover ) ? pMover->IsDead( *this ) : BossBase::IsDead();
}
BossType BossFirst::GetType() const { return BossType::First; }
std::vector<Element::Type> BossFirst::GetUncollidableTypes() const
{
	// Prevent to the boss collides my fired bullet.
	std::vector<Element::Type> tmp{};
	tmp.emplace_back( Element::Type::Flame );
	return tmp;
}
#if USE_IMGUI
void BossFirst::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::Text( u8"種類：%s", GetBossName( GetType() ).c_str() );

	ImGui::DragInt( u8"現在の体力", &hp ); hp = std::max( -1, hp );
	if ( ImGui::Button( u8"ダメージを与える" ) )
	{
		receiveDamage = true;
	}
	if ( pMover )
	{
		ImGui::Text( u8"現在の状態：%s", pMover->GetStateName().c_str() );
	}
	ImGui::DragInt( u8"内部タイマ",				&timer		);
	ImGui::DragInt( u8"オイルがついている時間",	&oilTimer	);

	const int actionPatternCount = scast<int>( FetchActionPatterns().size() );
	ImGui::Text( u8"現在の行動：%s",	ToString( FetchAction( actionIndex ) ).c_str() );
	ImGui::SameLine(); ImGui::SliderInt( "##", &actionIndex, 0, actionPatternCount - 1 );
	ImGui::Text
	(
		u8"次の行動：%s",
		( 0 < remainFeintCount )
		? ToString( FetchAction( actionIndex     ) ).c_str()
		: ToString( FetchAction( actionIndex + 1 ) ).c_str()
	);
	ImGui::Text( u8"残りフェイント使用数：%d", remainFeintCount );
	ImGui::Text( u8"現在のモーション：%s", GetMotionName( motionManager.GetCurrentKind() ).c_str() );

	ImGui::DragFloat3( u8"現在の座標", &pos.x,		0.01f );
	ImGui::DragFloat3( u8"現在の速度", &velocity.x,	0.01f );
	if ( pMover )
	{
		ImGui::DragFloat3( u8"狙っている座標", &aimingPos.x,	0.01f );
	}

	Donya::Vector3 localFront = orientation.LocalFront();
	ImGui::SliderFloat3( u8"現在の前方向", &localFront.x,	 -1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront, Donya::Quaternion::Freeze::Up );

	element.ShowImGuiNode( /* useTreeNode = */ false, "" );

	ImGui::TreePop();
}
#endif // USE_IMGUI
// region First
#pragma endregion
