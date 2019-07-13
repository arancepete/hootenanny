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
 * @copyright Copyright (C) 2017, 2018, 2019 DigitalGlobe (http://www.digitalglobe.com/)
 */

// geos
#include <geos/io/WKTReader.h>
#include <geos/geom/Point.h>

// Hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/TestUtils.h>
#include <hoot/core/io/OsmGeoJsonWriter.h>
#include <hoot/core/io/OsmXmlReader.h>
#include <hoot/core/util/Log.h>

// TGS
#include <tgs/Statistics/Random.h>

namespace hoot
{

class OsmGeoJsonWriterTest : public HootTestFixture
{
  CPPUNIT_TEST_SUITE(OsmGeoJsonWriterTest);
  CPPUNIT_TEST(runAllDataTypesTest);
  CPPUNIT_TEST(runDcTigerTest);
  CPPUNIT_TEST(runBostonSubsetRoadBuildingTest);
  CPPUNIT_TEST(runObjectGeoJsonTest);
  CPPUNIT_TEST(runObjectGeoJsonHootTest);
  CPPUNIT_TEST_SUITE_END();

public:

  OsmGeoJsonWriterTest()
    : HootTestFixture("test-files/io/GeoJson/",
                      "test-output/io/GeoJson/")
  {
    setResetType(ResetBasic);
  }

  void runAllDataTypesTest()
  {
    runTest("test-files/conflate/unified/AllDataTypesA.osm", "AllDataTypes.geojson");
  }

  void runDcTigerTest()
  {
    runTest("test-files/DcTigerRoads.osm", "DcTigerRoads.geojson");
  }

  void runBostonSubsetRoadBuildingTest()
  {
    //  Suppress the warning from the OsmXmlReader about missing nodes for ways by temporarily changing
    //  the log level.  We expect the nodes to be missing since the Boston data has issues
    Log::WarningLevel logLevel = Log::getInstance().getLevel();
    Log::getInstance().setLevel(Log::Error);
    runTest("test-files/BostonSubsetRoadBuilding_FromOsm.osm", "BostonSubsetRoadBuilding.geojson");
    Log::getInstance().setLevel(logLevel);
  }

  void runObjectGeoJsonTest()
  {
    runTest(_inputPath + "SampleObjectsWriter.osm", "SampleObjectsWriter.geojson");
  }

  void runObjectGeoJsonHootTest()
  {
    Settings s;
    s.set(ConfigOptions::getJsonPrettyPrintKey(), true);
    s.set(ConfigOptions::getJsonFormatHootenannyKey(), true);

    runTest(_inputPath + "SampleObjectsWriter.osm", "SampleObjectsWriterHoot.geojson", &s);
  }

  void runTest(const QString& input, const QString& output, Settings* settings = NULL)
  {
    OsmXmlReader reader;

    OsmMapPtr map(new OsmMap());
    reader.setDefaultStatus(Status::Unknown1);
    reader.read(input, map);

    OsmGeoJsonWriter writer;
    writer.open(_outputPath + output);

    if (settings)
      writer.setConfiguration(*settings);

    writer.setIncludeCircularError(false);

    writer.write(map);
    HOOT_FILE_EQUALS( _inputPath + output,
                     _outputPath + output);
  }

};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(OsmGeoJsonWriterTest, "quick");
//CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(OsmGeoJsonWriterTest, "current");

}
