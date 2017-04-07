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
 * @copyright Copyright (C) 2016, 2017 DigitalGlobe (http://www.digitalglobe.com/)
 */
package hoot.services.controllers.ogr;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import hoot.services.command.ExternalCommand;

/*
    #
    #  GetOrgAttrib Make file
    #

    OP_INPUT=$(HOOT_HOME)/userfiles/tmp/upload/$(jobid)
    OP_OUTPUT=$(HOOT_HOME)/userfiles/tmp/$(jobid).out

    ###
    # Transform and load data
    ###
    step1:
        bash $(HOOT_HOME)/scripts/util/unzipfiles.sh "$(INPUT_ZIPS)" "$(OP_INPUT)"
        cd "$(OP_INPUT)" && hoot attribute-count --error $(INPUT_FILES) >> "$(OP_OUTPUT)"
        cd .. && rm -rf "$(OP_INPUT)"
 */
class GetAttributesCommand extends ExternalCommand {

    GetAttributesCommand(List<File> files, String debugLevel, Class<?> caller) {
        String inputFiles = files.stream()
                .map((file) -> quote(file.getAbsolutePath()))
                .collect(Collectors.joining(" "));

        Map<String, String> substitutionMap = new HashMap<>();
        substitutionMap.put("DEBUG_LEVEL", debugLevel);
        substitutionMap.put("INPUT_FILES", inputFiles);

        String command = "hoot attribute-count --${DEBUG_LEVEL} ${INPUT_FILES}";

        super.configureCommand(command, substitutionMap, caller);
    }
}
