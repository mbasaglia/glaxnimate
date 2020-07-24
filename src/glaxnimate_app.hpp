#pragma once
#include "app/application.hpp"


class GlaxnimateApp : public app::Application
{
    Q_OBJECT

public:
    using app::Application::Application;

    void load_settings_metadata() const override;

protected:
    void on_initialize() override;
};
