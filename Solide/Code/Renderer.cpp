#include "Renderer.h"

#include "Donya/RenderingStates.h"

namespace
{
	static constexpr D3D11_DEPTH_STENCIL_DESC	DepthStencilDesc()
	{
		D3D11_DEPTH_STENCIL_DESC standard{};
		standard.DepthEnable			= TRUE;
		standard.DepthWriteMask			= D3D11_DEPTH_WRITE_MASK_ALL;
		standard.DepthFunc				= D3D11_COMPARISON_LESS;
		standard.StencilEnable			= FALSE;
		return standard;
	}
	static constexpr D3D11_RASTERIZER_DESC		RasterizerDesc()
	{
		D3D11_RASTERIZER_DESC standard{};
		standard.FillMode				= D3D11_FILL_SOLID;
		standard.CullMode				= D3D11_CULL_BACK;
		standard.FrontCounterClockwise	= TRUE;
		standard.DepthBias				= 0;
		standard.DepthBiasClamp			= 0;
		standard.SlopeScaledDepthBias	= 0;
		standard.DepthClipEnable		= TRUE;
		standard.ScissorEnable			= FALSE;
		standard.MultisampleEnable		= FALSE;
		standard.AntialiasedLineEnable	= TRUE;
		return standard;
	}
	static constexpr D3D11_SAMPLER_DESC			SamplerDesc()
	{
		D3D11_SAMPLER_DESC standard{};
		/*
		standard.MipLODBias		= 0;
		standard.MaxAnisotropy	= 16;
		*/
		standard.Filter					= D3D11_FILTER_ANISOTROPIC;
		standard.AddressU				= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.AddressV				= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.AddressW				= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.ComparisonFunc			= D3D11_COMPARISON_ALWAYS;
		standard.MinLOD					= 0;
		standard.MaxLOD					= D3D11_FLOAT32_MAX;
		return standard;
	}
}

bool RenderingHelper::CBuffer::Create()
{
	bool succeeded = true;
	if ( !scene.Create() ) { succeeded = false; }
	if ( !model.Create() ) { succeeded = false; }
	return succeeded;
}

bool RenderingHelper::Renderer::Create()
{
	pStatic		= std::make_unique<Donya::Model::StaticRenderer>();
	pSkinning	= std::make_unique<Donya::Model::SkinningRenderer>();
	return true;
}

bool RenderingHelper::Shader::Create( const std::vector<D3D11_INPUT_ELEMENT_DESC> &IEDescs, const std::string &fileNameVS, const std::string &fileNamePS )
{
	bool succeeded = true;
	if ( !VS.CreateByCSO( fileNameVS, IEDescs ) ) { succeeded = false; }
	if ( !PS.CreateByCSO( fileNamePS          ) ) { succeeded = false; }
	return succeeded;
}
void RenderingHelper::Shader::Activate()
{
	VS.Activate();
	PS.Activate();
}
void RenderingHelper::Shader::Deactivate()
{
	VS.Deactivate();
	PS.Deactivate();
}

bool RenderingHelper::ShaderSet::Create()
{
	constexpr const char *VSFilePathStatic		= "./Data/Shaders/ModelStaticVS.cso";
	constexpr const char *VSFilePathSkinning	= "./Data/Shaders/ModelSkinningVS.cso";
	constexpr const char *PSFilePath			= "./Data/Shaders/ModelPS.cso";
	constexpr auto IEDescsPos	= Donya::Model::Vertex::Pos::GenerateInputElements( 0 );
	constexpr auto IEDescsTex	= Donya::Model::Vertex::Tex::GenerateInputElements( 1 );
	constexpr auto IEDescsBone	= Donya::Model::Vertex::Bone::GenerateInputElements( 2 );

	auto Append = []( auto &dest, const auto &source )
	{
		dest.insert( dest.end(), source.begin(), source.end() );
	};

	std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsStatic{};
	Append( IEDescsStatic, IEDescsPos );
	Append( IEDescsStatic, IEDescsTex );
	std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsSkinning{ IEDescsStatic };
	Append( IEDescsSkinning, IEDescsBone );

	bool succeeded = true;
	if ( !normalStatic.Create( IEDescsStatic, VSFilePathStatic, PSFilePath ) ) { succeeded = false; }
	if ( !normalSkinning.Create( IEDescsSkinning, VSFilePathSkinning, PSFilePath ) ) { succeeded = false; }
	return succeeded;
}

bool RenderingHelper::State::Create()
{
	using FindFunction = std::function<bool( int )>;
	auto  FindAvailableID = []( const FindFunction &IsAlreadyExists )
	{
		for ( int i = 0; i < INT_MAX; ++i )
		{
			if ( IsAlreadyExists( i ) ) { continue; }
			// else

			return i;
		}

		_ASSERT_EXPR( 0, L"Error : An available identifier was not found!" );
		return NULL;
	};

	bool succeeded = true;

	if ( DS == DEFAULT_ID )
	{
		DS =  FindAvailableID( Donya::DepthStencil::IsAlreadyExists );
		if ( !Donya::DepthStencil::CreateState( DS, DepthStencilDesc() ) ) { succeeded = false; }
	}
	if ( RS == DEFAULT_ID )
	{
		RS = FindAvailableID( Donya::Rasterizer::IsAlreadyExists );
		if ( !Donya::Rasterizer::CreateState( DS, RasterizerDesc() ) ) { succeeded = false; }
	}
	if ( PS == DEFAULT_ID )
	{
		PS = FindAvailableID( Donya::Sampler::IsAlreadyExists );
		if ( !Donya::Sampler::CreateState( DS, SamplerDesc() ) ) { succeeded = false; }
	}

	return succeeded;
}

bool RenderingHelper::Init()
{
	if ( wasCreated ) { return true; }
	// else

	bool succeeded = true;

	pCBuffer	= std::make_unique<CBuffer>();
	pShader		= std::make_unique<ShaderSet>();
	pRenderer	= std::make_unique<Renderer>();

	if ( !state.Create() )		{ succeeded = false; }
	if ( !pCBuffer->Create() )	{ succeeded = false; }
	if ( !pShader->Create() )	{ succeeded = false; }
	if ( !pRenderer->Create() )	{ succeeded = false; }

	if ( succeeded ) { wasCreated = true; }
	return succeeded;
}

void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerScene::Common &constant )
{

}
void RenderingHelper::UpdateConstant( const Donya::Model::Constants::PerModel::Common &constant )
{

}
void RenderingHelper::ActivateConstantScene()
{

}
void RenderingHelper::ActivateConstantModel()
{

}
void RenderingHelper::DeactivateConstantScene()
{

}
void RenderingHelper::DeactivateConstantModel()
{

}

void RenderingHelper::ActivateShaderNormal()
{

}
void RenderingHelper::DeactivateShaderNormal()
{

}

void RenderingHelper::Render( const Donya::Model::StaticModel &model, const Donya::Model::Pose &pose )
{

}
void RenderingHelper::Render( const Donya::Model::SkinningModel &model, const Donya::Model::Pose &pose )
{

}
