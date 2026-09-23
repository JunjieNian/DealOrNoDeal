import { chromium } from 'playwright-core';
import { mkdir } from 'node:fs/promises';
import { resolve } from 'node:path';

const origin = process.env.WEB_TEST_URL ?? 'http://127.0.0.1:4173/';
const testUrl = new URL(origin);
testUrl.searchParams.set('seed', '20260914');
const images = resolve(import.meta.dirname, '../../Saved/WebQA');
await mkdir(images, { recursive: true });

const browser = await chromium.launch({
  headless: true,
  executablePath: process.env.CHROME_PATH ?? 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
  args: ['--enable-webgl', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});

const issues = [];
const page = await browser.newPage({ viewport: { width: 1600, height: 900 }, deviceScaleFactor: 1 });
page.on('pageerror', error => issues.push(`page: ${error.message}`));
page.on('requestfailed', request => issues.push(`request: ${request.url()} ${request.failure()?.errorText}`));
page.on('console', message => {
  if (message.type() === 'error') issues.push(`console: ${message.text()}`);
});

const action = async name => page.locator(`[data-action="${name}"]`).first().click();
const assertLanguage = async (target, lang, fragment) => {
  const actual = await target.locator('html').getAttribute('lang');
  if (actual !== lang) throw new Error(`Expected html lang ${lang}, got ${actual}`);
  const title = await target.locator('#phase-title').innerText();
  if (!title.includes(fragment)) throw new Error(`Expected ${fragment} in title, got ${title}`);
};
const assertCaseRail = async target => {
  const boxes = await target.evaluate(() => {
    const view = document.querySelector('.stage-view').getBoundingClientRect();
    const panel = document.querySelector('.selection-panel').getBoundingClientRect();
    return {
      view: { left: view.left, right: view.right, width: view.width },
      panel: { left: panel.left, right: panel.right, width: panel.width },
      sectionWidth: document.querySelector('.stage-section').getBoundingClientRect().width,
      scrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
  if (boxes.view.width < 350 || boxes.panel.width < 270 ||
      boxes.panel.left < boxes.view.right - 1 ||
      boxes.panel.right > boxes.sectionWidth + 1 ||
      boxes.scrollWidth > boxes.windowWidth + 2) {
    throw new Error(`Cam 3 case rail overlaps or overflows: ${JSON.stringify(boxes)}`);
  }
};
const phase = async name => {
  const title = await page.locator('#phase-title').innerText();
  if (!title.includes(name)) throw new Error(`Expected phase ${name}, got ${title}`);
};
const openOne = async () => {
  await page.locator('.case-button:not(:disabled)').first().click();
  await action('confirm');
  const title = await page.locator('#phase-title').innerText();
  if (title.includes('CASE REVEALED')) {
    // The automatic reveal timer may advance while Playwright scrolls to the button.
    await page.locator('[data-action="continue"]').click({ timeout: 1200 }).catch(() => undefined);
  }
  await page.waitForFunction(() => !document.querySelector('.reveal-panel'), null, { timeout: 5000 });
};

try {
  await page.goto(testUrl.href, { waitUntil: 'networkidle' });
  await page.waitForSelector('.model-ready', { timeout: 30000 });
  await assertLanguage(page, 'en', 'WELCOME TO THE STUDIO');
  if (!await page.locator('.stage-view canvas').getAttribute('aria-label').then(value => value?.startsWith('Interactive 3D'))) {
    throw new Error('English canvas accessibility label is missing');
  }
  if (!await page.locator('meta[name="description"]').getAttribute('content').then(value => value?.startsWith('Step into the studio'))) {
    throw new Error('English metadata is missing');
  }
  await page.screenshot({ path: resolve(images, 'desktop-welcome.png') });
  await action('start');
  await phase('CHOOSE YOUR CASE');
  await page.locator('[data-action="camera"][data-camera="2"]').click();
  await page.waitForTimeout(900);
  await assertCaseRail(page);
  await page.screenshot({ path: resolve(images, 'desktop-cam3-en.png') });
  await page.locator('.case-button[data-number="12"]').click();
  const beforeLanguageSwitch = await page.locator('.case-button.selected').getAttribute('data-number');
  await action('language');
  await assertLanguage(page, 'zh-CN', '选择你的箱子');
  if (!new URL(page.url()).searchParams.get('lang') ||
      new URL(page.url()).searchParams.get('seed') !== '20260914') {
    throw new Error('Language switch did not preserve seed or URL selection');
  }
  if (await page.locator('.case-button.selected').getAttribute('data-number') !== beforeLanguageSwitch ||
      await page.locator('#cases-stat').innerText() !== '26') {
    throw new Error('Language switch changed the game state');
  }
  await assertCaseRail(page);
  await page.screenshot({ path: resolve(images, 'desktop-cam3-zh.png') });
  await action('language');
  await assertLanguage(page, 'en', 'CHOOSE YOUR CASE');
  if (new URL(page.url()).searchParams.has('lang')) throw new Error('English default must not require a lang parameter');
  if (await page.locator('.case-button.selected').getAttribute('data-number') !== beforeLanguageSwitch) {
    throw new Error('Switching back to English changed the selected case');
  }
  await page.setViewportSize({ width: 820, height: 768 });
  await assertCaseRail(page);
  await page.screenshot({ path: resolve(images, 'desktop-cam3-narrow.png') });
  await page.setViewportSize({ width: 1600, height: 900 });
  await page.locator('[data-action="camera"][data-camera="3"]').click();
  await page.waitForTimeout(900);
  await page.screenshot({ path: resolve(images, 'desktop-cam4-choose.png') });
  await action('confirm');
  for (const count of [6, 5, 4, 3, 2, 1, 1, 1, 1]) {
    for (let i = 0; i < count; i++) await openOne();
    await phase('INCOMING CALL');
    await action('answer');
    await phase('DEAL OR NO DEAL');
    if (count === 6) {
      await page.waitForTimeout(900);
      await page.screenshot({ path: resolve(images, 'desktop-first-offer.png') });
      const offer = await page.locator('.offer-panel h2').innerText();
      await action('language');
      await assertLanguage(page, 'zh-CN', '成交还是继续');
      if (await page.locator('.offer-panel h2').innerText() !== offer ||
          !await page.locator('.offer-panel [data-action="no-deal"]').innerText().then(value => value.includes('继续'))) {
        throw new Error('Changing language at a Banker offer changed the offer or missed Chinese copy');
      }
      await action('language');
      await assertLanguage(page, 'en', 'DEAL OR NO DEAL');
    }
    await action('no-deal');
  }
  await phase('THE FINAL TWO CASES');
  await page.screenshot({ path: resolve(images, 'desktop-final-choice.png') });
  await action('swap');
  await phase('FINAL REVEAL');
  await page.screenshot({ path: resolve(images, 'desktop-result.png') });
  const winnings = await page.locator('.result-panel h2').innerText();
  await action('language');
  await assertLanguage(page, 'zh-CN', '最终揭晓');
  if (await page.locator('.result-panel h2').innerText() !== winnings ||
      !await page.locator('.result-panel p').innerText().then(value => value.includes('号箱'))) {
    throw new Error('Result translation changed winnings or missed Chinese case detail');
  }
  await action('language');
  await action('replay');
  await phase('CHOOSE YOUR CASE');

  await page.locator('.case-button[data-number="1"]').click();
  await action('confirm');
  for (let i = 0; i < 6; i++) await openOne();
  await action('answer');
  await action('deal');
  if (!await page.locator('[data-action="cancel"]').isVisible()) throw new Error('Deal confirmation missing');
  await action('deal');
  await phase('DEAL ACCEPTED');

  const mobile = await browser.newPage({ viewport: { width: 390, height: 844 }, deviceScaleFactor: 1, isMobile: true, hasTouch: true });
  mobile.on('pageerror', error => issues.push(`mobile page: ${error.message}`));
  await mobile.goto(testUrl.href, { waitUntil: 'networkidle' });
  await mobile.waitForSelector('.model-ready', { timeout: 30000 });
  await assertLanguage(mobile, 'en', 'WELCOME TO THE STUDIO');
  await mobile.locator('[data-action="start"]').tap();
  const mobileLayout = await mobile.evaluate(() => ({
    viewBottom: document.querySelector('.stage-view').getBoundingClientRect().bottom,
    panelTop: document.querySelector('.selection-panel').getBoundingClientRect().top,
  }));
  if (mobileLayout.panelTop < mobileLayout.viewBottom - 1) {
    throw new Error(`Mobile panel overlaps the stage: ${JSON.stringify(mobileLayout)}`);
  }
  await mobile.screenshot({ path: resolve(images, 'mobile-choose-en.png'), fullPage: true });
  await mobile.locator('[data-action="language"]').tap();
  await assertLanguage(mobile, 'zh-CN', '选择你的箱子');
  await mobile.screenshot({ path: resolve(images, 'mobile-choose-zh.png'), fullPage: true });
  const overflow = await mobile.evaluate(() => document.documentElement.scrollWidth - window.innerWidth);
  if (overflow > 2) throw new Error(`Mobile horizontal overflow: ${overflow}px`);
  await mobile.locator('.case-button[data-number="7"]').tap();
  await mobile.locator('[data-action="confirm"]').tap();
  await mobile.locator('.case-button:not(:disabled)').first().tap();
  await mobile.locator('[data-action="confirm"]').tap();
  if (!await mobile.locator('.reveal-panel').isVisible()) throw new Error('Mobile reveal missing');
  await mobile.close();

  const directChineseUrl = new URL(testUrl.href);
  directChineseUrl.searchParams.set('lang', 'zh');
  const directChinese = await browser.newPage();
  await directChinese.goto(directChineseUrl.href, { waitUntil: 'domcontentloaded' });
  await directChinese.waitForSelector('.welcome-panel');
  await assertLanguage(directChinese, 'zh-CN', '欢迎来到摄影棚');
  await directChinese.close();

  if (issues.length) throw new Error(issues.join('\n'));
  console.log(`BROWSER_QA PASS bilingual state, Cam 3 rail, Cam 4, desktop full game, early deal, 3D load, 390px touch; screenshots: ${images}`);
} catch (error) {
  console.error('BROWSER_QA DEBUG', await page.locator('#loading').allTextContents(), issues);
  throw error;
} finally {
  await browser.close();
}
