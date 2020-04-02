/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2019 DigitalGlobe (http://www.digitalglobe.com/)
 */

#include "CookieCutter.h"

// Hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/util/MapProjector.h>
#include <hoot/core/ops/SuperfluousNodeRemover.h>
#include <hoot/core/ops/SuperfluousWayRemover.h>
#include <hoot/core/util/GeometryUtils.h>
#include <hoot/core/visitors/UnionPolygonsVisitor.h>
#include <hoot/core/visitors/CalculateMapBoundsVisitor.h>
#include <hoot/core/io/OsmMapWriterFactory.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/ops/MapCropper.h>

using namespace geos::geom;

namespace hoot
{

CookieCutter::CookieCutter(bool crop, double outputBuffer, bool keepEntireFeaturesCrossingBounds,
                           bool keepOnlyFeaturesInsideBounds) :
_crop(crop),
_outputBuffer(outputBuffer),
_keepEntireFeaturesCrossingBounds(keepEntireFeaturesCrossingBounds),
_keepOnlyFeaturesInsideBounds(keepOnlyFeaturesInsideBounds)
{
}

void CookieCutter::cut(OsmMapPtr& cutterShapeOutlineMap, OsmMapPtr& doughMap)
{
  LOG_VARD(cutterShapeOutlineMap->getNodes().size());
  LOG_VARD(MapProjector::toWkt(cutterShapeOutlineMap->getProjection()));
  OsmMapWriterFactory::writeDebugMap(
    cutterShapeOutlineMap, "cookie-cutter-cutter-shape-outline-map");
  LOG_VARD(doughMap->getNodes().size());
  LOG_VARD(MapProjector::toWkt(doughMap->getProjection()));
  OsmMapWriterFactory::writeDebugMap(doughMap, "cookie-cutter-dough-map");

  OGREnvelope env = CalculateMapBoundsVisitor::getBounds(cutterShapeOutlineMap);
  LOG_VARD(GeometryUtils::toEnvelope(env)->toString());
  env.Merge(CalculateMapBoundsVisitor::getBounds(doughMap));

  // reproject the dough and cutter into the same planar projection.
  MapProjector::projectToPlanar(doughMap, env);
  MapProjector::projectToPlanar(cutterShapeOutlineMap, env);

  // create a complex geometry representing the alpha shape
  UnionPolygonsVisitor v;
  cutterShapeOutlineMap->visitRo(v);
  std::shared_ptr<Geometry> cutterShape = v.getUnion();
  if (_outputBuffer != 0.0)
  {
    cutterShape.reset(cutterShape->buffer(_outputBuffer));
  }
  if (cutterShape->getArea() == 0.0)
  {
    // would rather this be thrown than a warning logged, as the warning may go unoticed by web
    // clients who are expecting the cookie cutting to occur
    throw HootException("Cutter area is zero. Try increasing the buffer size or check the input.");
  }
  LOG_VARD(cutterShape->toString());

  // free up a little RAM
  cutterShapeOutlineMap.reset();

  // remove the cookie cutter portion from the dough
  MapCropper cropper(cutterShape);
  cropper.setConfiguration(conf());
  cropper.setKeepEntireFeaturesCrossingBounds(_keepEntireFeaturesCrossingBounds);
  cropper.setKeepOnlyFeaturesInsideBounds(_keepOnlyFeaturesInsideBounds);
  cropper.setInvert(!_crop);
  cropper.apply(doughMap);
  OsmMapWriterFactory::writeDebugMap(doughMap, "cookie-cutter-dough-crop");

  OsmMapPtr cookieCutMap = doughMap;

  // clean up any ugly bits left over
  // TODO: This can be removed now, since its already happening in MapCropper, right?
  SuperfluousWayRemover::removeWays(cookieCutMap);
  SuperfluousNodeRemover::removeNodes(cookieCutMap);

  LOG_VARD(cookieCutMap->getNodes().size());
  LOG_VARD(MapProjector::toWkt(cookieCutMap->getProjection()));
  OsmMapWriterFactory::writeDebugMap(cookieCutMap, "cookie-cutter-cookie-cut-map");
}

}
