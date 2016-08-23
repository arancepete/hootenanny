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
 * @copyright Copyright (C) 2014 DigitalGlobe (http://www.digitalglobe.com/)
 */

/*
    OSM+ to "English" TDS conversion script
*/

etds40 = {
    // This function converts the OSM+ to TDS and then translates the TDS into "English"
    toEnglish : function(tags, elementType, geometryType)
    {
        var tdsData = [];

        tdsData = tds.toNfdd(tags, elementType, geometryType)

        // Debug:
        if (config.getOgrDebugDumptags() == 'true')
        {
            for (var i = 0, fLen = tdsData.length; i < fLen; i++)
            {
                print('eTableName ' + i + ': ' + tdsData[i]['tableName'] + '  FCode: ' + tdsData[i]['attrs']['F_CODE'] + '  Geom: ' + geometryType);
                var kList = Object.keys(tdsData[i]['attrs']).sort()
                for (var j = 0, kLen = kList.length; j < kLen; j++) print('eOut Attrs:' + kList[j] + ': :' + tdsData[i]['attrs'][kList[j]] + ':');
            }
            print('');
        }

        var eAttrs = {}; // The final English output

        // Add a default value for the FCODE
        eAttrs['Feature Code'] = 'Not found';

        if (tdsData.length > 0)
        {
            for (var fNum = 0, fLen = tdsData.length; fNum < fLen; fNum++)
            {
                var tFCODE = tdsData[fNum]['attrs']['F_CODE'];

                if (tFCODE == 'Partial')
                {
                    // Go looking for "OSM:XXX" values and copy them to the output
                    for (var i in tdsData[fNum]['attrs'])
                    {
                        if (i.indexOf('OSM:') > -1)
                        {
                            eAttrs[i] = tdsData[fNum]['attrs'][i];
                            delete tdsData[fNum]['attrs'][i]
                        }
                    }
                }
                else
                {
                    // Go through the list of possible attributes and add the missing ones
                    var tmpList = etds40.rules.fcodeLookup[tFCODE]['enum'];

                    for (var i=0, elen = tmpList.length; i < elen; i++)
                    {
                        // If we don't find one, add it with it's default value
                        if (!(tdsData[fNum]['attrs'][tmpList[i]]))
                        {
                            tdsData[fNum]['attrs'][tmpList[i]] = etds40.rules.engDefault[tmpList[i]];
                        }
                    }
                }

                // Translate the single values
                for (var val in tdsData[fNum]['attrs'])
                {
                    if (val in etds40.rules.engSingle)
                    {
                        if (tdsData[fNum]['attrs'][val] == undefined)
                        {
                            eAttrs[etds40.rules.engSingle[val]] = etds40.rules.engDefault[val];
                        }
                        else
                        {
                            eAttrs[etds40.rules.engSingle[val]] = tdsData[fNum]['attrs'][val];
                        }

                        // Cleanup used attrs so we don't translate them again
                        delete tdsData[fNum]['attrs'][val];
                    }
                }

                // Apply the English one2one rules
                translate.applyOne2One(tdsData[fNum]['attrs'], eAttrs, etds40.rules.engEnum, {'k':'v'});

                // Find an FCODE
                if (tFCODE in etds40.rules.fcodeLookup)
                {
                    if (eAttrs['Feature Code'] !== 'Not found')
                    {
                        eAttrs['Feature Code'] = eAttrs['Feature Code'] + ' & ' + tFCODE + ':' + etds40.rules.fcodeLookup[tFCODE]['desc'];
                    }
                    else
                    {
                        eAttrs['Feature Code'] = tFCODE + ':' + etds40.rules.fcodeLookup[tFCODE]['desc'];
                    }
                }
            } // End for tdsData

        }
        else
        {
            // If we can't find an FCODE, just return the tags.
            // We _probably_ won't get here but just in case.....

            // Add "OSM:" to each of the tags
            for (var i in tags)
            {
                eAttrs['OSM:' + i] = tags[i];
            }
        }

        if (config.getOgrDebugDumptags() == 'true')
        {
            var kList = Object.keys(eAttrs).sort()
            for (var j = 0, kLen = kList.length; j < kLen; j++) print('Final Attrs:' + kList[j] + ': :' + eAttrs[kList[j]] + ':');
            print('');
        }

        // Return the English results. The "tableName" is not set
        return {attrs: eAttrs, tableName: ''};
    } // End of toEnglish

} // End of etds40