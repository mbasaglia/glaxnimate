#!/usr/bin/env python3
import sys
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

    #def skip(self):
        #self.out.write("\n")


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
        self.vars = []
        self.classes = []
        self.functions = []
        self.docs = inspect.getdoc(module)

    def name(self):
        return self.module.__name__

    def print(self, writer: MdWriter):
        writer.title(self.name())

        if self.docs:
            writer.p(self.docs)

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
            elif inspect.isfunction(val) or inspect.ismethod(val):
                self.functions.append(FunctionDoc(self.child_name(name), val))
            elif inspect.isclass(val):
                cls = ClassDoc(self.child_name(name), val)
                cls.inspect([], classes)
                classes.append(cls)
            elif type(val).__name__ == "instancemethod":
                self.functions.append(FunctionDoc(self.child_name(name), val.__func__))
            else:
                print("Skipped %s" % val)

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


class ClassDoc(ModuleDocs):
    def __init__(self, full_name, cls):
        super().__init__(cls)
        self.full_name = full_name

    def name(self):
        return self.full_name


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
