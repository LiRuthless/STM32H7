const fs=require('fs'),path=require('path');
const packages=process.env.REPORT_NODE_MODULES||'C:/Users/Li/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules';
const root=path.resolve(__dirname,'..');
const math=require('./math_renderer.cjs');
const css=`*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;color:#1c303c;background:#eaf0f4;font:16px/1.8 "Microsoft YaHei",Arial,sans-serif}.bar{position:sticky;top:0;z-index:5;background:#163d52;color:white;padding:10px 22px;display:flex;gap:20px;align-items:center;flex-wrap:wrap}.bar a{color:white}.bar button{background:white;color:#163d52;border:0;border-radius:4px;padding:6px 12px;cursor:pointer}main{max-width:1160px;margin:24px auto;padding:38px 50px;background:white;box-shadow:0 3px 20px #0001}h1{font-size:30px;line-height:1.4;color:#163d52}h2{font-size:25px;line-height:1.45;margin:42px 0 18px;padding-top:10px;border-top:2px solid #dae6ed;scroll-margin-top:90px;color:#163d52}h3{font-size:20px;line-height:1.5;margin:26px 0 12px;scroll-margin-top:90px}p{margin:13px 0}a{color:#076394;overflow-wrap:anywhere}code{font-family:Consolas,"Microsoft YaHei",monospace;font-size:.91em;overflow-wrap:anywhere}pre{margin:18px 0;padding:16px 18px;background:#eef4f7;border:1px solid #dae6ed;border-radius:6px;white-space:pre-wrap;overflow-wrap:anywhere;line-height:1.65}pre code{font-size:13.5px;white-space:pre-wrap}table{border-collapse:collapse;width:100%;table-layout:fixed;font-size:13.5px;line-height:1.65;margin:20px 0}th,td{border:1px solid #c8d7e0;padding:9px 10px;text-align:left;vertical-align:top;overflow-wrap:anywhere;word-break:normal}th{background:#e8f1f6;font-weight:600}tbody tr:nth-child(even){background:#f7fafb}li{margin:6px 0}blockquote{background:#eff7fa;padding:12px 20px;border-left:4px solid #3788a7;margin:18px 0}figure{margin:24px 0;break-inside:avoid}figure svg{width:100%;height:auto;display:block;border:1px solid #d9e4eb}figcaption{color:#526b79;font-size:13px;text-align:center;margin-top:8px}.toc{padding:16px 22px;background:#eef5f8;border-radius:6px}.toc ol{list-style:none;columns:2;column-gap:26px;margin:10px 0;padding-left:22px}.toc li{break-inside:avoid;font-size:14px}.footer{color:#526b79;border-top:1px solid #d9e4eb;padding-top:16px;font-size:13px}@media(max-width:700px){body{font-size:15px}main{margin:0;padding:22px 15px;box-shadow:none}h1{font-size:25px}h2{font-size:22px}.toc ol{columns:1}table{font-size:12px}th,td{padding:7px 6px}pre{padding:12px}pre code{font-size:12px}.bar{position:relative;font-size:13px}}@media print{@page{size:A4;margin:16mm 14mm}body{background:white;font-size:10.5pt;line-height:1.65}.bar{display:none}main{max-width:none;margin:0;padding:0;box-shadow:none}h1{font-size:21pt}h2{font-size:16pt;break-after:avoid;scroll-margin-top:0}h3{font-size:12pt;break-after:avoid;scroll-margin-top:0}table{font-size:8.5pt;line-height:1.5}th,td{padding:5px 6px}thead{display:table-header-group}tr{break-inside:avoid}pre{font-size:8.5pt;padding:8px 10px;break-inside:avoid}pre code{font-size:8.3pt}figure{break-inside:avoid;max-width:100%}a{color:#1c303c;text-decoration:underline}.toc{background:white;border:1px solid #cbd8df;padding:10px 15px}.toc ol{columns:2}.toc li{font-size:8.5pt}.footer{font-size:8pt}}`;
(async()=>{
 const {marked}=await import(require('url').pathToFileURL(path.join(packages,'marked/lib/marked.esm.js')).href);
 for(const [input,output] of [['report.md','报告.html'],['sources.md','sources.html'],['coverage.md','coverage.html']]){
  if(!fs.existsSync(path.join(root,input)))continue;
  const protectedMath=math.protectMath(fs.readFileSync(path.join(root,input),'utf8'));
  let html=protectedMath.restore(marked.parse(protectedMath.source,{gfm:true}));
  let heads=[],index=0;
  html=html.replace(/<h([123])>([\s\S]*?)<\/h\1>/g,(all,level,content)=>{
   const id=`section-${++index}`;if(level==='2')heads.push([id,content]);return `<h${level} id="${id}">${content}</h${level}>`;
  });
  html=html.replace(/<p><img src="(figures\/[^\"]+\.svg)" alt="([^\"]*)"\s*\/?><\/p>/g,(all,file,alt)=>{
   const target=path.join(root,file);
   const raw=fs.readFileSync(target,'utf8').replace(/<style data-math-diagram(?:="true")?>[\s\S]*?<\/style>/g,'');
   const rendered=math.typesetDiagram(raw);
   if(rendered.includes('math-diagram'))fs.writeFileSync(target,rendered.replace('</title>',`</title><style data-math-diagram="true">${math.mathCss+math.layoutCss}</style>`),'utf8');
   return `<figure>${rendered}<figcaption>${alt}</figcaption></figure>`;
  });
  const title=(html.match(/<h1[^>]*>([\s\S]*?)<\/h1>/)||[])[1]||'中文研究报告';
  const toc=`<nav class="toc" aria-label="目录"><strong>目录</strong><ol>${heads.map(([id,t])=>`<li><a href="#${id}">${t}</a></li>`).join('')}</ol></nav>`;
  const firstEnd=html.indexOf('</h1>')+5;html=html.slice(0,firstEnd)+toc+html.slice(firstEnd);
  const page=`<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>${title}</title><style>${css}</style></head><body><div class="bar"><a href="报告.html">主报告</a><a href="report.md" download>Markdown</a><a href="sources.html">文献索引</a><a href="coverage.html">16项覆盖表</a><button onclick="window.print()">打印 / 保存PDF</button></div><main>${html}<p class="footer">离线正文、图表与公式已内嵌。外部来源链接需联网。本次数值算例不代表实车性能。</p></main></body></html>`;
  const finalPage=page.replace('</style>',math.mathCss+math.layoutCss+'</style>').replace('figure svg{','figure>svg{');
  fs.writeFileSync(path.join(root,output),finalPage,'utf8');console.log(output,Buffer.byteLength(finalPage),'bytes',protectedMath.displays,'display',protectedMath.inlines,'inline formulas');
 }
})().catch(e=>{console.error(e);process.exit(1)});
