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
	public:
		bool Create();
	};
	struct Renderer
	{
		std::unique_ptr<Donya::Model::StaticRenderer>	pStatic;
		std::unique_ptr<Donya::Model::SkinningRenderer>	pSkinning;
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
		static constexpr int DEFAULT_ID = -1;
		int DS	= DEFAULT_ID;
		int RS	= DEFAULT_ID;
		int PS	= DEFAULT_ID;
	public:
		bool Create();
	};
private:
	State						state;
	std::unique_ptr<CBuffer>	pCBuffer;
	std::unique_ptr<ShaderSet>	pShader;
	std::unique_ptr<Renderer>	pRenderer;
	bool wasCreated = false;
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
