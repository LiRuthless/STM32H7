const fs = require('fs');
const path = require('path');
const sharp = require('C:/Users/Li/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
async function main(){
 const out=path.join(__dirname,'preview');fs.mkdirSync(out,{recursive:true});
 const files=fs.readdirSync(path.join(__dirname,'svg_output')).filter(x=>x.endsWith('.svg')).sort();
 const tiles=[];
 for(let i=0;i<files.length;i++){
  const png=await sharp(path.join(__dirname,'svg_output',files[i])).png().toBuffer();
  fs.writeFileSync(path.join(out,files[i].replace('.svg','.png')),png);
  const thumb=await sharp(png).resize(640,360).toBuffer();
  tiles.push({input:thumb,left:(i%4)*656,top:Math.floor(i/4)*376});
 }
 await sharp({create:{width:2608,height:2240,channels:3,background:'#DCE5EC'}}).composite(tiles).png().toFile(path.join(out,'overview.png'));
 console.log('Rendered 24 slide previews and overview.');
}
main().catch(e=>{console.error(e);process.exit(1)});
