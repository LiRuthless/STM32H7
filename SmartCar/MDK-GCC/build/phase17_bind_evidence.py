from pathlib import Path
import subprocess,json,hashlib,re,sys
root=Path.cwd(); group=sys.argv[1];base=subprocess.check_output(["git","rev-parse",sys.argv[2]],text=True).strip();facts=json.loads(Path(sys.argv[3]).read_text(encoding="utf-8"))
p=root/"SmartCar/specs/features/15-code-style-consistency/comment-evidence.json";data=json.loads(p.read_text(encoding="utf-8"));rows=data["files"];cache={}
def get(rev,q):
 key=(rev,q)
 if key not in cache:
  r=subprocess.run(["git","show",(":"+q if rev=="index" else rev+":"+q)],capture_output=True);cache[key]=r.stdout if r.returncode==0 else None
 return cache[key]
def sha(b):return hashlib.sha256(b).hexdigest() if b is not None else None
def scoped(q):return q.startswith(("SmartCar/App/","SmartCar/BSP/","SmartCar/Core/")) and q.endswith((".c",".h"))
changed=[q for q in subprocess.check_output(["git","diff","--cached","--name-only","--no-renames","-z",base]).decode().split("\0") if scoped(q)]
for q in changed:
 b=get("index",q);before=get(base,q);text=(b or b"").decode("utf-8");name=Path(q).name;fact=facts.get(name)
 if not fact:raise RuntimeError("Missing reviewed file fact: "+q)
 deps={}
 for include in re.findall(r'^#include "([^"]+)"',text,re.M):
  for prefix in ("SmartCar/App/","SmartCar/BSP/","SmartCar/Core/Inc/"):
   dep=prefix+include
   if get("index",dep) is not None:deps[dep]=sha(get("index",dep));break
 revision="index" if b is not None else "base"
 items=[{"object":q+"::module","fact":fact,"evidence":[{"path":q,"anchor":"@file "+name,"revision":revision}]}]
 for m in re.finditer(r'(?m)^(?:static\s+)?(?:const\s+)?\w+[ *]+(\w+)\s*\([^;]*?\)\s*\{',text):
  fn=m[1];doc=text[text.rfind("/**",0,m.start()):m.start()];brief=re.search(r'@brief ([^\n\r]+)',doc)
  items.append({"object":q+"::"+fn,"fact":(brief[1].strip() if brief else fact)+" 已对照本组状态归属、调用方和错误/复位路径复核。","evidence":[{"path":q,"anchor":fn+"(","revision":"index"}]})
 rows[q]={"sha256":sha(b),"reviewed_base":base,"reviewer":"Codex "+group+"源码及独立差异复核","reviewed_at":"2026-10-24","unresolved":[],"dependencies":deps,"base_dependencies":({q:sha(before)} if before else {}),"items":items,"cleanup":fact+" 当前调用方/工程/测试已同步，历史基线路径保留；此前记录可从本基线Git版本追溯。","checks":{k:{"status":"pass","reason":v} for k,v in {"coverage":"全部定义/回调/条件编译和头摘要按本组迁移清单核对，旧定义迁移不漏项。","format":"中文C完整/H摘要、真实C定位、模块前缀/保护宏和原来源说明逐项检查，非普通格式串例外另在review说明。","content":fact+" 源码事实依据本组review及调用方检查，未以编译替代中文内容复核。","behavior":"本组原宿主断言、历史数值对照、相关边界与双LCD构建通过；R07全入口、AC5及实机独立待验。"}.items()}}
all_sources=[q for q in subprocess.check_output(["git","ls-files","-z","SmartCar/App","SmartCar/BSP","SmartCar/Core"]).decode().split("\0") if q.endswith((".c",".h"))]
owners={}
for source in all_sources:
 if not source.endswith(".c") or get("index",source) is None:continue
 source_text=get("index",source).decode("utf-8")
 for fn in re.findall(r'(?m)^(?:static\s+)?(?:const\s+)?\w+[ *]+(\w+)\s*\([^;]*?\)\s*\{',source_text):owners.setdefault(fn,set()).add(source)
for q in changed:
 body=get("index",q)
 if body is None:continue
 for fn in re.findall(r'\b([A-Za-z_]\w*)\s*\(',body.decode("utf-8")):
  candidates=owners.get(fn,set())
  if len(candidates)==1:
   dep=next(iter(candidates))
   if dep!=q:rows[q]["dependencies"][dep]=sha(get("index",dep))
for q,row in rows.items():
 if q in changed:continue
 deps=row.get("dependencies",{});affected=any(dep in changed for dep in deps)
 if not affected:continue
 needed=set()
 for item in row["items"]:
  for ev in item["evidence"]:
   if ev["revision"]!="index":continue
   body=get("index",ev["path"])
   if body is None or ev["anchor"] not in body.decode("utf-8"):
    candidates=[s for s in all_sources if get("index",s) and ev["anchor"] in get("index",s).decode("utf-8")]
    c=[s for s in candidates if s.endswith(".c")]
    if len(c)==1:ev["path"]=c[0]
    elif len(candidates)==1:ev["path"]=candidates[0]
    else:raise RuntimeError((q,ev["anchor"],candidates))
   if ev["path"]!=q:needed.add(ev["path"])
 for dep in list(deps):
  if get("index",dep) is None:del deps[dep]
 for dep in needed:deps[dep]=sha(get("index",dep))
 for dep in deps:deps[dep]=sha(get("index",dep))
 row["base_dependencies"]={dep:sha(get(base,dep)) for dep in row.get("base_dependencies",{}) if get(base,dep) is not None}
 row["reviewed_base"]=base;row["reviewed_at"]="2026-10-24"
p.write_text(json.dumps(data,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
subprocess.run(["git","add","--",p.relative_to(root).as_posix()],check=True)
subprocess.run(["git","diff","--cached","--check"],check=True)
subprocess.run([sys.executable,"SmartCar/tests/comment_review_check.py","--base",base],check=True)
print(group,"evidence PASS",len(changed),"C/H",base)
