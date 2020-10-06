#include "raster_format.hpp"
#include "io/raster/raster_mime.hpp"

io::Autoreg<io::raster::RasterMime> io::raster::RasterMime::autoreg;
io::Autoreg<io::raster::RasterFormat> io::raster::RasterFormat::autoreg;
