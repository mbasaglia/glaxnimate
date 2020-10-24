#!/usr/bin/env python3
import re
import sys
import types
import inspect

import glaxnimate


class MdWriter:
    def __init__(self):
        self.out = sys.stdout
        self.level = 1

    def title(self, text, level=0):
        self.out.write("%s %s\n\n" % ("#"*(level+self.level), text))

    def p(self, text):
        self.out.write(text + "\n\n")

    def sublevel(self, amount: int = 1):
        return MdLevel(self, amount)

    def end_table(self):
        self.out.write("\n")

    def table_row(self, *cells):
        self.out.write("| ")
        for c in cells:
            self.out.write(str(c))
            self.out.write(" | ")
        self.out.write("\n")

    def table_header(self, *cells):
        self.table_row(*cells)
        self.out.write("| ---- " * len(cells))
        self.out.write("|\n")

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
        self.docs = inspect.getdoc(module)

    def name(self):
        return self.module.__name__

    def print(self, writer: MdWriter):
        writer.title(self.name())

        if self.docs:
            writer.p(self.docs)

        if self.props:
            writer.title("Properties", 1)
            writer.table_header("name", "type", "notes", "docs")
            for v in self.props:
                v.print(writer)
            writer.end_table()

        if self.const:
            writer.title("Constants", 1)
            writer.table_header("name", "type", "value", "docs")
            for v in self.const:
                v.print(writer)
            writer.end_table()

        with writer.sublevel():
            for func in self.functions:
                func.print(writer)

            for cls in self.classes:
                cls.print(writer)

    def inspect(self, modules, classes):
        for name, val in inspect.getmembers(self.module):
            if name.startswith("__"):
                continue
            elif inspect.ismodule(val):
                submod = ModuleDocs(val)
                submod.inspect(modules, submod.classes)
                modules.append(submod)
            elif inspect.isfunction(val) or inspect.ismethod(val) or isinstance(val, types.BuiltinMethodType):
                self.functions.append(FunctionDoc(self.child_name(name), val))
            elif inspect.isclass(val):
                cls = ClassDoc(self.child_name(name), val)
                cls.inspect([], classes)
                classes.append(cls)
            elif type(val).__name__ == "instancemethod":
                self.functions.append(FunctionDoc(self.child_name(name), val.__func__))
            elif isinstance(val, property):
                self.props.append(PropertyDoc(name, val))
            elif hasattr(type(val), "__int__"):
                self.const.append(Constant.enum(val))
            else:
                self.const.append(Constant(name, None, type(val)))

    def child_name(self, child):
        return self.name() + "." + child


class FunctionDoc:
    def __init__(self, full_name, function):
        self.function = function
        self.full_name = full_name
        self.docs = inspect.getdoc(function)

    def print(self, writer: MdWriter):
        writer.title(self.full_name)

        if self.docs:
            writer.p(self.docs)


class PropertyDoc:
    extract_type = re.compile("-> ([^\n]+)")

    def __init__(self, name, prop):
        self.name = name
        self.prop = prop
        self.docs = inspect.getdoc(prop)
        match = self.extract_type.search(prop.fget.__doc__ or "")
        if match:
            self.type = match.group(1)
        else:
            self.type = None
        self.readonly = prop.fset is None

    def print(self, writer: MdWriter):
        writer.table_row(
            writer.code(self.name),
            self.type,
            "Read only" if self.readonly else "",
            self.docs
        )


class ClassDoc(ModuleDocs):
    def __init__(self, full_name, cls):
        super().__init__(cls)
        self.full_name = full_name

    def name(self):
        return self.full_name


class Constant:
    def __init__(self, name, value, type):
        self.name = name
        self.value = value
        self.type = type.__module__ + "." + type.__name__
        self.docs = ""

    @classmethod
    def enum(cls, value):
        return cls(value.name, int(value), type(value))

    def print(self, writer: MdWriter):
        writer.table_row(
            writer.code(self.name),
            self.type,
            writer.code(repr(self.value)) if self.value is not None else "",
            self.docs
        )


class DocBuilder:
    def __init__(self):
        self.modules = []

    def module(self, module_obj):
        module = ModuleDocs(module_obj)
        self.modules.append(module)
        module.inspect(self.modules, None)

    def print(self):
        writer = MdWriter()
        for module in self.modules:
            module.print(writer)


doc = DocBuilder()
doc.module(glaxnimate)
doc.print()
