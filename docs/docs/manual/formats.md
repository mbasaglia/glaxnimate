Authors: Mattia Basaglia

# Supported Formats

## Lottie

[Lottie animations](http://airbnb.io/lottie/) can be opened and saved, as of 0.1.0 not all features are implemented.

You can also preview how the animation looks on Lottie web from the *Document* menu.

## Telegram Animated Stickers

TGS files can be opened and saved, since they are based lon [Lottie](#lottie).

In the *Document* menu you can validate the [restrictions for this format](https://core.telegram.org/animated_stickers).
The same validation is performed when saving, if there are some issues the file will be saved anyway but you'll see some warnings.

## Scalable Vector Graphics

[SVG files](https://developer.mozilla.org/en-US/docs/Web/SVG) can be opened and saved.<br/>
You can also copy and paste SVG documents from the canvas view.

As of 0.3.1 Glaxnimate can export SMIL animations for SVG.

In particular, Glaxnimate support most of the extra attributes used by [Inkscape](https://inkscape.org/),
making interoperability with it rather easy.

## Raster Images

Raster Images (such as PNG, etc) can be rendered from individual frames and there's
an option to copy the selection as an image.

Some animated format (namely WebP and GIF) are supported.

You can also convert imported raster images into vector data using the [Trace Bitmap](/manual/ui/dialogs.md#trace-bitmap) action.
