#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "DealStageModules.generated.h"

class UCameraComponent;
class UFont;
class UChildActorComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EDealGamePhase : uint8
{
    Welcome,
    ChoosePlayerCase UMETA(DisplayName="Choose Player Case"),
    OpenCases UMETA(DisplayName="Open Cases"),
    RevealAmount UMETA(DisplayName="Reveal Amount"),
    BankerCalling,
    BankerOffer UMETA(DisplayName="Banker Offer"),
    FinalChoice,
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
    virtual void Tick(float DeltaSeconds) override;

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

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Staircase")
    TArray<TObjectPtr<UStaticMeshComponent>> BriefcaseLids;
    TArray<bool> OpenTargets;
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
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Lighting")
    void ApplyLightingCue(FName CueName);

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Lighting")
    TArray<TObjectPtr<ULightComponent>> StageLights;
    TArray<FLinearColor> CueColors;
    TArray<float> CueIntensities;
};

UCLASS(Blueprintable)
class DEALORNODEALSTAGE_API AStageCameraRig : public AStageModuleBase
{
    GENERATED_BODY()

public:
    AStageCameraRig();
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

    UFUNCTION(BlueprintCallable, Category="Deal Stage|Camera")
    void ActivateCamera(int32 CameraIndex);

    UFUNCTION(BlueprintPure, Category="Deal Stage|Camera")
    int32 GetActiveCameraIndex() const { return ActiveCameraIndex; }
    bool UsesBoardSelectionPanel() const { return ActiveCameraIndex == 3 || BoardSelectionHoldSeconds > 0.f; }

private:
    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Camera")
    TArray<TObjectPtr<UCameraComponent>> StageCameras;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Camera")
    TArray<TObjectPtr<UTextRenderComponent>> CameraMarkers;

    UPROPERTY(VisibleAnywhere, Category="Deal Stage|Camera")
    int32 ActiveCameraIndex = 0;
    FMinimalViewInfo BlendedView;
    bool bViewInitialized = false;
    float BoardSelectionHoldSeconds = 0.f;
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

    void StartGame();
    void AnswerBanker();
    void ChooseFinalCase(bool bSwap);
    void ToggleMenu();
    void ToggleSound();
    void TogglePace();
    void ToggleAutoCamera();
    void CancelPending();
    void ClickCase(int32 Number);
    void PlayCue(const TCHAR* Cue);
    bool IsMenuOpen() const { return bMenuOpen; }
    bool IsSoundEnabled() const { return bSoundEnabled; }
    bool IsFastPace() const { return bFastPace; }
    bool IsAutoCamera() const { return bAutoCamera; }
    bool IsConfirmingDeal() const { return bConfirmingDeal; }
    bool IsSelectionArmed() const { return bSelectionArmed; }
    int32 GetFinalAlternative() const;
    int64 GetWinnings() const { return AcceptedOffer > 0 ? AcceptedOffer : LastRevealedAmount; }
    int64 GetHighestRemaining() const;
    const TArray<int64>& GetOfferHistory() const { return OfferHistory; }

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

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    int32 GetLastOpenedBriefcase() const { return LastOpenedBriefcase; }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    FString GetLastRevealedAmountText() const { return FormatCurrency(LastRevealedAmount); }

    UFUNCTION(BlueprintPure, Category="Deal Stage|Gameplay")
    float GetRevealElapsedSeconds() const;

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
    void CompleteCaseReveal();
    void BeginBankerOffer();
    void RevealFinalCase(bool bAcceptedDeal);
    void RefreshStageVisuals();
    void FocusCamera(int32 CameraIndex);
    void RunAutomatedGameTest();
    void RunAutomatedDealAcceptTest();
    void RunExperienceTest();
    void RunUIValidationStep();
    void RunBoardLayoutValidationStep();
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
    FTimerHandle CaseRevealTimer;
    FTimerHandle BankerCallTimer;
    FTimerHandle UIValidationTimer;
    int32 UIValidationStep = 0;
    int32 UIValidationFailures = 0;
    TArray<int64> OfferHistory;
    bool bMenuOpen = false;
    bool bSoundEnabled = true;
    bool bFastPace = false;
    bool bAutoCamera = true;
    bool bConfirmingDeal = false;
    bool bSelectionArmed = false;
    bool bAutomated = false;
    float RevealStartedAtSeconds = 0.0f;
    static constexpr float CaseRevealDurationSeconds = 2.8f;
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
    virtual void Tick(float DeltaSeconds) override;

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
    const FBox2D& GetSelectionPanelBounds() const { return SelectionPanelBounds; }

private:
    UPROPERTY(Transient)
    TObjectPtr<UFont> RuntimeFont;
    FBox2D SelectionPanelBounds = FBox2D(ForceInit);
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
