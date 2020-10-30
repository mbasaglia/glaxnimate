import glaxnimate


class Replacer(glaxnimate.model.Visitor):
    def __init__(self, color_search, color_replace):
        super().__init__()
        self.color_search = color_search
        self.color_replace = color_replace

    def on_visit_node(self, node):
        if (
            isinstance(node, glaxnimate.model.shapes.Styler) and
            node.color.value == self.color_search
        ):
            node.color.value = self.color_replace


def main(window, document, settings):
    dialog = window.create_dialog("replace.ui")
    if not dialog:
        return

    dialog.set_value("search", "color", window.current_color)
    if dialog.exec():
        with document.macro("Replace color"):
            Replacer(dialog.get_value("search", "color"), dialog.get_value("replace", "color")).visit(document, True)
