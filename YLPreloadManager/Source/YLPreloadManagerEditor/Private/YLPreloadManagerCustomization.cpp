#include "YLPreloadManagerCustomization.h"
#include "YLPreloadManagerType.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void FYLPreloadDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Make HorizontalBox
	TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

	// Add Property Datas
	uint32 ChildrenNum = 0;
	PropertyHandle->GetNumChildren(ChildrenNum);
	for (uint32 ChildIndex = 0; ChildIndex < ChildrenNum; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex);
		if (ChildHandle.IsValid() == false || ChildHandle->IsValidHandle() == false)
		{
			continue;
		}

		const FText DisplayName = ChildHandle->GetPropertyDisplayName();
		const FName PropertyName = ChildHandle->GetProperty() ? ChildHandle->GetProperty()->GetFName() : NAME_None;

		// Property마다 Widget의 간격을 다르게 조절해야 한다.
		// 간격을 조절해야 하는 Property가 추가되면, 아래에 내용을 추가한다.
		float PropertyWidth = 24.0f;
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FYLDataAssetPreloadData, DataAsset)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(FYLDataTablePreloadData, DataTable)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(FYLDataRegistryPreloadData, DataRegistry))
		{
			PropertyWidth = 240.0f;
		}

		// Make HorizontalBox Slot
		TSharedRef<SWidget> ChildWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(DisplayName)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.MinDesiredWidth(80.0f)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
						.MinDesiredWidth(PropertyWidth)
						[
							ChildHandle->CreatePropertyValueWidget()
						]
				];

		// Add HorizontalBox Slot
		HorizontalBox->AddSlot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 10.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				ChildWidget
			];
	}

	// Add Horizontal Box
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			HorizontalBox
		];
}
