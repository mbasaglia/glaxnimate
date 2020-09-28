import glaxnimate


class Replacer(glaxnimate.model.Visitor):
    def __init__(self, color_search, color_replace):
        super().__init__()
        self.color_search = color_search
        self.color_replace = color_replace

    def on_visit_node(self, node):
        if isinstance(node, glaxnimate.model.shapes.Styler) and node.color.value == self.color_search:
            node.color.value = self.color_replace


def main(window, document, settings):
    with document.macro("Replace color"):
        Replacer(settings["search"], settings["replace"]).visit(document)
