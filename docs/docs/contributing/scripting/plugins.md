Authors: Mattia Basaglia

# Plugins

Plugins go in the `plugins` data directory, each plugin must have
its own sub-directory and a `plugin.json` file that describes the plugin.

The plugin will be identified by the name of the directory it's in.

## plugin.json

This file describes the plugin and its functionality.

It's a JSON file with the following keys:

| Name          | Required  | Type   | Description                          |
| ------------- | --------- | ------ | ------------------------------------ |
| `name`        |           | string | Name shown in the list of plugins. If omitted it will be the plugin id.  |
| `description` |           | string | Longer description of what the plugin does. |
| `version`     |           | number | Plugin version number, used to resolve multiple installations of the same plugin |
| `engine`      | Required  | string | Script engine to use. (Currently must be `python`) |
| `author`      |           | string | Name of the plugin author.           |
| `icon`        |           | string | See [Icons](#icons)                  |
| `services`    | Required  | array  | Array of [services](#services).      |

### Icons

`icon` fields can use a file name relative to the plugin directory:

```json
{
    "icon": "my_icon.svg"
}
```

If the field value starts with "theme:" it will load an icon from the icon theme:

```json
{
    "icon": "theme:image-gif"
}
```

Theme icons follow the [Freedesktop icon naming specs](https://specifications.freedesktop.org/icon-naming-spec/latest/ar01s04.html).

There are also several icons not in the specs available in the default theme,
you can use [this icon searcher](https://icon-search.mattbas.org/) to find them.

## Services

A service is defined ad a JSON object, which must have a `type` field and
additional fields based on the service type.

### Action

Action services show an entry on the "Plugins" menu, and are triggered when the
user clicks on one of the actions.

The service JSON object has the following fields:

| Name      | Required  | Type   | Description                          |
| --------- | --------- | ------ | ------------------------------------ |
| `type`    | Required  | string | `action`                             |
| `script`  | Required  | object | [Script object](#scripts) to execute |
| `label`   |           | string | Text shown in the menu               |
| `tooltip` |           | string | Menu item tooltip                    |
| `icon`    |           | string | See [Icons](#icons)                  |

The script function will be invoked with these arguments:

* The application window
* The open document
* A `dict` with the [settings](#settings) values.

### Format

Format services services add support for opening and saving to more file formats.

They will be visible in the open and save dialogs.

The service JSON object has the following fields:

| Name          | Required  | Type   | Description                                                                   |
| ------------- | --------- | ------ | ----------------------------------------------------------------------------- |
| `type`        | Required  | string | `format`                                                                      |
| `name`        |           | string | Text shown in the dialog                                                      |
| `open`        |           | object | [Script object](#scripts) to execute on open                                  |
| `save`        |           | object | [Script object](#scripts) to execute on save                                  |
| `extensions`  | Required  | array  | Array of file extensions (without the dot)                                    |
| `slug`        |           | string | Format identifier, defaults to the first value in `extensions`                |
| `auto_open`   |           | bool   | If set to **false**, the file will not be opened before the plugin invokation |

At least one between `open` and `save` must be present.

`open` and `save` will be invoked with the following arguments:

* The application window
* The document to read from / write into
* A file-like object to perform io operations on
* The name of the file
* An ImportExport object to report back
* A `dict` with the [settings](#settings) values

## Scripts

A plugin script is defined as a JSON object that provides information on
how to run the plugin.

It has the following fields:

| Name      | Required  | Type   | Description                          |
| --------- | --------- | ------ | ------------------------------------ |
| `module`  | Required  | string | Python module to load                |
| `function`| Required  | string | Function within `module` to execute  |
| `settings`|           | object | [Settings](#settings) object         |

## Settings

Settings provide parameters to pass the invoked function.
The user will be shown a dialog with all these settings before the script execution.

If you need more advanced control over the dialog, see the [dialog example](examples.md#showing-a-dialog).

They are defined as a JSON array os settings objects:

| Name      | Required  | Type   | Description                                  |
| --------- | --------- | ------ | -------------------------------------------- |
| `name`    | Required  | string | Name, will be used as dict keys for values   |
| `type`    | Required  | string | Setting type                                 |
| `label`   |           | string | Form label (defaults to the setting name)    |
|`description`|         | string | Extra description                            |
| `default` |           | (any)  | Default value for the setting                |
| `min`     |           | number | Minimum value (for `int` and `float`)        |
| `max`     |           | number | Maximum value (for `int` and `float`)        |
| `choices` |           | array  | Available choices (for `choice`)             |

### Setting types

* `info`    Just displays the `description` without any input.
* `bool`    Shows a checkbox.
* `int`     Shows a spin box.
* `float`   Shows a spin box.
* `string`  Shows a line edit.
* `choice`  Shows a combo box.

## Example

Example file structure:

```
plugins/
    MyPlugin/
        plugin.json
        hello_world.py
```

The above plugin will have an ID of `MyPlugin`.

`plugin.json`:

```json
{
    "name": "Hello World Plugin",
    "description": "Shows a greeting",
    "author": "Glax",
    "engine": "python",
    "services": [
        {
            "type": "action",
            "label": "Hello world",
            "tooltip": "Does nothing useful",
            "script": {
                "module": "hello_world",
                "function": "main",
                "settings": [
                    {
                        "name": "greeting",
                        "label": "Type a greeting:",
                        "type": "string",
                        "default": "Hello World"
                    }
                ]
            }
        }
    ]
}

```

`hello_world.py`:

```py
def main(window, document, settings):
    window.warning(
        settings["greeting"]
    )
```
