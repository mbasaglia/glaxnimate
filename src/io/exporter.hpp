#pragma once

#include "base.hpp"

namespace io {

class Exporter : public ImportExport
{
public:
    static ImportExportFactory<Exporter>& factory()
    {
        static ImportExportFactory<Exporter> instance;
        return instance;
    }
};

} // namespace io
