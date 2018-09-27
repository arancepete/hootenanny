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
 * @copyright Copyright (C) 2015, 2017, 2018 DigitalGlobe (http://www.digitalglobe.com/)
 */
#include "Schema.h"

#include <hoot/core/util/HootException.h>

namespace hoot
{

Schema::Schema()
{
}

void Schema::addLayer(boost::shared_ptr<Layer> l)
{
  _layers.push_back(l);
  _layerNameMap[l->getName()] = _layers.size()-1;
}

boost::shared_ptr<const Layer> Schema::getLayer(size_t i) const
{
  return _layers[i];
}

boost::shared_ptr<const Layer> Schema::getLayer(QString name) const
{
  std::map<QString, size_t>::const_iterator lit = _layerNameMap.find(name);

  if (lit != _layerNameMap.end())
    return _layers[lit->second];

  throw IllegalArgumentException("Unable to find layer with name: " + name);
}

bool Schema::hasLayer(QString name) const
{
  std::map<QString, size_t>::const_iterator lit = _layerNameMap.find(name);

  if (lit != _layerNameMap.end())
    return true;

  return false;

}

}
