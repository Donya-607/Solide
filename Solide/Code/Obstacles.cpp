#include "Obstacles.h"

#include <memory>
#include <vector>

#include "Donya/Constant.h"
#include "Donya/Loader.h"
#include "Donya/StaticMesh.h"
#include "Donya/Useful.h"		// MultiByte char -> Wide char

namespace
{
	enum Kind
	{
		Stone,
		Log,

		KindCount
	};
	constexpr size_t KIND_COUNT = scast<size_t>( KindCount );
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Obstacle/";
	constexpr const char *EXTENSION = ".bin";
	constexpr std::array<const char *, KIND_COUNT> MODEL_NAMES
	{
		"Stone",
		"Log",
	};

	static std::array<std::shared_ptr<Donya::StaticMesh>, KIND_COUNT> models{};

	bool LoadModels()
	{
		bool result		= true;
		bool succeeded	= true;

		auto Load = []( const std::string &filePath, Donya::StaticMesh *pDest )->bool
		{
			bool result = true;
			Donya::Loader loader{};

			result = loader.Load( filePath, nullptr );
			if ( !result ) { return false; }
			// else

			result = Donya::StaticMesh::Create( loader, *pDest );
			return result;
		};

		std::string filePath{};
		const std::string prefix = MODEL_DIRECTORY;
		for ( size_t i = 0; i < KIND_COUNT; ++i )
		{
			filePath = prefix + MODEL_NAMES[i] + EXTENSION;
			if ( !Donya::IsExistFile( filePath ) )
			{
				const std::string outputMsgBase{ "Error : The model file does not exist. That is : " };
				Donya::OutputDebugStr( ( outputMsgBase + "[" + filePath + "]" + "\n" ).c_str() );
				continue;
			}
			// else

			models[i] = std::make_shared<Donya::StaticMesh>();
			result = Load( filePath, &( *models[i] ) ); // std::shared_ptr<T> -> T -> T *
			if ( !result )
			{
				const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
				const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
				_ASSERT_EXPR( 0, errMsg.c_str() );

				succeeded = false;
			}
		}

		return succeeded;
	}

	std::shared_ptr<Donya::StaticMesh> GetModel( Kind kind )
	{
		if ( kind < 0 || KindCount <= kind )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return nullptr;
		}
		// else

		return models[kind];
	}

	void DrawModel( Kind kind, const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
	{
		const auto pModel = GetModel( kind );
		if ( !pModel ) { return; }
		// else

		pModel->Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			W * VP, W,
			lightDir, color
		);
	}
}

bool ObstacleBase::LoadModels()
{
	return ::LoadModels();
}

void Stone::Draw( const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	DrawModel( Kind::Stone, GetWorldMatrix(), VP, lightDir, color );
}
void Log::Draw( const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	DrawModel( Kind::Log, GetWorldMatrix(), VP, lightDir, color );
}
