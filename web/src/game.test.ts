import { describe, expect, it } from 'vitest';
import { CASES_PER_ROUND, DealGame, PRIZES, calculateOffer, formatMoney } from './game';

function openOne(game: DealGame): void {
  const number = game.remaining.find(item => item.number !== game.playerCase)!.number;
  expect(game.select(number)).toBe(true);
  expect(game.confirm()).toBe(true);
  expect(game.phase).toBe('reveal');
  expect(game.completeReveal()).toBe(true);
}

describe('the original studio rules', () => {
  it('deals each of the 26 prizes exactly once and hides the kept case', () => {
    const game = new DealGame(20260914);
    expect(game.cases.map(item => item.amount).sort((a, b) => a - b)).toEqual([...PRIZES]);
    expect(game.start()).toBe(true);
    game.select(12);
    game.confirm();
    expect(game.playerCase).toBe(12);
    expect(game.canSelect(12)).toBe(false);
    expect(game.remaining).toHaveLength(26);
    expect(new DealGame(20260914).cases).toEqual(new DealGame(20260914).cases);
  });

  it('uses cents, the banker multiplier and the original rounding thresholds', () => {
    expect(formatMoney(1)).toBe('$0.01');
    expect(formatMoney(100000000)).toBe('$1,000,000');
    expect(calculateOffer([100, 100, 100], 0)).toBe(100);
    expect(calculateOffer([100000000, 100000000], 8)).toBe(88000000);
  });

  it('completes all nine rounds, then offers keep or swap', () => {
    const game = new DealGame(17);
    game.start();
    game.select(8);
    game.confirm();
    CASES_PER_ROUND.forEach((count, index) => {
      for (let i = 0; i < count; i++) openOne(game);
      expect(game.phase).toBe('calling');
      expect(game.answerBanker()).toBe(true);
      expect(game.phase).toBe('offer');
      expect(game.offerHistory).toHaveLength(index + 1);
      expect(game.rejectDeal()).toBe(true);
      expect(game.phase).toBe(index === 8 ? 'final' : 'open');
    });
    expect(game.remaining).toHaveLength(2);
    const alternative = game.finalAlternative!;
    const amount = game.cases[alternative - 1].amount;
    expect(game.chooseFinal(true)).toBe(true);
    expect(game.playerCase).toBe(alternative);
    expect(game.winnings).toBe(amount);
    expect(game.phase).toBe('gameover');
  });

  it('requires a second confirmation to take a banker offer', () => {
    const game = new DealGame(29);
    game.start();
    game.confirm();
    for (let i = 0; i < 6; i++) openOne(game);
    game.answerBanker();
    const offer = game.bankerOffer;
    game.acceptDeal();
    expect(game.phase).toBe('offer');
    expect(game.confirmingDeal).toBe(true);
    game.cancelPending();
    expect(game.confirmingDeal).toBe(false);
    game.acceptDeal();
    game.acceptDeal();
    expect(game.winnings).toBe(offer);
    expect(game.phase).toBe('gameover');
    expect(game.resultDetail).toContain(formatMoney(game.cases[game.playerCase! - 1].amount));
  });
});
