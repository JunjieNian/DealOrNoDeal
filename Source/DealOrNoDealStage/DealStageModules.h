#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "DealStageModules.generated.h"

class UCameraComponent;
class UChildActorComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EDealGamePhase : uint8
{
    ChoosePlayerCase UMETA(DisplayName="Choose Player Case"),
    OpenCases UMETA(DisplayName="Open Cases"),
    BankerOffer UMETA(DisplayName="Banker Offer"),
    GameOver UMETA(DisplayName="Game Over")
};

USTRUCT(BlueprintType)
struct FBriefcaseRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal Stage|Interaction")
    int32 BriefcaseNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal Stage|Interaction")
    bool bIsSelected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal Stage|Interaction")
    bool bIsOpened = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal Stage|Interaction")
    bool bIsPlayerCase = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal Stage|Interaction")
    int32 AmountIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal Stage|Interaction")
    int64 AmountCents = 0;
};

UCLASS(Abstract)
class DEALORNODEALSTAGE_API AStageModuleBase : public AActor
{
    GENERATED_BODY()

public:
    AStageModuleBase();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Module")
    TObjectPtr<USceneComponent> ModuleRoot;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageWorldShellModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageWorldShellModule();
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|World Shell")
    TObjectPtr<UStaticMeshComponent> StudioFloor;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|World Shell")
    TObjectPtr<UStaticMeshComponent> UpstageWall;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|World Shell")
    TObjectPtr<UTextRenderComponent> ScaleLegend;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStagePlatformModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStagePlatformModule();
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Platform")
    TObjectPtr<UStaticMeshComponent> MainPlatform;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Platform")
    TObjectPtr<UStaticMeshComponent> PlatformInset;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Platform")
    TObjectPtr<UStaticMeshComponent> TableTop;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Platform")
    TObjectPtr<UStaticMeshComponent> TablePedestal;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Platform")
    TObjectPtr<UStaticMeshComponent> PhonePlaceholder;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Platform")
    TObjectPtr<UTextRenderComponent> TableLabel;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageStaircaseModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageStaircaseModule();
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void SetBriefcaseHighlight(int32 BriefcaseNumber, bool bHighlighted);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void SetBriefcasePlayerCase(int32 BriefcaseNumber, bool bIsPlayerCase);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void SetBriefcaseOpened(int32 BriefcaseNumber, bool bOpened);

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Staircase")
    TArray<TObjectPtr<UStaticMeshComponent>> StairTiers;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Staircase")
    TArray<TObjectPtr<UStaticMeshComponent>> BriefcaseMarkers;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Staircase")
    TArray<TObjectPtr<UStaticMeshComponent>> BriefcaseHandles;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Staircase")
    TArray<TObjectPtr<UTextRenderComponent>> BriefcaseLabels;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageAmountBoardModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageAmountBoardModule();
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void SetAmountActive(int32 AmountIndex, bool bActive);

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Amount Board")
    TObjectPtr<UStaticMeshComponent> BoardHousing;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Amount Board")
    TObjectPtr<UTextRenderComponent> BoardHeader;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Amount Board")
    TArray<TObjectPtr<UStaticMeshComponent>> AmountTiles;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Amount Board")
    TArray<TObjectPtr<UTextRenderComponent>> AmountLabels;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageBackdropModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageBackdropModule();
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Backdrop")
    TArray<TObjectPtr<UStaticMeshComponent>> ArchSegments;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Backdrop")
    TArray<TObjectPtr<UStaticMeshComponent>> CityBuildings;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Backdrop")
    TObjectPtr<UInstancedStaticMeshComponent> CityWindows;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Backdrop")
    TObjectPtr<UTextRenderComponent> BackdropLabel;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageBankerBoothModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageBankerBoothModule();
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Banker Booth")
    TArray<TObjectPtr<UStaticMeshComponent>> BoothStructure;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Banker Booth")
    TObjectPtr<UStaticMeshComponent> GlassFront;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Banker Booth")
    TObjectPtr<UTextRenderComponent> BoothLabel;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageAudienceModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageAudienceModule();
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Audience")
    TObjectPtr<UInstancedStaticMeshComponent> AudienceSeats;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Audience")
    TArray<TObjectPtr<UStaticMeshComponent>> AudienceRisers;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Audience")
    TObjectPtr<UTextRenderComponent> AudienceLabel;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageLightingModule : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageLightingModule();

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Lighting")
    void ApplyLightingCue(FName CueName);

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Lighting")
    TArray<TObjectPtr<ULightComponent>> StageLights;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageCameraRig : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageCameraRig();

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Camera")
    void ActivateCamera(int32 CameraIndex);

    UFUNCTION(BlueprintPure, Category="Deal Stage|Camera")
    int32 GetActiveCameraIndex() const { return ActiveCameraIndex; }

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Camera")
    TArray<TObjectPtr<UCameraComponent>> StageCameras;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Camera")
    TArray<TObjectPtr<UTextRenderComponent>> CameraMarkers;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Camera")
    int32 ActiveCameraIndex = 0;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageInteractionDirector : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageInteractionDirector();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void SelectBriefcase(int32 BriefcaseNumber);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void OpenBriefcase(int32 BriefcaseNumber, int32 AmountIndex);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void SetBankerOffer(int64 NewOfferCents);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Interaction")
    void TriggerLightingCue(FName CueName);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Gameplay")
    void MoveSelection(int32 Direction);

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Gameplay")
    void ConfirmSelection();

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Gameplay")
    void AcceptDeal();

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Gameplay")
    void RejectDeal();

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Gameplay")
    void RestartGame();

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    EDealGamePhase GetGamePhase() const { return GamePhase; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    int32 GetHighlightedBriefcase() const { return HighlightedBriefcase; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    int32 GetPlayerBriefcase() const { return PlayerBriefcase; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    int32 GetCasesRemaining() const;

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    int32 GetCasesOpenedThisRound() const { return CasesOpenedThisRound; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    int32 GetCasesToOpenThisRound() const;

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetPhaseTitle() const;

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetInstructionText() const;

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetLastMessage() const { return LastMessage; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetResultHeadline() const { return ResultHeadline; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetResultDetail() const { return ResultDetail; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetBankerOfferText() const;

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetPlayerCaseValueText() const;

    static FString FormatCurrency(int64 AmountCents);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Interaction")
    TArray<FBriefcaseRuntimeState> Briefcases;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Interaction")
    int64 BankerOffer = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Gameplay")
    EDealGamePhase GamePhase = EDealGamePhase::ChoosePlayerCase;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Gameplay")
    int32 HighlightedBriefcase = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Gameplay")
    int32 PlayerBriefcase = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Gameplay")
    int32 RoundIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deal Stage|Gameplay")
    int32 CasesOpenedThisRound = 0;

private:
    void InitializeGame();
    void AssignPrizeValues();
    void OpenHighlightedCase();
    void BeginBankerOffer();
    void RevealFinalCase(bool bAcceptedDeal);
    void RefreshStageVisuals();
    void FocusCamera(int32 CameraIndex);
    void RunAutomatedGameTest();
    void RunAutomatedDealAcceptTest();
    int32 FindNextAvailableCase(int32 StartNumber, int32 Direction) const;
    int64 CalculateBankerOffer() const;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Gameplay")
    int64 AcceptedOffer = 0;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Gameplay")
    int32 LastOpenedBriefcase = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Gameplay")
    int64 LastRevealedAmount = 0;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Gameplay")
    FString LastMessage;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Gameplay")
    FString ResultHeadline;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Gameplay")
    FString ResultDetail;

    TArray<int32> CasesPerRound;
    TArray<int64> PrizeValuesCents;
    FTimerHandle PreviewCaptureTimer;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API ADealStageSet : public AStageModuleBase
{
    GENERATED_BODY()

public:
    ADealStageSet();

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> WorldShell;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> CentralPlatform;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> ModelStaircase;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> AmountBoard;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> CityBackdrop;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> BankerBooth;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> Audience;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> LightingRig;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> CameraRig;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Assembly")
    TObjectPtr<UChildActorComponent> InteractionDirector;
};

UCLASS()
class DEALORNODEALSTAGE_API ADealStagePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Camera")
    void SwitchToCamera(int32 CameraIndex);

private:
    AStageInteractionDirector* GetInteractionDirector() const;
    void PreviousCase();
    void NextCase();
    void ConfirmCase();
    void Deal();
    void NoDeal();
    void Restart();
    void PrimaryClick();
    void QuitPrototype();
    void Camera1();
    void Camera2();
    void Camera3();
    void Camera4();
    void CycleCamera();
};

UCLASS()
class DEALORNODEALSTAGE_API ADealStageHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;

private:
    AStageInteractionDirector* FindInteractionDirector() const;
};

UCLASS()
class DEALORNODEALSTAGE_API ADealStageGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADealStageGameMode();
    virtual void BeginPlay() override;
};
