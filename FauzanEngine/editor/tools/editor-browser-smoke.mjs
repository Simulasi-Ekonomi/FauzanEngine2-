import { chromium } from 'playwright';

const baseUrl = process.env.EDITOR_URL || 'http://127.0.0.1:4173';
const browser = await chromium.launch({ headless: true, executablePath: '/usr/bin/chromium', args: ['--no-sandbox', '--disable-dev-shm-usage'] });
const context = await browser.newContext();
const page = await context.newPage();
try {
  await page.goto(baseUrl, { waitUntil: 'networkidle' });
  await page.getByText('NEOENGINE', { exact: true }).waitFor();
  const actorCount = () => page.locator('text=/Actors: \\d+/').first().innerText();
  if (!(await actorCount()).includes('Actors: 4')) throw new Error(`unexpected initial actor count: ${await actorCount()}`);
  await page.getByTitle('Add Cube').click();
  await page.getByText('Actors: 5', { exact: true }).first().waitFor();
  await page.getByTitle('Save All (Ctrl+S)').waitFor();
  if (!(await page.locator('body').innerText()).includes('Unsaved')) throw new Error('add actor did not set dirty state');
  const locationX = page.locator('input[type="number"]').first();
  await locationX.fill('2.5');
  await page.keyboard.press('Enter');
  if (!(await locationX.inputValue()).includes('2.5')) throw new Error('inspector transform did not update');
  await page.getByText('+ Add Component', { exact: true }).click();
  await page.getByText(/Scene \(SceneComponent\)/).first().waitFor();
  const parent = page.locator('select').first();
  await parent.selectOption({ label: 'DirectionalLight' });
  await page.getByText('DirectionalLight', { exact: true }).first().waitFor();
  await page.keyboard.press('e');
  if (!(await page.getByTitle('Rotate (E)').getAttribute('class')).includes('active')) throw new Error('E shortcut did not select rotate mode');
  await page.getByTitle('Save All (Ctrl+S)').click();
  await page.getByText(/✓ Saved/).waitFor();
  console.log(`EDITOR_BROWSER_SMOKE_OK initial=4 add=5 transform=1 component=1 reparent=1 shortcut=1 save=1`);
} finally {
  await browser.close();
}
