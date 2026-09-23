import './style.css';
import { DealGame, PRIZES, formatMoney, randomSeed, type Phase } from './game';
import { translate, type Language, type TranslationKey } from './i18n';
import { StudioStage } from './stage';

const app = document.querySelector<HTMLDivElement>('#app')!;
const params = new URLSearchParams(location.search);
const seedParam = params.get('seed');
const seed = seedParam && /^\d{1,10}$/.test(seedParam) ? Number(seedParam) >>> 0 : randomSeed();
const game = new DealGame(seed);
let language: Language = params.get('lang') === 'zh' ? 'zh' : 'en';
const t = (key: TranslationKey, values?: Record<string, string | number>) => translate(language, key, values);
let stage: StudioStage | null = null;
let loadFailure: TranslationKey | null = null;
let paused = false;
let soundEnabled = true;
let fastPace = false;
let automaticCameras = true;
let timedPhase: Phase | null = null;
let phaseTimer: number | null = null;
let timerDeadline = 0;
let timerRemaining = 0;

const audio = Object.fromEntries(['Select', 'Reveal', 'Ring', 'Deal'].map(name => {
  const sound = new Audio(`${import.meta.env.BASE_URL}assets/${name}.wav`);
  sound.preload = 'auto';
  return [name, sound];
})) as Record<string, HTMLAudioElement>;

app.innerHTML = `
  <div class="app-shell">
    <header class="masthead">
      <div class="brand"><span class="brand-mark">D / N</span><div><h1>DEAL <span>or</span> NO DEAL</h1><p data-i18n="studioTagline"></p></div></div>
      <div class="masthead-center"><span class="eyebrow" data-i18n="onAir"></span><strong id="phase-title"></strong></div>
      <div class="masthead-stats"><span id="your-case"></span><small id="remaining-stat"></small></div>
    </header>
    <main>
      <section class="stage-section" data-i18n-aria="stageLabel">
        <div id="stage-view" class="stage-view">
          <img class="stage-poster" src="${import.meta.env.BASE_URL}assets/studio-overview.png" data-i18n-alt="stagePosterAlt" />
          <div id="loading" class="loading"><span class="loading-dot"></span><span data-i18n="loading"></span></div>
        </div>
        <div id="stage-overlay" class="stage-overlay" aria-live="polite"></div>
      </section>
      <nav class="toolbar" data-i18n-aria="toolbarLabel">
        <div class="camera-controls" id="camera-controls"></div>
        <div class="toolbar-right">
          <button type="button" class="tool-button language-button" data-action="language" id="language-button"></button>
          <button type="button" class="tool-button" data-action="auto-camera" id="auto-camera"></button>
          <button type="button" class="tool-button" data-action="sound" id="sound-button"></button>
          <button type="button" class="tool-button" data-action="pace" id="pace-button"></button>
          <button type="button" class="tool-button" data-action="fullscreen" data-i18n-aria="fullscreenAria" data-i18n="fullscreen"></button>
          <button type="button" class="tool-button menu-button" data-action="menu" data-i18n="menu"></button>
        </div>
      </nav>
      <div class="dashboard">
        <section class="progress-card" aria-labelledby="round-heading">
          <div class="section-label" data-i18n="gameSection"></div>
          <div class="progress-heading"><div><h2 id="round-heading"></h2><p id="instruction"></p></div><span id="round-badge" class="round-badge">01 / 09</span></div>
          <div class="round-track" id="round-track" data-i18n-aria="roundProgressLabel"></div>
          <div class="progress-facts"><div><span data-i18n="openedThisRound"></span><strong id="opened-stat"></strong></div><div><span data-i18n="casesRemaining"></span><strong id="cases-stat"></strong></div><div><span data-i18n="bankerOfferLabel"></span><strong id="offer-stat"></strong></div></div>
          <div class="history"><span data-i18n="offerHistory"></span><div id="offer-history"></div></div>
          <p class="help-note" data-i18n="helpNote"></p>
        </section>
        <section class="prize-card" aria-labelledby="prize-heading">
          <div class="section-label" data-i18n="prizeSection"></div>
          <div class="board-heading"><h2 id="prize-heading" data-i18n="prizeHeading"></h2><span data-i18n="prizeSubheading"></span></div>
          <div id="prize-board" class="prize-board"></div>
        </section>
      </div>
    </main>
    <footer class="site-footer"><span data-i18n="footerSource"></span><span data-i18n="footerDisclaimer"></span></footer>
  </div>`;

const view = document.querySelector<HTMLElement>('#stage-view')!;
const overlay = document.querySelector<HTMLElement>('#stage-overlay')!;
const section = document.querySelector<HTMLElement>('.stage-section')!;
const loading = document.querySelector<HTMLElement>('#loading')!;

function renderStaticCopy(): void {
  document.documentElement.lang = language === 'zh' ? 'zh-CN' : 'en';
  document.title = t('pageTitle');
  document.querySelector<HTMLMetaElement>('meta[name="description"]')!.content = t('metaDescription');
  document.querySelectorAll<HTMLElement>('[data-i18n]').forEach(element => {
    element.textContent = t(element.dataset.i18n as TranslationKey);
  });
  document.querySelectorAll<HTMLElement>('[data-i18n-aria]').forEach(element => {
    element.setAttribute('aria-label', t(element.dataset.i18nAria as TranslationKey));
  });
  document.querySelectorAll<HTMLImageElement>('[data-i18n-alt]').forEach(element => {
    element.alt = t(element.dataset.i18nAlt as TranslationKey);
  });
  const switcher = document.querySelector<HTMLButtonElement>('#language-button')!;
  switcher.textContent = t('switchLanguage');
  switcher.setAttribute('aria-label', t('switchLanguageAria'));
  switcher.lang = language === 'en' ? 'zh-CN' : 'en';
  view.querySelector('canvas')?.setAttribute('aria-label', t('stageCanvasLabel'));
  stage?.setBoardHeading(t('sceneBoardHeading'));
  if (loadFailure) loading.textContent = t(loadFailure);
}

try {
  stage = new StudioStage(view, number => takeAction(() => game.clickCase(number), 'Select'));
  stage.ready.then(() => {
    loading.remove();
    view.classList.add('model-ready');
    stage!.update(game);
    stage!.setCamera(0, true);
  }).catch(error => {
    console.error('Studio load failed; the 2D game remains playable', error);
    loadFailure = 'loadFailed';
    loading.textContent = t(loadFailure);
    loading.classList.add('loading-error');
  });
} catch (error) {
  console.error('WebGL is unavailable; the 2D game remains playable', error);
  loadFailure = 'webglFailed';
  loading.textContent = t(loadFailure);
  loading.classList.add('loading-error');
}

function playCue(name: string): void {
  if (!soundEnabled || !audio[name]) return;
  const sound = audio[name];
  sound.pause();
  sound.currentTime = 0;
  void sound.play().catch(() => undefined);
}

function phaseTitle(): string {
  switch (game.phase) {
    case 'welcome': return t('phaseWelcome');
    case 'choose': return t('phaseChoose');
    case 'open': return t('phaseOpen', { round: game.roundIndex + 1 });
    case 'reveal': return t('phaseReveal');
    case 'calling': return t('phaseCalling');
    case 'offer': return t('phaseOffer');
    case 'final': return t('phaseFinal');
    case 'gameover': return t(game.acceptedOffer ? 'phaseAccepted' : 'phaseResult');
  }
}

function resultDetail(): string {
  const ownCase = game.playerCase!;
  const ownAmount = formatMoney(game.cases[ownCase - 1].amount);
  if (game.acceptedOffer) {
    return t('resultAcceptedDetail', {
      offer: formatMoney(game.acceptedOffer), case: ownCase, amount: ownAmount,
    });
  }
  const other = game.finalAlternative!;
  return t('resultFinalDetail', {
    case: ownCase, amount: ownAmount,
    other, otherAmount: formatMoney(game.cases[other - 1].amount),
  });
}

function instruction(): string {
  if (game.phase === 'welcome') return t('instructionWelcome');
  if (game.phase === 'choose') return t('instructionChoose');
  if (game.phase === 'open') {
    const count = game.roundTarget - game.openedThisRound;
    return t('instructionOpen', {
      round: game.roundIndex + 1, count, unit: t(count === 1 ? 'caseSingular' : 'casePlural'),
    });
  }
  if (game.phase === 'reveal') return t('instructionReveal', {
    case: game.lastOpened!, amount: formatMoney(game.revealedAmount),
  });
  if (game.phase === 'calling') return t('instructionCalling');
  if (game.phase === 'offer') return t('instructionOffer');
  if (game.phase === 'final') return t('instructionFinal');
  return resultDetail();
}

function caseGrid(): string {
  return `<div class="case-grid" role="group" aria-label="${t('caseGroupLabel')}">${game.cases.map(item => {
    const owned = game.playerCase === item.number;
    const selected = game.selected === item.number && game.canSelect(item.number);
    const state = item.opened ? 'opened' : owned ? 'owned' : selected ? 'selected' : '';
    const label = item.opened ? t('caseOpened') : owned ? t('caseYours') : '';
    const aria = t('caseNumber', { number: item.number }) + (label ? `, ${label}` : '');
    return `<button type="button" class="case-button ${state}" data-action="case" data-number="${item.number}" ${game.canSelect(item.number) ? '' : 'disabled'} aria-label="${aria}" aria-pressed="${selected}"><span>${String(item.number).padStart(2, '0')}</span>${label ? `<small>${label}</small>` : ''}</button>`;
  }).join('')}</div>`;
}

function panel(): string {
  if (paused) return `<div class="game-panel pause-panel"><div class="panel-eyebrow">${t('pauseTag')}</div><h2>${t('pauseTitle')}</h2><p>${t('pauseText')}</p><div class="panel-actions"><button class="primary-button" data-action="resume">${t('resume')}</button><button class="secondary-button" data-action="new-game">${t('newGame')}</button></div><p class="panel-footnote">${t('newGameNote')}</p></div>`;
  switch (game.phase) {
    case 'welcome':
      return `<div class="game-panel welcome-panel"><div class="panel-eyebrow">${t('welcomeTag')}</div><h2>${t('welcomeTitleOne')}<br /><em>${t('welcomeTitleTwo')}</em></h2><ol><li>${t('welcomeStepOne')}</li><li>${t('welcomeStepTwo')}</li><li>${t('welcomeStepThree')}</li></ol><button class="primary-button" data-action="start">${t('enterGame')} <span>→</span></button><p class="panel-footnote">${t('welcomeNote')}</p></div>`;
    case 'choose':
    case 'open': {
      const count = game.roundTarget - game.openedThisRound;
      const unit = t(count === 1 ? 'caseSingular' : 'casePlural');
      const eyebrow = game.phase === 'choose' ? t('phaseChoose') :
        t('selectionOpenTag', { round: String(game.roundIndex + 1).padStart(2, '0') });
      const title = game.phase === 'choose' ? t('selectionChooseTitle') :
        t('selectionOpenTitle', { count, unit });
      return `<div class="game-panel selection-panel"><div class="panel-eyebrow">${eyebrow}</div><h2>${title}</h2><p>${t('selectionHint')}</p>${caseGrid()}<div class="selection-footer"><span>${t('selectedLabel')} <strong>#${String(game.selected).padStart(2, '0')}</strong></span><button class="primary-button" data-action="confirm">${t(game.phase === 'choose' ? 'keepCaseButton' : 'openCaseButton')} <span>→</span></button></div></div>`;
    }
    case 'reveal':
      return `<div class="game-panel reveal-panel"><div class="panel-eyebrow">${t('revealTag', { case: String(game.lastOpened).padStart(2, '0') })}</div><h2>${formatMoney(game.revealedAmount)}</h2><p>${t('removedFromBoard')}</p><button class="primary-button" data-action="continue">${t('continue')} <span>→</span></button><p class="panel-footnote">${t('revealNote', { pace: t(fastPace ? 'paceQuick' : 'paceShow') })}</p></div>`;
    case 'calling':
      return `<div class="game-panel call-panel"><div class="panel-eyebrow">${t('phaseCalling')}</div><h2>${t('callingTitle')}</h2><p>${t('callingText')}</p><button class="primary-button" data-action="answer">${t('answerPhone')} <span>↗</span></button></div>`;
    case 'offer':
      return `<div class="game-panel offer-panel"><div class="panel-eyebrow">${t(game.confirmingDeal ? 'offerConfirmTag' : 'offerTag')}</div><h2>${formatMoney(game.bankerOffer)}</h2><p>${t(game.confirmingDeal ? 'offerConfirmText' : 'offerText')}</p><div class="panel-actions"><button class="primary-button" data-action="deal">${t(game.confirmingDeal ? 'confirmDeal' : 'deal')}</button><button class="secondary-button" data-action="${game.confirmingDeal ? 'cancel' : 'no-deal'}">${t(game.confirmingDeal ? 'backToOffer' : 'noDeal')}</button></div><p class="panel-footnote">${game.offerHistory.length > 1 ? t('previousOffer', { amount: formatMoney(game.offerHistory.at(-2)!) }) : t('firstOfferNote')}</p></div>`;
    case 'final':
      return `<div class="game-panel final-panel"><div class="panel-eyebrow">${t('finalTag')}</div><h2>${t('finalTitle')}</h2><p>${t('finalText')}</p><div class="panel-actions"><button class="primary-button" data-action="keep">${t('keepNumber', { number: String(game.playerCase).padStart(2, '0') })}</button><button class="secondary-button" data-action="swap">${t('swapNumber', { number: String(game.finalAlternative).padStart(2, '0') })}</button></div></div>`;
    case 'gameover':
      return `<div class="game-panel result-panel"><div class="panel-eyebrow">${phaseTitle()}</div><h2>${formatMoney(game.winnings)}</h2><p>${resultDetail()}</p><div class="result-meta">${t('resultMeta', { count: game.offerHistory.length, unit: t(game.offerHistory.length === 1 ? 'offerSingular' : 'offerPlural') })}</div><button class="primary-button" data-action="replay">${t('playAgain')} <span>↻</span></button></div>`;
  }
}

function renderBoard(): void {
  const opened = new Set(game.cases.filter(item => item.opened).map(item => item.amount));
  document.querySelector<HTMLElement>('#prize-board')!.innerHTML = PRIZES.map((value, index) =>
    `<div class="prize-value ${index < 13 ? 'low' : 'high'} ${opened.has(value) ? 'removed' : ''}" aria-label="${t(opened.has(value) ? 'boardRemovedAria' : 'boardActiveAria', { amount: formatMoney(value) })}">${formatMoney(value)}</div>`).join('');
}

function render(): void {
  renderStaticCopy();
  document.querySelector<HTMLElement>('#phase-title')!.textContent = phaseTitle();
  document.querySelector<HTMLElement>('#your-case')!.textContent = game.playerCase === null
    ? t('yourCaseUnset')
    : t('yourCaseSet', {
      number: String(game.playerCase).padStart(2, '0'),
      value: game.phase === 'gameover' ? formatMoney(game.cases[game.playerCase - 1].amount) : t('sealed'),
    });
  document.querySelector<HTMLElement>('#remaining-stat')!.textContent =
    t('inPlayTopPrize', { count: game.remaining.length, amount: formatMoney(game.highestRemaining) });
  document.querySelector<HTMLElement>('#round-heading')!.textContent =
    game.phase === 'gameover' ? t('roundHeadingResult') : game.phase === 'welcome' ?
      t('roundHeadingWelcome') : t('roundHeadingGame', { round: game.roundIndex + 1 });
  document.querySelector<HTMLElement>('#instruction')!.textContent = instruction();
  document.querySelector<HTMLElement>('#round-badge')!.textContent = `${String(game.roundIndex + 1).padStart(2, '0')} / 09`;
  document.querySelector<HTMLElement>('#round-track')!.innerHTML = Array.from({ length: 9 }, (_, index) =>
    `<div class="round-segment ${index < game.roundIndex ? 'done' : index === game.roundIndex ? 'current' : ''}" title="${t('roundTrackTitle', { round: index + 1, count: [6, 5, 4, 3, 2, 1, 1, 1, 1][index] })}"></div>`).join('');
  document.querySelector<HTMLElement>('#opened-stat')!.textContent = `${game.openedThisRound} / ${game.roundTarget}`;
  document.querySelector<HTMLElement>('#cases-stat')!.textContent = String(game.remaining.length);
  document.querySelector<HTMLElement>('#offer-stat')!.textContent = game.offerHistory.length
    ? formatMoney(game.offerHistory.at(-1)!) : t('noOffer');
  document.querySelector<HTMLElement>('#offer-history')!.innerHTML = game.offerHistory.length
    ? game.offerHistory.map((offer, index) => `<span class="history-chip">${index + 1}. ${formatMoney(offer)}</span>`).join('')
    : t('noOffer');
  renderBoard();
  overlay.innerHTML = panel();
  section.classList.toggle('board-camera', stage?.cameraIndex === 3);
  const selecting = !paused && (game.phase === 'choose' || game.phase === 'open');
  section.classList.toggle('selection-active', selecting);
  section.classList.toggle('case-camera', stage?.cameraIndex === 2 && selecting);
  section.classList.toggle('is-paused', paused);
  const controls = document.querySelector<HTMLElement>('#camera-controls')!;
  controls.innerHTML = (['cameraWide', 'cameraTable', 'cameraCases', 'cameraBoard'] as const).map((key, index) =>
    `<button type="button" class="camera-button ${stage?.cameraIndex === index ? 'active' : ''}" data-action="camera" data-camera="${index}" aria-pressed="${stage?.cameraIndex === index}"><span>0${index + 1}</span> ${t(key)}</button>`).join('');
  const state = (enabled: boolean) => t(enabled ? 'on' : 'off');
  document.querySelector<HTMLElement>('#auto-camera')!.textContent = t('autoCamera', { state: state(automaticCameras) });
  document.querySelector<HTMLElement>('#sound-button')!.textContent = t('sound', { state: state(soundEnabled) });
  document.querySelector<HTMLElement>('#pace-button')!.textContent = t('pace', { state: t(fastPace ? 'paceQuick' : 'paceShow') });
  stage?.update(game);
}

function clearPhaseTimer(): void {
  if (phaseTimer !== null) window.clearTimeout(phaseTimer);
  phaseTimer = null;
}

function startPhaseTimer(delay: number): void {
  clearPhaseTimer();
  timerRemaining = delay;
  if (paused) return;
  timerDeadline = performance.now() + delay;
  phaseTimer = window.setTimeout(() => {
    phaseTimer = null;
    if (paused || game.phase !== timedPhase) return;
    takeAction(() => game.confirm());
  }, delay);
}

function syncPhaseTimer(previous: Phase): void {
  if (game.phase === previous) return;
  clearPhaseTimer();
  timedPhase = game.phase;
  if (game.phase === 'reveal') startPhaseTimer(fastPace ? 1100 : 2800);
  else if (game.phase === 'calling') startPhaseTimer(fastPace ? 1200 : 3200);
}

function switchAutomaticCamera(previous: Phase): void {
  if (!stage || !automaticCameras || previous === game.phase) return;
  const camera = game.phase === 'welcome' || game.phase === 'gameover' ? 0
    : game.phase === 'calling' ? 1 : game.phase === 'offer' ? 3 : 2;
  stage.setCamera(camera);
}

function takeAction(action: () => boolean, cue?: string): void {
  if (paused) return;
  const previous = game.phase;
  if (!action()) return;
  if (cue && game.phase !== 'reveal') playCue(cue);
  if (game.phase === 'reveal' && previous !== 'reveal') playCue('Reveal');
  if (game.phase === 'calling' && previous !== 'calling') playCue('Ring');
  if (game.phase === 'gameover' && previous !== 'gameover') playCue('Deal');
  syncPhaseTimer(previous);
  switchAutomaticCamera(previous);
  render();
}

function togglePause(): void {
  if (game.confirmingDeal) {
    game.cancelPending();
    render();
    return;
  }
  paused = !paused;
  if (paused && phaseTimer !== null) {
    timerRemaining = Math.max(0, timerDeadline - performance.now());
    clearPhaseTimer();
  } else if (!paused && timedPhase === game.phase &&
    (game.phase === 'reveal' || game.phase === 'calling') && phaseTimer === null) {
    startPhaseTimer(timerRemaining);
  }
  render();
}

function newGame(): void {
  clearPhaseTimer();
  timedPhase = null;
  paused = false;
  game.reset(seedParam ? seed : randomSeed());
  game.start();
  if (automaticCameras) stage?.setCamera(2);
  render();
}

function switchLanguage(): void {
  language = language === 'en' ? 'zh' : 'en';
  const url = new URL(location.href);
  if (language === 'zh') url.searchParams.set('lang', 'zh');
  else url.searchParams.delete('lang');
  history.replaceState(history.state, '', url);
  render();
}

function perform(action: string, element?: HTMLElement): void {
  switch (action) {
    case 'start': takeAction(() => game.start(), 'Select'); break;
    case 'case': takeAction(() => game.clickCase(Number(element?.dataset.number)), 'Select'); break;
    case 'confirm': case 'continue': case 'answer': takeAction(() => game.confirm()); break;
    case 'deal': takeAction(() => game.acceptDeal()); break;
    case 'no-deal': takeAction(() => game.rejectDeal()); break;
    case 'cancel': game.cancelPending(); render(); break;
    case 'keep': takeAction(() => game.chooseFinal(false)); break;
    case 'swap': takeAction(() => game.chooseFinal(true)); break;
    case 'replay': case 'new-game': newGame(); break;
    case 'resume': case 'menu': togglePause(); break;
    case 'language': switchLanguage(); break;
    case 'sound': soundEnabled = !soundEnabled; render(); break;
    case 'pace': fastPace = !fastPace; render(); break;
    case 'auto-camera': automaticCameras = !automaticCameras; render(); break;
    case 'camera': stage?.setCamera(Number(element?.dataset.camera)); render(); break;
    case 'fullscreen':
      if (document.fullscreenElement) void document.exitFullscreen();
      else void document.documentElement.requestFullscreen();
      break;
  }
}

app.addEventListener('click', event => {
  const button = (event.target as HTMLElement).closest<HTMLElement>('[data-action]');
  if (button) perform(button.dataset.action!, button);
});

document.addEventListener('keydown', event => {
  if (event.altKey || event.ctrlKey || event.metaKey) return;
  const key = event.key.toLowerCase();
  if (['arrowleft', 'arrowright', 'arrowup', 'arrowdown', ' ', 'enter'].includes(key)) event.preventDefault();
  if (key === 'escape') togglePause();
  else if (key === 'm') perform('sound');
  else if (key === 't') perform('pace');
  else if (key === 'a') perform('auto-camera');
  else if (key === 'c') { stage?.setCamera(((stage?.cameraIndex ?? 0) + 1) % 4); render(); }
  else if (/^[1-4]$/.test(key)) { stage?.setCamera(Number(key) - 1); render(); }
  else if (paused) return;
  else if (key === 'arrowleft' || key === 'arrowup') takeAction(() => game.moveSelection(-1));
  else if (key === 'arrowright' || key === 'arrowdown') takeAction(() => game.moveSelection(1));
  else if (key === 'enter' || key === ' ') takeAction(() => game.confirm());
  else if (key === 'd') takeAction(() => game.phase === 'final' ? game.chooseFinal(false) : game.acceptDeal());
  else if (key === 'n') takeAction(() => game.phase === 'final' ? game.chooseFinal(true) : game.rejectDeal());
  else if (key === 'r' && game.phase === 'gameover') newGame();
});

render();
