#include "DealStageModules.h"

#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Sound/SoundBase.h"
#include "GameFramework/GameUserSettings.h"

namespace DealStageVisuals
{
    static UStaticMesh* Cube()
    {
        static UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        return Mesh;
    }

    static UStaticMesh* Cylinder()
    {
        static UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        return Mesh;
    }

    static void Tint(UPrimitiveComponent* Component, const FLinearColor& Color, bool bTranslucent = false)
    {
        if (!Component) return;
        // Reuse a material instance: hovering a case must not allocate dozens of MIDs.
        UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
        if (!Material)
        {
            const TCHAR* Path = bTranslucent ? TEXT("/Game/Studio/Materials/M_Glass.M_Glass")
                : TEXT("/Game/Studio/Materials/M_Tint.M_Tint");
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, Path);
            if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
            Material = UMaterialInstanceDynamic::Create(Base, Component);
            Component->SetMaterial(0, Material);
        }
        Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
    }

    static UStaticMesh* Asset(const TCHAR* Name)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/Studio/Meshes/%s.%s"), Name, Name));
        return Mesh ? Mesh : Cube();
    }

    static void SetupMesh(
        UStaticMeshComponent* Component,
        UStaticMesh* Mesh,
        USceneComponent* Parent,
        const FVector& Location,
        const FVector& Scale,
        bool bCollision = false,
        const FRotator& Rotation = FRotator::ZeroRotator)
    {
        Component->SetupAttachment(Parent);
        Component->SetStaticMesh(Mesh);
        Component->SetRelativeLocation(Location);
        Component->SetRelativeRotation(Rotation);
        Component->SetRelativeScale3D(Scale);
        Component->SetMobility(EComponentMobility::Static);
        Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    static void SetupText(
        UTextRenderComponent* Component,
        USceneComponent* Parent,
        const FString& Text,
        const FVector& Location,
        float WorldSize,
        const FLinearColor& Color = FLinearColor::White,
        const FRotator& Rotation = FRotator(0.0f, 180.0f, 0.0f))
    {
        Component->SetupAttachment(Parent);
        Component->SetText(FText::FromString(Text));
        Component->SetRelativeLocation(Location);
        Component->SetRelativeRotation(Rotation);
        Component->SetWorldSize(WorldSize);
        Component->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
        Component->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
        Component->SetTextRenderColor(Color.ToFColor(true));
        Component->SetMobility(EComponentMobility::Static);
    }
}

AStageModuleBase::AStageModuleBase()
{
    PrimaryActorTick.bCanEverTick = false;
    ModuleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModuleRoot"));
    ModuleRoot->SetMobility(EComponentMobility::Static);
    RootComponent = ModuleRoot;
}

AStageWorldShellModule::AStageWorldShellModule()
{
    StudioFloor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StudioFloor_24m_x_18m"));
    DealStageVisuals::SetupMesh(StudioFloor, DealStageVisuals::Cube(), ModuleRoot,
        FVector(0.0f, 0.0f, -20.0f), FVector(18.0f, 24.0f, 0.4f), true);

    UpstageWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpstageMaskingWall"));
    DealStageVisuals::SetupMesh(UpstageWall, DealStageVisuals::Cube(), ModuleRoot,
        FVector(920.0f, 0.0f, 450.0f), FVector(0.25f, 12.0f, 9.0f));

    ScaleLegend = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ScaleLegend"));
    DealStageVisuals::SetupText(ScaleLegend, ModuleRoot,
        TEXT("GRAYBOX CORE: 24m WIDE x 18m DEEP | 1 UU = 1 cm"),
        FVector(-800.0f, -920.0f, 20.0f), 34.0f, FLinearColor(0.35f, 0.55f, 0.8f));
    ScaleLegend->SetVisibility(false);
}

void AStageWorldShellModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    DealStageVisuals::Tint(StudioFloor, FLinearColor(0.018f, 0.022f, 0.032f));
    DealStageVisuals::Tint(UpstageWall, FLinearColor(0.006f, 0.009f, 0.018f));
}

AStagePlatformModule::AStagePlatformModule()
{
    MainPlatform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CentralPolygonPlatform_7p8m_x_5p8m"));
    DealStageVisuals::SetupMesh(MainPlatform, DealStageVisuals::Cylinder(), ModuleRoot,
        FVector::ZeroVector, FVector(7.8f, 5.8f, 0.22f), true);

    PlatformInset = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CentralPlatformInset"));
    DealStageVisuals::SetupMesh(PlatformInset, DealStageVisuals::Cylinder(), ModuleRoot,
        FVector(0.0f, 0.0f, 16.0f), FVector(6.9f, 4.9f, 0.08f));

    TablePedestal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GameTable_Pedestal"));
    DealStageVisuals::SetupMesh(TablePedestal, DealStageVisuals::Cylinder(), ModuleRoot,
        FVector(-40.0f, 0.0f, 78.0f), FVector(0.48f, 0.48f, 1.35f), true);

    TableTop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GameTable_TransparentTop_Placeholder"));
    DealStageVisuals::SetupMesh(TableTop, DealStageVisuals::Cube(), ModuleRoot,
        FVector(-40.0f, 0.0f, 150.0f), FVector(1.35f, 1.8f, 0.10f), true);
    TableTop->SetCastShadow(false);

    PhonePlaceholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BankerPhone_InteractionSocket"));
    DealStageVisuals::SetupMesh(PhonePlaceholder, DealStageVisuals::Cube(), ModuleRoot,
        FVector(-70.0f, 32.0f, 168.0f), FVector(0.38f, 0.16f, 0.10f), true,
        FRotator(0.0f, -18.0f, 0.0f));
    PhonePlaceholder->ComponentTags.Add(TEXT("Interactable.BankerPhone"));

    TableLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GameTable_Label"));
    DealStageVisuals::SetupText(TableLabel, ModuleRoot, TEXT("GAME TABLE / PHONE"),
        FVector(-112.0f, 0.0f, 188.0f), 14.0f, FLinearColor(0.1f, 0.85f, 1.0f));
    TableLabel->SetVisibility(false);
}

void AStagePlatformModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    DealStageVisuals::Tint(MainPlatform, FLinearColor(0.018f, 0.11f, 0.19f));
    DealStageVisuals::Tint(PlatformInset, FLinearColor(0.04f, 0.28f, 0.40f));
    DealStageVisuals::Tint(TablePedestal, FLinearColor(0.04f, 0.42f, 0.52f), true);
    DealStageVisuals::Tint(TableTop, FLinearColor(0.08f, 0.72f, 0.9f, 0.28f), true);
    DealStageVisuals::Tint(PhonePlaceholder, FLinearColor(0.02f, 0.02f, 0.02f));
}

AStageStaircaseModule::AStageStaircaseModule()
{
    PrimaryActorTick.bCanEverTick = true;
    OpenTargets.Init(false, 26);
    static const int32 RowCounts[4] = { 6, 7, 7, 6 };
    static const float TierTopHeights[4] = { 55.0f, 135.0f, 215.0f, 295.0f };
    static const float TierX[4] = { 285.0f, 405.0f, 525.0f, 645.0f };

    int32 BriefcaseNumber = 1;
    for (int32 Row = 0; Row < 4; ++Row)
    {
        UStaticMeshComponent* Tier = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("Tier_%d_%dPositions"), Row + 1, RowCounts[Row]));
        DealStageVisuals::SetupMesh(Tier, DealStageVisuals::Cube(), ModuleRoot,
            FVector(TierX[Row], 0.0f, TierTopHeights[Row] * 0.5f),
            FVector(1.20f, 10.70f, TierTopHeights[Row] / 100.0f), true);
        StairTiers.Add(Tier);

        const float Spacing = 145.0f;
        const float StartY = -0.5f * Spacing * static_cast<float>(RowCounts[Row] - 1);
        for (int32 Position = 0; Position < RowCounts[Row]; ++Position)
        {
            const float Y = StartY + Spacing * static_cast<float>(Position);

            UStaticMeshComponent* Case = CreateDefaultSubobject<UStaticMeshComponent>(
                *FString::Printf(TEXT("Briefcase_%02d_InteractionSocket"), BriefcaseNumber));
            DealStageVisuals::SetupMesh(Case, DealStageVisuals::Asset(TEXT("SM_BriefcaseBody")), ModuleRoot,
                FVector(TierX[Row], Y, TierTopHeights[Row] + 108.0f),
                FVector::OneVector, true);
            Case->ComponentTags.Add(TEXT("Interactable.Briefcase"));
            Case->ComponentTags.Add(*FString::Printf(TEXT("Briefcase.%02d"), BriefcaseNumber));
            BriefcaseMarkers.Add(Case);
            UStaticMeshComponent* Lid = CreateDefaultSubobject<UStaticMeshComponent>(
                *FString::Printf(TEXT("BriefcaseLid_%02d"), BriefcaseNumber));
            DealStageVisuals::SetupMesh(Lid, DealStageVisuals::Asset(TEXT("SM_BriefcaseLid")), ModuleRoot,
                FVector(TierX[Row] - 8.0f, Y, TierTopHeights[Row] + 91.0f), FVector::OneVector);
            Lid->SetMobility(EComponentMobility::Movable);
            BriefcaseLids.Add(Lid);

            UStaticMeshComponent* Handle = CreateDefaultSubobject<UStaticMeshComponent>(
                *FString::Printf(TEXT("BriefcaseHandle_%02d"), BriefcaseNumber));
            DealStageVisuals::SetupMesh(Handle, DealStageVisuals::Cube(), ModuleRoot,
                FVector(TierX[Row] - 14.0f, Y, TierTopHeights[Row] + 87.0f),
                FVector(0.02f, 0.47f, 0.025f));
            BriefcaseHandles.Add(Handle);

            UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(
                *FString::Printf(TEXT("BriefcaseLabel_%02d"), BriefcaseNumber));
            DealStageVisuals::SetupText(Label, ModuleRoot, FString::FromInt(BriefcaseNumber),
                FVector(TierX[Row] - 11.0f, Y, TierTopHeights[Row] + 108.0f),
                18.0f, FLinearColor::Black);
            Label->SetMobility(EComponentMobility::Movable);
            BriefcaseLabels.Add(Label);
            ++BriefcaseNumber;
        }
    }
}

void AStageStaircaseModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    for (UStaticMeshComponent* Handle : BriefcaseHandles)
        DealStageVisuals::Tint(Handle, FLinearColor(0.12f, 0.24f, 0.35f));
}

void AStageStaircaseModule::SetBriefcaseHighlight(int32 Number, bool bHighlighted)
{
    if (BriefcaseHandles.IsValidIndex(Number - 1))
        DealStageVisuals::Tint(BriefcaseHandles[Number - 1], bHighlighted
            ? FLinearColor(1.0f, 0.58f, 0.06f) : FLinearColor(0.12f, 0.24f, 0.35f));
}

void AStageStaircaseModule::SetBriefcasePlayerCase(int32 Number, bool bPlayer)
{
    if (bPlayer && BriefcaseHandles.IsValidIndex(Number - 1))
        DealStageVisuals::Tint(BriefcaseHandles[Number - 1], FLinearColor(0.02f, 0.85f, 1.0f));
}

void AStageStaircaseModule::SetBriefcaseOpened(int32 Number, bool bOpened)
{
    const int32 I = Number - 1;
    if (!BriefcaseMarkers.IsValidIndex(I)) return;
    if (OpenTargets.Num() != 26) OpenTargets.Init(false, 26);
    OpenTargets[I] = bOpened;
    BriefcaseMarkers[I]->SetCollisionEnabled(bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
    BriefcaseLabels[I]->SetTextRenderColor(bOpened ? FColor(90,110,125) : FColor::Black);
}

void AStageStaircaseModule::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (int32 I = 0; I < BriefcaseLids.Num() && I < OpenTargets.Num(); ++I)
    {
        UStaticMeshComponent* Lid = BriefcaseLids[I];
        const float Target = OpenTargets[I] ? 105.0f : 0.0f;
        const float Angle = FMath::FInterpTo(Lid->GetRelativeRotation().Pitch, Target, DeltaSeconds, 5.5f);
        Lid->SetRelativeRotation(FRotator(Angle, 0, 0));
        // Case number remains attached to the moving lid.
        BriefcaseLabels[I]->SetWorldLocation(Lid->GetComponentTransform().TransformPosition(FVector(-3,0,17)));
        BriefcaseLabels[I]->SetWorldRotation(Lid->GetComponentQuat() * FRotator(0,180,0).Quaternion());
    }
}

AStageAmountBoardModule::AStageAmountBoardModule()
{
    static const TCHAR* Amounts[26] =
    {
        TEXT("$0.01"), TEXT("$1"), TEXT("$5"), TEXT("$10"), TEXT("$25"), TEXT("$50"), TEXT("$75"),
        TEXT("$100"), TEXT("$200"), TEXT("$300"), TEXT("$400"), TEXT("$500"), TEXT("$750"),
        TEXT("$1,000"), TEXT("$5,000"), TEXT("$10,000"), TEXT("$25,000"), TEXT("$50,000"),
        TEXT("$75,000"), TEXT("$100,000"), TEXT("$200,000"), TEXT("$300,000"), TEXT("$400,000"),
        TEXT("$500,000"), TEXT("$750,000"), TEXT("$1,000,000")
    };

    BoardHousing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmountBoard_Housing_4m_x_7m"));
    DealStageVisuals::SetupMesh(BoardHousing, DealStageVisuals::Cube(), ModuleRoot,
        FVector(0.0f, 0.0f, 350.0f), FVector(0.25f, 2.0f, 3.5f), true);

    BoardHeader = CreateDefaultSubobject<UTextRenderComponent>(TEXT("AmountBoard_Header"));
    DealStageVisuals::SetupText(BoardHeader, ModuleRoot, TEXT("AMOUNTS"),
        FVector(-31.0f, 0.0f, 660.0f), 44.0f, FLinearColor(1.0f, 0.78f, 0.08f));

    for (int32 Index = 0; Index < 26; ++Index)
    {
        const int32 Column = Index / 13;
        const int32 Row = Index % 13;
        const float Y = Column == 0 ? -98.0f : 98.0f;
        const float Z = 603.0f - 46.0f * static_cast<float>(Row);

        UStaticMeshComponent* Tile = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("AmountTile_%02d_Active"), Index + 1));
        DealStageVisuals::SetupMesh(Tile, DealStageVisuals::Cube(), ModuleRoot,
            FVector(-30.0f, Y, Z), FVector(0.08f, 1.84f, 0.34f));
        Tile->ComponentTags.Add(TEXT("Interactable.AmountTile"));
        Tile->ComponentTags.Add(*FString::Printf(TEXT("Amount.Index.%02d"), Index));
        AmountTiles.Add(Tile);

        UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(
            *FString::Printf(TEXT("AmountLabel_%02d"), Index + 1));
        DealStageVisuals::SetupText(Label, ModuleRoot, Amounts[Index],
            FVector(-39.0f, Y, Z), 22.0f, FLinearColor::White);
        AmountLabels.Add(Label);
    }
}

void AStageAmountBoardModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    DealStageVisuals::Tint(BoardHousing, FLinearColor(0.008f, 0.012f, 0.02f));
    for (int32 Index = 0; Index < AmountTiles.Num(); ++Index)
    {
        const bool bHighValue = Index >= 13;
        DealStageVisuals::Tint(AmountTiles[Index], bHighValue
            ? FLinearColor(0.48f, 0.19f, 0.035f)
            : FLinearColor(0.025f, 0.20f, 0.52f));
    }
}

void AStageAmountBoardModule::SetAmountActive(int32 AmountIndex, bool bActive)
{
    if (!AmountTiles.IsValidIndex(AmountIndex) || !AmountLabels.IsValidIndex(AmountIndex))
    {
        return;
    }

    const bool bHighValue = AmountIndex >= 13;
    const FLinearColor ActiveColor = bHighValue
        ? FLinearColor(0.48f, 0.19f, 0.035f)
        : FLinearColor(0.025f, 0.20f, 0.52f);
    DealStageVisuals::Tint(AmountTiles[AmountIndex], bActive ? ActiveColor : FLinearColor(0.025f, 0.025f, 0.028f));
    AmountLabels[AmountIndex]->SetTextRenderColor((bActive ? FLinearColor::White : FLinearColor(0.12f, 0.12f, 0.12f)).ToFColor(true));
}

AStageBackdropModule::AStageBackdropModule()
{
    static const float BuildingY[7] = { -510.0f, -355.0f, -195.0f, -30.0f, 150.0f, 335.0f, 515.0f };
    static const float BuildingWidth[7] = { 125.0f, 110.0f, 135.0f, 145.0f, 120.0f, 155.0f, 115.0f };
    static const float BuildingHeight[7] = { 310.0f, 470.0f, 390.0f, 560.0f, 420.0f, 500.0f, 350.0f };

    for (int32 Index = 0; Index < 7; ++Index)
    {
        UStaticMeshComponent* Building = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("CityBuilding_%02d"), Index + 1));
        DealStageVisuals::SetupMesh(Building, DealStageVisuals::Cube(), ModuleRoot,
            FVector(50.0f, BuildingY[Index], BuildingHeight[Index] * 0.5f),
            FVector(0.55f, BuildingWidth[Index] / 100.0f, BuildingHeight[Index] / 100.0f));
        CityBuildings.Add(Building);
    }

    CityWindows = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CityNightWindows_Instanced"));
    CityWindows->SetupAttachment(ModuleRoot);
    CityWindows->SetStaticMesh(DealStageVisuals::Cube());
    CityWindows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CityWindows->SetMobility(EComponentMobility::Static);
    for (int32 BuildingIndex = 0; BuildingIndex < 7; ++BuildingIndex)
    {
        const int32 ColumnCount = FMath::Max(2, FMath::FloorToInt(BuildingWidth[BuildingIndex] / 34.0f));
        const int32 RowCount = FMath::FloorToInt(BuildingHeight[BuildingIndex] / 58.0f);
        for (int32 Column = 0; Column < ColumnCount; ++Column)
        {
            for (int32 Row = 0; Row < RowCount; ++Row)
            {
                const float Y = BuildingY[BuildingIndex] - 0.5f * (ColumnCount - 1) * 28.0f + Column * 28.0f;
                const float Z = 38.0f + Row * 56.0f;
                CityWindows->AddInstance(FTransform(FRotator::ZeroRotator,
                    FVector(-7.0f, Y, Z), FVector(0.08f, 0.16f, 0.12f)));
            }
        }
    }

    const int32 ArchPieceCount = 19;
    const float Radius = 650.0f;
    for (int32 Index = 0; Index < ArchPieceCount; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(ArchPieceCount - 1);
        const float AngleDegrees = FMath::Lerp(0.0f, 180.0f, Alpha);
        const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
        const FVector Location(-55.0f, Radius * FMath::Cos(AngleRadians), 35.0f + Radius * FMath::Sin(AngleRadians));
        UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("GrandArch_Segment_%02d"), Index + 1));
        DealStageVisuals::SetupMesh(Segment, DealStageVisuals::Cube(), ModuleRoot,
            Location, FVector(0.75f, 1.12f, 0.20f), false, FRotator(0.0f, 0.0f, AngleDegrees + 90.0f));
        ArchSegments.Add(Segment);
    }

    for (int32 Side = 0; Side < 2; ++Side)
    {
        UStaticMeshComponent* Column = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("GrandArch_Column_%s"), Side == 0 ? TEXT("Left") : TEXT("Right")));
        DealStageVisuals::SetupMesh(Column, DealStageVisuals::Cube(), ModuleRoot,
            FVector(-55.0f, Side == 0 ? -650.0f : 650.0f, 190.0f), FVector(0.75f, 0.20f, 3.8f));
        ArchSegments.Add(Column);
    }

    BackdropLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CityBackdrop_Label"));
    DealStageVisuals::SetupText(BackdropLabel, ModuleRoot, TEXT("CULVER CITY NIGHT BACKDROP"),
        FVector(-90.0f, 0.0f, 640.0f), 34.0f, FLinearColor(0.15f, 0.78f, 1.0f));
    BackdropLabel->SetVisibility(false);
}

void AStageBackdropModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    for (int32 Index = 0; Index < CityBuildings.Num(); ++Index)
    {
        const float Blue = 0.055f + 0.018f * static_cast<float>(Index % 3);
        DealStageVisuals::Tint(CityBuildings[Index], FLinearColor(0.008f, 0.018f, Blue));
    }
    DealStageVisuals::Tint(CityWindows, FLinearColor(1.0f, 0.58f, 0.08f));
    for (UStaticMeshComponent* Segment : ArchSegments)
    {
        DealStageVisuals::Tint(Segment, FLinearColor(0.02f, 0.46f, 0.73f));
    }
}

AStageBankerBoothModule::AStageBankerBoothModule()
{
    struct FBoothPiece
    {
        const TCHAR* Name;
        FVector Location;
        FVector Scale;
    };

    static const FBoothPiece Pieces[] =
    {
        { TEXT("BankerBooth_BaseTower"), FVector(85.0f, 0.0f, 235.0f), FVector(2.8f, 4.2f, 4.7f) },
        { TEXT("BankerBooth_Roof"), FVector(-30.0f, 0.0f, 690.0f), FVector(1.4f, 2.2f, 0.18f) },
        { TEXT("BankerBooth_LeftColumn"), FVector(-25.0f, -205.0f, 580.0f), FVector(1.25f, 0.18f, 2.2f) },
        { TEXT("BankerBooth_RightColumn"), FVector(-25.0f, 205.0f, 580.0f), FVector(1.25f, 0.18f, 2.2f) },
        { TEXT("BankerBooth_BackWall"), FVector(95.0f, 0.0f, 580.0f), FVector(0.18f, 2.05f, 2.2f) },
        { TEXT("BankerBooth_Desk"), FVector(-80.0f, 0.0f, 505.0f), FVector(0.55f, 1.35f, 0.18f) }
    };

    for (const FBoothPiece& Piece : Pieces)
    {
        UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Piece.Name);
        const bool bBaseCollision = FCString::Strcmp(Piece.Name, TEXT("BankerBooth_BaseTower")) == 0;
        DealStageVisuals::SetupMesh(Component, DealStageVisuals::Cube(), ModuleRoot,
            Piece.Location, Piece.Scale, bBaseCollision);
        BoothStructure.Add(Component);
    }

    GlassFront = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BankerBooth_GlassFront"));
    DealStageVisuals::SetupMesh(GlassFront, DealStageVisuals::Cube(), ModuleRoot,
        FVector(-158.0f, 0.0f, 600.0f), FVector(0.06f, 1.85f, 1.65f));
    GlassFront->SetCastShadow(false);

    BoothLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BankerBooth_Label"));
    DealStageVisuals::SetupText(BoothLabel, ModuleRoot, TEXT("THE BANKER\nHIGH BOOTH"),
        FVector(-168.0f, 0.0f, 645.0f), 31.0f, FLinearColor(0.95f, 0.12f, 0.08f));
    BoothLabel->SetVisibility(false);
}

void AStageBankerBoothModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    for (int32 Index = 0; Index < BoothStructure.Num(); ++Index)
    {
        const FLinearColor Color = Index == 0
            ? FLinearColor(0.012f, 0.016f, 0.025f)
            : FLinearColor(0.13f, 0.025f, 0.028f);
        DealStageVisuals::Tint(BoothStructure[Index], Color);
    }
    DealStageVisuals::Tint(GlassFront, FLinearColor(0.08f, 0.20f, 0.28f, 0.24f), true);
}

AStageAudienceModule::AStageAudienceModule()
{
    AudienceSeats = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AudienceSeats_UShape_Approx200"));
    AudienceSeats->SetupAttachment(ModuleRoot);
    AudienceSeats->SetStaticMesh(DealStageVisuals::Asset(TEXT("SM_StudioChair")));
    AudienceSeats->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AudienceSeats->SetMobility(EComponentMobility::Static);

    // Downstage audience bank. Five stepped rows wrap the camera line without closing the U shape.
    for (int32 Row = 0; Row < 5; ++Row)
    {
        const float X = -720.0f - static_cast<float>(Row) * 90.0f;
        const float Z = static_cast<float>(Row) * 28.0f;
        for (int32 Seat = 0; Seat < 21; ++Seat)
        {
            const float Y = -800.0f + static_cast<float>(Seat) * 80.0f;
            AudienceSeats->AddInstance(FTransform(FRotator::ZeroRotator,
                FVector(X, Y, Z), FVector::OneVector));
        }
    }

    // Side audience banks, angled inward toward the central platform.
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const float Sign = Side == 0 ? -1.0f : 1.0f;
        for (int32 Row = 0; Row < 4; ++Row)
        {
            const float Y = Sign * (810.0f + static_cast<float>(Row) * 88.0f);
            const float Z = static_cast<float>(Row) * 28.0f;
            for (int32 Seat = 0; Seat < 9; ++Seat)
            {
                const float X = -560.0f + static_cast<float>(Seat) * 90.0f;
                AudienceSeats->AddInstance(FTransform(FRotator(0.0f, -Sign * 90.0f, 0.0f),
                    FVector(X, Y, Z), FVector::OneVector));
            }
        }
    }

    struct FRiserPiece
    {
        const TCHAR* Name;
        FVector Location;
        FVector Scale;
    };
    static const FRiserPiece Risers[] =
    {
        { TEXT("AudienceRiser_Downstage"), FVector(-900.0f, 0.0f, 22.0f), FVector(4.7f, 9.2f, 0.35f) },
        { TEXT("AudienceRiser_Left"), FVector(-20.0f, -980.0f, 20.0f), FVector(9.0f, 2.0f, 0.32f) },
        { TEXT("AudienceRiser_Right"), FVector(-20.0f, 980.0f, 20.0f), FVector(9.0f, 2.0f, 0.32f) }
    };
    for (const FRiserPiece& Piece : Risers)
    {
        UStaticMeshComponent* Riser = CreateDefaultSubobject<UStaticMeshComponent>(Piece.Name);
        DealStageVisuals::SetupMesh(Riser, DealStageVisuals::Cube(), ModuleRoot,
            Piece.Location, Piece.Scale);
        AudienceRisers.Add(Riser);
    }

    AudienceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Audience_Label"));
    DealStageVisuals::SetupText(AudienceLabel, ModuleRoot,
        TEXT("BROKEN U-SHAPE AUDIENCE | GRAYBOX ~200 OF REPORTED ~360 SEATS"),
        FVector(-1090.0f, 0.0f, 205.0f), 31.0f, FLinearColor(0.65f, 0.65f, 0.7f),
        FRotator(0.0f, 0.0f, 0.0f));
    AudienceLabel->SetVisibility(false);
}

void AStageAudienceModule::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    for (UStaticMeshComponent* Riser : AudienceRisers)
    {
        DealStageVisuals::Tint(Riser, FLinearColor(0.022f, 0.028f, 0.038f));
    }
}

AStageLightingModule::AStageLightingModule()
{
    PrimaryActorTick.bCanEverTick = true;
    UDirectionalLightComponent* KeySun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Ambient_Key_Directional"));
    KeySun->SetupAttachment(ModuleRoot);
    KeySun->SetRelativeRotation(FRotator(-52.0f, -35.0f, 0.0f));
    KeySun->SetIntensity(3.0f);
    KeySun->SetLightColor(FLinearColor(0.65f, 0.73f, 0.88f));
    KeySun->SetMobility(EComponentMobility::Movable);
    StageLights.Add(KeySun);

    static const FVector PointLocations[5] =
    {
        FVector(-50.0f, 0.0f, 820.0f),
        FVector(350.0f, -520.0f, 650.0f),
        FVector(350.0f, 520.0f, 650.0f),
        FVector(650.0f, -760.0f, 720.0f),
        FVector(520.0f, 760.0f, 680.0f)
    };
    static const FLinearColor PointColors[5] =
    {
        FLinearColor(0.24f, 0.58f, 1.0f),
        FLinearColor(0.10f, 0.62f, 1.0f),
        FLinearColor(0.05f, 0.48f, 1.0f),
        FLinearColor(1.0f, 0.08f, 0.04f),
        FLinearColor(1.0f, 0.65f, 0.12f)
    };
    for (int32 Index = 0; Index < 5; ++Index)
    {
        UPointLightComponent* Point = CreateDefaultSubobject<UPointLightComponent>(
            *FString::Printf(TEXT("StageAccent_Point_%02d"), Index + 1));
        Point->SetupAttachment(ModuleRoot);
        Point->SetRelativeLocation(PointLocations[Index]);
        Point->SetIntensityUnits(ELightUnits::Lumens);
        Point->SetIntensity(4200.0f);
        Point->SetCastShadows(false);
        Point->SetAttenuationRadius(1250.0f);
        Point->SetLightColor(PointColors[Index]);
        Point->SetMobility(EComponentMobility::Movable);
        StageLights.Add(Point);
    }

    static const FVector SpotLocations[4] =
    {
        FVector(-450.0f, -620.0f, 850.0f),
        FVector(-450.0f, 620.0f, 850.0f),
        FVector(450.0f, -600.0f, 900.0f),
        FVector(450.0f, 600.0f, 900.0f)
    };
    const FVector SpotTarget(250.0f, 0.0f, 100.0f);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        USpotLightComponent* Spot = CreateDefaultSubobject<USpotLightComponent>(
            *FString::Printf(TEXT("OverheadSpot_%02d"), Index + 1));
        Spot->SetupAttachment(ModuleRoot);
        Spot->SetRelativeLocation(SpotLocations[Index]);
        Spot->SetRelativeRotation((SpotTarget - SpotLocations[Index]).Rotation());
        Spot->SetIntensityUnits(ELightUnits::Lumens);
        Spot->SetIntensity(10000.0f);
        Spot->SetSourceRadius(28.0f);
        Spot->SetVolumetricScatteringIntensity(0.65f);
        Spot->SetAttenuationRadius(1800.0f);
        Spot->SetInnerConeAngle(18.0f);
        Spot->SetOuterConeAngle(34.0f);
        Spot->SetLightColor(Index < 2 ? FLinearColor(0.68f, 0.82f, 1.0f) : FLinearColor(1.0f, 0.76f, 0.35f));
        Spot->SetMobility(EComponentMobility::Movable);
        StageLights.Add(Spot);
    }

    URectLightComponent* ModelWash = CreateDefaultSubobject<URectLightComponent>(TEXT("ModelStaircase_RectWash"));
    ModelWash->SetupAttachment(ModuleRoot);
    ModelWash->SetRelativeLocation(FVector(210.0f, 0.0f, 580.0f));
    ModelWash->SetRelativeRotation(FRotator(-35.0f, 0.0f, 0.0f));
    ModelWash->SetIntensityUnits(ELightUnits::Lumens);
    ModelWash->SetIntensity(15000.0f);
    ModelWash->SetAttenuationRadius(1600.0f);
    ModelWash->SetSourceWidth(1000.0f);
    ModelWash->SetSourceHeight(250.0f);
    ModelWash->SetLightColor(FLinearColor(0.8f, 0.86f, 1.0f));
    ModelWash->SetMobility(EComponentMobility::Movable);
    StageLights.Add(ModelWash);
}

void AStageLightingModule::ApplyLightingCue(FName CueName)
{
    const bool Banker = CueName == TEXT("Banker");
    const bool Deal = CueName == TEXT("Deal");
    CueColors.Reset(); CueIntensities.Reset();
    const FLinearColor Colors[] = {
        {0.65f,0.73f,0.88f}, {0.3f,0.58f,1.f}, {0.12f,0.48f,1.f}, {0.12f,0.48f,1.f},
        {1.f,0.14f,0.05f}, {1.f,0.72f,0.4f}, {0.85f,0.9f,1.f}, {0.85f,0.9f,1.f},
        {1.f,0.78f,0.5f}, {1.f,0.78f,0.5f}, {0.8f,0.86f,1.f}
    };
    for (int32 I=0; I<StageLights.Num(); ++I)
    {
        // Keep white key and display lighting stable; only decorative accents change hue.
        const bool Accent = I>=1 && I<=4;
        CueColors.Add(Accent && Banker ? FLinearColor(0.65f,0.04f,0.025f)
            : Accent && Deal ? FLinearColor(1.f,0.55f,0.12f) : Colors[I]);
        const float Base = I==0 ? 3.f : I<=5 ? 4200.f : I<=9 ? 10000.f : 15000.f;
        CueIntensities.Add(Base * (Banker && Accent ? 0.7f : 1.f));
    }
}

void AStageLightingModule::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (int32 I=0; I<StageLights.Num() && I<CueColors.Num(); ++I)
    {
        ULightComponent* L=StageLights[I];
        L->SetLightColor(FMath::Lerp(L->GetLightColor(),CueColors[I], FMath::Clamp(DeltaSeconds*2.f,0.f,1.f)));
        L->SetIntensity(FMath::FInterpTo(L->Intensity,CueIntensities[I],DeltaSeconds,2.f));
    }
}

AStageCameraRig::AStageCameraRig()
{
    static const TCHAR* CameraNames[4] =
    {
        TEXT("CAM_01_Wide_Master"),
        TEXT("CAM_02_Central_Table"),
        TEXT("CAM_03_Model_Staircase"),
        TEXT("CAM_04_Amount_Board")
    };
    static const FVector CameraLocations[4] =
    {
        FVector(-2350.0f, 0.0f, 1050.0f),
        FVector(-760.0f, 360.0f, 310.0f),
        FVector(-960.0f, -65.0f, 575.0f),
        FVector(-500.0f, 0.0f, 500.0f)
    };
    static const FVector CameraTargets[4] =
    {
        FVector(300.0f, 0.0f, 255.0f),
        FVector(-80.0f, 0.0f, 120.0f),
        FVector(490.0f, 0.0f, 230.0f),
        FVector(420.0f, 760.0f, 350.0f)
    };
    static const float CameraFov[4] = { 69.0f, 52.0f, 51.0f, 80.0f };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        UCameraComponent* Camera = CreateDefaultSubobject<UCameraComponent>(CameraNames[Index]);
        Camera->SetupAttachment(ModuleRoot);
        Camera->SetRelativeLocation(CameraLocations[Index]);
        Camera->SetRelativeRotation((CameraTargets[Index] - CameraLocations[Index]).Rotation());
        Camera->SetFieldOfView(CameraFov[Index]);
        Camera->SetAspectRatio(16.0f / 9.0f);
        Camera->bConstrainAspectRatio = false;
        Camera->SetActive(Index == 0);
        StageCameras.Add(Camera);

        UTextRenderComponent* Marker = CreateDefaultSubobject<UTextRenderComponent>(
            *FString::Printf(TEXT("CameraMarker_%02d"), Index + 1));
        DealStageVisuals::SetupText(Marker, ModuleRoot, FString::Printf(TEXT("CAM %d"), Index + 1),
            CameraLocations[Index] + FVector(0.0f, 0.0f, 45.0f), 24.0f,
            FLinearColor(0.25f, 1.0f, 0.35f),
            (FVector::ZeroVector - CameraLocations[Index]).Rotation());
        Marker->SetVisibility(false);
        CameraMarkers.Add(Marker);
    }
}

void AStageCameraRig::ActivateCamera(int32 CameraIndex)
{
    if (!StageCameras.IsValidIndex(CameraIndex))
    {
        return;
    }
    if (ActiveCameraIndex == 3 && CameraIndex != 3)
        BoardSelectionHoldSeconds = 0.8f;
    for (int32 Index = 0; Index < StageCameras.Num(); ++Index)
    {
        StageCameras[Index]->SetActive(Index == CameraIndex);
    }
    ActiveCameraIndex = CameraIndex;
}

void AStageCameraRig::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    // Opt-in inspection shots for rendered geometry regression captures.
    int32 InspectionSide=0;
    FParse::Value(FCommandLine::Get(),TEXT("StageInspectSide="),InspectionSide);
    if (InspectionSide==1 || InspectionSide==-1)
    {
        OutResult.Location=FVector(-300.f,InspectionSide*950.f,420.f);
        OutResult.Rotation=(FVector(440.f,InspectionSide*580.f,145.f)-OutResult.Location).Rotation();
        OutResult.FOV=55.f;
        return;
    }
    BoardSelectionHoldSeconds = FMath::Max(0.f, BoardSelectionHoldSeconds - DeltaTime);
    FMinimalViewInfo Target;
    StageCameras[ActiveCameraIndex]->GetCameraView(DeltaTime,Target);
    if (!bViewInitialized) { BlendedView=Target; bViewInitialized=true; }
    const float Alpha=1.f-FMath::Exp(-DeltaTime*5.f);
    BlendedView.Location=FMath::Lerp(BlendedView.Location,Target.Location,Alpha);
    BlendedView.Rotation=FMath::Lerp(BlendedView.Rotation.Quaternion(),Target.Rotation.Quaternion(),Alpha).Rotator();
    BlendedView.FOV=FMath::Lerp(BlendedView.FOV,Target.FOV,Alpha);
    OutResult=Target;
    OutResult.Location=BlendedView.Location; OutResult.Rotation=BlendedView.Rotation; OutResult.FOV=BlendedView.FOV;
}

AStageInteractionDirector::AStageInteractionDirector()
{
    Tags.Add(TEXT("Stage.InteractionDirector"));
    CasesPerRound = { 6, 5, 4, 3, 2, 1, 1, 1, 1 };
    PrizeValuesCents =
    {
        1, 100, 500, 1000, 2500, 5000, 7500,
        10000, 20000, 30000, 40000, 50000, 75000,
        100000, 500000, 1000000, 2500000, 5000000,
        7500000, 10000000, 20000000, 30000000, 40000000,
        50000000, 75000000, 100000000
    };
}

void AStageInteractionDirector::BeginPlay()
{
    Super::BeginPlay();
    bAutomated = FParse::Param(FCommandLine::Get(),TEXT("DealAutoTest")) ||
        FParse::Param(FCommandLine::Get(),TEXT("DealAutoAcceptTest")) ||
        FParse::Param(FCommandLine::Get(),TEXT("DealExperienceTest")) ||
        FParse::Param(FCommandLine::Get(),TEXT("DealOfferPreview"));
    InitializeGame();
    if (FParse::Param(FCommandLine::Get(),TEXT("DealAutoTest"))) RunAutomatedGameTest();
    else if (FParse::Param(FCommandLine::Get(),TEXT("DealAutoAcceptTest"))) RunAutomatedDealAcceptTest();
    else if (FParse::Param(FCommandLine::Get(),TEXT("DealExperienceTest"))) RunExperienceTest();
    else if (FParse::Param(FCommandLine::Get(),TEXT("DealOfferPreview")))
    {
        ConfirmSelection();
        for (int32 I=0;I<6;++I) ConfirmSelection();
    }
    else if (FParse::Param(FCommandLine::Get(),TEXT("DealRevealPreview")))
    { ConfirmSelection(); ConfirmSelection(); }
    else if (!FParse::Param(FCommandLine::Get(),TEXT("DealSkipWelcome")))
    { GamePhase=EDealGamePhase::Welcome; FocusCamera(0); }

    if (FParse::Param(FCommandLine::Get(),TEXT("DealUIValidation")))
        GetWorldTimerManager().SetTimer(UIValidationTimer,this,&AStageInteractionDirector::RunUIValidationStep,3.f,true);
    else if (FParse::Param(FCommandLine::Get(),TEXT("DealBoardLayoutTest")))
        GetWorldTimerManager().SetTimer(UIValidationTimer,this,&AStageInteractionDirector::RunBoardLayoutValidationStep,2.f,true);

    if (FParse::Param(FCommandLine::Get(),TEXT("DealCapturePreview")))
    {
        FString Name=TEXT("StudioPreview");
        FParse::Value(FCommandLine::Get(),TEXT("CaptureName="),Name);
        const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/Windows"),Name+TEXT(".png"));
        float Delay=5.f;
        FParse::Value(FCommandLine::Get(),TEXT("CaptureDelay="),Delay);
        GetWorldTimerManager().SetTimer(PreviewCaptureTimer,FTimerDelegate::CreateWeakLambda(this,[Path]()
        { FScreenshotRequest::RequestScreenshot(Path,true,false,false); }),Delay,false);
    }
}

void AStageInteractionDirector::InitializeGame()
{
    GetWorldTimerManager().ClearTimer(CaseRevealTimer);
    GetWorldTimerManager().ClearTimer(BankerCallTimer);
    bMenuOpen=false; bConfirmingDeal=false; bSelectionArmed=false;
    OfferHistory.Reset();
    Briefcases.Reset();
    for (int32 Number = 1; Number <= 26; ++Number)
    {
        FBriefcaseRuntimeState State;
        State.BriefcaseNumber = Number;
        Briefcases.Add(State);
    }

    AssignPrizeValues();
    GamePhase = EDealGamePhase::ChoosePlayerCase;
    HighlightedBriefcase = 1;
    PlayerBriefcase = INDEX_NONE;
    RoundIndex = 0;
    CasesOpenedThisRound = 0;
    BankerOffer = 0;
    AcceptedOffer = 0;
    LastOpenedBriefcase = INDEX_NONE;
    LastRevealedAmount = 0;
    RevealStartedAtSeconds = 0.0f;
    LastMessage = TEXT("Choose the briefcase you want to keep until the end.");
    ResultHeadline.Empty();
    ResultDetail.Empty();
    TriggerLightingCue(TEXT("Default"));
    RefreshStageVisuals();
    FocusCamera(2);
}

void AStageInteractionDirector::AssignPrizeValues()
{
    TArray<int64> ShuffledValues = PrizeValuesCents;
    int32 Seed = static_cast<int32>(FPlatformTime::Cycles64() & 0x7fffffff);
    FParse::Value(FCommandLine::Get(),TEXT("DealSeed="),Seed);
    FRandomStream Random(Seed);
    for (int32 Index = ShuffledValues.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex = Random.RandRange(0, Index);
        ShuffledValues.Swap(Index, SwapIndex);
    }

    for (int32 Index = 0; Index < Briefcases.Num() && Index < ShuffledValues.Num(); ++Index)
    {
        Briefcases[Index].AmountIndex = PrizeValuesCents.IndexOfByKey(ShuffledValues[Index]);
        Briefcases[Index].AmountCents = ShuffledValues[Index];
    }
}

void AStageInteractionDirector::SelectBriefcase(int32 BriefcaseNumber)
{
    if ((GamePhase != EDealGamePhase::ChoosePlayerCase && GamePhase != EDealGamePhase::OpenCases) ||
        BriefcaseNumber < 1 || BriefcaseNumber > Briefcases.Num())
    {
        return;
    }
    const FBriefcaseRuntimeState& TargetState = Briefcases[BriefcaseNumber - 1];
    if (TargetState.bIsOpened || (GamePhase == EDealGamePhase::OpenCases && TargetState.bIsPlayerCase))
    {
        return;
    }
    if (bMenuOpen) return;
    if (HighlightedBriefcase != BriefcaseNumber) bSelectionArmed=false;
    HighlightedBriefcase = BriefcaseNumber;
    for (FBriefcaseRuntimeState& State : Briefcases)
    {
        State.bIsSelected = State.BriefcaseNumber == HighlightedBriefcase;
    }
    RefreshStageVisuals();
}

void AStageInteractionDirector::MoveSelection(int32 Direction)
{
    if (bMenuOpen) return;
    bSelectionArmed=false;
    if (GamePhase != EDealGamePhase::ChoosePlayerCase && GamePhase != EDealGamePhase::OpenCases)
    {
        return;
    }
    HighlightedBriefcase = FindNextAvailableCase(HighlightedBriefcase, Direction >= 0 ? 1 : -1);
    for (FBriefcaseRuntimeState& State : Briefcases)
    {
        State.bIsSelected = State.BriefcaseNumber == HighlightedBriefcase;
    }
    RefreshStageVisuals();
}

int32 AStageInteractionDirector::FindNextAvailableCase(int32 StartNumber, int32 Direction) const
{
    const int32 Step = Direction >= 0 ? 1 : -1;
    int32 Candidate = StartNumber;
    for (int32 Attempt = 0; Attempt < Briefcases.Num(); ++Attempt)
    {
        Candidate += Step;
        if (Candidate > Briefcases.Num())
        {
            Candidate = 1;
        }
        else if (Candidate < 1)
        {
            Candidate = Briefcases.Num();
        }

        const FBriefcaseRuntimeState& State = Briefcases[Candidate - 1];
        const bool bSelectable = !State.bIsOpened &&
            (GamePhase == EDealGamePhase::ChoosePlayerCase || !State.bIsPlayerCase);
        if (bSelectable)
        {
            return Candidate;
        }
    }
    return StartNumber;
}

void AStageInteractionDirector::ConfirmSelection()
{
    if (bMenuOpen) return;
    if (GamePhase==EDealGamePhase::Welcome) { StartGame(); return; }
    if (GamePhase==EDealGamePhase::BankerCalling) { AnswerBanker(); return; }
    if (GamePhase==EDealGamePhase::RevealAmount)
    { if (GetRevealElapsedSeconds()>.45f) CompleteCaseReveal(); return; }
    bSelectionArmed=false;
    if (GamePhase == EDealGamePhase::ChoosePlayerCase)
    {
        PlayCue(TEXT("Select"));
        PlayerBriefcase = HighlightedBriefcase;
        FBriefcaseRuntimeState& PlayerState = Briefcases[PlayerBriefcase - 1];
        PlayerState.bIsPlayerCase = true;
        PlayerState.bIsSelected = false;
        GamePhase = EDealGamePhase::OpenCases;
        RoundIndex = 0;
        CasesOpenedThisRound = 0;
        LastMessage = FString::Printf(TEXT("Briefcase #%d is yours. Now open %d other cases."),
            PlayerBriefcase, GetCasesToOpenThisRound());
        HighlightedBriefcase = FindNextAvailableCase(PlayerBriefcase, 1);
        RefreshStageVisuals();
        FocusCamera(2);
        UE_LOG(LogTemp, Display, TEXT("[DealGame] Player selected case #%d"), PlayerBriefcase);
    }
    else if (GamePhase == EDealGamePhase::OpenCases)
    {
        OpenHighlightedCase();
    }
}

void AStageInteractionDirector::OpenHighlightedCase()
{
    const int32 Index = HighlightedBriefcase - 1;
    if (!Briefcases.IsValidIndex(Index))
    {
        return;
    }

    FBriefcaseRuntimeState& State = Briefcases[Index];
    if (State.bIsOpened || State.bIsPlayerCase)
    {
        HighlightedBriefcase = FindNextAvailableCase(HighlightedBriefcase, 1);
        RefreshStageVisuals();
        return;
    }

    PlayCue(TEXT("Reveal"));
    State.bIsOpened = true;
    State.bIsSelected = false;
    LastOpenedBriefcase = State.BriefcaseNumber;
    LastRevealedAmount = State.AmountCents;
    ++CasesOpenedThisRound;
    LastMessage = FString::Printf(TEXT("Briefcase #%d contained %s."),
        State.BriefcaseNumber, *FormatCurrency(State.AmountCents));
    OpenBriefcase(State.BriefcaseNumber, State.AmountIndex);
    UE_LOG(LogTemp, Display, TEXT("[DealGame] Opened case #%d: %s"),
        State.BriefcaseNumber, *FormatCurrency(State.AmountCents));

    GamePhase = EDealGamePhase::RevealAmount;
    RevealStartedAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    RefreshStageVisuals();
    const bool bSkipRevealDelay = bAutomated;
    if (bSkipRevealDelay)
    {
        CompleteCaseReveal();
    }
    else
    {
        GetWorldTimerManager().SetTimer(CaseRevealTimer, this,
            &AStageInteractionDirector::CompleteCaseReveal, bFastPace ? 1.1f : CaseRevealDurationSeconds, false);
    }
}

void AStageInteractionDirector::CompleteCaseReveal()
{
    if (GamePhase != EDealGamePhase::RevealAmount)
    {
        return;
    }
    GetWorldTimerManager().ClearTimer(CaseRevealTimer);
    if (CasesOpenedThisRound >= GetCasesToOpenThisRound() || GetCasesRemaining() <= 2)
    {
        BeginBankerOffer();
    }
    else
    {
        GamePhase = EDealGamePhase::OpenCases;
        HighlightedBriefcase = FindNextAvailableCase(HighlightedBriefcase, 1);
        RefreshStageVisuals();
        FocusCamera(2);
    }
}

void AStageInteractionDirector::BeginBankerOffer()
{
    GamePhase=EDealGamePhase::BankerCalling;
    bConfirmingDeal=false;
    LastMessage=TEXT("The Banker is calling. Answer the phone to hear the offer.");
    TriggerLightingCue(TEXT("Banker"));
    FocusCamera(1);
    PlayCue(TEXT("Ring"));
    if (bAutomated) AnswerBanker();
    else GetWorldTimerManager().SetTimer(BankerCallTimer,this,&AStageInteractionDirector::AnswerBanker,
        bFastPace ? 1.2f : 3.2f,false);
}

void AStageInteractionDirector::AnswerBanker()
{
    if (bMenuOpen || GamePhase!=EDealGamePhase::BankerCalling) return;
    GetWorldTimerManager().ClearTimer(BankerCallTimer);
    BankerOffer=CalculateBankerOffer();
    OfferHistory.Add(BankerOffer);
    GamePhase=EDealGamePhase::BankerOffer;
    LastMessage=TEXT("Take the guaranteed offer or continue opening cases.");
    FocusCamera(3);
    UE_LOG(LogTemp,Display,TEXT("[DealGame] Banker offer %d: %s with %d cases remaining"),
        RoundIndex+1,*GetBankerOfferText(),GetCasesRemaining());
}

int64 AStageInteractionDirector::CalculateBankerOffer() const
{
    int64 RemainingTotal = 0;
    int32 RemainingCount = 0;
    for (const FBriefcaseRuntimeState& State : Briefcases)
    {
        if (!State.bIsOpened)
        {
            RemainingTotal += State.AmountCents;
            ++RemainingCount;
        }
    }
    if (RemainingCount == 0)
    {
        return 0;
    }

    const double Average = static_cast<double>(RemainingTotal) / static_cast<double>(RemainingCount);
    const double Multiplier = FMath::Min(0.20 + 0.085 * static_cast<double>(RoundIndex), 0.90);
    const double RawOffer = Average * Multiplier;
    const int64 RoundingUnit = RawOffer >= 100000.0 ? 10000 : 100; // $100 or $1, expressed in cents.
    const int64 RoundedOffer = static_cast<int64>(FMath::RoundToDouble(RawOffer / static_cast<double>(RoundingUnit))) * RoundingUnit;
    return FMath::Max<int64>(100, RoundedOffer);
}

void AStageInteractionDirector::AcceptDeal()
{
    if (bMenuOpen || GamePhase != EDealGamePhase::BankerOffer)
    {
        return;
    }
    if (!bAutomated && !bConfirmingDeal) { bConfirmingDeal=true; return; }
    bConfirmingDeal=false;
    PlayCue(TEXT("Deal"));
    AcceptedOffer = BankerOffer;
    TriggerLightingCue(TEXT("Deal"));
    RevealFinalCase(true);
}

void AStageInteractionDirector::RejectDeal()
{
    if (bMenuOpen || GamePhase != EDealGamePhase::BankerOffer)
    {
        return;
    }

    bConfirmingDeal=false;
    TriggerLightingCue(TEXT("NoDeal"));
    if (GetCasesRemaining() <= 2)
    {
        GamePhase=EDealGamePhase::FinalChoice;
        LastMessage=TEXT("Two cases remain. Keep your original case or swap for the other one.");
        FocusCamera(2);
        return;
    }

    RoundIndex = FMath::Min(RoundIndex + 1, CasesPerRound.Num() - 1);
    CasesOpenedThisRound = 0;
    BankerOffer = 0;
    GamePhase = EDealGamePhase::OpenCases;
    HighlightedBriefcase = FindNextAvailableCase(HighlightedBriefcase, 1);
    LastMessage = FString::Printf(TEXT("NO DEAL. Open %d more case%s."),
        GetCasesToOpenThisRound(), GetCasesToOpenThisRound() == 1 ? TEXT("") : TEXT("s"));
    RefreshStageVisuals();
    FocusCamera(2);
}

void AStageInteractionDirector::RevealFinalCase(bool bAcceptedDeal)
{
    if (!Briefcases.IsValidIndex(PlayerBriefcase - 1))
    {
        return;
    }

    const FBriefcaseRuntimeState& PlayerState = Briefcases[PlayerBriefcase - 1];
    int32 OtherCaseNumber = INDEX_NONE;
    int64 OtherCaseAmount = 0;
    for (const FBriefcaseRuntimeState& State : Briefcases)
    {
        if (!State.bIsOpened && !State.bIsPlayerCase)
        {
            OtherCaseNumber = State.BriefcaseNumber;
            OtherCaseAmount = State.AmountCents;
            break;
        }
    }

    GamePhase = EDealGamePhase::GameOver;
    LastRevealedAmount = PlayerState.AmountCents;
    ResultHeadline = bAcceptedDeal ? TEXT("DEAL ACCEPTED") : TEXT("FINAL REVEAL");
    if (bAcceptedDeal)
    {
        ResultDetail = FString::Printf(TEXT("You accepted %s. Your case #%d held %s."),
            *FormatCurrency(AcceptedOffer), PlayerBriefcase, *FormatCurrency(PlayerState.AmountCents));
    }
    else if (OtherCaseNumber != INDEX_NONE)
    {
        ResultDetail = FString::Printf(TEXT("Your case #%d held %s. Case #%d held %s."),
            PlayerBriefcase, *FormatCurrency(PlayerState.AmountCents),
            OtherCaseNumber, *FormatCurrency(OtherCaseAmount));
    }
    else
    {
        ResultDetail = FString::Printf(TEXT("Your case #%d held %s."),
            PlayerBriefcase, *FormatCurrency(PlayerState.AmountCents));
    }
    LastMessage = ResultDetail;
    FocusCamera(0);
    RefreshStageVisuals();
    TArray<AActor*> Stairs;
    UGameplayStatics::GetAllActorsOfClass(this, AStageStaircaseModule::StaticClass(), Stairs);
    for (AActor* Actor : Stairs)
        CastChecked<AStageStaircaseModule>(Actor)->SetBriefcaseOpened(PlayerBriefcase, true);
    UE_LOG(LogTemp, Display, TEXT("[DealGame] %s %s"), *ResultHeadline, *ResultDetail);
}

void AStageInteractionDirector::RestartGame()
{
    if (GamePhase!=EDealGamePhase::GameOver && !bMenuOpen) return;
    InitializeGame();
    UE_LOG(LogTemp, Display, TEXT("[DealGame] New game started"));
}

int32 AStageInteractionDirector::GetCasesRemaining() const
{
    int32 Count = 0;
    for (const FBriefcaseRuntimeState& State : Briefcases)
    {
        if (!State.bIsOpened)
        {
            ++Count;
        }
    }
    return Count;
}

int32 AStageInteractionDirector::GetCasesToOpenThisRound() const
{
    return CasesPerRound.IsValidIndex(RoundIndex) ? CasesPerRound[RoundIndex] : 1;
}

FString AStageInteractionDirector::GetPhaseTitle() const
{
    switch (GamePhase)
    {
        case EDealGamePhase::Welcome: return TEXT("WELCOME TO THE STUDIO");
        case EDealGamePhase::BankerCalling: return TEXT("INCOMING CALL");
        case EDealGamePhase::FinalChoice: return TEXT("THE FINAL TWO CASES");
        case EDealGamePhase::ChoosePlayerCase: return TEXT("CHOOSE YOUR CASE");
        case EDealGamePhase::OpenCases: return FString::Printf(TEXT("ROUND %d - OPEN CASES"), RoundIndex + 1);
        case EDealGamePhase::RevealAmount: return TEXT("CASE REVEALED");
        case EDealGamePhase::BankerOffer: return TEXT("DEAL OR NO DEAL?");
        case EDealGamePhase::GameOver: return ResultHeadline;
        default: return TEXT("DEAL OR NO DEAL");
    }
}

FString AStageInteractionDirector::GetInstructionText() const
{
    switch (GamePhase)
    {
        case EDealGamePhase::ChoosePlayerCase:
            return TEXT("SELECT A CASE, THEN CONFIRM     ARROW KEYS + ENTER ALSO WORK");
        case EDealGamePhase::OpenCases:
            return FString::Printf(TEXT("SELECT THEN OPEN     %d OF %d OPENED THIS ROUND"),
                CasesOpenedThisRound, GetCasesToOpenThisRound());
        case EDealGamePhase::RevealAmount:
            return TEXT("AMOUNT REMOVED FROM THE BOARD     SPACE: CONTINUE");
        case EDealGamePhase::BankerOffer:
            return TEXT("CLICK DEAL OR NO DEAL     KEYBOARD: D / N");
        case EDealGamePhase::GameOver:
            return TEXT("CLICK PLAY AGAIN OR PRESS R     1-4: CAMERA VIEWS");
        default:
            return FString();
    }
}

FString AStageInteractionDirector::GetBankerOfferText() const
{
    return FormatCurrency(BankerOffer);
}

FString AStageInteractionDirector::GetPlayerCaseValueText() const
{
    if (!Briefcases.IsValidIndex(PlayerBriefcase - 1))
    {
        return TEXT("NOT SELECTED");
    }
    return GamePhase == EDealGamePhase::GameOver
        ? FormatCurrency(Briefcases[PlayerBriefcase - 1].AmountCents)
        : TEXT("SEALED");
}

float AStageInteractionDirector::GetRevealElapsedSeconds() const
{
    if (GetWorld() && GetWorld()->GetTimerManager().TimerExists(CaseRevealTimer))
        return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerElapsed(CaseRevealTimer));
    return GetWorld() ? FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - RevealStartedAtSeconds) : 0.0f;
}

FString AStageInteractionDirector::FormatCurrency(int64 AmountCents)
{
    const int64 Dollars = AmountCents / 100;
    const int32 Cents = static_cast<int32>(FMath::Abs(AmountCents % 100));
    const FString DollarText = FText::AsNumber(Dollars).ToString();
    if (Cents != 0)
    {
        return FString::Printf(TEXT("$%s.%02d"), *DollarText, Cents);
    }
    return FString::Printf(TEXT("$%s"), *DollarText);
}

void AStageInteractionDirector::RefreshStageVisuals()
{
    TArray<AActor*> Staircases;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageStaircaseModule::StaticClass(), Staircases);
    for (AActor* Actor : Staircases)
    {
        if (AStageStaircaseModule* Staircase = Cast<AStageStaircaseModule>(Actor))
        {
            for (const FBriefcaseRuntimeState& State : Briefcases)
            {
                Staircase->SetBriefcaseOpened(State.BriefcaseNumber, State.bIsOpened);
                Staircase->SetBriefcaseHighlight(State.BriefcaseNumber, false);
                Staircase->SetBriefcasePlayerCase(State.BriefcaseNumber, State.bIsPlayerCase);
            }
            if (GamePhase == EDealGamePhase::ChoosePlayerCase || GamePhase == EDealGamePhase::OpenCases)
            {
                Staircase->SetBriefcaseHighlight(HighlightedBriefcase, true);
            }
        }
    }

    TArray<AActor*> Boards;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageAmountBoardModule::StaticClass(), Boards);
    for (AActor* Actor : Boards)
    {
        if (AStageAmountBoardModule* Board = Cast<AStageAmountBoardModule>(Actor))
        {
            for (int32 AmountIndex = 0; AmountIndex < PrizeValuesCents.Num(); ++AmountIndex)
            {
                Board->SetAmountActive(AmountIndex, true);
            }
            for (const FBriefcaseRuntimeState& State : Briefcases)
            {
                if (State.bIsOpened && State.AmountIndex != INDEX_NONE)
                {
                    Board->SetAmountActive(State.AmountIndex, false);
                }
            }
        }
    }
}

void AStageInteractionDirector::FocusCamera(int32 CameraIndex)
{
    if (!bAutoCamera) return;
    if (ADealStagePlayerController* Controller = Cast<ADealStagePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
    {
        Controller->SwitchToCamera(CameraIndex);
    }
}

void AStageInteractionDirector::RunAutomatedGameTest()
{
    UE_LOG(LogTemp, Display, TEXT("[DealGameAutoTest] Starting full no-deal test game"));
    ConfirmSelection();
    int32 Guard = 0;
    while (GamePhase != EDealGamePhase::GameOver && Guard++ < 100)
    {
        if (GamePhase == EDealGamePhase::OpenCases)
        {
            ConfirmSelection();
        }
        else if (GamePhase == EDealGamePhase::BankerOffer)
        {
            RejectDeal();
        }
        else if (GamePhase == EDealGamePhase::FinalChoice) ChooseFinalCase(false);
    }
    UE_LOG(LogTemp, Display, TEXT("[DealGameAutoTest] Completed=%s Steps=%d Remaining=%d Result=%s"),
        GamePhase == EDealGamePhase::GameOver ? TEXT("true") : TEXT("false"),
        Guard, GetCasesRemaining(), *ResultDetail);
}

void AStageInteractionDirector::RunAutomatedDealAcceptTest()
{
    UE_LOG(LogTemp, Display, TEXT("[DealGameAutoAcceptTest] Starting first-offer deal test"));
    ConfirmSelection();
    int32 Guard = 0;
    while (GamePhase == EDealGamePhase::OpenCases && Guard++ < 30)
    {
        ConfirmSelection();
    }
    if (GamePhase == EDealGamePhase::BankerOffer)
    {
        AcceptDeal();
    }
    UE_LOG(LogTemp, Display, TEXT("[DealGameAutoAcceptTest] Completed=%s Steps=%d Result=%s"),
        GamePhase == EDealGamePhase::GameOver ? TEXT("true") : TEXT("false"),
        Guard, *ResultDetail);
}

void AStageInteractionDirector::OpenBriefcase(int32 BriefcaseNumber, int32 AmountIndex)
{
    const int32 BriefcaseIndex = BriefcaseNumber - 1;
    if (Briefcases.IsValidIndex(BriefcaseIndex))
    {
        Briefcases[BriefcaseIndex].bIsOpened = true;
    }

    TArray<AActor*> Staircases;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageStaircaseModule::StaticClass(), Staircases);
    for (AActor* Actor : Staircases)
    {
        if (AStageStaircaseModule* Staircase = Cast<AStageStaircaseModule>(Actor))
        {
            Staircase->SetBriefcaseOpened(BriefcaseNumber, true);
        }
    }

    TArray<AActor*> Boards;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageAmountBoardModule::StaticClass(), Boards);
    for (AActor* Actor : Boards)
    {
        if (AStageAmountBoardModule* Board = Cast<AStageAmountBoardModule>(Actor))
        {
            Board->SetAmountActive(AmountIndex, false);
        }
    }
}

void AStageInteractionDirector::SetBankerOffer(int64 NewOfferCents)
{
    BankerOffer = FMath::Max<int64>(0, NewOfferCents);
}

void AStageInteractionDirector::TriggerLightingCue(FName CueName)
{
    TArray<AActor*> LightingActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageLightingModule::StaticClass(), LightingActors);
    for (AActor* Actor : LightingActors)
    {
        if (AStageLightingModule* Lighting = Cast<AStageLightingModule>(Actor))
        {
            Lighting->ApplyLightingCue(CueName);
        }
    }
}

ADealStageSet::ADealStageSet()
{
    auto CreateModule = [this](const TCHAR* ComponentName, TSubclassOf<AActor> ModuleClass, const FVector& Location)
    {
        UChildActorComponent* Component = CreateDefaultSubobject<UChildActorComponent>(ComponentName);
        Component->SetupAttachment(ModuleRoot);
        Component->SetChildActorClass(ModuleClass);
        Component->SetRelativeLocation(Location);
        return Component;
    };

    WorldShell = CreateModule(TEXT("00_WorldShell"), AStageWorldShellModule::StaticClass(), FVector::ZeroVector);
    CentralPlatform = CreateModule(TEXT("01_CentralPlatform"), AStagePlatformModule::StaticClass(), FVector(-100.0f, 0.0f, 0.0f));
    ModelStaircase = CreateModule(TEXT("02_ModelStaircase_6_7_7_6"), AStageStaircaseModule::StaticClass(), FVector::ZeroVector);
    AmountBoard = CreateModule(TEXT("03_AmountBoard_26Values"), AStageAmountBoardModule::StaticClass(), FVector(420.0f, 895.0f, 0.0f));
    CityBackdrop = CreateModule(TEXT("04_CityBackdrop_GrandArch"), AStageBackdropModule::StaticClass(), FVector(820.0f, 0.0f, 0.0f));
    BankerBooth = CreateModule(TEXT("05_BankerHighBooth"), AStageBankerBoothModule::StaticClass(), FVector(540.0f, -845.0f, 0.0f));
    Audience = CreateModule(TEXT("06_Audience_UShape"), AStageAudienceModule::StaticClass(), FVector::ZeroVector);
    LightingRig = CreateModule(TEXT("07_LightingRig"), AStageLightingModule::StaticClass(), FVector::ZeroVector);
    CameraRig = CreateModule(TEXT("08_CameraRig_4Shots"), AStageCameraRig::StaticClass(), FVector::ZeroVector);
    InteractionDirector = CreateModule(TEXT("09_InteractionDirector"), AStageInteractionDirector::StaticClass(), FVector::ZeroVector);

    Tags.Add(TEXT("Stage.MasterAssembly"));
}

void ADealStagePlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void ADealStagePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent)
    {
        return;
    }
    InputComponent->BindAction(TEXT("Camera1"), IE_Pressed, this, &ADealStagePlayerController::Camera1);
    InputComponent->BindAction(TEXT("Camera2"), IE_Pressed, this, &ADealStagePlayerController::Camera2);
    InputComponent->BindAction(TEXT("Camera3"), IE_Pressed, this, &ADealStagePlayerController::Camera3);
    InputComponent->BindAction(TEXT("Camera4"), IE_Pressed, this, &ADealStagePlayerController::Camera4);
    InputComponent->BindAction(TEXT("CycleCamera"), IE_Pressed, this, &ADealStagePlayerController::CycleCamera);
    InputComponent->BindAction(TEXT("PreviousCase"), IE_Pressed, this, &ADealStagePlayerController::PreviousCase);
    InputComponent->BindAction(TEXT("NextCase"), IE_Pressed, this, &ADealStagePlayerController::NextCase);
    InputComponent->BindAction(TEXT("ConfirmCase"), IE_Pressed, this, &ADealStagePlayerController::ConfirmCase);
    InputComponent->BindAction(TEXT("AcceptDeal"), IE_Pressed, this, &ADealStagePlayerController::Deal);
    InputComponent->BindAction(TEXT("RejectDeal"), IE_Pressed, this, &ADealStagePlayerController::NoDeal);
    InputComponent->BindAction(TEXT("RestartGame"), IE_Pressed, this, &ADealStagePlayerController::Restart);
    InputComponent->BindAction(TEXT("PrimaryClick"), IE_Pressed, this, &ADealStagePlayerController::PrimaryClick);
    InputComponent->BindAction(TEXT("QuitPrototype"), IE_Pressed, this, &ADealStagePlayerController::QuitPrototype);
}

AStageInteractionDirector* ADealStagePlayerController::GetInteractionDirector() const
{
    TArray<AActor*> Directors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageInteractionDirector::StaticClass(), Directors);
    return Directors.Num() > 0 ? Cast<AStageInteractionDirector>(Directors[0]) : nullptr;
}

void ADealStagePlayerController::PreviousCase()
{
    if (AStageInteractionDirector* Director = GetInteractionDirector())
    {
        Director->MoveSelection(-1);
    }
}

void ADealStagePlayerController::NextCase()
{
    if (AStageInteractionDirector* Director = GetInteractionDirector())
    {
        Director->MoveSelection(1);
    }
}

void ADealStagePlayerController::ConfirmCase()
{
    if (AStageInteractionDirector* Director = GetInteractionDirector())
    {
        Director->ConfirmSelection();
    }
}

void ADealStagePlayerController::Deal()
{
    if (AStageInteractionDirector* Director = GetInteractionDirector())
    {
        if (Director->GetGamePhase()==EDealGamePhase::FinalChoice) Director->ChooseFinalCase(false);
        else Director->AcceptDeal();
    }
}

void ADealStagePlayerController::NoDeal()
{
    if (AStageInteractionDirector* Director = GetInteractionDirector())
    {
        if (Director->GetGamePhase()==EDealGamePhase::FinalChoice) Director->ChooseFinalCase(true);
        else Director->RejectDeal();
    }
}

void ADealStagePlayerController::Restart()
{
    if (AStageInteractionDirector* Director = GetInteractionDirector())
    {
        Director->RestartGame();
    }
}

void ADealStagePlayerController::PrimaryClick()
{
    AStageInteractionDirector* Director = GetInteractionDirector();
    float MX=0, MY=0;
    GetMousePosition(MX,MY);
    if (GetHUD() && GetHUD()->GetHitBoxAtCoordinates(FVector2D(MX,MY),true)) return;
    if (!Director || Director->IsMenuOpen() || (Director->GetGamePhase() != EDealGamePhase::ChoosePlayerCase &&
        Director->GetGamePhase() != EDealGamePhase::OpenCases))
    {
        return;
    }

    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit) || !Hit.GetComponent())
    {
        return;
    }

    for (const FName& Tag : Hit.GetComponent()->ComponentTags)
    {
        const FString TagText = Tag.ToString();
        if (!TagText.StartsWith(TEXT("Briefcase.")))
        {
            continue;
        }

        const int32 BriefcaseNumber = FCString::Atoi(*TagText.RightChop(10));
        Director->ClickCase(BriefcaseNumber);
        return;
    }
}

void ADealStagePlayerController::QuitPrototype()
{
    if (AStageInteractionDirector* Director=GetInteractionDirector()) Director->ToggleMenu();
}

void ADealStagePlayerController::SwitchToCamera(int32 CameraIndex)
{
    TArray<AActor*> CameraRigs;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageCameraRig::StaticClass(), CameraRigs);
    if (CameraRigs.Num() == 0)
    {
        return;
    }
    if (AStageCameraRig* Rig = Cast<AStageCameraRig>(CameraRigs[0]))
    {
        Rig->ActivateCamera(CameraIndex);
        SetViewTarget(Rig);
    }

}

void ADealStagePlayerController::Camera1() { SwitchToCamera(0); }
void ADealStagePlayerController::Camera2() { SwitchToCamera(1); }
void ADealStagePlayerController::Camera3() { SwitchToCamera(2); }
void ADealStagePlayerController::Camera4() { SwitchToCamera(3); }

void ADealStagePlayerController::CycleCamera()
{
    TArray<AActor*> CameraRigs;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageCameraRig::StaticClass(), CameraRigs);
    if (CameraRigs.Num() > 0)
    {
        if (AStageCameraRig* Rig = Cast<AStageCameraRig>(CameraRigs[0]))
        {
            SwitchToCamera((Rig->GetActiveCameraIndex() + 1) % 4);
        }
    }
}

AStageInteractionDirector* ADealStageHUD::FindInteractionDirector() const
{
    if (!GetWorld())
    {
        return nullptr;
    }
    TArray<AActor*> Directors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageInteractionDirector::StaticClass(), Directors);
    return Directors.Num() > 0 ? Cast<AStageInteractionDirector>(Directors[0]) : nullptr;
}

ADealStageGameMode::ADealStageGameMode()
{
    PlayerControllerClass = ADealStagePlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    HUDClass = ADealStageHUD::StaticClass();
    bStartPlayersAsSpectators = true;
}

void ADealStageGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (ADealStagePlayerController* Controller = Cast<ADealStagePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
    {
        int32 StartupCamera = 0;
        FParse::Value(FCommandLine::Get(), TEXT("StageCamera="), StartupCamera);
        TArray<AActor*> CameraRigs;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStageCameraRig::StaticClass(), CameraRigs);
        if (CameraRigs.Num() > 0)
        {
            if (AStageCameraRig* Rig = Cast<AStageCameraRig>(CameraRigs[0]))
            {
                Rig->ActivateCamera(FMath::Clamp(StartupCamera, 0, 3));
                Controller->SetViewTarget(Rig);
            }
        }
    }
}
