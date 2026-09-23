/** Browser-native port of AStageInteractionDirector's rules. All money is cents. */
export const PRIZES = [
  1, 100, 500, 1000, 2500, 5000, 7500,
  10000, 20000, 30000, 40000, 50000, 75000,
  100000, 500000, 1000000, 2500000, 5000000,
  7500000, 10000000, 20000000, 30000000, 40000000,
  50000000, 75000000, 100000000,
] as const;

export const CASES_PER_ROUND = [6, 5, 4, 3, 2, 1, 1, 1, 1] as const;

export type Phase = 'welcome' | 'choose' | 'open' | 'reveal' | 'calling' |
  'offer' | 'final' | 'gameover';

export interface CaseState {
  number: number;
  amount: number;
  opened: boolean;
}

export function formatMoney(cents: number): string {
  const dollars = Math.floor(cents / 100).toLocaleString('en-US');
  const rest = cents % 100;
  return `$${dollars}${rest ? `.${String(rest).padStart(2, '0')}` : ''}`;
}

export function calculateOffer(remaining: readonly number[], roundIndex: number): number {
  if (!remaining.length) return 0;
  const mean = remaining.reduce((sum, value) => sum + value, 0) / remaining.length;
  const raw = mean * Math.min(0.20 + 0.085 * roundIndex, 0.90);
  const unit = raw >= 100000 ? 10000 : 100;
  // All amounts are positive; this matches FMath::RoundToDouble in the UE game.
  return Math.max(100, Math.floor(raw / unit + 0.5) * unit);
}

function shuffledPrizes(seed: number): number[] {
  let state = seed >>> 0;
  const random = () => {
    state = (state + 0x6d2b79f5) >>> 0;
    let value = state;
    value = Math.imul(value ^ (value >>> 15), value | 1);
    value ^= value + Math.imul(value ^ (value >>> 7), value | 61);
    return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
  };
  const values: number[] = [...PRIZES];
  for (let i = values.length - 1; i > 0; i--) {
    const j = Math.floor(random() * (i + 1));
    [values[i], values[j]] = [values[j], values[i]];
  }
  return values;
}

export function randomSeed(): number {
  if (typeof crypto !== 'undefined' && 'getRandomValues' in crypto) {
    return crypto.getRandomValues(new Uint32Array(1))[0];
  }
  return Math.floor(Math.random() * 4294967296);
}

export class DealGame {
  phase: Phase = 'welcome';
  cases: CaseState[] = [];
  seed: number;
  selected = 1;
  selectionArmed = false;
  playerCase: number | null = null;
  roundIndex = 0;
  openedThisRound = 0;
  lastOpened: number | null = null;
  bankerOffer = 0;
  acceptedOffer = 0;
  confirmingDeal = false;
  offerHistory: number[] = [];
  winnings = 0;
  resultHeadline = '';
  resultDetail = '';

  constructor(seed = randomSeed()) {
    this.seed = seed >>> 0;
    this.reset(this.seed);
  }

  reset(seed = randomSeed()): void {
    this.seed = seed >>> 0;
    const values = shuffledPrizes(this.seed);
    this.cases = values.map((amount, index) => ({ number: index + 1, amount, opened: false }));
    this.phase = 'welcome';
    this.selected = 1;
    this.selectionArmed = false;
    this.playerCase = null;
    this.roundIndex = 0;
    this.openedThisRound = 0;
    this.lastOpened = null;
    this.bankerOffer = 0;
    this.acceptedOffer = 0;
    this.confirmingDeal = false;
    this.offerHistory = [];
    this.winnings = 0;
    this.resultHeadline = '';
    this.resultDetail = '';
  }

  get remaining(): CaseState[] { return this.cases.filter(item => !item.opened); }
  get highestRemaining(): number { return Math.max(...this.remaining.map(item => item.amount)); }
  get roundTarget(): number { return CASES_PER_ROUND[this.roundIndex]; }
  get finalAlternative(): number | null {
    return this.remaining.find(item => item.number !== this.playerCase)?.number ?? null;
  }
  get revealedAmount(): number { return this.lastOpened ? this.cases[this.lastOpened - 1].amount : 0; }

  start(): boolean {
    if (this.phase !== 'welcome') return false;
    this.phase = 'choose';
    return true;
  }

  canSelect(number: number): boolean {
    const item = this.cases[number - 1];
    return Number.isInteger(number) && !!item && !item.opened &&
      (this.phase === 'choose' || (this.phase === 'open' && number !== this.playerCase));
  }

  select(number: number): boolean {
    if (!this.canSelect(number)) return false;
    if (this.selected !== number) this.selectionArmed = false;
    this.selected = number;
    return true;
  }

  /** A second click on the same case confirms, exactly as in the UE HUD. */
  clickCase(number: number): boolean {
    if (!this.canSelect(number)) return false;
    if (this.selected === number && this.selectionArmed) return this.confirm();
    this.select(number);
    this.selectionArmed = true;
    return true;
  }

  moveSelection(direction: number): boolean {
    if (this.phase !== 'choose' && this.phase !== 'open') return false;
    this.selectionArmed = false;
    let candidate = this.selected;
    for (let i = 0; i < 26; i++) {
      candidate = ((candidate - 1 + (direction >= 0 ? 1 : 25)) % 26) + 1;
      if (this.canSelect(candidate)) {
        this.selected = candidate;
        return true;
      }
    }
    return false;
  }

  confirm(): boolean {
    if (this.phase === 'welcome') return this.start();
    if (this.phase === 'calling') return this.answerBanker();
    if (this.phase === 'reveal') return this.completeReveal();
    if (!this.canSelect(this.selected)) return false;
    this.selectionArmed = false;
    if (this.phase === 'choose') {
      this.playerCase = this.selected;
      this.phase = 'open';
      this.selected = this.nextAvailable(this.selected);
      return true;
    }
    const chosen = this.cases[this.selected - 1];
    chosen.opened = true;
    this.lastOpened = chosen.number;
    this.openedThisRound++;
    this.phase = 'reveal';
    return true;
  }

  private nextAvailable(start: number): number {
    let candidate = start;
    for (let i = 0; i < 26; i++) {
      candidate = candidate % 26 + 1;
      if (!this.cases[candidate - 1].opened && candidate !== this.playerCase) return candidate;
    }
    return start;
  }

  completeReveal(): boolean {
    if (this.phase !== 'reveal') return false;
    if (this.openedThisRound >= this.roundTarget || this.remaining.length <= 2) {
      this.phase = 'calling';
    } else {
      this.phase = 'open';
      this.selected = this.nextAvailable(this.selected);
    }
    return true;
  }

  answerBanker(): boolean {
    if (this.phase !== 'calling') return false;
    this.bankerOffer = calculateOffer(this.remaining.map(item => item.amount), this.roundIndex);
    this.offerHistory.push(this.bankerOffer);
    this.phase = 'offer';
    return true;
  }

  acceptDeal(): boolean {
    if (this.phase !== 'offer') return false;
    if (!this.confirmingDeal) {
      this.confirmingDeal = true;
      return true;
    }
    this.confirmingDeal = false;
    this.acceptedOffer = this.bankerOffer;
    this.finish(true);
    return true;
  }

  rejectDeal(): boolean {
    if (this.phase !== 'offer') return false;
    this.confirmingDeal = false;
    if (this.remaining.length <= 2) {
      this.phase = 'final';
      return true;
    }
    this.roundIndex = Math.min(this.roundIndex + 1, CASES_PER_ROUND.length - 1);
    this.openedThisRound = 0;
    this.bankerOffer = 0;
    this.phase = 'open';
    this.selected = this.nextAvailable(this.selected);
    return true;
  }

  cancelPending(): void {
    this.confirmingDeal = false;
    this.selectionArmed = false;
  }

  chooseFinal(swap: boolean): boolean {
    if (this.phase !== 'final' || this.playerCase === null) return false;
    const other = this.finalAlternative;
    if (other === null) return false;
    if (swap) this.playerCase = other;
    this.finish(false);
    return true;
  }

  private finish(accepted: boolean): void {
    if (this.playerCase === null) return;
    const player = this.cases[this.playerCase - 1];
    const other = this.finalAlternative;
    this.winnings = accepted ? this.acceptedOffer : player.amount;
    this.resultHeadline = accepted ? 'DEAL ACCEPTED' : 'FINAL REVEAL';
    this.resultDetail = accepted
      ? `你接受了 ${formatMoney(this.acceptedOffer)}。你的 ${this.playerCase} 号箱中是 ${formatMoney(player.amount)}。`
      : `你的 ${this.playerCase} 号箱中是 ${formatMoney(player.amount)}。另一个 ${other} 号箱中是 ${formatMoney(this.cases[other! - 1].amount)}。`;
    this.phase = 'gameover';
  }
}
