Authors: Mattia Basaglia

# Plugins

Plugins go in the `plugins` data directory, each plugin must have
its own sub-directory and a `plugin.json` file that describes the plugin.

The plugin will be identified by the name of the directory it's in.

## plugin.json

This file describes the plugin and its functionality.

It's a JSON file with the following keys:

| Name      | Required  | Type   | Description                          |
| --------- | --------- | ------ | ------------------------------------ |
| `name`    |           | string | Name shown in the list of plugins. If omitted it will be the plugin id.  |
| `version` |           | number | Plugin version number, used to resolve multiple installations of the same plugin |
| `engine`  | Required  | string | Script engine to use. (Currently must be `python`) |
| `author`  |           | string | Name of the plugin author.           |
| `icon`    |           | string | File name or theme icon name.        |
| `services`| Required  | array  | Array of [services](#services).      |


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
| `icon`    |           | string | File name or theme icon name         |

The script function will be invoked with these arguments:

* The application window
* The open document
* A `dict` with the [settings](#settings) values.

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
