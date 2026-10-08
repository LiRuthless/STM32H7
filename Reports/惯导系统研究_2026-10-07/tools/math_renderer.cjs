// Build-time KaTeX only. The delivered HTML includes all CSS/fonts and needs no JS.
const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'..');
const vendor=path.join(root,'assets/katex');
const katex=require(path.join(vendor,'dist/katex.js'));
const mathCss=fs.readFileSync(path.join(vendor,'dist/katex.min.css'),'utf8').replace(/src:[^;}]+/g,src=>{
 const match=src.match(/url\(["']?fonts\/([^\)"']+\.woff2)/);
 if(!match)throw new Error('Font declaration has no WOFF2 source');
 const bytes=fs.readFileSync(path.join(vendor,'dist/fonts',match[1]));
 return `src:url(data:font/woff2;base64,${bytes.toString('base64')}) format("woff2")`;
});
const layoutCss=`.katex{font-size:1.08em}.math-inline{white-space:normal}.math-display{padding:20px 24px;margin:22px 0;background:#f8fbfd;border:1px solid #d9e6ed;border-radius:8px;overflow-x:auto;overflow-y:hidden;line-height:normal}.math-display .katex{font-size:1.17em}.math-display .katex-display{margin:0;text-align:left}.math-display .katex-display.fleqn>.katex{padding-left:0}.math-display .tag{color:#537181}.katex .mord.text .cjk_fallback{font-family:"Microsoft YaHei",sans-serif}td .katex,th .katex{font-size:1em}.math-diagram .katex{font-size:20px;color:#183d50}.math-diagram{display:flex;justify-content:center;align-items:center;height:100%;width:100%;line-height:normal}@media(max-width:700px){.math-display{padding:16px 12px}.math-display .katex{font-size:1em}}@media print{.math-display{background:white;padding:10px 12px;margin:12px 0;break-inside:avoid;overflow:visible}.math-display .katex{font-size:1.04em}.katex{font-size:1em}}`;
function render(tex,display){return katex.renderToString(tex,{displayMode:display,output:'htmlAndMathml',throwOnError:true,strict:'ignore',trust:false,fleqn:display});}
function protectMath(markdown){
 const fragments=[];
 function token(html){const key=`MATHFRAGMENTTOKEN${fragments.length}END`;fragments.push([key,html]);return key;}
 let displays=0,inlines=0;
 let source=markdown.replace(/\$\$\s*\n?([\s\S]*?)\n?\s*\$\$/g,(_,tex)=>{
  displays++;const tag=(tex.match(/\\tag\{([^}]+)\}/)||[])[1];
  const id=tag?`equation-${tag}`:`math-display-${displays}`;
  return '\n\n'+token(`<div class="math-display" id="${id}">${render(tex.trim(),true)}</div>`)+'\n\n';
 });
 source=source.replace(/(?<!\\)\$([^$\n]+)(?<!\\)\$/g,(_,tex)=>{inlines++;return token(`<span class="math-inline">${render(tex,false)}</span>`);});
 return {source,displays,inlines,restore(html){for(const [key,fragment] of fragments){html=html.replace(`<p>${key}</p>`,fragment).replaceAll(key,fragment);}return html;}};
}
// Typeset mathematical labels inside the offline engineering SVGs as well.
const diagramExpressions={
 'q、v、p、bias':String.raw`q,\;v,\;p,\;\mathrm{bias}`,
 '姿态更新 q_nb':String.raw`\text{姿态更新}\quad q_{nb}`,
 '坐标变换 R_nb f_b':String.raw`\text{坐标变换}\quad R_{nb}f_b`,
 '加 g_n 补偿重力':String.raw`\text{加}\;g_n\;\text{补偿重力}`,
 'v_(k+1)=v_k+aΔt':String.raw`v_{k+1}=v_k+a\,\Delta t`,
 'p+=vΔt+½aΔt²':String.raw`\Delta p=v\,\Delta t+\tfrac12a\,\Delta t^2`,
 '初值 q₀、v₀、p₀ 与协方差':String.raw`\text{初值}\quad q_0,\;v_0,\;p_0\quad\text{与协方差}`,
 '静止：f_b≈(0,0,+g)，a_n≈0':String.raw`\text{静止:}\;f_b\approx(0,0,+g),\;a_n\approx0`,
 '名义状态 x̂⁻、P⁻':String.raw`\text{名义状态}\quad\hat{x}^{-},\;P^{-}`,
 '预测观测 h(x̂⁻)':String.raw`\text{预测观测}\quad h(\hat{x}^{-})`,
 '辅助观测 z':String.raw`\text{辅助观测}\quad z`,
 '创新 r=z−h(x̂⁻)':String.raw`\text{创新}\quad r=z-h(\hat{x}^{-})`,
 '接受：K、δx与P更新':String.raw`\text{接受:}\;K,\;\delta x\;\text{与}\;P\;\text{更新}`,
 'P增长 / 降级监督':String.raw`P\;\text{增长 / 降级监督}`,
 'v_ref、ω_ref / δ_ref':String.raw`v_{\mathrm{ref}},\;\omega_{\mathrm{ref}}\,/\,\delta_{\mathrm{ref}}`,
 'eψ、ey、ev、eD':String.raw`e_\psi,\;e_y,\;e_v,\;e_D`,
 'p̂、v̂、ψ̂、置信度':String.raw`\hat{p},\;\hat{v},\;\hat{\psi},\;\text{置信度}`,
};
function typesetDiagram(svg){
 return svg.replace(/<text class="box" x="([^"]+)" y="([^"]+)" text-anchor="middle">([^<]+)<\/text>/g,(all,x,y,text)=>{
  const tex=diagramExpressions[text];if(!tex)return all;
  const width=svg.includes('初值')&&text.startsWith('初值')?390:svg.includes('初值')&&text.startsWith('静止')?390:230;
  return `<foreignObject x="${Number(x)-width/2}" y="${Number(y)-24}" width="${width}" height="36"><div xmlns="http://www.w3.org/1999/xhtml" class="math-diagram">${render(tex,false)}</div></foreignObject>`;
 });
}
module.exports={mathCss,layoutCss,protectMath,typesetDiagram,katexVersion:katex.version};
