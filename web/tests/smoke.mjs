import { chromium } from 'playwright-core';
import { mkdir } from 'node:fs/promises';
import { resolve } from 'node:path';

const origin = process.env.WEB_TEST_URL ?? 'http://127.0.0.1:4173/';
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
const phase = async name => {
  const title = await page.locator('#phase-title').innerText();
  if (!title.includes(name)) throw new Error(`Expected phase ${name}, got ${title}`);
};
const openOne = async () => {
  await page.locator('.case-button:not(:disabled)').first().click();
  await action('confirm');
  await phase('CASE REVEALED');
  await action('continue');
};

try {
  await page.goto(origin, { waitUntil: 'networkidle' });
  await page.waitForSelector('.model-ready', { timeout: 30000 });
  await page.screenshot({ path: resolve(images, 'desktop-welcome.png') });
  await action('start');
  await phase('CHOOSE YOUR CASE');
  await page.locator('[data-action="camera"][data-camera="3"]').click();
  await page.waitForTimeout(900);
  await page.screenshot({ path: resolve(images, 'desktop-cam4-choose.png') });
  await page.locator('.case-button[data-number="12"]').click();
  await action('confirm');
  for (const count of [6, 5, 4, 3, 2, 1, 1, 1, 1]) {
    for (let i = 0; i < count; i++) await openOne();
    await phase('INCOMING CALL');
    await action('answer');
    await phase('DEAL OR NO DEAL');
    if (count === 6) {
      await page.waitForTimeout(900);
      await page.screenshot({ path: resolve(images, 'desktop-first-offer.png') });
    }
    await action('no-deal');
  }
  await phase('THE FINAL TWO CASES');
  await page.screenshot({ path: resolve(images, 'desktop-final-choice.png') });
  await action('swap');
  await phase('FINAL REVEAL');
  await page.screenshot({ path: resolve(images, 'desktop-result.png') });
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
  await mobile.goto(origin, { waitUntil: 'networkidle' });
  await mobile.waitForSelector('.model-ready', { timeout: 30000 });
  await mobile.locator('[data-action="start"]').tap();
  await mobile.screenshot({ path: resolve(images, 'mobile-choose.png'), fullPage: true });
  const overflow = await mobile.evaluate(() => document.documentElement.scrollWidth - window.innerWidth);
  if (overflow > 2) throw new Error(`Mobile horizontal overflow: ${overflow}px`);
  await mobile.locator('.case-button[data-number="7"]').tap();
  await mobile.locator('[data-action="confirm"]').tap();
  await mobile.locator('.case-button:not(:disabled)').first().tap();
  await mobile.locator('[data-action="confirm"]').tap();
  if (!await mobile.locator('.reveal-panel').isVisible()) throw new Error('Mobile reveal missing');
  await mobile.close();

  if (issues.length) throw new Error(issues.join('\n'));
  console.log(`BROWSER_QA PASS desktop full game, early deal, 3D load, 390px touch; screenshots: ${images}`);
} catch (error) {
  console.error('BROWSER_QA DEBUG', await page.locator('#loading').allTextContents(), issues);
  throw error;
} finally {
  await browser.close();
}
