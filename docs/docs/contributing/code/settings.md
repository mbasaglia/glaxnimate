Authors: Mattia Basaglia

# Settings

Glaxnimate uses an internal settings framework that automates saving/loading
settings and also what is displayed in the settings dialog.

Settings are organized into groups, which will result into tabs in the user-facing
settings dialog.

Reading and writing settings is easy enough, an example follows:

```c++
#include <QString>

#include "app/settings/settings.hpp"

void myfunc()
{
    // loads the current value stored in settings
    QString mycolor = app::settings::get<QString>("group_name", "setting_name");

    // ...

    // updates the value stored in settings
    // it will get saved when the application exits
    app::settings::set("group_name", "setting_name", mycolor);
}
```

## Definining new settings

To add a new setting, you need to register it in `src/gui/glaxnimate_app.cpp`.

User-facing settings have more options, that will affect the settings dialog.

Otherwise, if a setting is used only to store some internal state, you can
define an internal setting that can leave most fields empty.

See [GlaxnimateApp::load_settings_metadata()](https://gitlab.com/mattia.basaglia/glaxnimate/-/blob/master/src/gui/glaxnimate_app.cpp)
for examples of how the current settings are defined.
