#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Donya/Shader.h"
#include "Donya/CBuffer.h"
#include "Donya/Model.h"
#include "Donya/ModelCommon.h"
#include "Donya/ModelPose.h"
#include "Donya/ModelRenderer.h"

class RenderingHelper
{
private:
	struct CBuffer
	{
		Donya::CBuffer<Donya::Model::Constants::PerScene::Common> scene;
		Donya::CBuffer<Donya::Model::Constants::PerModel::Common> model;
	};
	struct Renderer
	{
		Donya::Model::StaticRenderer	normalStatic;
		Donya::Model::SkinningRenderer	normalSkinning;
	public:
		bool Create();
	};
	struct Shader
	{
		Donya::VertexShader	VS;
		Donya::PixelShader	PS;
	public:
		bool Create( const std::vector<D3D11_INPUT_ELEMENT_DESC> &IEDescs, const std::string &fileNameVS, const std::string &fileNamePS );
	public:
		void Activate();
		void Deactivate();
	};
	struct ShaderSet
	{
		Shader	normalStatic;
		Shader	normalSkinning;
	public:
		bool Create();
	};
	struct State
	{
		int DS	= 0;
		int RS	= 0;
		int PS	= 0;
	public:
		bool Create();
	};
private:
	State						state;
	std::unique_ptr<CBuffer>	pCBuffer;
	std::unique_ptr<ShaderSet>	pShader;
	std::unique_ptr<Renderer>	pRenderer;
public:
	bool Init();
public:
	void UpdateConstant( const Donya::Model::Constants::PerScene::Common &constant );
	void UpdateConstant( const Donya::Model::Constants::PerModel::Common &constant );
	void ActivateConstantScene();
	void ActivateConstantModel();
	void DeactivateConstantScene();
	void DeactivateConstantModel();
public:
	void ActivateShaderNormal();
	void DeactivateShaderNormal();
public:
	void Render( const Donya::Model::StaticModel	&model, const Donya::Model::Pose &pose );
	void Render( const Donya::Model::SkinningModel	&model, const Donya::Model::Pose &pose );
};
