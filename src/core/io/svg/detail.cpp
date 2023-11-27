/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "detail.hpp"
#include "app/utils/qstring_literal.hpp"


const std::map<QString, QString> glaxnimate::io::svg::detail::xmlns = {
    {"osb"_qs, "http://www.openswatchbook.org/uri/2009/osb"_qs},
    {"dc"_qs, "http://purl.org/dc/elements/1.1/"_qs},
    {"cc"_qs, "http://creativecommons.org/ns#"_qs},
    {"rdf"_qs, "http://www.w3.org/1999/02/22-rdf-syntax-ns#"_qs},
    {"svg"_qs, "http://www.w3.org/2000/svg"_qs},
    {"sodipodi"_qs, "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"_qs},
    {"inkscape"_qs, "http://www.inkscape.org/namespaces/inkscape"_qs},
    {"xlink"_qs, "http://www.w3.org/1999/xlink"_qs},
    {"android"_qs, "http://schemas.android.com/apk/res/android"_qs},
    {"aapt"_qs, "http://schemas.android.com/aapt"_qs},
};

const std::unordered_set<QString> glaxnimate::io::svg::detail::css_atrrs = {
    "fill"_qs,
    "alignment-baseline"_qs,
    "baseline-shift"_qs,
    "clip-path"_qs,
    "clip-rule"_qs,
    "color"_qs,
    "color-interpolation"_qs,
    "color-interpolation-filters"_qs,
    "color-rendering"_qs,
    "cursor"_qs,
    "direction"_qs,
    "display"_qs,
    "dominant-baseline"_qs,
    "fill-opacity"_qs,
    "fill-rule"_qs,
    "filter"_qs,
    "flood-color"_qs,
    "flood-opacity"_qs,
    "font-family"_qs,
    "font-size"_qs,
    "font-size-adjust"_qs,
    "font-stretch"_qs,
    "font-style"_qs,
    "font-variant"_qs,
    "font-weight"_qs,
    "glyph-orientation-horizontal"_qs,
    "glyph-orientation-vertical"_qs,
    "image-rendering"_qs,
    "letter-spacing"_qs,
    "lighting-color"_qs,
    "marker-end"_qs,
    "marker-mid"_qs,
    "marker-start"_qs,
    "mask"_qs,
    "opacity"_qs,
    "overflow"_qs,
    "paint-order"_qs,
    "pointer-events"_qs,
    "shape-rendering"_qs,
    "stop-color"_qs,
    "stop-opacity"_qs,
    "stroke"_qs,
    "stroke-dasharray"_qs,
    "stroke-dashoffset"_qs,
    "stroke-linecap"_qs,
    "stroke-linejoin"_qs,
    "stroke-miterlimit"_qs,
    "stroke-opacity"_qs,
    "stroke-width"_qs,
    "text-anchor"_qs,
    "text-decoration"_qs,
    "text-overflow"_qs,
    "text-rendering"_qs,
    "unicode-bidi"_qs,
    "vector-effect"_qs,
    "visibility"_qs,
    "white-space"_qs,
    "word-spacing"_qs,
    "writing-mode"_qs
};
