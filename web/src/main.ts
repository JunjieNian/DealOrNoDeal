import './style.css';
import { DealGame, PRIZES, formatMoney, randomSeed, type Phase } from './game';
import { StudioStage } from './stage';

const app = document.querySelector<HTMLDivElement>('#app')!;
const seedParam = new URLSearchParams(location.search).get('seed');
const seed = seedParam && /^\d{1,10}$/.test(seedParam) ? Number(seedParam) >>> 0 : randomSeed();
const game = new DealGame(seed);
let stage: StudioStage | null = null;
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
      <div class="brand"><span class="brand-mark">D / N</span><div><h1>DEAL <span>or</span> NO DEAL</h1><p>THE STUDIO · 网页原生游玩版</p></div></div>
      <div class="masthead-center"><span class="eyebrow">ON AIR · 01</span><strong id="phase-title">WELCOME TO THE STUDIO</strong></div>
      <div class="masthead-stats"><span id="your-case">你的箱子 · 未选择</span><small id="remaining-stat">26 箱仍在场 · 最高奖金 $1,000,000</small></div>
    </header>
    <main>
      <section class="stage-section" aria-label="三维摄影棚">
        <div id="stage-view" class="stage-view">
          <img class="stage-poster" src="${import.meta.env.BASE_URL}assets/studio-overview.png" alt="Deal or No Deal 摄影棚全景" />
          <div id="loading" class="loading"><span class="loading-dot"></span>正在布置摄影棚…</div>
        </div>
        <div id="stage-overlay" class="stage-overlay" aria-live="polite"></div>
      </section>
      <nav class="toolbar" aria-label="摄影机和游玩设置">
        <div class="camera-controls" id="camera-controls"></div>
        <div class="toolbar-right">
          <button type="button" class="tool-button" data-action="auto-camera" id="auto-camera">自动机位 · 开</button>
          <button type="button" class="tool-button" data-action="sound" id="sound-button">声音 · 开</button>
          <button type="button" class="tool-button" data-action="pace" id="pace-button">节奏 · 演出</button>
          <button type="button" class="tool-button" data-action="fullscreen" aria-label="全屏">⛶ 全屏</button>
          <button type="button" class="tool-button menu-button" data-action="menu">☰ 菜单</button>
        </div>
      </nav>
      <div class="dashboard">
        <section class="progress-card" aria-labelledby="round-heading">
          <div class="section-label">THE GAME</div>
          <div class="progress-heading"><div><h2 id="round-heading">你的选择，决定结局</h2><p id="instruction">留下一只箱子，再逐轮打开其他箱子。</p></div><span id="round-badge" class="round-badge">01 / 09</span></div>
          <div class="round-track" id="round-track" aria-label="九轮进度"></div>
          <div class="progress-facts"><div><span>本轮已开</span><strong id="opened-stat">0 / 6</strong></div><div><span>剩余箱数</span><strong id="cases-stat">26</strong></div><div><span>Banker 报价</span><strong id="offer-stat">尚未报价</strong></div></div>
          <div class="history"><span>报价记录</span><div id="offer-history">尚无报价</div></div>
          <p class="help-note">鼠标/触屏：点击箱号预选，再点一次或按确认。键盘：方向键选箱、Enter 确认、D / N 决策、1–4 切换机位、Esc 暂停。</p>
        </section>
        <section class="prize-card" aria-labelledby="prize-heading">
          <div class="section-label">THE PRIZE BOARD</div>
          <div class="board-heading"><h2 id="prize-heading">26 个金额</h2><span>打开后从金额板熄灭</span></div>
          <div id="prize-board" class="prize-board"></div>
        </section>
      </div>
    </main>
    <footer class="site-footer"><span>原创可编辑摄影棚 · 纯浏览器运行 · 无需安装</span><span>致敬经典电视游戏 · 奖金仅为虚构游戏数值</span></footer>
  </div>`;

const view = document.querySelector<HTMLElement>('#stage-view')!;
const overlay = document.querySelector<HTMLElement>('#stage-overlay')!;
const section = document.querySelector<HTMLElement>('.stage-section')!;
const loading = document.querySelector<HTMLElement>('#loading')!;

try {
  stage = new StudioStage(view, number => takeAction(() => game.clickCase(number), 'Select'));
  stage.ready.then(() => {
    loading.remove();
    view.classList.add('model-ready');
    stage!.update(game);
    stage!.setCamera(0, true);
  }).catch(error => {
    console.error('摄影棚加载失败，保留可玩的 2D 界面', error);
    loading.textContent = '三维场景暂不可用，仍可使用箱号和按钮完整游玩。';
    loading.classList.add('loading-error');
  });
} catch (error) {
  console.error('WebGL 不可用，保留可玩的 2D 界面', error);
  loading.textContent = '当前浏览器无法显示 3D 场景，仍可使用箱号和按钮完整游玩。';
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
    case 'welcome': return 'WELCOME TO THE STUDIO';
    case 'choose': return 'CHOOSE YOUR CASE';
    case 'open': return `ROUND ${game.roundIndex + 1} · OPEN CASES`;
    case 'reveal': return 'CASE REVEALED';
    case 'calling': return 'INCOMING CALL';
    case 'offer': return 'DEAL OR NO DEAL?';
    case 'final': return 'THE FINAL TWO CASES';
    case 'gameover': return game.resultHeadline;
  }
}

function instruction(): string {
  if (game.phase === 'welcome') return '留下一只箱子，再逐轮打开其他箱子。';
  if (game.phase === 'choose') return '先选一只箱子。它会一直封存到最终抉择。';
  if (game.phase === 'open') return `第 ${game.roundIndex + 1} 轮：还需打开 ${game.roundTarget - game.openedThisRound} 只箱子。`;
  if (game.phase === 'reveal') return `${game.lastOpened} 号箱开出 ${formatMoney(game.revealedAmount)}，该金额已从金额板移除。`;
  if (game.phase === 'calling') return 'Banker 来电。接听后听取这一轮的报价。';
  if (game.phase === 'offer') return '接受有保障的报价，还是继续冒险？没有决策倒计时。';
  if (game.phase === 'final') return '最后两只箱子仍然封存。保留原箱，或与另一只交换。';
  return game.resultDetail;
}

function caseGrid(): string {
  return `<div class="case-grid" role="group" aria-label="26 只箱子">${game.cases.map(item => {
    const owned = game.playerCase === item.number;
    const selected = game.selected === item.number && game.canSelect(item.number);
    const state = item.opened ? 'opened' : owned ? 'owned' : selected ? 'selected' : '';
    const label = item.opened ? '已开' : owned ? '你的' : '';
    return `<button type="button" class="case-button ${state}" data-action="case" data-number="${item.number}" ${game.canSelect(item.number) ? '' : 'disabled'} aria-label="${item.number} 号箱${label ? `，${label}` : ''}" aria-pressed="${selected}"><span>${String(item.number).padStart(2, '0')}</span>${label ? `<small>${label}</small>` : ''}</button>`;
  }).join('')}</div>`;
}

function panel(): string {
  if (paused) return `<div class="game-panel pause-panel"><div class="panel-eyebrow">PAUSED · 暂停</div><h2>稍作休息</h2><p>所有自动过场都已暂停。回来后继续当前这一局。</p><div class="panel-actions"><button class="primary-button" data-action="resume">继续游玩</button><button class="secondary-button" data-action="new-game">开始新的一局</button></div><p class="panel-footnote">新的一局会舍弃当前进度。</p></div>`;
  switch (game.phase) {
    case 'welcome':
      return `<div class="game-panel welcome-panel"><div class="panel-eyebrow">26 CASES · ONE DECISION</div><h2>YOUR CASE.<br /><em>YOUR CALL.</em></h2><ol><li>留下一只封存的箱子</li><li>逐轮开箱，看金额板变化</li><li>接受报价，或战斗到最后两只</li></ol><button class="primary-button" data-action="start">进入游戏 <span>→</span></button><p class="panel-footnote">无决策时限 · 奖金仅为虚构数值</p></div>`;
    case 'choose':
    case 'open':
      return `<div class="game-panel selection-panel"><div class="panel-eyebrow">${game.phase === 'choose' ? 'CHOOSE YOUR CASE' : `ROUND ${String(game.roundIndex + 1).padStart(2, '0')} / 09`}</div><h2>${game.phase === 'choose' ? '哪只箱子留给你？' : `再打开 ${game.roundTarget - game.openedThisRound} 只箱子`}</h2><p>点击箱号预选；再次点击或按确认。</p>${caseGrid()}<div class="selection-footer"><span>当前选中 <strong>#${String(game.selected).padStart(2, '0')}</strong></span><button class="primary-button" data-action="confirm">${game.phase === 'choose' ? '留住这只箱子' : '打开选中的箱子'} <span>→</span></button></div></div>`;
    case 'reveal':
      return `<div class="game-panel reveal-panel"><div class="panel-eyebrow">CASE #${String(game.lastOpened).padStart(2, '0')} CONTAINED</div><h2>${formatMoney(game.revealedAmount)}</h2><p>已从金额板移除</p><button class="primary-button" data-action="continue">继续 <span>→</span></button><p class="panel-footnote">${fastPace ? '快速' : '演出'}节奏会自动继续，也可以立即点击。</p></div>`;
    case 'calling':
      return `<div class="game-panel call-panel"><div class="panel-eyebrow">INCOMING CALL</div><h2>Banker 来电</h2><p>这轮的报价，即将揭晓。</p><button class="primary-button" data-action="answer">接听电话 <span>↗</span></button></div>`;
    case 'offer':
      return `<div class="game-panel offer-panel"><div class="panel-eyebrow">${game.confirmingDeal ? 'FINAL CONFIRMATION' : 'THE BANKER OFFERS'}</div><h2>${formatMoney(game.bankerOffer)}</h2><p>${game.confirmingDeal ? '确定接受吗？接受后本局立即结束。' : '这是有保障的奖金。下一步由你决定。'}</p><div class="panel-actions"><button class="primary-button" data-action="deal">${game.confirmingDeal ? '确定成交' : 'DEAL · 成交'}</button><button class="secondary-button" data-action="${game.confirmingDeal ? 'cancel' : 'no-deal'}">${game.confirmingDeal ? '返回报价' : 'NO DEAL · 继续'}</button></div><p class="panel-footnote">${game.offerHistory.length > 1 ? `上一轮报价 ${formatMoney(game.offerHistory.at(-2)!)}` : '首次报价 · 决策不限时'}</p></div>`;
    case 'final':
      return `<div class="game-panel final-panel"><div class="panel-eyebrow">ONE LAST CHOICE</div><h2>最后两只箱子</h2><p>两只都仍然封存。你会留下自己的，还是交换？</p><div class="panel-actions"><button class="primary-button" data-action="keep">保留 #${String(game.playerCase).padStart(2, '0')}</button><button class="secondary-button" data-action="swap">换成 #${String(game.finalAlternative).padStart(2, '0')}</button></div></div>`;
    case 'gameover':
      return `<div class="game-panel result-panel"><div class="panel-eyebrow">${game.resultHeadline}</div><h2>${formatMoney(game.winnings)}</h2><p>${game.resultDetail}</p><div class="result-meta">你听到了 ${game.offerHistory.length} 次 Banker 报价</div><button class="primary-button" data-action="replay">再玩一局 <span>↻</span></button></div>`;
  }
}

function renderBoard(): void {
  const opened = new Set(game.cases.filter(item => item.opened).map(item => item.amount));
  document.querySelector<HTMLElement>('#prize-board')!.innerHTML = PRIZES.map((value, index) =>
    `<div class="prize-value ${index < 13 ? 'low' : 'high'} ${opened.has(value) ? 'removed' : ''}" aria-label="${formatMoney(value)}${opened.has(value) ? '，已移除' : '，仍在场'}">${formatMoney(value)}</div>`).join('');
}

function render(): void {
  document.querySelector<HTMLElement>('#phase-title')!.textContent = phaseTitle();
  document.querySelector<HTMLElement>('#your-case')!.textContent = game.playerCase === null
    ? '你的箱子 · 未选择'
    : `你的箱子 · #${String(game.playerCase).padStart(2, '0')} · ${game.phase === 'gameover' ? formatMoney(game.cases[game.playerCase - 1].amount) : '封存中'}`;
  document.querySelector<HTMLElement>('#remaining-stat')!.textContent =
    `${game.remaining.length} 箱仍在场 · 最高奖金 ${formatMoney(game.highestRemaining)}`;
  document.querySelector<HTMLElement>('#round-heading')!.textContent =
    game.phase === 'gameover' ? '这一局的结局' : game.phase === 'welcome' ? '你的选择，决定结局' : `第 ${game.roundIndex + 1} 轮 · 26 箱游戏`;
  document.querySelector<HTMLElement>('#instruction')!.textContent = instruction();
  document.querySelector<HTMLElement>('#round-badge')!.textContent = `${String(game.roundIndex + 1).padStart(2, '0')} / 09`;
  document.querySelector<HTMLElement>('#round-track')!.innerHTML = Array.from({ length: 9 }, (_, index) =>
    `<div class="round-segment ${index < game.roundIndex ? 'done' : index === game.roundIndex ? 'current' : ''}" title="第 ${index + 1} 轮 · 开 ${[6, 5, 4, 3, 2, 1, 1, 1, 1][index]} 箱"></div>`).join('');
  document.querySelector<HTMLElement>('#opened-stat')!.textContent = `${game.openedThisRound} / ${game.roundTarget}`;
  document.querySelector<HTMLElement>('#cases-stat')!.textContent = String(game.remaining.length);
  document.querySelector<HTMLElement>('#offer-stat')!.textContent = game.offerHistory.length
    ? formatMoney(game.offerHistory.at(-1)!) : '尚未报价';
  document.querySelector<HTMLElement>('#offer-history')!.innerHTML = game.offerHistory.length
    ? game.offerHistory.map((offer, index) => `<span class="history-chip">${index + 1}. ${formatMoney(offer)}</span>`).join('')
    : '尚无报价';
  renderBoard();
  overlay.innerHTML = panel();
  section.classList.toggle('board-camera', stage?.cameraIndex === 3);
  section.classList.toggle('selection-active', game.phase === 'choose' || game.phase === 'open');
  section.classList.toggle('is-paused', paused);
  const controls = document.querySelector<HTMLElement>('#camera-controls')!;
  controls.innerHTML = ['全景', '中央桌', '箱阵', '金额板'].map((name, index) =>
    `<button type="button" class="camera-button ${stage?.cameraIndex === index ? 'active' : ''}" data-action="camera" data-camera="${index}" aria-pressed="${stage?.cameraIndex === index}"><span>0${index + 1}</span> ${name}</button>`).join('');
  document.querySelector<HTMLElement>('#auto-camera')!.textContent = `自动机位 · ${automaticCameras ? '开' : '关'}`;
  document.querySelector<HTMLElement>('#sound-button')!.textContent = `声音 · ${soundEnabled ? '开' : '关'}`;
  document.querySelector<HTMLElement>('#pace-button')!.textContent = `节奏 · ${fastPace ? '快速' : '演出'}`;
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
