import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {createRequire} from 'node:module';
const root=path.dirname(fileURLToPath(import.meta.url));
const markedPath=process.env.MARKED_PATH;
if(!markedPath) throw Error('Set MARKED_PATH to marked/lib/marked.esm.js');
const {marked}=await import(pathToFileURL(markedPath));
const require=createRequire(import.meta.url);
const katex=require('./vendor/katex/dist/katex.js');
let mathCss=fs.readFileSync(path.join(root,'vendor/katex/dist/katex.min.css'),'utf8');
mathCss=mathCss.replace(/src:[^;]*;/g,src=>{
  const match=src.match(/url\((?:"|')?(fonts\/[^)"']+\.woff2)(?:"|')?\)/);
  if(!match)throw Error('Unexpected KaTeX font source');
  const font=fs.readFileSync(path.join(root,'vendor/katex/dist',match[1])).toString('base64');
  return `src:url("data:font/woff2;base64,${font}") format("woff2");`;
});
const escape=s=>s.replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;');
const md=fs.readFileSync(path.join(root,'report.md'),'utf8');
const codeNames=['optical.py','inertial.py','demo.py','test_algorithms.py'];
let appendix='\n\n## 附录 完整可运行代码\n\n以下文件与下载目录中的代码一致，Python 3.10 及以上，仅依赖标准库。\n';
for(const f of codeNames) appendix+=`\n### ${f}\n\n\`\`\`python\n${fs.readFileSync(path.join(root,'code',f),'utf8')}\n\`\`\`\n`;
fs.writeFileSync(path.join(root,'report_full.md'),md+appendix,'utf8');
const slots=[];
const audit=[];
function mathSlot(latex,display){
  const id=slots.length;
  const rendered=katex.renderToString(latex.trim(),{displayMode:display,throwOnError:true,strict:'error',output:'htmlAndMathml',trust:false});
  slots.push(display?`<div class="equation">${rendered}</div>`:`<span class="math-inline">${rendered}</span>`);
  audit.push({id,display,latex:latex.trim()});
  return display?`\n\n<div data-math-slot="${id}"></div>\n\n`:`<span data-math-slot="${id}"></span>`;
}
// Protect code before recognizing mathematical delimiters; code stays byte-for-byte intact.
function mathSource(source){
  return source.split(/(```[\s\S]*?```|`[^`\n]*`)/g).map((part,i)=>{
    if(i%2)return part;
    return part.replace(/\$\$([\s\S]*?)\$\$/g,(_,latex)=>mathSlot(latex,true))
      .replace(/\$([^$\n]+)\$/g,(_,latex)=>mathSlot(latex,false));
  }).join('');
}
let content=marked.parse(mathSource(md+appendix));
content=content.replace(/<(?:div|span) data-math-slot="(\d+)"><\/(?:div|span)>/g,(_,id)=>slots[Number(id)]);
fs.writeFileSync(path.join(root,'qa/math_render_audit.json'),JSON.stringify({engine:'KaTeX '+katex.version,total:audit.length,display:audit.filter(x=>x.display).length,inline:audit.filter(x=>!x.display).length,formulas:audit},null,2),'utf8');
content=content.replace(/<img[^>]*src="([^"]+)"[^>]*>/g,(tag,src)=>{
  const f=decodeURIComponent(src);
  return ['流程图.svg','思维导图.svg'].includes(f)?`<figure class="diagram">${fs.readFileSync(path.join(root,f),'utf8')}</figure>`:tag;
});
let sectionIndex=0;
const toc=[];
content=content.replace(/<h2>(.*?)<\/h2>/g,(tag,title)=>{
  const id='section-'+(++sectionIndex);
  toc.push(`<a href="#${id}">${title}</a>`);
  return `<h2 id="${id}">${title}</h2>`;
});
content=content.replace(/(<h1>.*?<\/h1>)/,`$1<details><summary>展开目录</summary><nav>${toc.join('<br>')}</nav></details>`);
let html=`<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>光电感知与惯性导航 技术报告</title><style>
${mathCss}
*{box-sizing:border-box}body{margin:0;background:#eaf0f5;color:#172633;font:16px/1.8 'Microsoft YaHei','Noto Sans CJK SC',sans-serif}main{max-width:1100px;margin:24px auto;padding:54px 60px;background:white;box-shadow:0 5px 24px #142d4512}h1{font-size:32px;line-height:1.4;color:#12354d}h2{font-size:25px;color:#12354d;margin-top:44px}h3{font-size:20px;margin-top:28px}h4{font-size:17px}p{margin:12px 0}a{color:#09659e;overflow-wrap:anywhere}table{border-collapse:collapse;width:100%;font-size:14px;margin:20px 0}th,td{border:1px solid #cdd9e2;padding:9px 12px;text-align:left;vertical-align:top}th{background:#eaf2f7}tr:nth-child(even){background:#fafcfe}pre{background:#f0f5f8;border-radius:6px;padding:18px;overflow-x:auto;font:13px/1.6 Consolas,'Microsoft YaHei',monospace;white-space:pre-wrap;overflow-wrap:anywhere}code{font-family:Consolas,'Microsoft YaHei',monospace;font-size:0.9em}blockquote{margin:20px 0;padding:12px 20px;background:#eef7fc;border-left:4px solid #2982ae}svg{width:100%;height:auto;margin:20px 0}img{max-width:100%}.toolbar{position:sticky;top:0;background:#12354d;color:white;padding:10px 24px;z-index:2}.toolbar a{color:white;margin-right:20px}.toolbar button{padding:7px 14px;border:0;border-radius:4px;color:#12354d;cursor:pointer} @media(max-width:760px){main{padding:24px 18px}table{font-size:12px}h1{font-size:26px}}@media print{body{background:white;font-size:10.5pt}main{max-width:none;margin:0;padding:0;box-shadow:none}.toolbar{display:none}h2,h3,h4{break-after:avoid}tr{break-inside:avoid}pre{font-size:8pt}a{color:#172633} @page{size:A4;margin:18mm}}
</style><style>
/* Keep mathematical glyphs independent of diagram sizing and code typography. */
.diagram{margin:20px 0}.diagram svg{width:100%;height:auto;margin:0}
.math-inline{white-space:normal}.math-inline .katex{font-size:1.08em}
.equation{margin:22px 0;padding:18px 22px;background:#f7fafc;border:1px solid #e1e9ef;border-radius:8px;overflow-x:auto;overflow-y:hidden;break-inside:avoid}
.equation .katex-display{margin:0}.equation .katex{font-size:1.15em}.equation .katex-display>.katex{white-space:nowrap}
td .katex,th .katex{font-size:1.02em}@media print{.equation{background:white;padding:10px 0;border:0}.equation .katex{font-size:1.05em}}
</style></head><body><div class="toolbar"><a href="#">技术报告</a><a href="report_full.md" download>下载 Markdown</a><a href="交付包.zip" download>报告与代码包</a><button onclick="window.print()">打印 / 保存 PDF</button></div><main>${content}</main></body></html>`;
html=html.replace('svg{width:100%;height:auto;margin:20px 0}', '.diagram svg{width:100%;height:auto;margin:20px 0}');
fs.writeFileSync(path.join(root,'光电感知与惯性导航_技术报告.html'),html,'utf8');
console.log(JSON.stringify({htmlCharacters:html.length,markdownCharacters:(md+appendix).length,codeFiles:codeNames}));
