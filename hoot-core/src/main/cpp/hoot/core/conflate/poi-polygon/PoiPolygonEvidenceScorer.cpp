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
 * @copyright Copyright (C) 2015 DigitalGlobe (http://www.digitalglobe.com/)
 */
#include "PoiPolygonEvidenceScorer.h"

// hoot
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/conflate/AlphaShapeGenerator.h>
#include <hoot/core/schema/OsmSchema.h>

#include "PoiPolygonTypeMatcher.h"
#include "PoiPolygonNameMatcher.h"
#include "PoiPolygonAddressMatcher.h"
#include "PoiPolygonDistanceMatcher.h"
#include "PoiPolygonCustomMatchRules.h"

namespace hoot
{

PoiPolygonEvidenceScorer::PoiPolygonEvidenceScorer(double matchDistance, double reviewDistance,
                                                   double distance, double typeScoreThreshold,
                                                   double nameScoreThreshold,
                                                   unsigned int matchEvidenceThreshold,
                                                   shared_ptr<Geometry> poiGeom,
                                                   shared_ptr<Geometry> polyGeom,
                                                   ConstOsmMapPtr map,
                                                   set<ElementId> polyNeighborIds,
                                                   set<ElementId> poiNeighborIds) :
_matchDistance(matchDistance),
_reviewDistance(reviewDistance),
_distance(distance),
_typeScoreThreshold(typeScoreThreshold),
_nameScoreThreshold(nameScoreThreshold),
_matchEvidenceThreshold(matchEvidenceThreshold),
_poiGeom(poiGeom),
_polyGeom(polyGeom),
_map(map),
_polyNeighborIds(polyNeighborIds),
_poiNeighborIds(poiNeighborIds)
{
}

unsigned int PoiPolygonEvidenceScorer::_getDistanceEvidence(ConstElementPtr poi,
                                                            ConstElementPtr poly)
{
  //search radius taken from PoiPolygonMatchCreator
  PoiPolygonDistanceMatcher distanceCalc(
    _matchDistance, _reviewDistance, poly->getTags(),
    poi->getCircularError() + ConfigOptions().getPoiPolygonMatchReviewDistance());
  _matchDistance =
    max(
      distanceCalc.getMatchDistanceForType(_t1BestKvp),
      distanceCalc.getMatchDistanceForType(_t2BestKvp));
  _reviewDistance =
    max(
      distanceCalc.getReviewDistanceForType(_t1BestKvp),
      distanceCalc.getReviewDistanceForType(_t2BestKvp));
  /*if (poi->getTags().get("station") != "light_rail" &&
      poi->getTags().get("amenity") != "fuel")
  {
    distanceCalc.modifyMatchDistanceForPolyDensity(_matchDistance);
    //distanceCalc.modifyReviewDistanceForPolyDensity(_reviewDistance);
  }*/

  // calculate the 2 sigma for the distance between the two objects
  const double sigma1 = poi->getCircularError() / 2.0;
  const double sigma2 = poly->getCircularError() / 2.0;
  const double ce = sqrt(sigma1 * sigma1 + sigma2 * sigma2) * 2;
  const double reviewDistancePlusCe = _reviewDistance + ce;
  _closeMatch = _distance <= reviewDistancePlusCe;
  //close match is a requirement, regardless of the evidence count
  if (!_closeMatch)
  {
    return 0;
  }
  unsigned int evidence = _distance <= _matchDistance ? 2 : 0;

  LOG_VART(_closeMatch);
  LOG_VART(_distance);
  LOG_VART(_matchDistance);
  LOG_VART(_reviewDistance);
  LOG_VART(reviewDistancePlusCe);
  LOG_VART(ce);
  LOG_VART(poi->getCircularError());
  LOG_VART(poly->getCircularError());

  return evidence;
}

unsigned int PoiPolygonEvidenceScorer::_getConvexPolyDistanceEvidence(ConstElementPtr poly)
{
  unsigned int evidence = 0;
  OsmMapPtr polyMap(new OsmMap());
  ElementPtr polyTemp(poly->clone());
  polyMap->addElement(polyTemp);
  shared_ptr<Geometry> polyAlphaShape = AlphaShapeGenerator(1000.0, 0.0).generateGeometry(polyMap);
  //oddly, even if the area is zero calc'ing the distance can have a positive effect
  /*if (polyAlphaShape->getArea() == 0.0)
  {
    return evidence;
  }*/
  const double alphaShapeDist = polyAlphaShape->distance(_poiGeom.get());
  evidence += alphaShapeDist <= _matchDistance ? 2 : 0;

  LOG_VART(alphaShapeDist);

  return evidence;
}

unsigned int PoiPolygonEvidenceScorer::_getTypeEvidence(ConstElementPtr poi, ConstElementPtr poly)
{
  PoiPolygonTypeMatcher typeScorer(_typeScoreThreshold);
  _typeScore = typeScorer.getTypeScore(poi, poly, _t1BestKvp, _t2BestKvp);
  if (poi->getTags().get("historic") == "monument")
  {
    //monuments can represent just about any poi type, so lowering this some to account for that
    _typeScoreThreshold = 0.3; //TODO: move to config
  }
  _typeMatch = _typeScore >= _typeScoreThreshold;
  unsigned int evidence = _typeMatch ? 1 : 0;

  LOG_VART(_typeScore);
  LOG_VART(_typeMatch);
  LOG_VART(_t1BestKvp);
  LOG_VART(_t2BestKvp);

  return evidence;
}

unsigned int PoiPolygonEvidenceScorer::_getNameEvidence(ConstElementPtr poi, ConstElementPtr poly)
{
  PoiPolygonNameMatcher nameScorer(_nameScoreThreshold);
  _nameScore = nameScorer.getNameScore(poi, poly);
  _nameMatch = _nameScore >= _nameScoreThreshold;
  _exactNameMatch = nameScorer.getExactNameScore(poi, poly) == 1.0;
  unsigned int evidence = _nameMatch ? 1 : 0;

  LOG_VART(_nameMatch);
  LOG_VART(_exactNameMatch);
  LOG_VART(_nameScore);

  return evidence;
}

unsigned int PoiPolygonEvidenceScorer::_getAddressEvidence(ConstElementPtr poi,
                                                           ConstElementPtr poly)
{
  const bool addressMatch = PoiPolygonAddressMatcher(_map).isMatch(poly, poi);
  LOG_VART(addressMatch);
  return addressMatch ? 1 : 0;
}

unsigned int PoiPolygonEvidenceScorer::calculateEvidence(ConstElementPtr poi, ConstElementPtr poly)
{
  unsigned int evidence = 0;

  evidence += _getDistanceEvidence(poi, poly);

  //close match is a requirement, regardless of the evidence count
  if (!_closeMatch && !ConfigOptions().getPoiPolygonPrintMatchDistanceTruth())
  {
    //don't exit early here if printing truths, b/c we need to calculate type match for that first
    //before exiting
    return 0;
  }

  evidence += _getTypeEvidence(poi, poly);

  //second chance to exit early if printing distance truths
  if (!_closeMatch)
  {
    return 0;
  }

  evidence += _getNameEvidence(poi, poly);

  //no point in calc'ing the address match if we already have a match from the other evidence
  //LOG_VARD(_matchEvidenceThreshold);
  if (evidence < _matchEvidenceThreshold)
  {
    if (ConfigOptions().getPoiPolygonEnableAddressMatching())
    {
      evidence += _getAddressEvidence(poi, poly);
    }
    //TODO: move values to config
    if (evidence < _matchEvidenceThreshold && _distance <= 35.0 &&
        poi->getTags().get("amenity") == "school" && OsmSchema::getInstance().isBuilding(poly))
    {
      evidence += _getConvexPolyDistanceEvidence(poly);
    }
  }

  if (evidence == 0)
  {
    if (_nameScore >= 0.4 && _typeScore >= 0.6) //TODO: move values to config
    {
      evidence++;
    }
    else if (ConfigOptions().getPoiPolygonEnableMatchRules())
    {
      PoiPolygonCustomMatchRules matchRules(
        _map, _polyNeighborIds, _poiNeighborIds, _distance, _polyGeom, _poiGeom);
      matchRules.collectInfo(poi, poly);
      if (matchRules.ruleTriggered())
      {
        evidence++;
      }
    }
  }

  LOG_VART(evidence);
  return evidence;
}

}

