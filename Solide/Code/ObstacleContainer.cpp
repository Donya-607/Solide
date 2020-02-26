#include "ObstacleContainer.h"

std::vector<Donya::AABB> ObstacleContainer::GetHitBoxes() const
{
	std::vector<Donya::AABB> hitBoxes{};
	for ( const auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else

		hitBoxes.emplace_back( pIt->GetHitBox() );
	}

	return hitBoxes;
}

#if USE_IMGUI
void ObstacleContainer::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	static int addKind = 0;
	ImGui::SliderInt( u8"追加する種類", &addKind, 0, ObstacleBase::GetModelKindCount() - 1 );
	ImGui::Text( u8"種類名：%s", ObstacleBase::GetModelName( addKind ) );

	auto &data = pObstacles;
	if ( ImGui::Button( u8"追加" ) )
	{
		std::shared_ptr<ObstacleBase> tmp{};

		ObstacleBase::AssignDerivedModel( addKind, &tmp );

		if ( tmp )
		{
			data.push_back( std::move( tmp ) );
		}
	}
	if ( 1 <= data.size() && ImGui::Button( u8"末尾を削除" ) )
	{
		data.pop_back();
	}

	const size_t count = data.size();
	size_t removeIndex = count;
	std::string caption{};
	for ( size_t i = 0; i < count; ++i )
	{
		caption = u8"[" + std::to_string( i ) + u8"：" + ObstacleBase::GetModelName( scast<int>( i ) ) + u8"]";
		if ( ImGui::Button( ( caption + u8"を削除" ).c_str() ) )
		{
			removeIndex = i;
		}

		if ( !data[i] ) { continue; }
		// else

		data[i]->ShowImGuiNode( caption );
	}

	if ( removeIndex != count )
	{
		data.erase( data.begin() + removeIndex );
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI
