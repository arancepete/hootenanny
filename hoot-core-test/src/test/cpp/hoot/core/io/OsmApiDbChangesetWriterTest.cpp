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
 * @copyright Copyright (C) 2013, 2014, 2015 DigitalGlobe (http://www.digitalglobe.com/)
 */

// CPP Unit
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>

// Hoot
#include <hoot/core/io/OsmApiDb.h>
#include <hoot/core/io/OsmApiDbChangesetWriter.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/GeometryUtils.h>

// Qt
#include <QDateTime>
#include <QSqlError>

#include "../TestUtils.h"
#include "ServicesDbTestUtils.h"

namespace hoot
{

class OsmApiDbChangesetWriterTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(OsmApiDbChangesetWriterTest);
  CPPUNIT_TEST(runSqlChangesetWriteTest);
  CPPUNIT_TEST(runConflictWithConflictsTest);
  CPPUNIT_TEST(runConflictNonIntersectingBoundsTest);
  CPPUNIT_TEST(runConflictCreatedAtAfterTimeTest);
  CPPUNIT_TEST(runConflictBadTimeTest);
  CPPUNIT_TEST_SUITE_END();

public:

  static QString userEmail() { return "OsmApiDbTest@hoottestcpp.org"; }

  long mapId;

  void setUp()
  {
    deleteUser(userEmail());
  }

  void tearDown()
  {
    deleteUser(userEmail());

    OsmApiDb database;
    database.open(ServicesDbTestUtils::getOsmApiDbUrl());
    database.deleteData();
    database.close();
  }

  void deleteUser(QString email)
  {
    OsmApiDb db;
    db.open(ServicesDbTestUtils::getOsmApiDbUrl());

    long userId = db.getUserId(email, false);
    if (userId != -1)
    {
      db.transaction();
      db.deleteUser(userId);
      db.commit();
    }
  }

  void insertChangeset(const QDateTime& createdAt, const QString boundsStr)
  {
    OsmApiDb db;
    db.open(ServicesDbTestUtils::getOsmApiDbUrl());

    const Envelope bounds = GeometryUtils::envelopeFromConfigString(boundsStr);
    LOG_VARD(boundsStr);
    LOG_VARD(createdAt.toString(OsmApiDb::TIME_FORMAT));
    const QString sql =
      QString("INSERT INTO changesets (id, user_id, created_at, closed_at, min_lat, max_lat, min_lon, max_lon) ") +
      QString("VALUES (1, 1, '%1', '%2', %3, %4, %5, %6)")
        .arg(createdAt.toString(OsmApiDb::TIME_FORMAT))
        .arg(createdAt.toString(OsmApiDb::TIME_FORMAT))
        .arg(bounds.getMinY())
        .arg(bounds.getMaxY())
        .arg(bounds.getMinX())
        .arg(bounds.getMaxX());
    QSqlQuery q(db.getDB());
    LOG_VARD(sql);
    if (q.exec(sql) == false)
    {
      throw HootException(QString("Error executing query: %1 (%2)").arg(q.lastError().text()).arg(sql));
    }
    LOG_VARD(q.numRowsAffected());
  }

  void runSqlChangesetWriteTest()
  {
    OsmApiDb database;
    database.open(ServicesDbTestUtils::getOsmApiDbUrl());
    database.deleteData();
    ServicesDbTestUtils::execOsmApiDbSqlTestScript("users.sql");
    ServicesDbTestUtils::execOsmApiDbSqlTestScript("changesets.sql");

    shared_ptr<QSqlQuery> nodesItr = database.selectElements(ElementType::Node);
    assert(nodesItr->isActive());
    CPPUNIT_ASSERT_EQUAL(0, nodesItr->size());

    ServicesDbTestUtils::execOsmApiDbSqlTestScript("nodes.sql");

    nodesItr = database.selectElements(ElementType::Node);
    assert(nodesItr->isActive());
    int nodesCountBefore = 0;
    long existingChangesetId = -1;
    while (nodesItr->next())
    {
      existingChangesetId = nodesItr->value(3).toLongLong();
      LOG_VARD(existingChangesetId)
      nodesCountBefore++;
    }
    nodesCountBefore--;
    LOG_VARD(nodesCountBefore);

    const long nextNodeId = database.getNextId("current_nodes");
    LOG_VARD(nextNodeId);

    const long nextChangesetId = existingChangesetId + 1;
    QString sql =
      QString("INSERT INTO changesets (id, user_id, created_at, closed_at) VALUES (%1, 1, now(), now());\n")
        .arg(nextChangesetId);
    sql +=
      QString("INSERT INTO current_nodes (id, latitude, longitude, changeset_id, visible, \"timestamp\", tile,  version) VALUES (%1, 0, 0, %2, true, now(), 3221225472, 1);")
        .arg(nextNodeId)
        .arg(nextChangesetId);

    OsmApiDbChangesetWriter(ServicesDbTestUtils::getOsmApiDbUrl()).write(sql);

    nodesItr = database.selectElements(ElementType::Node);
    assert(nodesItr->isActive());
    int nodesCountAfter = 0;
    while (nodesItr->next())
    {
      nodesCountAfter++;
    }
    nodesCountAfter--;
    LOG_VARD(nodesCountAfter);

    CPPUNIT_ASSERT(nodesCountAfter == (nodesCountBefore + 1));
  }

  void runConflictWithConflictsTest()
  {
    OsmApiDb database;
    database.open(ServicesDbTestUtils::getOsmApiDbUrl());
    database.deleteData();
    ServicesDbTestUtils::execOsmApiDbSqlTestScript("users.sql");

    //grab the current time
    const QDateTime startTime = QDateTime::currentDateTime();

    //define an aoi
    const QString aoi = "-10,-10,10,10";

    //write a changeset with an intersecting aoi and some created time in the future
    insertChangeset(startTime.toUTC().addSecs(60 * 60), "-15,-15,5,5");

    //changeset writer should detect a conflict when passed the same aoi and the current time,
    //since a changeset was written intersecting with the aoi after the specified time
    CPPUNIT_ASSERT(
      OsmApiDbChangesetWriter(ServicesDbTestUtils::getOsmApiDbUrl())
        .conflictExistsInTarget(aoi, startTime.toString(OsmApiDb::TIME_FORMAT)));
  }

  void runConflictNonIntersectingBoundsTest()
  {
    OsmApiDb database;
    database.open(ServicesDbTestUtils::getOsmApiDbUrl());
    database.deleteData();
    ServicesDbTestUtils::execOsmApiDbSqlTestScript("users.sql");

    //grab the current time
    const QDateTime startTime = QDateTime::currentDateTime().toUTC();

    //define an aoi
    const QString aoi = "-10,-10,10,10";

    //write a changeset with an non-intersecting aoi and some created time in the future
    insertChangeset(startTime.toUTC().addSecs(60 * 60), "-20,-20,-11,-11");

    //changeset writer should not detect a conflict since the aois don't intersect
    CPPUNIT_ASSERT(
      !OsmApiDbChangesetWriter(ServicesDbTestUtils::getOsmApiDbUrl())
        .conflictExistsInTarget(aoi, startTime.toString(OsmApiDb::TIME_FORMAT)));
  }

  void runConflictCreatedAtAfterTimeTest()
  {
    OsmApiDb database;
    database.open(ServicesDbTestUtils::getOsmApiDbUrl());
    database.deleteData();
    ServicesDbTestUtils::execOsmApiDbSqlTestScript("users.sql");

    //define an aoi
    const QString aoi = "-10,-10,10,10";

    //write a changeset with an intersecting aoi and the current time
    insertChangeset(QDateTime::currentDateTime().toUTC(), "-15,-15,5,5");

    //wait just a sec to make sure time passes
    sleep(1);

    //changeset writer should not detect a conflict when passed the same aoi and the current time,
    //since the changeset was written beforehand
    CPPUNIT_ASSERT(
      !OsmApiDbChangesetWriter(ServicesDbTestUtils::getOsmApiDbUrl())
        .conflictExistsInTarget(
            aoi, QDateTime::currentDateTime().toUTC().toString(OsmApiDb::TIME_FORMAT)));
  }

  //Bounds error checking tests are covered in GeometryUtilsTest.

  void runConflictBadTimeTest()
  {
    QString exceptionMsg;

    try
    {
      OsmApiDbChangesetWriter(ServicesDbTestUtils::getOsmApiDbUrl())
        .conflictExistsInTarget("-10,-10,10,10", "2016-05-04 10:15");
    }
    catch (HootException e)
    {
      exceptionMsg = e.what();
    }
    CPPUNIT_ASSERT(exceptionMsg.contains("Invalid timestamp"));

    try
    {
      OsmApiDbChangesetWriter(ServicesDbTestUtils::getOsmApiDbUrl())
        .conflictExistsInTarget("-10,-10,10,10", " ");
    }
    catch (HootException e)
    {
      exceptionMsg = e.what();
    }
    CPPUNIT_ASSERT(exceptionMsg.contains("Invalid timestamp"));
  }
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(OsmApiDbChangesetWriterTest, "slow");

}
