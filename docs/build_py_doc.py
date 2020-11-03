#!/usr/bin/env python3
import re
import sys
import types
import inspect
import argparse

import glaxnimate


class MdWriter:
    def __init__(self, file):
        self.out = file
        self.level = 1
        self._table_data = []

    def title(self, text, level=0):
        self.out.write("%s %s\n\n" % ("#"*(level+self.level), text))
        #self.fancy_title(text, text, level)

    def fancy_title(self, text, id, level=0):
        self.out.write("<h{level} id='{id}'><a href='#{id}'>{text}</a></h{level}>\n\n".format(
            text=text,
            id=id,
            level=level+self.level
        ))

    def p(self, text):
        self.out.write(text + "\n\n")

    def li(self, text):
        self.out.write("* %s\n" % text)

    def a(self, text, href):
        return "[%s](%s)" % (text, href)

    def nl(self):
        self.out.write("\n")

    def sublevel(self, amount: int = 1):
        return MdLevel(self, amount)

    def end_table(self):
        lengths = [
            max(len(row[1][col]) for row in self._table_data)
            for col in range(len(self._table_data[0][1]))
        ]

        for row in self._table_data:
            self.out.write("| ")
            for cell, length in zip(row[1], lengths):
                self.out.write(cell.ljust(length))
                self.out.write(" | ")
            self.out.write("\n")

            if row[0]:
                self.out.write("| ")
                for length in lengths:
                    self.out.write("-" * length)
                    self.out.write(" | ")
                self.out.write("\n")

        self.out.write("\n")
        self._table_data = []

    def table_row(self, *cells):
        self._table_data.append(
            (False, list(map(str, cells)))
        )

    def table_header(self, *cells):
        self._table_data.append(
            (True, list(map(str, cells)))
        )

    def code(self, string):
        return "`%s`" % string


class MdLevel:
    def __init__(self, mdw: MdWriter, amount: int):
        self.mdw = mdw
        self.amount = amount

    def __enter__(self):
        self.mdw.level += self.amount

    def __exit__(self, *a):
        self.mdw.level -= self.amount


class ModuleDocs:
    def __init__(self, module):
        self.module = module
        self.props = []
        self.const = []
        self.classes = []
        self.functions = []
        self.file_props = []
        self.docs = inspect.getdoc(module)
        if self.docs and "Members:" in self.docs:
            self.docs = ""

    def name(self):
        return self.module.__name__

    def print_intro(self, writer: MdWriter):
        pass

    def print(self, writer: MdWriter):
        writer.title(self.name())

        if self.docs:
            writer.p(self.docs)

        self.print_intro(writer)

        if self.props:
            writer.p("Properties:")
            writer.table_header("name", "type", "notes", "docs")
            for v in self.props:
                v.print(writer)
            writer.end_table()

        if self.const:
            writer.p("Constants:")
            writer.table_header("name", "type", "value", "docs")
            for v in self.const:
                v.print(writer)
            writer.end_table()

        if self.functions:
            with writer.sublevel(1):
                for func in self.functions:
                    func.print(writer)

        if self.classes:
            with writer.sublevel(1):
                for cls in self.classes:
                    cls.print(writer)

    def inspect(self, modules, classes):
        for name, val in vars(self.module).items():
            if name.startswith("__") and name != "__version__":
                continue
            elif inspect.ismodule(val):
                submod = ModuleDocs(val)
                submod.inspect(modules, submod.classes)
                modules.append(submod)
            elif inspect.isfunction(val) or inspect.ismethod(val) or isinstance(val, types.BuiltinMethodType):
                self.functions.append(FunctionDoc(name, self.child_name(name), val))
            elif inspect.isclass(val):
                cls = ClassDoc(self.child_name(name), val)
                cls.inspect([], classes)
                classes.append(cls)
            elif type(val).__name__ == "instancemethod":
                self.functions.append(FunctionDoc(name, self.child_name(name), val.__func__))
            elif isinstance(val, property):
                classinfo = getattr(self.module, "__classinfo__", {}).get(name)
                prop = PropertyDoc(name, val, classinfo)
                self.props.append(prop)
                if classinfo:
                    self.file_props.append(prop)
            elif hasattr(type(val), "__int__"):
                self.const.append(Constant.enum(val))
            else:
                self.const.append(Constant(name, val if " at 0x" not in repr(val) else None, type(val)))

    def child_name(self, child):
        return self.name() + "." + child


class TypeFixer:
    basic = [
        ("QString", "str"),
        ("QByteArray", "bytes"),
        ("QUuid", "uuid.UUID"),
        ("glaxnimate.__detail.__QObject", "object"),
        ("QColor", "glaxnimate.utils.Color"),
        ("QPointF", "glaxnimate.utils.Point"),
        ("QSizeF", "glaxnimate.utils.Size"),
        ("QSize", "glaxnimate.utils.IntSize"),
        ("QVector2D", "glaxnimate.utils.Vector2D"),
        ("QRectF", "glaxnimate.utils.Rect"),
        ("QObject", "object"),
        ("List[QVariant]", "list"),
        ("AnimatableBase", "glaxnimate.model.AnimatableBase"),
        ("QVariantMap", "dict"),
        ("QVariant", "<type>"),
        ("QGradientStops", "List[GradientStop]"),
        ("builtins.", "")
    ]
    wrong_ns = re.compile(r"\b([a-z]+)::([a-zA-Z0-9_]+)")
    link_re = re.compile(r"(glaxnimate\.[a-zA-Z0-9._]+\.([a-zA-Z0-9_]+))")
    types = {}
    unlinked = set()

    @classmethod
    def add_class(cls, class_):
        cls.types[class_.__name__] = class_.__module__ + "." + class_.__qualname__

    @classmethod
    def fix(cls, text: str) -> str:
        for qt, py in cls.basic:
            text = text.replace(qt, py)
        text = cls.wrong_ns.sub(r"glaxnimate.\1.\2", text)
        return text

    @classmethod
    def classhref(cls, full_name):
        return "#" + full_name.replace(".", "").lower()

    @classmethod
    def classlink(cls, name, full_name, json):
        return r"[{txt}]({link})".format(
            txt=name,
            link=cls.classhref(full_name) if not json else "#" + name.lower()
        )

    @classmethod
    def format(cls, text: str, json=False) -> str:
        text = cls.fix(text)
        text = cls.link_re.sub(
            lambda m: cls.classlink(m.group(2), m.group(0), json),
            text
        )

        if json:
            text = text.replace("List[", "array of ")
            if text.endswith("]"):
                text = text[:-1]
            text = text.replace("GradientStop", "[Gradient Stop](#gradient-stop)")
            text = text.replace("uuid.UUID", "[UUID](#uuid)")
            text = text.replace("str", "string")
            text = text.replace("bytes", "Base64 string")
        else:
            text = text.replace("GradientStop", "Tuple[`float`, [Color](#glaxnimateutilscolor)]")

        if text in cls.types:
            text = cls.classlink(text, cls.types[text], json)

        if "glaxnimate" not in text and "](" not in text and " " not in text:
            TypeFixer.unlinked.add(text)
            text = "`%s`" % text
        return text


class FunctionDoc:
    re_sig = re.compile(r"^(?:[0-9]+\. )?([a-zA-Z0-9_]+\(.*\)(?: -> .*)?)$", re.M)

    def __init__(self, name, full_name, function):
        self.function = function
        self.name = name
        self.full_name = full_name
        self.docs = inspect.getdoc(function)
        if self.docs:
            if "Signature:" in self.docs:
                lines = self.docs.splitlines()
                self.docs = "Signature:\n\n```python\n"
                for i in range(len(lines)):
                    if lines[i] == "Signature:":
                        self.docs += TypeFixer.fix(lines[i+1]) + "\n"
                self.docs += "```"
            else:
                self.docs = self.docs.replace("(self: glaxnimate.__detail.__QObject", "(self")
                self.docs = TypeFixer.fix(self.docs)
                self.docs = self.re_sig.sub("```python\n\\1\n```", self.docs)

    def print(self, writer: MdWriter):
        writer.fancy_title(self.name + "()", self.full_name)

        if self.docs:
            writer.p(self.docs)


class PropertyDoc:
    extract_type = re.compile("-> ([^\n]+)")
    extract_type_qt = re.compile("Type: (.*)")
    extract_type_classinfo = re.compile("^property (?:([a-z]+) )?(.*)$")

    def __init__(self, name, prop, classinfo=None):
        self.name = name
        self.prop = prop
        self.docs = inspect.getdoc(prop) or ""
        self.reference = False

        if classinfo:
            match = self.extract_type_classinfo.match(classinfo)
            self.type = TypeFixer.fix(match.group(2))
            if match.group(1) == "list":
                self.type = "List[%s]" % self.type
            elif match.group(1) == "ref":
                self.reference = True
        elif not prop.fget.__doc__:
            self.type = None
        elif "Type: " in prop.fget.__doc__:
            self.type = TypeFixer.fix(self.extract_type_qt.search(prop.fget.__doc__).group(1))
        else:
            match = self.extract_type.search(prop.fget.__doc__)
            if match:
                self.type = TypeFixer.fix(match.group(1))
            else:
                self.type = None

        if "->" in self.docs:
            self.docs = "\n".join(self.docs.splitlines()[1:])

        self.readonly = prop.fset is None

    def print(self, writer: MdWriter):
        notes = []
        if self.readonly:
            notes.append("Read only")
        if self.reference:
            notes.append("Reference")

        writer.table_row(
            writer.code(self.name),
            TypeFixer.format(self.type),
            ",".join(notes),
            self.docs
        )

    def print_json(self, writer: MdWriter):
        if self.reference:
            writer.table_row(
                writer.code(self.name),
                TypeFixer.format("uuid.UUID", True),
                "References " + TypeFixer.format(self.type, True) + ". " + self.docs
            )
        else:
            writer.table_row(
                writer.code(self.name),
                TypeFixer.format(self.type, True),
                self.docs
            )


class ClassDoc(ModuleDocs):
    def __init__(self, full_name, cls):
        super().__init__(cls)
        TypeFixer.add_class(cls)
        self.full_name = full_name
        self.bases = [
            base
            for base in cls.__bases__
            if "__" not in base.__name__ and "pybind11" not in base.__name__
        ]
        self.children = [
            cls
            for cls in cls.__subclasses__()
            if "AnimatedProperty_" not in cls.__name__
        ]

    def name(self):
        return self.full_name

    def print_intro(self, writer: MdWriter):
        if self.bases:
            writer.p("Base classes:")
            for base in self.bases:
                writer.li(writer.a(
                    base.__name__,
                    TypeFixer.classhref(base.__module__ + '.' + base.__name__)
                ))
            writer.nl()

        if self.children:
            writer.p("Sub classes:")
            for base in self.children:
                writer.li(writer.a(
                    base.__name__,
                    TypeFixer.classhref(base.__module__ + '.' + base.__name__)
                ))
            writer.nl()

    def print_json(self, writer: MdWriter):
        writer.title(self.module.__name__, 1)
        if self.bases:
            writer.p("Base types:")
            for base in self.bases:
                writer.li(writer.a(
                    base.__name__,
                    "#" + base.__name__.lower()
                ))
            writer.nl()

        if self.children:
            writer.p("Sub types:")
            for base in self.children:
                writer.li(writer.a(
                    base.__name__,
                    "#" + base.__name__.lower()
                ))
            writer.nl()

        if self.file_props:
            writer.p("Properties:")

            writer.table_header("name", "type", "docs")
            for v in self.file_props:
                v.print_json(writer)
            writer.end_table()

    def print_json_enum(self, writer: MdWriter):
        writer.title(self.module.__name__, 1)

        writer.table_header("value", "docs")
        for v in self.const:
            v.print_json(writer)
        writer.end_table()


class Constant:
    def __init__(self, name, value, type):
        self.name = name
        self.value = value
        self.type = type.__module__ + "." + type.__qualname__
        self.docs = ""

    @classmethod
    def enum(cls, value):
        return cls(value.name, int(value), type(value))

    def print(self, writer: MdWriter):
        writer.table_row(
            writer.code(self.name),
            TypeFixer.format(self.type),
            writer.code(repr(self.value)) if self.value is not None else "",
            self.docs
        )

    def print_json(self, writer: MdWriter):
        writer.table_row(
            writer.code(self.name),
            self.docs
        )


class DocBuilder:
    def __init__(self):
        self.modules = []

    def module(self, module_obj):
        module = ModuleDocs(module_obj)
        self.modules.append(module)
        module.inspect(self.modules, None)

    def print(self, py_doc_file):
        writer = MdWriter(py_doc_file)
        for module in self.modules:
            module.print(writer)

    def print_json(self, json_doc_file):
        writer = MdWriter(json_doc_file)
        enums = []
        writer.level = 2
        for module in self.modules:
            for cls in module.classes:
                if issubclass(cls.module, glaxnimate.model.Object):
                    cls.print_json(writer)
                elif cls.const and not cls.functions and all(type(x.value) is int for x in cls.const):
                    enums.append(cls)

        writer.title("Enumerations")
        for enum in enums:
            enum.print_json_enum(writer)


parser = argparse.ArgumentParser()
parser.add_argument("py_doc_file")
parser.add_argument("json_doc_file")
ns = parser.parse_args()

doc = DocBuilder()
doc.module(glaxnimate)

with open(ns.py_doc_file, "w") as py_doc_file:
    doc.print(py_doc_file)

with open(ns.json_doc_file, "w") as json_doc_file:
    doc.print_json(json_doc_file)

print(TypeFixer.unlinked)
