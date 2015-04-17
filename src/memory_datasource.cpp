/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2014 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

// mapnik
#include <mapnik/debug.hpp>
#include <mapnik/query.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/memory_featureset.hpp>
#include <mapnik/boolean.hpp>
// boost

// stl
#include <algorithm>

using mapnik::datasource;
using mapnik::parameters;

DATASOURCE_PLUGIN(mapnik::memory_datasource)

namespace mapnik {

struct accumulate_extent
{
    accumulate_extent(box2d<double> & ext)
        : ext_(ext),first_(true) {}

    void operator() (feature_ptr const& feat)
    {
        auto size = feat->num_geometries();
        for (std::size_t i = 0; i < size; ++i)
        {
            geometry_type const& geom = feat->get_geometry(i);
            auto bbox = ::mapnik::envelope(geom);
            if ( first_ )
            {
                first_ = false;
                ext_ = bbox;
            }
            else
            {
                ext_.expand_to_include(bbox);
            }
        }
    }

    box2d<double> & ext_;
    bool first_;
};

const char * memory_datasource::name()
{
    return "memory";
}

memory_datasource::memory_datasource(parameters const& params)
    : datasource(params),
      desc_(memory_datasource::name(),
            *params.get<std::string>("encoding","utf-8")),
      type_(datasource::Vector),
      bbox_check_(*params.get<boolean_type>("bbox_check", true)) {}

memory_datasource::~memory_datasource() {}

void memory_datasource::push(feature_ptr feature)
{
    // TODO - collect attribute descriptors?
    //desc_.add_descriptor(attribute_descriptor(fld_name,mapnik::Integer));
    features_.push_back(feature);
}

datasource::datasource_t memory_datasource::type() const
{
    return type_;
}

featureset_ptr memory_datasource::features(const query& q) const
{
    return std::make_shared<memory_featureset>(q.get_bbox(),*this,bbox_check_);
}


featureset_ptr memory_datasource::features_at_point(coord2d const& pt, double tol) const
{
    box2d<double> box = box2d<double>(pt.x, pt.y, pt.x, pt.y);
    box.pad(tol);
    MAPNIK_LOG_DEBUG(memory_datasource) << "memory_datasource: Box=" << box << ", Point x=" << pt.x << ",y=" << pt.y;
    return std::make_shared<memory_featureset>(box,*this);
}

void memory_datasource::set_envelope(box2d<double> const& box)
{
    extent_ = box;
}

box2d<double> memory_datasource::envelope() const
{
    if (!extent_.valid())
    {
        accumulate_extent func(extent_);
        std::for_each(features_.begin(),features_.end(),func);
    }
    return extent_;
}

boost::optional<datasource::geometry_t> memory_datasource::get_geometry_type() const
{
    // TODO - detect this?
    return datasource::Collection;
}

layer_descriptor memory_datasource::get_descriptor() const
{
    return desc_;
}

size_t memory_datasource::size() const
{
    return features_.size();
}

void memory_datasource::clear()
{
    features_.clear();
}

}