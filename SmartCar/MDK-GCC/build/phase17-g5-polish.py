from pathlib import Path
import re,importlib.util,textwrap,json
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('g5prepare',ROOT/'MDK-GCC/build/phase17-g5-prepare.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
paths=json.loads((ROOT/'MDK-GCC/build/phase17-g5-paths.json').read_text(encoding='utf-8'))
def write(path,text):
    data=path.read_bytes();nl='\r\n' if b'\r\n' in data else '\n'
    path.write_bytes(text.replace('\r\n','\n').replace('\n',nl).encode())
def replace_doc(text,name,replacements):
    f=g.functions(text)[name]
    doc=f[:f.index('*/')+2]
    for old,new in replacements:
        assert old in doc,(name,old)
        doc=doc.replace(old,new)
    return text.replace(f,doc+f[f.index('*/')+2:])
for rel in paths:
    path=ROOT/rel
    if path.suffix not in ('.c','.h') or not path.exists():continue
    text=path.read_text(encoding='utf-8');before=text
    text=text.replace('READ 不清 active，其他值按原逻辑清零。','READ不清active；任何其他传入值均清active。')
    text=text.replace('会话头擦除或写入失败。','标定无效、写头、回读或比较失败。')
    text=text.replace('已写记录达到会话容量上限。','已接受记录达到会话容量上限。')
    if path.name=='scb_frame.c':
        for name,detail in [('SCB_ReadU16','按低字节到高字节移位并按位或，不要求输入对齐或依赖主机端序。'),
                            ('SCB_ReadU32','四个字节分别先转uint32_t，再按0/8/16/24位移合并，避免窄整数高位移位。'),
                            ('SCB_WriteU16','依次写低八位及右移八位的字节，不访问第三个字节。'),
                            ('SCB_WriteU32','按低位到高位依次截取0/8/16/24位移结果，固定写四字节。')]:
            f=g.functions(text)[name];doc=f[:f.index('*/')+2]
            brief=re.search(r' \* @brief [^\n]+',doc)[0]
            text=replace_doc(text,name,[(brief,brief+'\n * @details '+detail)])
        text=text.replace('typedef char scb_header_assert_t[(SCB_HEADER_SIZE + SCB_PAYLOAD_MAX == SCB_SPI_SIZE) ? 1 : -1];',
                          'typedef char scb_header_assert_t[\n    (SCB_HEADER_SIZE + SCB_PAYLOAD_MAX == SCB_SPI_SIZE) ? 1 : -1];')
    if path.name=='scb_json.c':
        text=replace_doc(text,'SCB_ParseString',[
            ('输入UTF-8已整体验证，原始多字节直接复制；Unicode转义合并代理对再编码。',
             '输入UTF-8已整体验证，原始多字节直接复制；Unicode转义合并代理对再编码。\n'
             ' *     开始就写type/text并前移position，失败可能保留部分池内容和当前位置，不能继续分发。'),
            ('非空已分配令牌，成功写入字符串池位置和长度。',
             '非空已分配令牌；入口写类型和池起点，遇闭引号写长度，失败允许部分更新。')])
        text=replace_doc(text,'SCB_ParseNumber',[
            ('拒绝前导零、空小数/指数以及非有限转换；整数标记用于速度风扇类型校验。',
             '扫描符号、整数、小数和指数，不消费数字后面的分隔符；仅零后紧跟分隔符可由外层接受。\n'
             ' *     小数/指数使integer清零，长度上限63B；十进制累加后逐次乘除10，过小指数归零。\n'
             ' *     类型和integer可能在失败前已写；number与position只有成功结束才发布。'),
            ('非空令牌，成功写数字和整数标志。','非空令牌，入口写类型和整数标志，成功才写最终数字。')])
        text=replace_doc(text,'SCB_ParseJson',[
            ('拒绝非法UTF-8、NUL、尾随数据与容量越界；失败工作区不得继续作为有效文档读取。',
             '先验证指针、1..992B和整份UTF-8，再清工作区计数并解析根。\n'
             ' *     显式八层栈推进容器，按已解码键排重，最后只允许四种空白后到输入末尾。\n'
             ' *     前置校验失败保持json，解析中失败允许部分令牌/字符串；失败工作区不能当作有效文档。'),
            ('非空无NUL负载。','1..992B无NUL负载；空指针直接失败。'),
            ('非空调用方工作区，允许静态分配。','调用方工作区，允许静态分配；空指针直接失败。'),
            ('工作区与输入不能重叠；无动态分配。','前台串行且不可重入，共享容器栈；工作区与输入不重叠，无动态分配。')])
        text=replace_doc(text,'SCB_JsonBegin',[
            ('@brief 初始化静态有界输出。','@brief 初始化调用方持有的有界输出。\n'
             ' * @details 保存缓冲与容量后清length，空缓冲、零容量或大于992时锁failed；不写缓冲内容。'),
            ('非空调用方缓冲，不自动写NUL。','调用方缓冲；可为空但将锁存failed，不写缓冲或自动NUL。'),
            ('缓冲容量，1～992B。','缓冲容量；1..992B合法，其他值只锁存failed。')])
    if path.name=='scb_command_codec.c':
        text=replace_doc(text,'SCB_ParseCommand',[
            ('先验证整份JSON，随后检查必需字段类型及同源配置限值，',
             '先写INVALID_ARG并清command，再验证整份JSON，随后检查必需字段类型及同源限值，'),
            ('非空完整命令，只有status为OK时可执行。',
             '非空命令，入口清零；失败可有部分字段，只有status为OK时可执行。'),
            ('非空业务结果；语法错误返回0且不可ACK。',
             '非空业务结果，入口INVALID_ARG；语法错误返回0且不可ACK。')])
    if path.name=='logger.c':
        text=text.replace('先清会话与队列状态并检查 Flash/标定，再填充固定 256 字节头及 CRC；擦除和写头全部成功后才发布有效会话和 active。',
                          '先保留原tracking读取时点并检查导出互斥，再复位会话/队列；配置和标定通过后，\n'
                          '     * 才构造头并擦写、回读校验。全部成功后发布头有效和active。')
    # Normalize only comment tokens, never string/character contents or source order.
    changes=[]
    for token in g.TOKEN.finditer(text):
        block=token[0]
        if not block.startswith('/*') or block.startswith('/**<') or ('\n' not in block and len(block)<=96):continue
        start=token.start();line_start=text.rfind('\n',0,start)+1
        indent=text[line_start:start]
        if indent.strip():continue
        doxygen=block.startswith('/**');opening='/**' if doxygen else '/*'
        content=block[len(opening):-2].strip()
        lines=content.splitlines();out=[opening]
        for raw in lines:
            raw=raw.strip()
            if raw.startswith('*'):raw=raw[1:]
            raw=raw.lstrip(' ') if not raw.startswith('     ') else '    '+raw.lstrip()
            prefix=' *     ' if raw.startswith('    ') else ' * '
            raw=raw.strip()
            wrapped=textwrap.wrap(raw,width=100-len(indent)-len(prefix),break_long_words=True,break_on_hyphens=False) or ['']
            for i,part in enumerate(wrapped):
                continuation=' *     ' if i else prefix
                out.append(continuation+part)
        out.append(' */')
        replacement=('\n'+indent).join(out)
        changes.append((token.start(),token.end(),replacement))
    for start,end,replacement in reversed(changes):text=text[:start]+replacement+text[end:]
    if text!=before:
        assert [m[0] for m in g.TOKEN.finditer(text) if not m[0].startswith(('/*','//'))]==[m[0] for m in g.TOKEN.finditer(before) if not m[0].startswith(('/*','//'))],path
        write(path,text)
print('Polished G5 comments and checked every non-comment token unchanged')
