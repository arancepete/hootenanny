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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018 DigitalGlobe (http://www.digitalglobe.com/)
 */
package hoot.services.controllers.export;

import static hoot.services.HootProperties.CHANGESET_DERIVE_BUFFER;
import static hoot.services.HootProperties.OSMAPI_DB_URL;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import hoot.services.models.db.Users;

class DeriveChangesetCommand extends ExportCommand {
    DeriveChangesetCommand(String jobId, ExportParams params, String debugLevel, Class<?> caller, Users user) {
        super(jobId, params);
        hoot.services.models.osm.Map conflatedMap = getConflatedMap(params.getInputId());

        String aoi = getAOI(params, conflatedMap);

        //This is set up for the XML changeset workflow.
        List<String> options = super.getCommonExportHootOptions(user);
        options.add("convert.bounding.box=" + aoi);
        if(user == null) {
            options.add("api.db.email=" + Users.TEST_USER.getEmail());
            options.add("changeset.user.id=" + Users.TEST_USER.getId());
        } else {
            options.add("api.db.email=" + user.getEmail());
            options.add("changeset.user.id=" + user.getId());
        }
        options.add("reader.use.file.status=true");
        options.add("reader.keep.status.tag=true");
        double changesetBufferSize = Double.parseDouble(CHANGESET_DERIVE_BUFFER); //in degrees
        options.add("changeset.buffer=" + String.valueOf(changesetBufferSize));
        options.add("changeset.allow.deleting.reference.features=false");

        List<String> hootOptions = toHootOptions(options);

        Map<String, Object> substitutionMap = new HashMap<>();
        substitutionMap.put("DEBUG_LEVEL", debugLevel);
        substitutionMap.put("HOOT_OPTIONS", hootOptions);
        substitutionMap.put("OSMAPI_DB_URL", OSMAPI_DB_URL);
        substitutionMap.put("INPUT", super.getInput());

        String command;

        if(params.getOutputType().equalsIgnoreCase("osc")) {
            // Just derive without apply (Will return .osc file to the REST caller)
            substitutionMap.put("CHANGESET_OUTPUT_PATH", super.getOutputPath());
            command = "hoot changeset-derive --${DEBUG_LEVEL} ${HOOT_OPTIONS} ${OSMAPI_DB_URL} ${INPUT} ${CHANGESET_OUTPUT_PATH}";
        } else {
            // Derive changeset here.  The actual apply command is issued via ApplyChangesetCommand from another class.
            substitutionMap.put("CHANGESET_OUTPUT_PATH", super.getSQLChangesetPath()); //"changeset-" + getJobId() + ".osc.sql"
            command = "hoot changeset-derive --${DEBUG_LEVEL} ${HOOT_OPTIONS} ${OSMAPI_DB_URL} ${INPUT} ${CHANGESET_OUTPUT_PATH} ${OSMAPI_DB_URL}";
        }

        super.configureCommand(command, substitutionMap, caller);
    }
}
