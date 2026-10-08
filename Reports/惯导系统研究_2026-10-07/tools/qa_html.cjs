// Headless local artifact rendering only; no external page is visited.
const fs = require('fs');
const path = require('path');
const {pathToFileURL} = require('url');
const packages = process.env.REPORT_NODE_MODULES || 'C:/Users/Li/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules';
const { chromium } = require(path.join(packages,'playwright'));
const root = path.resolve(__dirname,'..');
(async () => {
 const qa=path.join(root,'qa',process.env.REPORT_QA_RUN||'math_2026-10-08');fs.mkdirSync(qa,{recursive:true});
 const browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true,args:['--disable-gpu']});
 const context=await browser.newContext({viewport:{width:1280,height:1000},offline:true});
 const page=await context.newPage();let requests=[];let errors=[];
 page.on('request',r=>{if(/^https?:/.test(r.url()))requests.push(r.url());});
 page.on('pageerror',e=>errors.push(e.message));
 await page.goto(pathToFileURL(path.join(root,'报告.html')).href,{waitUntil:'load'});
 await page.evaluate(()=>document.fonts.ready);
 await page.locator('#equation-6-6').screenshot({path:path.join(qa,'equation_6_6.png')});
 await page.locator('#equation-6-7').screenshot({path:path.join(qa,'equation_6_7.png')});
 await page.screenshot({path:path.join(qa,'desktop_top.png')});
 for(let i=0;i<4;i++)await page.locator('figure').nth(i<2?i:i+1).screenshot({path:path.join(qa,`diagram_${i+1}.png`)});
 const sections=[['fusion','6.'],['control','11.'],['cases','16.']];
 for(const [name,prefix] of sections){
  const h=page.locator('h2').filter({hasText:new RegExp('^'+prefix.replace('.','\\.'))}).first();
  await h.scrollIntoViewIfNeeded();await page.screenshot({path:path.join(qa,`desktop_${name}.png`)});
 }
 const desktop=await page.evaluate(()=>({title:document.title,h2:document.querySelectorAll('h2').length,svg:document.querySelectorAll('svg').length,tables:document.querySelectorAll('table').length,bodyWidth:document.body.scrollWidth,viewport:innerWidth,brokenAnchors:[...document.querySelectorAll('a[href^="#"]')].map(a=>a.getAttribute('href')).filter(id=>id.length>1&&!document.getElementById(decodeURIComponent(id.slice(1))))}));
 await page.emulateMedia({media:'print'});await page.setViewportSize({width:794,height:1123});
 await page.locator('h2').filter({hasText:/^6\./}).first().scrollIntoViewIfNeeded();
 await page.screenshot({path:path.join(qa,'print_fusion.png')});
 const print=await page.evaluate(()=>({bodyWidth:document.body.scrollWidth,viewport:innerWidth,overflowElements:[...document.querySelectorAll('table,pre,figure>svg,.math-display,.math-display>.katex-display>.katex,.math-inline')].filter(e=>e.getBoundingClientRect().right>innerWidth+2).map(e=>({tag:e.tagName,id:e.id,text:e.textContent.slice(0,60),right:e.getBoundingClientRect().right}))}));
 await page.emulateMedia({media:'screen'});await page.setViewportSize({width:390,height:844});await page.evaluate(()=>scrollTo(0,0));
 await page.screenshot({path:path.join(qa,'mobile_top.png')});
 const mobile=await page.evaluate(()=>({bodyWidth:document.body.scrollWidth,viewport:innerWidth}));
 const formulas=await page.evaluate(()=>({display:document.querySelectorAll('.math-display').length,inline:document.querySelectorAll('.math-inline').length,parseErrors:document.querySelectorAll('.katex-error').length,rawFormulaFences:document.querySelectorAll('pre code.language-text').length}));
 const result={scope:'离线HTML屏幕及打印样式检查，不是实车验证',browserVersion:browser.version(),nodeVersion:process.version,desktop,print,mobile,formulas,externalRequests:requests,pageErrors:errors};
 fs.writeFileSync(path.join(qa,'browser_check.json'),JSON.stringify(result,null,2),'utf8');
 console.log(JSON.stringify(result,null,2));await browser.close();
 if(requests.length||errors.length||desktop.brokenAnchors.length||print.overflowElements.length||desktop.bodyWidth>desktop.viewport+2||mobile.bodyWidth>mobile.viewport+2)process.exitCode=1;
})().catch(e=>{console.error(e);process.exit(1);});
