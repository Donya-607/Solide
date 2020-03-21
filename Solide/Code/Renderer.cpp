#include "Renderer.h"

bool RenderingHelper::Renderer::Create()
{

}

bool RenderingHelper::Shader::Create( const std::vector<D3D11_INPUT_ELEMENT_DESC> &IEDescs, const std::string &fileNameVS, const std::string &fileNamePS )
{

}
void RenderingHelper::Shader::Activate()
{

}
void RenderingHelper::Shader::Deactivate()
{

}

bool RenderingHelper::ShaderSet::Create()
{

}

bool RenderingHelper::State::Create()
{

}

bool RenderingHelper::Init()
{

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
