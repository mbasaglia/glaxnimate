Authors: Mattia Basaglia

# Assets

This page describes where Glaxnimate looks for assets and how to contribute to them.

## Emoji sets

The list of available sets are listed in `data/emoji/sets.json`.
This file contains all the information on how to download and preview the emoji.

This data is shown in the [Emoji Sets Dialog](/manual/ui/dialogs/#emoji-sets).

It's a JSON file with an array of objects with the following structure:


```js
    {
        // Name of the set, as shown in the dialog
        "name" : "Twemoji",
        // Link to the website fot this set
        "url": "https://github.com/twitter/twemoji",
        // License name, shown on the dialog
        "license": "CC BY-SA 4.0",
        // Slug format, more on this later
        "slug_format": "low-",
        // URL used to download emoji SVG to preview, %1 will be replaced with the emoji slug
        "preview": "https://raw.githubusercontent.com/twitter/emoji/master/assets/svg/%1.svg";
        // Download info
        "download": {
            // URL used to download a tarball with the emoji files
            "url": "https://github.com/twitter/twemoji/archive/refs/heads/master.tar.gz",
            // List of paths within the tarball
            "paths": [
                {
                    // Path relative from the tarball
                    "path": "twemoji-master/assets/svg/",
                    // File format for files in this path
                    "format": "svg",
                    // Image size or "scalable" for vectors
                    "size": "scalable"
                },
                {
                    "path": "twemoji-master/assets/72x72/",
                    "format": "png",
                    "size": 72
                }
            ]
        }
    }
```

Since a lot of emoji sets are hosted on GitHub there's a shorthand version of
defining the data above:

```js
    {
        "name" : "Twemoji",
        "template": {
            // We are using the GitHub template to add missing data
            "type": "github",
            // Organization name
            "org": "twitter",
            // Repository name within the organization
            "repo": "twemoji",
            // Branch to download
            // You can also specify a tag instead of a branch using "tag": "some-tag"
            "branch": "master"
        },
        "license": "CC BY-SA 4.0",
        "slug_format": "low-",
        "download": {
            "paths": [
                {
                    // Note here we don't specify the root directory of the tarball
                    "path": "assets/svg/",
                    "format": "svg",
                    "size": "scalable"
                },
                {
                    "path": "assets/72x72/",
                    "format": "png",
                    "size": 72
                }
            ]
        }
    }
```

### Slug format

Different emoji sets use different naming conventions.

Glaxnimate allows configuration for each set to match their naming scheme,
as long as they have the hexadecimal unicode codepoints in the file name.

The `slug_format` attribute is interpreted as follows:

* Whether to use upper or lower case for the hex codes, it uses the case of the first character
* The separator between code points is given by the last character

If the file name has a prefix, you can specify it in the attribute `slug_prefix`.

Given the Emoji EU flag, defined by the sequence U+1f1ea U+1f1fa:

Twemoji uses the file `1f1ea-1f1fa.svg` so we have `slug_format` equal to `low-`.

OpenMoji uses the file `1F1EA-1F1FA.svg` so we have `slug_format` equal to `HIGH-`.

Noto emoji uses the file `emoji_u1f1ea_1f1fa.svg` so we have `slug_format` equal to `low_` and `slug_prefix` equal to `emoji_u`.
