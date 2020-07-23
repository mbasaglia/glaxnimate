#include "app/application.hpp"
#include "app_info.hpp"

class GlaxnimateApp : public app::Application
{
    Q_OBJECT

public:
    using app::Application::Application;

    void load_settings_metadata() const override;

protected:
    void on_initialize() override
    {
        AppInfo& info = AppInfo::instance();
        setApplicationName(info.slug());
        setApplicationDisplayName(info.name());
        setApplicationVersion(info.version());
        setOrganizationName(info.organization());
        setWindowIcon(QIcon(data_file("icon.svg")));
//
//         QStringList search_paths = data_paths("icons");
//         search_paths += QIcon::themeSearchPaths();
//         QIcon::setThemeSearchPaths(search_paths);
    }
};
