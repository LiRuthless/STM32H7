"""Lexical/AST inventory for Phase 17 traceability, including inactive branches.

This is an index, not a compiler or a proof of behavior. Includes/directives are
masked and known type names are seeded so the parser sees each local definition.
Conditions remain on every item; parsing must fail rather than omit a file.
"""
import re

from pycparser import c_ast, c_generator, c_parser


def mask_comments(text):
    token = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
    return token.sub(lambda m: re.sub(r'[^\n]', ' ', m[0])
                     if m[0].startswith(('/*', '//')) else m[0], text)


def inventory(path, text):
    clean = mask_comments(text)
    lines = clean.splitlines()
    conditions = {}
    stack = []
    result = []
    for number, line in enumerate(lines, 1):
        conditions[number] = ' && '.join(stack)
        directive = re.match(r'\s*#\s*(\w+)\s*(.*)', line)
        if not directive:
            continue
        op, body = directive.groups()
        if op in ('if', 'ifdef', 'ifndef'):
            stack.append(f'#{op} {body}')
        elif op in ('else', 'elif'):
            assert stack, (path, number)
            stack[-1] = stack[-1] + f' -> #{op} {body}'
        elif op == 'endif':
            assert stack, (path, number)
            stack.pop()
        if op == 'define':
            name = re.match(r'\w+', body)[0]
            result.append(dict(path=path, symbol=name, kind='macro', line=number,
                               conditional=conditions[number], signature=body.strip()))
        elif op in ('if', 'ifdef', 'ifndef', 'else', 'elif', 'endif'):
            result.append(dict(path=path, symbol=f'#{op} {body}'.strip(), kind='branch',
                               line=number, conditional=conditions[number], signature=line.strip()))
    assert not stack, (path, stack)
    # Preserve line numbers through preprocessing directives, including continuations.
    stripped = []
    continued = False
    for line in lines:
        directive = continued or bool(re.match(r'\s*#', line))
        continued = directive and line.rstrip().endswith('\\')
        stripped.append('' if directive else line)
    source = '\n'.join(stripped)
    # File-scope layout-assert macro calls are declarations after expansion.
    # Index their original expression instead of pretending to evaluate it.
    for match in list(re.finditer(r'\bDATALOG_ASSERT_LAYOUT\s*\([^;]+;', source, re.S))[::-1]:
        number = source[:match.start()].count('\n') + 1
        name = re.search(r'\(\s*(\w+)', match[0])[1]
        result.append(dict(path=path, symbol=name, kind='layout_assertion', line=number,
                           conditional=conditions[number], signature=' '.join(match[0].split())))
        source = source[:match.start()] + re.sub(r'[^\n]', ' ', match[0]) + source[match.end():]
    source = re.sub(r'__attribute__\s*\(\([^\n]*?\)\)', '', source)
    source = re.sub(r'\b__(?:weak|packed|IO|I|O)\b', '', source)
    names = set(re.findall(r'\b(?:\w+_t|\w+TypeDef)\b', source))
    names.update(('size_t', 'FILE', 'va_list', 'TIM_HandleTypeDef'))
    prefix = '\n'.join(f'typedef int {name};' for name in sorted(names)) + '\n'
    offset = prefix.count('\n')
    tree = c_parser.CParser().parse(prefix + source, filename=path)
    generator = c_generator.CGenerator()

    def add(node, name, kind, scope='', **extra):
        number = node.coord.line - offset
        if number > 0:
            result.append(dict(path=path, symbol=name, kind=kind, line=number,
                               conditional=conditions.get(number, ''), scope=scope,
                               signature=generator.visit(node).split('{', 1)[0].strip(), **extra))

    def members(node, owner):
        if isinstance(node, c_ast.Struct) and node.decls:
            for field in node.decls:
                if field.name:
                    add(field, field.name, 'member', owner)
        if isinstance(node, c_ast.Enum) and node.values:
            for field in node.values.enumerators:
                add(field, field.name, 'enumerator', owner)
        for _, child in node.children():
            if not isinstance(child, (c_ast.Decl, c_ast.Enumerator)):
                members(child, owner)

    class Body(c_ast.NodeVisitor):
        def __init__(self, name):
            self.name = name
            self.calls = []
            self.references = []

        def visit_ID(self, node):
            self.references.append(node.name)

        def visit_FuncCall(self, node):
            if isinstance(node.name, c_ast.ID):
                self.calls.append(node.name.name)
            self.generic_visit(node)

        def visit_Decl(self, node):
            if 'static' in node.storage and node.name:
                add(node, node.name, 'static_local', self.name)
            self.generic_visit(node)

    for node in tree.ext:
        if node.coord.line <= offset:
            continue
        if isinstance(node, c_ast.FuncDef):
            body = Body(node.decl.name)
            body.visit(node.body)
            add(node.decl, node.decl.name, 'function', calls=sorted(set(body.calls)),
                references=sorted(set(body.references)))
        elif isinstance(node, c_ast.Typedef):
            add(node, node.name, 'typedef')
            members(node.type, node.name)
        elif isinstance(node, c_ast.Decl):
            if isinstance(node.type, c_ast.FuncDecl):
                add(node, node.name, 'declaration')
            elif node.name:
                add(node, node.name, 'object', storage=node.storage,
                    const='const' in node.quals)
                members(node.type, node.name)
            else:
                members(node.type, '')
    return sorted(result, key=lambda row: (row['line'], row['kind'], row['symbol']))
