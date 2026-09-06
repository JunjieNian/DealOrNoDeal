#include "DealStageModules.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "CanvasItem.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"

void AStageInteractionDirector::StartGame()
{
    if (GamePhase != EDealGamePhase::Welcome) return;
    GamePhase = EDealGamePhase::ChoosePlayerCase;
    FocusCamera(2);
    PlayCue(TEXT("Select"));
}

void AStageInteractionDirector::ClickCase(int32 Number)
{
    if (bMenuOpen || !Briefcases.IsValidIndex(Number-1)) return;
    const FBriefcaseRuntimeState& State=Briefcases[Number-1];
    if (State.bIsOpened || State.bIsPlayerCase ||
        (GamePhase!=EDealGamePhase::ChoosePlayerCase && GamePhase!=EDealGamePhase::OpenCases)) return;
    if (HighlightedBriefcase==Number && bSelectionArmed) { ConfirmSelection(); return; }
    SelectBriefcase(Number);
    bSelectionArmed=true;
    PlayCue(TEXT("Select"));
}

void AStageInteractionDirector::CancelPending()
{
    bConfirmingDeal=false;
    bSelectionArmed=false;
}

int32 AStageInteractionDirector::GetFinalAlternative() const
{
    for (const FBriefcaseRuntimeState& State:Briefcases)
        if (!State.bIsOpened && !State.bIsPlayerCase) return State.BriefcaseNumber;
    return INDEX_NONE;
}

int64 AStageInteractionDirector::GetHighestRemaining() const
{
    int64 Value=0;
    for (const FBriefcaseRuntimeState& State:Briefcases)
        if (!State.bIsOpened) Value=FMath::Max(Value,State.AmountCents);
    return Value;
}

void AStageInteractionDirector::ChooseFinalCase(bool bSwap)
{
    if (bMenuOpen || GamePhase!=EDealGamePhase::FinalChoice) return;
    const int32 Other=GetFinalAlternative();
    if (!Briefcases.IsValidIndex(Other-1)) return;
    if (bSwap)
    {
        Briefcases[PlayerBriefcase-1].bIsPlayerCase=false;
        PlayerBriefcase=Other;
        Briefcases[Other-1].bIsPlayerCase=true;
    }
    PlayCue(TEXT("Deal"));
    TriggerLightingCue(TEXT("Deal"));
    RevealFinalCase(false);
    // Reveal both physical lids without changing the prize pool used in the final comparison.
    TArray<AActor*> Stairs;
    UGameplayStatics::GetAllActorsOfClass(this,AStageStaircaseModule::StaticClass(),Stairs);
    for (AActor* Actor:Stairs)
        for (const FBriefcaseRuntimeState& State:Briefcases)
            if (!State.bIsOpened) CastChecked<AStageStaircaseModule>(Actor)->SetBriefcaseOpened(State.BriefcaseNumber,true);
}

void AStageInteractionDirector::ToggleMenu()
{
    if (bConfirmingDeal) { CancelPending(); return; }
    bMenuOpen=!bMenuOpen;
    if (bMenuOpen)
    {
        GetWorldTimerManager().PauseTimer(CaseRevealTimer);
        GetWorldTimerManager().PauseTimer(BankerCallTimer);
    }
    else
    {
        GetWorldTimerManager().UnPauseTimer(CaseRevealTimer);
        GetWorldTimerManager().UnPauseTimer(BankerCallTimer);
    }
}

void AStageInteractionDirector::ToggleSound() { bSoundEnabled=!bSoundEnabled; }
void AStageInteractionDirector::TogglePace() { bFastPace=!bFastPace; }
void AStageInteractionDirector::ToggleAutoCamera() { bAutoCamera=!bAutoCamera; }

void AStageInteractionDirector::PlayCue(const TCHAR* Cue)
{
    if (!bSoundEnabled || bAutomated) return;
    USoundBase* Sound=LoadObject<USoundBase>(nullptr,
        *FString::Printf(TEXT("/Game/Studio/Audio/%s.%s"),Cue,Cue));
    if (Sound) UGameplayStatics::PlaySound2D(this,Sound,.5f);
}

void ADealStagePlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AStageInteractionDirector* D=GetInteractionDirector();
    if (!D) return;
    if (WasInputKeyJustPressed(EKeys::M)) D->ToggleSound();
    if (WasInputKeyJustPressed(EKeys::T)) D->TogglePace();
    if (WasInputKeyJustPressed(EKeys::A)) D->ToggleAutoCamera();
    if (WasInputKeyJustPressed(EKeys::F5) && GEngine)
    {
        UGameUserSettings* Settings=GEngine->GetGameUserSettings();
        const int32 Next=Settings->GetOverallScalabilityLevel()==2 ? 3 : 2;
        Settings->SetOverallScalabilityLevel(Next);
        Settings->ApplySettings(false);
    }
}

// A deterministic regression suite that drives the same public gameplay commands as input.
void AStageInteractionDirector::RunExperienceTest()
{
    int32 Failures=0;
    auto Check=[&Failures](bool Good,const TCHAR* What)
    {
        if (!Good) { ++Failures; UE_LOG(LogTemp,Error,TEXT("[DealExperienceTest] FAILED: %s"),What); }
    };
    TSet<int64> Values;
    for (const FBriefcaseRuntimeState& State:Briefcases) Values.Add(State.AmountCents);
    Check(Values.Num()==26,TEXT("26 unique prizes after shuffle"));
    ClickCase(9);
    Check(PlayerBriefcase==INDEX_NONE && bSelectionArmed,TEXT("First click selects without opening"));
    ClickCase(9);
    Check(PlayerBriefcase==9 && GetCasesRemaining()==26,TEXT("Second click keeps the selected case"));
    const int32 HighlightBefore=HighlightedBriefcase;
    ClickCase(9);
    Check(HighlightedBriefcase==HighlightBefore && GetCasesRemaining()==26,TEXT("Cannot open own case"));
    RestartGame();
    Check(PlayerBriefcase==9,TEXT("R cannot discard an active game"));
    ToggleMenu(); ConfirmSelection(); RejectDeal();
    Check(GetCasesRemaining()==26,TEXT("Menu blocks gameplay"));
    ToggleMenu();
    for (int32 I=0;I<6;++I) ConfirmSelection();
    Check(GamePhase==EDealGamePhase::BankerOffer && OfferHistory.Num()==1,TEXT("Six cases lead to first offer"));
    const int64 Offer=BankerOffer;
    bAutomated=false;
    AcceptDeal();
    Check(GamePhase==EDealGamePhase::BankerOffer && bConfirmingDeal,TEXT("Accept requires confirmation"));
    CancelPending();
    Check(!bConfirmingDeal && GamePhase==EDealGamePhase::BankerOffer,TEXT("Confirmation can be cancelled"));
    bSoundEnabled=false;
    AcceptDeal(); AcceptDeal();
    Check(GamePhase==EDealGamePhase::GameOver && GetWinnings()==Offer,TEXT("Accepted offer is the payout"));
    bAutomated=true;
    for (int32 Swap=0;Swap<2;++Swap)
    {
        RestartGame();
        Check(GetCasesRemaining()==26 && OfferHistory.Num()==0 && PlayerBriefcase==INDEX_NONE,TEXT("Replay resets all round state"));
        ConfirmSelection();
        const int32 Original=PlayerBriefcase;
        int32 Guard=0;
        while (GamePhase!=EDealGamePhase::FinalChoice && Guard++<100)
        {
            if (GamePhase==EDealGamePhase::OpenCases) ConfirmSelection();
            else if (GamePhase==EDealGamePhase::BankerOffer) RejectDeal();
        }
        Check(Guard<100 && OfferHistory.Num()==9 && GetCasesRemaining()==2,TEXT("Nine offers lead to final two"));
        const int32 Other=GetFinalAlternative();
        const int64 Expected=Briefcases[(Swap ? Other : Original)-1].AmountCents;
        ChooseFinalCase(Swap!=0);
        Check(GamePhase==EDealGamePhase::GameOver && PlayerBriefcase==(Swap ? Other : Original) && GetWinnings()==Expected,
            Swap ? TEXT("Swap awards other case") : TEXT("Keep awards original case"));
    }
    // Exercise live timer and menu transitions without the accelerated test flag.
    RestartGame(); bAutomated=false; bSoundEnabled=false;
    ConfirmSelection(); ConfirmSelection();
    Check(GamePhase==EDealGamePhase::RevealAmount && GetWorldTimerManager().IsTimerActive(CaseRevealTimer),TEXT("Normal reveal uses a live timer"));
    ToggleMenu();
    Check(GetWorldTimerManager().IsTimerPaused(CaseRevealTimer),TEXT("Pause suspends reveal timer"));
    ToggleMenu(); CompleteCaseReveal();
    for (int32 I=1;I<6;++I) { ConfirmSelection(); CompleteCaseReveal(); }
    Check(GamePhase==EDealGamePhase::BankerCalling && GetWorldTimerManager().IsTimerActive(BankerCallTimer),TEXT("Normal round rings the phone"));
    ToggleMenu();
    Check(GetWorldTimerManager().IsTimerPaused(BankerCallTimer),TEXT("Pause suspends call timer"));
    ToggleMenu(); AnswerBanker();
    Check(GamePhase==EDealGamePhase::BankerOffer && OfferHistory.Num()==1,TEXT("Answer advances exactly once"));
    AnswerBanker();
    Check(OfferHistory.Num()==1,TEXT("Repeated answer cannot duplicate offer"));
    bAutomated=true;
    UE_LOG(LogTemp,Display,TEXT("[DealExperienceTest] Completed=%s Failures=%d"),Failures==0 ? TEXT("true") : TEXT("false"),Failures);
}

void AStageInteractionDirector::RunUIValidationStep()
{
    APlayerController* PC=UGameplayStatics::GetPlayerController(this,0);
    ADealStageHUD* HUD=PC ? Cast<ADealStageHUD>(PC->GetHUD()) : nullptr;
    if (!HUD) { ++UIValidationFailures; return; }
    int32 Width=0,Height=0; PC->GetViewportSize(Width,Height);
    const float S=FMath::Min(Width/1600.f,Height/900.f),W=Width/S,H=Height/S;
    auto Click=[&](float X,float Y,FName Expected)
    {
        const FHUDHitBox* Box=HUD->GetHitBoxAtCoordinates(FVector2D(X*S,Y*S),true);
        if (!Box || Box->GetName()!=Expected)
        {
            ++UIValidationFailures;
            UE_LOG(LogTemp,Error,TEXT("[DealUIValidation] Hitbox mismatch: %s at %.1f,%.1f"),*Expected.ToString(),X,Y);
            return;
        }
        HUD->NotifyHitBoxClick(Box->GetName());
    };
    auto Capture=[&](const TCHAR* Name)
    {
        const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/Windows"),
            FString::Printf(TEXT("UI-%dx%d-%s.png"),Width,Height,Name));
        FScreenshotRequest::RequestScreenshot(Path,true,false,false);
    };
    switch(UIValidationStep++)
    {
        case 0: Capture(TEXT("Welcome")); break;
        case 1: Click(200,H*.21f+340,TEXT("Start")); break;
        case 2:
            Capture(TEXT("Cases"));
            Click(70,H-205+65,TEXT("Case.1"));
            if (PlayerBriefcase!=INDEX_NONE) ++UIValidationFailures;
            break;
        case 3:
            Click(W-140,H-205+120,TEXT("Confirm"));
            if (PlayerBriefcase!=1) ++UIValidationFailures;
            bAutomated=true;
            for (int32 I=0;I<6;++I) ConfirmSelection();
            bAutomated=false;
            break;
        case 4: Capture(TEXT("Offer")); break;
        case 5: Click(240,H*.23f+210,TEXT("Deal")); break;
        case 6:
            Capture(TEXT("ConfirmDeal"));
            if (!bConfirmingDeal || GamePhase!=EDealGamePhase::BankerOffer) ++UIValidationFailures;
            break;
        case 7: Click(240,H*.23f+270,TEXT("Cancel")); break;
        case 8: ToggleMenu(); break;
        case 9: Capture(TEXT("Menu")); break;
        case 10: Click(W/2,H/2-270+134,TEXT("Resume")); break;
        case 11: Click(240,H*.23f+210,TEXT("Deal")); break;
        case 12: Click(240,H*.23f+210,TEXT("Deal")); break;
        case 13:
            Capture(TEXT("Result"));
            if (GamePhase!=EDealGamePhase::GameOver) ++UIValidationFailures;
            break;
        case 14:
            Click(W/2,H*.27f+260,TEXT("Replay"));
            bAutomated=true; ConfirmSelection();
            for (int32 Guard=0;Guard<60 && GamePhase!=EDealGamePhase::FinalChoice;++Guard)
            {
                if (GamePhase==EDealGamePhase::OpenCases) ConfirmSelection();
                else if (GamePhase==EDealGamePhase::BankerOffer) RejectDeal();
            }
            bAutomated=false;
            break;
        case 15: Capture(TEXT("FinalChoice")); break;
        case 16:
            Click(W/2+160,H*.33f+165,TEXT("Swap"));
            if (GamePhase!=EDealGamePhase::GameOver || PlayerBriefcase==1) ++UIValidationFailures;
            break;
        case 17: Capture(TEXT("SwapResult")); break;
        default:
            GetWorldTimerManager().ClearTimer(UIValidationTimer);
            UE_LOG(LogTemp,Display,TEXT("[DealUIValidation] Completed=%s Failures=%d Viewport=%dx%d"),
                UIValidationFailures==0 ? TEXT("true") : TEXT("false"),UIValidationFailures,Width,Height);
            break;
    }
}

void AStageInteractionDirector::RunBoardLayoutValidationStep()
{
    ADealStagePlayerController* PC=Cast<ADealStagePlayerController>(UGameplayStatics::GetPlayerController(this,0));
    ADealStageHUD* HUD=PC ? Cast<ADealStageHUD>(PC->GetHUD()) : nullptr;
    if (!HUD) { ++UIValidationFailures; return; }
    int32 Width=0,Height=0; PC->GetViewportSize(Width,Height);
    const float S=FMath::Min(Width/1600.f,Height/900.f),H=Height/S;
    const float X=28.f,Y=FMath::Max(104.f,(H-536.f)*.5f);
    auto Check=[&](bool Passed,const TCHAR* Message)
    {
        if (!Passed) { ++UIValidationFailures; UE_LOG(LogTemp,Error,TEXT("[DealBoardLayoutTest] %s"),Message); }
    };
    auto Click=[&](float CX,float CY,FName Expected)
    {
        const FHUDHitBox* Box=HUD->GetHitBoxAtCoordinates(FVector2D(CX*S,CY*S),true);
        Check(Box && Box->GetName()==Expected,TEXT("Expected clickable selection control"));
        if (Box && Box->GetName()==Expected) HUD->NotifyHitBoxClick(Expected);
    };
    auto Capture=[&](const TCHAR* State)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/Windows"),
            FString::Printf(TEXT("Cam4-%dx%d-%s.png"),Width,Height,State)),true,false,false);
    };
    auto CheckBoardClear=[&]()
    {
        const FBox2D Panel=HUD->GetSelectionPanelBounds();
        Check(Panel.bIsValid,TEXT("Selection panel exists"));
        TArray<AActor*> Boards;
        UGameplayStatics::GetAllActorsOfClass(this,AStageAmountBoardModule::StaticClass(),Boards);
        int32 Checked=0;
        for (AActor* Board:Boards)
        {
            TArray<UTextRenderComponent*> Labels; Board->GetComponents(Labels);
            for (UTextRenderComponent* Label:Labels)
            {
                if (!Label->GetName().StartsWith(TEXT("AmountLabel_"))) continue;
                FBox2D Tile(ForceInit);
                for (float DY:{-92.f,92.f}) for (float DZ:{-17.f,17.f})
                {
                    FVector2D Screen;
                    Check(PC->ProjectWorldLocationToScreen(Label->GetComponentLocation()+FVector(0,DY,DZ),Screen),
                        TEXT("Amount tile projects into camera"));
                    Tile+=Screen;
                }
                Check(!Panel.Intersect(Tile),TEXT("Selection background overlaps prize tile"));
                Check(Tile.Min.X>=0 && Tile.Max.X<=Width && Tile.Min.Y>83*S && Tile.Max.Y<Height-36*S,
                    TEXT("Prize tile is inside unobstructed viewport"));
                ++Checked;
            }
        }
        Check(Checked==26,TEXT("All 26 prize tiles checked"));
    };
    switch (UIValidationStep++)
    {
        case 0:
            bAutoCamera=false; bAutomated=true;
            StartGame(); PC->SwitchToCamera(3);
            break;
        case 1:
        {
            // Validate the cooked mesh envelopes too: a correct Blender source
            // cannot protect against stale imports or displaced level actors.
            TArray<AActor*> Architecture;
            UGameplayStatics::GetAllActorsOfClass(this,AStaticMeshActor::StaticClass(),Architecture);
            TMap<FString,FBox> MeshBounds;
            for (AActor* Actor:Architecture)
            {
                UStaticMeshComponent* Mesh=Cast<AStaticMeshActor>(Actor)->GetStaticMeshComponent();
                if (Mesh && Mesh->GetStaticMesh()) MeshBounds.Add(Mesh->GetStaticMesh()->GetName(),Mesh->Bounds.GetBox());
            }
            const TCHAR* Pairs[][2]={
                {TEXT("SM_CaseTerraces"),TEXT("SM_AmountDisplay")},
                {TEXT("SM_CaseTerraces"),TEXT("SM_BankerSuite")},
                {TEXT("SM_CaseTerraces"),TEXT("SM_ArchSkyline")},
                {TEXT("SM_AudienceArchitecture"),TEXT("SM_AmountDisplay")},
                {TEXT("SM_AudienceArchitecture"),TEXT("SM_BankerSuite")}};
            for (const auto& Pair:Pairs)
            {
                const FBox* A=MeshBounds.Find(Pair[0]); const FBox* B=MeshBounds.Find(Pair[1]);
                Check(A && B && !A->Intersect(*B),TEXT("Cooked architecture envelopes must be separated"));
            }
            CheckBoardClear(); Capture(TEXT("Choose"));
            Click(X+18+3*90+41,Y+83+2*47+20,TEXT("Case.12"));
            Check(HighlightedBriefcase==12 && PlayerBriefcase==INDEX_NONE,TEXT("Sidebar previews case before confirmation"));
            break;
        }
        case 2:
            Click(X+194,Y+493,TEXT("Confirm"));
            Check(PlayerBriefcase==12 && GamePhase==EDealGamePhase::OpenCases,TEXT("Sidebar confirmation keeps case"));
            break;
        case 3:
            CheckBoardClear(); Capture(TEXT("Open"));
            Click(X+59,Y+103,TEXT("Case.1"));
            Click(X+194,Y+493,TEXT("Confirm"));
            Check(Briefcases[0].bIsOpened && GetCasesRemaining()==25,TEXT("Sidebar can open another case"));
            break;
        case 4:
            CheckBoardClear(); Capture(TEXT("AfterOpen"));
            Check(!HUD->GetHitBoxAtCoordinates(FVector2D((X+59)*S,(Y+103)*S),true),TEXT("Opened case has no click target"));
            Check(!HUD->GetHitBoxAtCoordinates(FVector2D((X+329)*S,(Y+197)*S),true),TEXT("Kept case has no click target"));
            break;
        case 5:
            PC->SwitchToCamera(2);
            break;
        case 6:
            Check(HUD->GetSelectionPanelBounds().Min.Y>Height*.6f && HUD->GetSelectionPanelBounds().GetSize().X>=Width-1,
                TEXT("Other cameras restore the bottom tray"));
            Capture(TEXT("ReturnToCases"));
            break;
        default:
            GetWorldTimerManager().ClearTimer(UIValidationTimer);
            UE_LOG(LogTemp,Display,TEXT("[DealBoardLayoutTest] Completed=%s Failures=%d Viewport=%dx%d"),
                UIValidationFailures==0 ? TEXT("true") : TEXT("false"),UIValidationFailures,Width,Height);
            break;
    }
}

void ADealStageHUD::DrawHUD()
{
    Super::DrawHUD();
    SelectionPanelBounds = FBox2D(ForceInit);
    if (!Canvas || FParse::Param(FCommandLine::Get(),TEXT("DealCleanPreview"))) return;
    AStageInteractionDirector* D=FindInteractionDirector();
    if (!D) return;
    // All artwork and hitboxes use one reference-space transform, including small and ultrawide windows.
    const float S=FMath::Min(Canvas->ClipX/1600.f,Canvas->ClipY/900.f);
    const float W=Canvas->ClipX/S, H=Canvas->ClipY/S;
    if (!RuntimeFont)
    {
        RuntimeFont=NewObject<UFont>(this);
        RuntimeFont->FontCacheType=EFontCacheType::Runtime;
        RuntimeFont->CompositeFont=*FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),12).GetCompositeFont();
    }
    const FLinearColor Ink(.006f,.013f,.026f,.94f), White(.92f,.95f,1.f), Dim(.43f,.54f,.67f);
    const FLinearColor Gold(1.f,.63f,.16f), Blue(.08f,.66f,.92f), Line(.08f,.15f,.23f,.95f);
    float MX=-100,MY=-100;
    if (PlayerOwner) PlayerOwner->GetMousePosition(MX,MY);
    MX/=S; MY/=S;
    auto Rect=[&](float X,float Y,float WW,float HH,FLinearColor C)
        { DrawRect(C,X*S,Y*S,WW*S,HH*S); };
    auto FontFor=[&](float Scale,bool Large)
    {
        // Rasterize at the actual pixel size; enlarging the legacy HUD font atlas blurs large prizes.
        return FSlateFontInfo(RuntimeFont,FMath::Max(5,FMath::RoundToInt(9.f*Scale*S)),
            Large ? TEXT("Bold") : TEXT("Regular"));
    };
    auto Measure=[&](const FString& T,float Scale,bool Large=false)
    {
        return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T,FontFor(Scale,Large))/S;
    };
    auto Text=[&](const FString& T,float X,float Y,float Scale,FLinearColor C,bool Large=false)
    {
        FCanvasTextItem Item(FVector2D(X*S,Y*S),FText::FromString(T),FontFor(Scale,Large),C);
        Canvas->DrawItem(Item);
    };
    auto Center=[&](const FString& T,float X,float Y,float Scale,FLinearColor C,bool Large=false)
    {
        Text(T,X-Measure(T,Scale,Large).X*.5f,Y,Scale,C,Large);
    };
    auto Button=[&](FName ID,const FString& T,float X,float Y,float WW,float HH,bool Accent=false)
    {
        const bool Hover=MX>=X && MX<=X+WW && MY>=Y && MY<=Y+HH;
        Rect(X,Y,WW,HH,Accent ? (Hover ? FLinearColor(1,.76f,.3f) : Gold)
            : Hover ? FLinearColor(.12f,.23f,.34f) : Line);
        const FVector2D Size=Measure(T,.92f);
        Text(T,X+(WW-Size.X)*.5f,Y+(HH-Size.Y)*.5f,.92f,Accent ? Ink : White);
        AddHitBox(FVector2D(X*S,Y*S),FVector2D(WW*S,HH*S),ID,true,50);
    };
    const EDealGamePhase Phase=D->GetGamePhase();
    Rect(0,0,W,83,Ink); Rect(28,80,W-56,1,Line);
    Text(TEXT("DEAL / NO DEAL"),30,22,1.38f,Gold,true);
    Text(TEXT("THE STUDIO   /   26 CASES. ONE DECISION."),31,57,.66f,Dim);
    Center(D->GetPhaseTitle(),W*.5f,27,1.f,White);
    Text(D->GetPlayerBriefcase()==INDEX_NONE ? TEXT("YOUR CASE  --") : FString::Printf(TEXT("YOUR CASE  #%02d  /  %s"),
        D->GetPlayerBriefcase(),*D->GetPlayerCaseValueText()),W-363,23,.9f,Blue);
    Text(FString::Printf(TEXT("%d IN PLAY   /   TOP PRIZE %s"),D->GetCasesRemaining(),
        *D->FormatCurrency(D->GetHighestRemaining())),W-363,53,.69f,Dim);

    if (D->IsMenuOpen())
    {
        Rect(0,83,W,H-83,FLinearColor(0,0,0,.64f));
        const float X=W/2-265,Y=H/2-270;
        Rect(X,Y,530,540,Ink); Rect(X,Y,4,540,Gold);
        Text(TEXT("TAKE A BREAK"),X+35,Y+25,1.45f,White,true);
        Text(TEXT("Your round is paused. Resume whenever you are ready."),X+35,Y+68,.85f,Dim);
        Button(TEXT("Resume"),TEXT("RESUME  /  ESC"),X+35,Y+110,460,48,true);
        Button(TEXT("Sound"),D->IsSoundEnabled()?TEXT("SOUND: ON  /  M"):TEXT("SOUND: OFF  /  M"),X+35,Y+171,460,44);
        Button(TEXT("Pace"),D->IsFastPace()?TEXT("REVEAL PACE: QUICK  /  T"):TEXT("REVEAL PACE: SHOW  /  T"),X+35,Y+224,460,44);
        Button(TEXT("AutoCam"),D->IsAutoCamera()?TEXT("AUTOMATIC CAMERAS: ON  /  A"):TEXT("AUTOMATIC CAMERAS: OFF  /  A"),X+35,Y+277,460,44);
        Button(TEXT("NewGame"),TEXT("START A NEW GAME"),X+35,Y+339,460,44);
        Text(TEXT("New game discards the current round."),X+35,Y+391,.78f,Dim);
        Button(TEXT("Quit"),TEXT("EXIT TO DESKTOP"),X+35,Y+434,460,44);
        Text(TEXT("F5: switch High / Epic graphics    F11: full screen"),X+35,Y+502,.76f,Dim);
        return;
    }

    if (Phase==EDealGamePhase::Welcome)
    {
        const float X=58,Y=H*.21f;
        Rect(X,Y,500,424,Ink); Rect(X,Y,4,424,Gold);
        Text(TEXT("YOUR CASE."),X+32,Y+30,2.3f,White,true);
        Text(TEXT("YOUR CALL."),X+32,Y+83,2.3f,Gold,true);
        Text(TEXT("01   Keep one sealed briefcase."),X+34,Y+165,1.f,White);
        Text(TEXT("02   Open others. Watch the prize board change."),X+34,Y+207,1.f,White);
        Text(TEXT("03   Take the offer or play to the final two."),X+34,Y+249,1.f,White);
        Button(TEXT("Start"),TEXT("ENTER THE GAME  /  ENTER"),X+32,Y+314,436,59,true);
        Text(TEXT("No time limit on decisions.  All prizes are fictional."),X+34,Y+390,.78f,Dim);
    }
    else if (Phase==EDealGamePhase::ChoosePlayerCase || Phase==EDealGamePhase::OpenCases)
    {
        const AStageCameraRig* Rig=PlayerOwner ? Cast<AStageCameraRig>(PlayerOwner->GetViewTarget()) : nullptr;
        const bool bBoardView=Rig && Rig->UsesBoardSelectionPanel();
        if (bBoardView)
        {
            // The board reaches below the usual tray. Keep every selector and its
            // background in the free left margin, without changing the approved camera.
            const float X=28.f,Y=FMath::Max(104.f,(H-536.f)*.5f),PanelW=388.f;
            SelectionPanelBounds=FBox2D(FVector2D(X,Y)*S,FVector2D(X+PanelW,Y+536.f)*S);
            Rect(X,Y,PanelW,536,Ink); Rect(X,Y,3,536,Gold);
            Text(Phase==EDealGamePhase::ChoosePlayerCase ? TEXT("CHOOSE YOUR CASE")
                : FString::Printf(TEXT("ROUND %d / 9  -  OPEN %d MORE"),D->RoundIndex+1,
                    D->GetCasesToOpenThisRound()-D->GetCasesOpenedThisRound()),X+18,Y+20,1.f,White);
            Text(TEXT("Select a number, then confirm."),X+18,Y+48,.83f,Dim);
            for (int32 I=0;I<26;++I)
            {
                const FBriefcaseRuntimeState& C=D->Briefcases[I];
                const float CX=X+18+(I%4)*90.f,CY=Y+83+(I/4)*47.f;
                if (C.bIsOpened || C.bIsPlayerCase)
                {
                    Rect(CX,CY,82,40,C.bIsPlayerCase ? FLinearColor(.015f,.19f,.27f) : FLinearColor(.02f,.035f,.05f));
                    Center(C.bIsPlayerCase ? FString::Printf(TEXT("%02d YOURS"),I+1) : FString::Printf(TEXT("%02d --"),I+1),
                        CX+41,CY+13,.76f,C.bIsPlayerCase ? Blue : Dim);
                }
                else Button(*FString::Printf(TEXT("Case.%d"),I+1),FString::Printf(TEXT("%02d"),I+1),CX,CY,82,40,
                    D->GetHighlightedBriefcase()==I+1);
            }
            Text(FString::Printf(TEXT("SELECTED  #%02d"),D->GetHighlightedBriefcase()),X+18,Y+428,.96f,Gold);
            Button(TEXT("Confirm"),Phase==EDealGamePhase::ChoosePlayerCase ? TEXT("KEEP CASE  /  ENTER") : TEXT("OPEN CASE  /  ENTER"),
                X+18,Y+470,352,46,true);
        }
        else
        {
        const float Y=H-205;
        SelectionPanelBounds=FBox2D(FVector2D(0,Y)*S,FVector2D(W,H-36)*S);
        Rect(0,Y,W,169,Ink); Rect(28,Y,W-56,1,Line);
        Text(Phase==EDealGamePhase::ChoosePlayerCase ? TEXT("WHICH CASE WILL YOU KEEP?")
            : FString::Printf(TEXT("ROUND %d / 9    -    OPEN %d MORE"),D->RoundIndex+1,
                D->GetCasesToOpenThisRound()-D->GetCasesOpenedThisRound()),30,Y+15,1.f,White);
        for (int32 I=0;I<9;++I)
            Rect(W-294+I*28,Y+22,20,3,I<=D->RoundIndex ? Gold : Line);
        const float GridW=FMath::Min(W-315.f,1190.f), CW=GridW/13.f;
        for (int32 I=0;I<26;++I)
        {
            const FBriefcaseRuntimeState& C=D->Briefcases[I];
            const float X=30+(I%13)*CW, CY=Y+48+(I/13)*52;
            if (C.bIsOpened || C.bIsPlayerCase)
            {
                Rect(X,CY,CW-7,43,C.bIsPlayerCase ? FLinearColor(.015f,.19f,.27f) : FLinearColor(.02f,.035f,.05f));
                Center(C.bIsPlayerCase ? FString::Printf(TEXT("%02d  YOURS"),I+1) : FString::Printf(TEXT("%02d  --"),I+1),
                    X+(CW-7)/2,CY+14,.76f,C.bIsPlayerCase ? Blue : Dim);
            }
            else Button(*FString::Printf(TEXT("Case.%d"),I+1),FString::Printf(TEXT("%02d"),I+1),X,CY,CW-7,43,
                D->GetHighlightedBriefcase()==I+1);
        }
        Text(FString::Printf(TEXT("SELECTED  #%02d"),D->GetHighlightedBriefcase()),W-245,Y+57,.96f,Gold);
        Button(TEXT("Confirm"),Phase==EDealGamePhase::ChoosePlayerCase ? TEXT("KEEP CASE  /  ENTER") : TEXT("OPEN CASE  /  ENTER"),
            W-251,Y+98,220,43,true);
        }
    }
    else if (Phase==EDealGamePhase::RevealAmount)
    {
        const float X=W/2-264,Y=H*.35f;
        Rect(X,Y,528,214,Ink); Rect(X,Y,528,4,Blue);
        Center(FString::Printf(TEXT("CASE #%02d CONTAINED"),D->GetLastOpenedBriefcase()),W/2,Y+25,1.f,Dim);
        Center(D->GetLastRevealedAmountText(),W/2,Y+65,2.8f,Gold,true);
        Center(TEXT("REMOVED FROM THE PRIZE BOARD"),W/2,Y+131,.87f,White);
        Button(TEXT("Continue"),TEXT("CONTINUE  /  SPACE"),X+120,Y+165,288,34);
    }
    else if (Phase==EDealGamePhase::BankerCalling)
    {
        Rect(42,H*.26f,438,185,Ink); Rect(42,H*.26f,4,185,Gold);
        Text(TEXT("THE BANKER IS CALLING"),70,H*.26f+26,1.35f,White,true);
        Text(TEXT("An offer is on its way."),70,H*.26f+72,1.f,Dim);
        Button(TEXT("Answer"),TEXT("ANSWER PHONE  /  ENTER"),70,H*.26f+113,382,47,true);
    }
    else if (Phase==EDealGamePhase::BankerOffer)
    {
        const float X=40,Y=H*.23f;
        Rect(X,Y,415,355,Ink); Rect(X,Y,4,355,Gold);
        Text(D->IsConfirmingDeal()?TEXT("LOCK IN THIS OFFER?"):TEXT("THE BANKER OFFERS"),X+26,Y+25,1.15f,White,true);
        Text(D->GetBankerOfferText(),X+26,Y+76,2.8f,Gold,true);
        Text(D->IsConfirmingDeal()?TEXT("Accepting ends this game."):TEXT("Guaranteed payout. The next decision is yours."),X+26,Y+145,.86f,Dim);
        Button(TEXT("Deal"),D->IsConfirmingDeal()?TEXT("CONFIRM DEAL  /  D"):TEXT("DEAL  /  D"),X+26,Y+187,362,48,true);
        Button(D->IsConfirmingDeal()?TEXT("Cancel"):TEXT("NoDeal"),D->IsConfirmingDeal()?TEXT("BACK TO OFFER  /  ESC"):TEXT("NO DEAL  /  N"),X+26,Y+247,362,48);
        const TArray<int64>& History=D->GetOfferHistory();
        Text(History.Num()>1 ? FString::Printf(TEXT("PREVIOUS OFFER  %s"),*D->FormatCurrency(History[History.Num()-2]))
            : TEXT("FIRST OFFER  /  NO DECISION TIMER"),X+26,Y+321,.83f,Dim);
    }
    else if (Phase==EDealGamePhase::FinalChoice)
    {
        const float X=W/2-325,Y=H*.33f;
        Rect(X,Y,650,239,Ink); Rect(X,Y,650,4,Gold);
        Center(TEXT("ONE LAST CHOICE"),W/2,Y+28,1.6f,White,true);
        Center(TEXT("Keep your original case or swap. Both are still sealed."),W/2,Y+84,1.f,Dim);
        Button(TEXT("Keep"),FString::Printf(TEXT("KEEP #%02d  /  D"),D->GetPlayerBriefcase()),X+30,Y+139,280,61,true);
        Button(TEXT("Swap"),FString::Printf(TEXT("SWAP TO #%02d  /  N"),D->GetFinalAlternative()),X+340,Y+139,280,61);
    }
    else if (Phase==EDealGamePhase::GameOver)
    {
        const float X=W/2-410,Y=H*.27f;
        Rect(X,Y,820,332,Ink); Rect(X,Y,820,4,Gold);
        Center(D->GetResultHeadline(),W/2,Y+24,1.15f,Dim);
        Center(D->FormatCurrency(D->GetWinnings()),W/2,Y+68,3.5f,Gold,true);
        Center(D->GetResultDetail(),W/2,Y+147,1.f,White);
        Center(FString::Printf(TEXT("%d BANKER OFFER%s RECEIVED"),D->GetOfferHistory().Num(),D->GetOfferHistory().Num()==1?TEXT(""):TEXT("S")),
            W/2,Y+190,.85f,Dim);
        Button(TEXT("Replay"),TEXT("PLAY AGAIN  /  R"),W/2-165,Y+238,330,58,true);
    }
    Rect(0,H-36,W,36,Ink);
    Text(TEXT("1 WIDE    2 TABLE    3 CASES    4 BOARD    C CYCLE"),28,H-25,.74f,Dim);
    Text(FString::Printf(TEXT("A AUTO CAM: %s    T PACE: %s    M SOUND: %s    ESC MENU"),D->IsAutoCamera()?TEXT("ON"):TEXT("OFF"),
        D->IsFastPace()?TEXT("QUICK"):TEXT("SHOW"),D->IsSoundEnabled()?TEXT("ON"):TEXT("OFF")),W-618,H-25,.74f,Dim);
}

void ADealStageHUD::NotifyHitBoxClick(FName Name)
{
    Super::NotifyHitBoxClick(Name);
    AStageInteractionDirector* D=FindInteractionDirector();
    if (!D) return;
    if (Name==TEXT("Resume")) D->ToggleMenu();
    else if (Name==TEXT("Sound")) D->ToggleSound();
    else if (Name==TEXT("Pace")) D->TogglePace();
    else if (Name==TEXT("AutoCam")) D->ToggleAutoCamera();
    else if (Name==TEXT("NewGame")) D->RestartGame();
    else if (Name==TEXT("Quit") && D->IsMenuOpen()) UKismetSystemLibrary::QuitGame(this,PlayerOwner,EQuitPreference::Quit,false);
    else if (D->IsMenuOpen()) return;
    else if (Name==TEXT("Start")) D->StartGame();
    else if (Name==TEXT("Confirm") || Name==TEXT("Continue")) D->ConfirmSelection();
    else if (Name==TEXT("Answer")) D->AnswerBanker();
    else if (Name==TEXT("Deal")) D->AcceptDeal();
    else if (Name==TEXT("NoDeal")) D->RejectDeal();
    else if (Name==TEXT("Cancel")) D->CancelPending();
    else if (Name==TEXT("Keep")) D->ChooseFinalCase(false);
    else if (Name==TEXT("Swap")) D->ChooseFinalCase(true);
    else if (Name==TEXT("Replay")) D->RestartGame();
    else if (Name.ToString().StartsWith(TEXT("Case."))) D->ClickCase(FCString::Atoi(*Name.ToString().RightChop(5)));
}
