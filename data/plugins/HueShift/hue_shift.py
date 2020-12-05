import glaxnimate


class HueShiftVisitor(glaxnimate.model.Visitor):
    def __init__(self, amount):
        super().__init__()
        self.amount = amount
        self.gradients = set()
        self.named_colors = set()

    def _color(self, color):
        return glaxnimate.utils.Color.from_hsv(
            (color.hue + self.amount) % 360,
            color.saturation,
            color.value,
            color.alpha
        )

    def on_visit_node(self, node):
        if isinstance(node, glaxnimate.model.shapes.Styler):
            node.color.value = self._color(node.color.value)
            if node.use:
                if isinstance(node.use, glaxnimate.model.defs.NamedColor):
                    if node.use not in self.named_colors:
                        node.use.color.value = self._color(node.use.color.value)
                        self.named_colors.add(node.use)
                elif isinstance(node.use, glaxnimate.model.defs.Gradient) and node.use.colors:
                    if node.use.colors not in self.gradients:
                        node.use.colors.colors.value = [
                            (offset, self._color(color))
                            for offset, color in node.use.colors.colors.value
                        ]
                        self.gradients.add(node.use.colors)


def main(window, document, settings):
    with document.macro("Hue Shift"):
        visitor = HueShiftVisitor(settings["amount"]);
        for shape in window.cleaned_selection:
            print(shape)
            visitor.visit(shape, True)
