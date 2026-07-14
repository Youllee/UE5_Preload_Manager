#pragma once

#include "IPropertyTypeCustomization.h"

// FYLPreloadDataBase를 상속받는 구조체를 한 줄로 표시하는 커스터마이징이다.
class FYLPreloadDataCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FYLPreloadDataCustomization>(); }

	// 구조체의 모든 Property를 Header 영역에 한 줄로 표시한다.
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	// 모든 프로퍼티를 Header 영역에 표시하기 때문에, Children 영역은 만들지 않는다.
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

};
