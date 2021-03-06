/*
  3dfier: takes 2D GIS datasets and "3dfies" to create 3D city models.

  Copyright (C) 2015-2016  3D geoinformation research group, TU Delft

  This file is part of 3dfier.

  3dfier is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  3dfier is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with 3difer.  If not, see <http://www.gnu.org/licenses/>.

  For any information or further details about the use of 3dfier, contact
  Hugo Ledoux
  <h.ledoux@tudelft.nl>
  Faculty of Architecture & the Built Environment
  Delft University of Technology
  Julianalaan 134, Delft 2628BL, the Netherlands
*/

#include "TopoFeature.h"
#include "io.h"

int TopoFeature::_count = 0;

//-----------------------------------------------------------------------------

TopoFeature::TopoFeature(char *wkt, std::string layername, std::vector<std::tuple<std::string, OGRFieldType, std::string>> attributes, std::string pid) {
  _id = pid;
  _counter = _count++;
  _toplevel = true;
  _bVerticalWalls = false;
  _p2 = new Polygon2();
  bg::read_wkt(wkt, *_p2);
  bg::unique(*_p2); //-- remove duplicate vertices
  bg::correct(*_p2); //-- correct the orientation of the polygons!

  _adjFeatures = new std::vector<TopoFeature*>;
  _p2z.resize(bg::num_interior_rings(*_p2) + 1);
  _p2z[0].resize(bg::num_points(_p2->outer()));
  _lidarelevs.resize(bg::num_interior_rings(*_p2) + 1);
  _lidarelevs[0].resize(bg::num_points(_p2->outer()));
  for (int i = 0; i < bg::num_interior_rings(*_p2); i++) {
    _p2z[i + 1].resize(bg::num_points(_p2->inners()[i]));
    _lidarelevs[i + 1].resize(bg::num_points(_p2->inners()[i]));
  }
  _attributes = attributes;
  _layername = layername;
}

TopoFeature::~TopoFeature() {
  // TODO: clear memory properly
  std::clog << "I am dead now." << std::endl;
}

Box2 TopoFeature::get_bbox2d() {
  return bg::return_envelope<Box2>(*_p2);
}

std::string TopoFeature::get_id() {
  return _id;
}

bool TopoFeature::buildCDT() {
  getCDT(_p2, _p2z, _vertices, _triangles);
  return true;
}

int TopoFeature::get_counter() {
  return _counter;
}

bool TopoFeature::get_top_level() {
  return _toplevel;
}

void TopoFeature::set_top_level(bool toplevel) {
  _toplevel = toplevel;
}

Polygon2* TopoFeature::get_Polygon2() {
  return _p2;
}

std::string TopoFeature::get_obj(std::unordered_map< std::string, unsigned long > &dPts, std::string mtl) {
  std::stringstream ss;
  ss << mtl << std::endl;
  for (auto& t : _triangles) {
    unsigned long a, b, c;
    auto it = dPts.find(gen_key_bucket(&_vertices[t.v0]));
    if (it == dPts.end()) {
      // first get the size + 1 and then store the size in dPts due to unspecified order of execution
      a = dPts.size() + 1;
      dPts[gen_key_bucket(&_vertices[t.v0])] = a;
    }
    else
      a = it->second;
    it = dPts.find(gen_key_bucket(&_vertices[t.v1]));
    if (it == dPts.end()) {
      b = dPts.size() + 1;
      dPts[gen_key_bucket(&_vertices[t.v1])] = b;
    }
    else
      b = it->second;
    it = dPts.find(gen_key_bucket(&_vertices[t.v2]));
    if (it == dPts.end()) {
      c = dPts.size() + 1;
      dPts[gen_key_bucket(&_vertices[t.v2])] = c;
    }
    else
      c = it->second;
    if ((a != b) && (a != c) && (b != c))
      ss << "f " << a << " " << b << " " << c << std::endl;
    // else
    //   std::clog << "COLLAPSED TRIANGLE REMOVED" << std::endl;
  }

  //-- vertical triangles
  if (_bVerticalWalls == true && _triangles_vw.size() > 0)
    ss << mtl << "Wall" << std::endl;

  for (auto& t : _triangles_vw) {
    unsigned long a, b, c;
    auto it = dPts.find(gen_key_bucket(&_vertices_vw[t.v0]));
    if (it == dPts.end()) {
      a = dPts.size() + 1;
      dPts[gen_key_bucket(&_vertices_vw[t.v0])] = a;
    }
    else
      a = it->second;
    it = dPts.find(gen_key_bucket(&_vertices_vw[t.v1]));
    if (it == dPts.end()) {
      b = dPts.size() + 1;
      dPts[gen_key_bucket(&_vertices_vw[t.v1])] = b;
    }
    else
      b = it->second;
    it = dPts.find(gen_key_bucket(&_vertices_vw[t.v2]));
    if (it == dPts.end()) {
      c = dPts.size() + 1;
      dPts[gen_key_bucket(&_vertices_vw[t.v2])] = c;
    }
    else
      c = it->second;
    if ((a != b) && (a != c) && (b != c))
      ss << "f " << a << " " << b << " " << c << std::endl;
    // else
    //   std::clog << "COLLAPSED TRIANGLE REMOVED" << std::endl;
  }
  return ss.str();
}

std::string TopoFeature::get_imgeo_object_info(std::string id) {
  std::stringstream ss;
  std::string attribute;
  if (get_attribute("creationDate", attribute)) {
    ss << "<imgeo:creationDate>" << attribute << "</imgeo:creationDate>" << std::endl;
  }
  if (get_attribute("terminationDate", attribute)) {
    ss << "<imgeo:terminationDate>" << attribute << "</imgeo:terminationDate>" << std::endl;
  }
  if (get_attribute("lokaalid", attribute)) {
    ss << "<imgeo:identificatie>" << std::endl;
    ss << "<imgeo:NEN3610ID>" << std::endl;
    ss << "<imgeo:namespace>NL.IMGeo</imgeo:namespace>" << std::endl;
    ss << "<imgeo:lokaalID>" << attribute << "</imgeo:lokaalID>" << std::endl;
    ss << "</imgeo:NEN3610ID>" << std::endl;
    ss << "</imgeo:identificatie>" << std::endl;
  }
  if (get_attribute("tijdstipregistratie", attribute)) {
    ss << "<imgeo:tijdstipRegistratie>" << attribute << "</imgeo:tijdstipRegistratie>" << std::endl;
  }
  if (get_attribute("eindregistratie", attribute)) {
    ss << "<imgeo:eindRegistratie>" << attribute << "</imgeo:eindRegistratie>" << std::endl;
  }
  if (get_attribute("lv-publicatiedatum", attribute)) {
    ss << "<imgeo:LV-publicatiedatum>" << attribute << "</imgeo:LV-publicatiedatum>" << std::endl;
  }
  if (get_attribute("bronhouder", attribute)) {
    ss << "<imgeo:bronhouder>" << attribute << "</imgeo:bronhouder>" << std::endl;
  }
  if (get_attribute("inonderzoek", attribute)) {
    ss << "<imgeo:inOnderzoek>" << attribute << "</imgeo:inOnderzoek>" << std::endl;
  }
  if (get_attribute("relatievehoogteligging", attribute)) {
    ss << "<imgeo:relatieveHoogteligging>" << attribute << "</imgeo:relatieveHoogteligging>" << std::endl;
  }
  if (get_attribute("bgt-status", attribute, "bestaand")) {
    ss << "<imgeo:bgt-status codeSpace=\"http://www.geostandaarden.nl/imgeo/def/2.1#Status\">" << attribute << "</imgeo:bgt-status>" << std::endl;
  }
  if (get_attribute("plus-status", attribute)) {
    ss << "<imgeo:plus-status>" << attribute << "</imgeo:plus-status>" << std::endl;
  }
  return ss.str();
}

std::string TopoFeature::get_citygml_attributes(std::vector<std::tuple<std::string, OGRFieldType, std::string>> attributes) {
  std::stringstream ss;
  for (auto& attribute : attributes) {
    // add attributes except gml_id
    if (std::get<0>(attribute).compare("gml_id") != 0) {
      std::string type;
      switch (std::get<1>(attribute)) {
      case OFTInteger:
        type = "int";
      case OFTReal:
        type = "double";
      case OFTDate:
        type = "date";
      default:
        type = "string";
      }
      ss << "<gen:" + type + "Attribute name=\"" + std::get<0>(attribute) + "\">" << std::endl;
      ss << "<gen:value>" + std::get<2>(attribute) + "</gen:value>" << std::endl;
      ss << "</gen:" + type << "Attribute>" << std::endl;
    }
  }
  return ss.str();
}

std::string TopoFeature::get_wkt() {
  //  std::string wkt;
  //  wkt = "MULTIPOLYGONZ (";
  //
  //  for (auto& t: _triangles) {
  //    char* buf = new char[200];
  //    Point3 p1 = _vertices[t.v0];
  //    Point3 p2 = _vertices[t.v1];
  //    Point3 p3 = _vertices[t.v2];
  //    std::sprintf(buf, "((%.3f %.3f %.3f,%.3f %.3f %.3f,%.3f %.3f %.3f,%.3f %.3f %.3f)),",
  //      p1.get<0>(), p1.get<1>(), p1.get<2>(),
  //      p2.get<0>(), p2.get<1>(), p2.get<2>(),
  //      p3.get<0>(), p3.get<1>(), p3.get<2>(),
  //      p1.get<0>(), p1.get<1>(), p1.get<2>());
  //    wkt += std::string(buf);
  //  }
  //
  //  for (auto& t: _triangles_vw) {
  //    char* buf = new char[200];
  //    Point3 p1 = *t.v0;
  //    Point3 p2 = *t.v1;
  //    Point3 p3 = *t.v2;
  //    std::sprintf(buf, "((%.3f %.3f %.3f,%.3f %.3f %.3f,%.3f %.3f %.3f,%.3f %.3f %.3f)),",
  //      p1.get<0>(), p1.get<1>(), p1.get<2>(),
  //      p2.get<0>(), p2.get<1>(), p2.get<2>(),
  //      p3.get<0>(), p3.get<1>(), p3.get<2>(),
  //      p1.get<0>(), p1.get<1>(), p1.get<2>());
  //    wkt += std::string(buf);
  //  }
  //  wkt.pop_back();
  //  wkt += ")";
  //  return wkt;
  return "";
}

bool TopoFeature::get_shape_features(OGRLayer* layer, std::string className) {
    OGRFeatureDefn *featureDefn = layer->GetLayerDefn();
    OGRFeature *feature = OGRFeature::CreateFeature(featureDefn);
    OGRMultiPolygon multipolygon = OGRMultiPolygon();
    Point3 p;
  
    //-- add all triangles to the layer
    for (auto& t: _triangles) {
      OGRPolygon polygon = OGRPolygon();
      OGRLinearRing ring = OGRLinearRing();
      
      p = _vertices[t.v0];
      ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
      p = _vertices[t.v1];
      ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
      p = _vertices[t.v2];
      ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
  
      ring.closeRings();
      polygon.addRing(&ring);
      multipolygon.addGeometry(&polygon);
    }
  
    //-- add all vertical wall triangles to the layer
    for (auto& t: _triangles_vw) {
      OGRPolygon polygon = OGRPolygon();
      OGRLinearRing ring = OGRLinearRing();
  
      p = _vertices_vw[t.v0];
      ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
      p = _vertices_vw[t.v1];
      ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
      p = _vertices_vw[t.v2];
      ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
  
      ring.closeRings();
      polygon.addRing(&ring);
      multipolygon.addGeometry(&polygon);
    }
  
    feature->SetGeometry(&multipolygon);
    feature->SetField("Id", this->get_id().c_str());
    feature->SetField("Class", className.c_str());
  
    if (layer->CreateFeature(feature) != OGRERR_NONE)
    {
      std::cerr << "Failed to create feature " << this->get_id() << " in shapefile." << std::endl;
      return false;
    }
    OGRFeature::DestroyFeature(feature);
  return true;
}

void TopoFeature::fix_bowtie() {
  //-- gather all rings
  std::vector<Ring2> therings;
  therings.push_back(bg::exterior_ring(*(_p2)));
  for (auto& iring : bg::interior_rings(*(_p2)))
    therings.push_back(iring);

  //-- process each vertex of the polygon separately
  std::vector<int> anc, bnc;
  Point2 a, b;
  TopoFeature* fadj;
  int ringi = -1;
  for (auto& ring : therings) {
    ringi++;
    for (int ai = 0; ai < ring.size(); ai++) {
      //-- Point a
      a = ring[ai];
      //-- find Point b
      int bi;
      if (ai == (ring.size() - 1)) {
        b = ring.front();
        bi = 0;
      }
      else {
        b = ring[ai + 1];
        bi = ai + 1;
      }
      //-- find the adjacent polygon to segment ab (fadj)
      fadj = nullptr;
      int adj_a_ringi = 0;
      int adj_a_pi = 0;
      int adj_b_ringi = 0;
      int adj_b_pi = 0;
      for (auto& adj : *(_adjFeatures)) {
        if (adj->has_segment(b, a, adj_b_ringi, adj_b_pi, adj_a_ringi, adj_a_pi) == true) {
          // if (adj->has_segment(b, a) == true) {
          fadj = adj;
          break;
        }
      }
      if (fadj == nullptr)
        continue;
      //-- check height differences: f > fadj for *both* Points a and b
      int az = this->get_vertex_elevation(ringi, ai);
      int bz = this->get_vertex_elevation(ringi, bi);
      int fadj_az = fadj->get_vertex_elevation(adj_a_ringi, adj_a_pi);
      int fadj_bz = fadj->get_vertex_elevation(adj_b_ringi, adj_b_pi);

      //-- Fix bow-ties
      if (((az > fadj_az) && (bz < fadj_bz)) || ((az < fadj_az) && (bz > fadj_bz))) {
        //std::clog << "BOWTIE:" << this->get_id() << " & " << fadj->get_id() << std::endl;
        //std::clog << this->get_class() << " & " << fadj->get_class() << std::endl;
        if (this->is_hard() && fadj->is_hard() == false) {
          //- this is hard, snap the smallest height of the soft feature to this
          if (abs(az - fadj_az) < abs(bz - fadj_bz)) {
            fadj->set_vertex_elevation(adj_a_ringi, adj_a_pi, az);
          }
          else {
            fadj->set_vertex_elevation(adj_b_ringi, adj_b_pi, bz);
          }
        }
        else if (this->is_hard() == false && fadj->is_hard()) {
          //- this is soft, snap the smallest height to the hard feature
          if (abs(az - fadj_az) < abs(bz - fadj_bz)) {
            this->set_vertex_elevation(ringi, ai, fadj_az);
          }
          else {
            this->set_vertex_elevation(ringi, bi, fadj_bz);
          }
        }
        else {
          if (abs(az - fadj_az) < abs(bz - fadj_bz)) {
            //- snap a to lowest
            if (az < fadj_az) {
              fadj->set_vertex_elevation(adj_a_ringi, adj_a_pi, az);
            }
            else
            {
              this->set_vertex_elevation(ringi, ai, fadj_az);
            }
          }
          else {
            //- snap b to lowest
            if (bz < fadj_bz) {
              fadj->set_vertex_elevation(adj_b_ringi, adj_b_pi, bz);
            }
            else {
              this->set_vertex_elevation(ringi, bi, fadj_bz);
            }
          }
        }
      }
    }
  }
}

void TopoFeature::construct_vertical_walls(std::unordered_map<std::string, std::vector<int>> &nc, int baseheight) {
  //std::clog << this->get_id() << std::endl;
  // if (this->get_id() == "bbdc52a89-00b3-11e6-b420-2bdcc4ab5d7f")
  //   std::clog << "break" << std::endl;

  if (this->has_vertical_walls() == false)
    return;

  //-- gather all rings
  std::vector<Ring2> therings;
  therings.push_back(bg::exterior_ring(*(_p2)));
  for (auto& iring : bg::interior_rings(*(_p2)))
    therings.push_back(iring);

  //-- process each vertex of the polygon separately
  std::vector<int> anc, bnc;
  std::unordered_map<std::string, std::vector<int>>::const_iterator ncit;
  Point2 a, b;
  TopoFeature* fadj;
  int ringi = -1;
  //if (this->get_id() == "107720546")
  //  std::clog << "yo" << std::endl;
  for (auto& ring : therings) {
    ringi++;
    for (int ai = 0; ai < ring.size(); ai++) {
      //-- Point a
      a = ring[ai];
      //-- find Point b
      int bi;
      if (ai == (ring.size() - 1)) {
        b = ring.front();
        bi = 0;
      }
      else {
        b = ring[ai + 1];
        bi = ai + 1;
      }
      //-- check if there's a nc for either
      ncit = nc.find(gen_key_bucket(&a));
      if (ncit != nc.end())
        anc = ncit->second;
      ncit = nc.find(gen_key_bucket(&b));
      if (ncit != nc.end())
        bnc = ncit->second;

      if ((anc.empty() == true) && (bnc.empty() == true))
        continue;

      //-- find the adjacent polygon to segment ab (fadj)
      fadj = nullptr;
      int adj_a_ringi = 0;
      int adj_a_pi = 0;
      int adj_b_ringi = 0;
      int adj_b_pi = 0;
      for (auto& adj : *(_adjFeatures)) {
        if (adj->has_segment(b, a, adj_b_ringi, adj_b_pi, adj_a_ringi, adj_a_pi) == true) {
          fadj = adj;
          break;
        }
      }
      if (fadj == nullptr && this->get_class() != BUILDING) {
        continue;
      }

      int az = this->get_vertex_elevation(ringi, ai);
      int bz = this->get_vertex_elevation(ringi, bi);
      int fadj_az, fadj_bz;
      if(fadj == nullptr) {
        fadj_az = baseheight;
        fadj_bz = baseheight;
      }
      else {
        fadj_az = fadj->get_vertex_elevation(adj_a_ringi, adj_a_pi);
        fadj_bz = fadj->get_vertex_elevation(adj_b_ringi, adj_b_pi);
      }

      //-- check height differences: f > fadj for *both* Points a and b
      if ((az < fadj_az) || (bz < fadj_bz))
        continue;
      if ((az == fadj_az) && (bz == fadj_bz))
        continue;

      //std::clog << "az: " << az << std::endl;
      //std::clog << "bz: " << bz << std::endl;
      //std::clog << "fadj_az: " << fadj_az << std::endl;
      //std::clog << "fadj_bz: " << fadj_bz << std::endl;

      //-- find the height of the vertex in the node column
      std::vector<int>::const_iterator sait, eait, sbit, ebit;
      sait = std::find(anc.begin(), anc.end(), fadj_az);
      eait = std::find(anc.begin(), anc.end(), az);
      sbit = std::find(bnc.begin(), bnc.end(), fadj_bz);
      ebit = std::find(bnc.begin(), bnc.end(), bz);

      //-- iterate to triangulate
      while ((sbit != ebit) && (sbit != bnc.end()) && ((sbit + 1) != bnc.end())) {
        if (anc.size() == 0 || sait == anc.end())
          _vertices_vw.push_back(Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(az)));
        else
          _vertices_vw.push_back(Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait)));
        _vertices_vw.push_back(Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*sbit)));
        sbit++;
        _vertices_vw.push_back(Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*sbit)));
        Triangle t;
        int size = int(_vertices_vw.size());
        t.v0 = size - 2;
        t.v1 = size - 3;
        t.v2 = size - 1;
        _triangles_vw.push_back(t);
      }
      while (sait != eait && sait != anc.end() && (sait + 1) != anc.end()) {
        if (bnc.size() == 0 || ebit == bnc.end())
          _vertices_vw.push_back(Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(bz)));
        else
          _vertices_vw.push_back(Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*ebit)));
        _vertices_vw.push_back(Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait)));
        sait++;
        _vertices_vw.push_back(Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait)));
        Triangle t;
        int size = int(_vertices_vw.size());
        t.v0 = size - 3;
        t.v1 = size - 2;
        t.v2 = size - 1;
        _triangles_vw.push_back(t);
      }
    }
  }
}

bool TopoFeature::has_segment(Point2& a, Point2& b, int& aringi, int& api, int& bringi, int& bpi) {
  double threshold = 0.001;
  std::vector<int> ringis, pis;
  Point2 tmp;
  if (this->has_point2_(a, ringis, pis) == true) {
    for (int k = 0; k < ringis.size(); k++) {
      // nextpi = pis[k];
      int nextpi;
      tmp = this->get_next_point2_in_ring(ringis[k], pis[k], nextpi);
      if (distance(b, tmp) <= threshold) {
        aringi = ringis[k];
        api = pis[k];
        bringi = ringis[k];
        bpi = nextpi;
        return true;
      }
    }
  }
  return false;
}

float TopoFeature::get_distance_to_boundaries(Point2& p) {
  //-- collect the rings of the polygon
  std::vector<Ring2> therings;
  therings.push_back(bg::exterior_ring(*(_p2)));
  for (auto& iring : bg::interior_rings(*(_p2)))
    therings.push_back(iring);
  //-- process each vertex of the polygon separately
  Point2 a, b;
  Segment2 s;
  int ringi = -1;
  double dmin = 99999;
  for (auto& ring : therings) {
    ringi++;
    for (int ai = 0; ai < ring.size(); ai++) {
      a = ring[ai];
      if (ai == (ring.size() - 1))
        b = ring.front();
      else
        b = ring[ai + 1];
      //-- calculate distance
      bg::set<0, 0>(s, bg::get<0>(a));
      bg::set<0, 1>(s, bg::get<1>(a));
      bg::set<1, 0>(s, bg::get<0>(b));
      bg::set<1, 1>(s, bg::get<1>(b));
      double d = bg::distance(p, s);
      if (d < dmin)
        dmin = d;
    }
  }
  return dmin;
}

bool TopoFeature::has_point2_(const Point2& p, std::vector<int>& ringis, std::vector<int>& pis) {
  double threshold = 0.001;
  Ring2 oring = bg::exterior_ring(*_p2);
  int ringi = 0;
  bool re = false;
  for (int i = 0; i < oring.size(); i++) {
    if (distance(p, oring[i]) <= threshold) {
      ringis.push_back(ringi);
      pis.push_back(i);
      re = true;
      break;
    }
  }
  ringi++;
  auto irings = bg::interior_rings(*_p2);
  for (Ring2& iring : irings) {
    for (int i = 0; i < iring.size(); i++) {
      if (distance(p, iring[i]) <= threshold) {
        ringis.push_back(ringi);
        pis.push_back(i);
        re = true;
        break;
      }
    }
    ringi++;
  }
  return re;
}

Point2 TopoFeature::get_point2(int ringi, int pi) {
  Ring2 ring;
  if (ringi == 0)
    ring = _p2->outer();
  else
    ring = _p2->inners()[ringi - 1];
  return ring[pi];
}

Point2 TopoFeature::get_next_point2_in_ring(int ringi, int i, int& pi) {
  Ring2 ring;
  if (ringi == 0)
    ring = _p2->outer();
  else
    ring = _p2->inners()[ringi - 1];

  if (i == (ring.size() - 1)) {
    pi = 0;
    return ring.front();
  }
  else {
    pi = i + 1;
    return ring[pi];
  }
}

bool TopoFeature::has_vertical_walls() {
  return _bVerticalWalls;
}

void TopoFeature::add_vertical_wall() {
  _bVerticalWalls = true;
}

int TopoFeature::get_vertex_elevation(int ringi, int pi) {
  return _p2z[ringi][pi];
}

int TopoFeature::get_vertex_elevation(Point2& p) {
  std::vector<int> ringis, pis;
  has_point2_(p, ringis, pis);
  return _p2z[ringis[0]][pis[0]];
}

void TopoFeature::set_vertex_elevation(int ringi, int pi, int z) {
  _p2z[ringi][pi] = z;
}

//-- used to collect all points linked to the polygon
//-- later all these values are used to lift the polygon (and put values in _p2z)
bool TopoFeature::assign_elevation_to_vertex(Point2 &p, double z, float radius) {
  int zcm = int(z * 100);
  int ringi = 0;
  Ring2 oring = bg::exterior_ring(*(_p2));
  for (int i = 0; i < oring.size(); i++) {
    if (distance(p, oring[i]) <= radius)
      (_lidarelevs[ringi][i]).push_back(zcm);
  }
  ringi++;
  auto irings = bg::interior_rings(*(_p2));
  for (Ring2& iring : irings) {
    for (int i = 0; i < iring.size(); i++) {
      if (distance(p, iring[i]) <= radius) {
        (_lidarelevs[ringi][i]).push_back(zcm);
      }
    }
    ringi++;
  }
  return true;
}

double TopoFeature::distance(const Point2 &p1, const Point2 &p2) {
  return sqrt((p1.x() - p2.x())*(p1.x() - p2.x()) + (p1.y() - p2.y())*(p1.y() - p2.y()));
}

bool TopoFeature::within_range(Point2 &p, Polygon2 &poly, double radius) {
  Ring2 oring = bg::exterior_ring(poly);
  //-- point is within range of the polygon rings
  for (int i = 0; i < oring.size(); i++) {
    if (distance(p, oring[i]) <= radius) {
      return true;
    }
  }
  auto irings = bg::interior_rings(*(_p2));
  for (Ring2& iring : irings) {
    for (int i = 0; i < iring.size(); i++) {
      if (distance(p, iring[i]) <= radius) {
        return true;
      }
    }
  }
  //-- point is within the polygon
  if (point_in_polygon(p, poly)) {
    return true;
  }
  return false;
}

// based on http://stackoverflow.com/questions/217578/how-can-i-determine-whether-a-2d-point-is-within-a-polygon/2922778#2922778
bool TopoFeature::point_in_polygon(Point2 &p, Polygon2 &poly) {
  //test outer ring
  Ring2 oring = bg::exterior_ring(poly);
  int nvert = oring.size();
  int i, j = 0;
  bool insideOuter = false;
  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    double py = p.y();
    if (((oring[i].y()>py) != (oring[j].y()>py)) &&
      (p.x() < (oring[j].x() - oring[i].x()) * (py - oring[i].y()) / (oring[j].y() - oring[i].y()) + oring[i].x()))
      insideOuter = !insideOuter;
  }
  if (insideOuter) {
    //test inner rings
    auto irings = bg::interior_rings(poly);
    for (Ring2& iring : irings) {
      bool insideInner = false;
      int nvert = iring.size();
      int i, j = 0;
      for (i = 0, j = nvert - 1; i < nvert; j = i++) {
        double py = p.y();
        if (((iring[i].y() > py) != (iring[j].y() > py)) &&
          (p.x() < (iring[j].x() - iring[i].x()) * (py - iring[i].y()) / (iring[j].y() - iring[i].y()) + iring[i].x()))
          insideInner = !insideInner;
      }
      if (insideInner) {
        return false;
      }
    }
  }
  return insideOuter;
}

std::string TopoFeature::get_triangle_as_gml_surfacemember(Triangle& t, bool verticalwall) {
  std::stringstream ss;
  ss << std::setprecision(3) << std::fixed;
  ss << "<gml:surfaceMember>" << std::endl;
  ss << "<gml:Polygon>" << std::endl;
  ss << "<gml:exterior>" << std::endl;
  ss << "<gml:LinearRing>" << std::endl;
  if (verticalwall == false) {
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v0]) << " " << bg::get<1>(_vertices[t.v0]) << " " << bg::get<2>(_vertices[t.v0]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v1]) << " " << bg::get<1>(_vertices[t.v1]) << " " << bg::get<2>(_vertices[t.v1]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v2]) << " " << bg::get<1>(_vertices[t.v2]) << " " << bg::get<2>(_vertices[t.v2]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v0]) << " " << bg::get<1>(_vertices[t.v0]) << " " << bg::get<2>(_vertices[t.v0]) << "</gml:pos>" << std::endl;
  }
  else {
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v0]) << " " << bg::get<1>(_vertices_vw[t.v0]) << " " << bg::get<2>(_vertices_vw[t.v0]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v1]) << " " << bg::get<1>(_vertices_vw[t.v1]) << " " << bg::get<2>(_vertices_vw[t.v1]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v2]) << " " << bg::get<1>(_vertices_vw[t.v2]) << " " << bg::get<2>(_vertices_vw[t.v2]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v0]) << " " << bg::get<1>(_vertices_vw[t.v0]) << " " << bg::get<2>(_vertices_vw[t.v0]) << "</gml:pos>" << std::endl;
  }
  ss << "</gml:LinearRing>" << std::endl;
  ss << "</gml:exterior>" << std::endl;
  ss << "</gml:Polygon>" << std::endl;
  ss << "</gml:surfaceMember>" << std::endl;
  return ss.str();
}

std::string TopoFeature::get_triangle_as_gml_triangle(Triangle& t, bool verticalwall) {
  std::stringstream ss;
  ss << std::setprecision(3) << std::fixed;
  ss << "<gml:Triangle>" << std::endl;
  ss << "<gml:exterior>" << std::endl;
  ss << "<gml:LinearRing>" << std::endl;
  if (verticalwall == false) {
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v0]) << " " << bg::get<1>(_vertices[t.v0]) << " " << bg::get<2>(_vertices[t.v0]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v1]) << " " << bg::get<1>(_vertices[t.v1]) << " " << bg::get<2>(_vertices[t.v1]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v2]) << " " << bg::get<1>(_vertices[t.v2]) << " " << bg::get<2>(_vertices[t.v2]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices[t.v0]) << " " << bg::get<1>(_vertices[t.v0]) << " " << bg::get<2>(_vertices[t.v0]) << "</gml:pos>" << std::endl;
  }
  else {
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v0]) << " " << bg::get<1>(_vertices_vw[t.v0]) << " " << bg::get<2>(_vertices_vw[t.v0]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v1]) << " " << bg::get<1>(_vertices_vw[t.v1]) << " " << bg::get<2>(_vertices_vw[t.v1]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v2]) << " " << bg::get<1>(_vertices_vw[t.v2]) << " " << bg::get<2>(_vertices_vw[t.v2]) << "</gml:pos>" << std::endl;
    ss << "<gml:pos>" << bg::get<0>(_vertices_vw[t.v0]) << " " << bg::get<1>(_vertices_vw[t.v0]) << " " << bg::get<2>(_vertices_vw[t.v0]) << "</gml:pos>" << std::endl;
  }
  ss << "</gml:LinearRing>" << std::endl;
  ss << "</gml:exterior>" << std::endl;
  ss << "</gml:Triangle>" << std::endl;
  return ss.str();
}

bool TopoFeature::get_attribute(std::string attributeName, std::string &attribute, std::string defaultValue)
{
  for (auto& at : _attributes) {
    if (std::get<0>(at).compare(attributeName) == 0) {
      if (!std::get<2>(at).empty()) {
        attribute = std::get<2>(at);
      }
      else if (!defaultValue.empty()) {
        attribute = defaultValue;
      }
      return true;
    }
  }
  // Return false if attribute does not exist
  return false;
}

void TopoFeature::lift_all_boundary_vertices_same_height(int height) {
  int ringi = 0;
  Ring2 oring = bg::exterior_ring(*(_p2));
  for (int i = 0; i < oring.size(); i++)
    _p2z[ringi][i] = height;
  ringi++;
  auto irings = bg::interior_rings(*(_p2));
  for (Ring2& iring : irings) {
    for (int i = 0; i < iring.size(); i++)
      _p2z[ringi][i] = height;
    ringi++;
  }
}

void TopoFeature::add_adjacent_feature(TopoFeature* adjFeature) {
  _adjFeatures->push_back(adjFeature);
}

std::vector<TopoFeature*>* TopoFeature::get_adjacent_features() {
  return _adjFeatures;
}

void TopoFeature::lift_each_boundary_vertices(float percentile) {
  //-- 1. assign value for each vertex based on percentile
  int ringi = 0;
  Ring2 oring = bg::exterior_ring(*(_p2));
  for (int i = 0; i < oring.size(); i++) {
    std::vector<int> &l = _lidarelevs[ringi][i];
    if (l.empty() == true)
      _p2z[ringi][i] = -9999;
    else {
      std::nth_element(l.begin(), l.begin() + (l.size() * percentile), l.end());
      _p2z[ringi][i] = l[l.size() * percentile];
    }
  }
  ringi++;
  auto irings = bg::interior_rings(*(_p2));
  for (Ring2& iring : irings) {
    for (int i = 0; i < iring.size(); i++) {
      std::vector<int> &l = _lidarelevs[ringi][i];
      if (l.empty() == true)
        _p2z[ringi][i] = -9999;
      else {
        std::nth_element(l.begin(), l.begin() + (l.size() * percentile), l.end());
        _p2z[ringi][i] = l[l.size() * percentile];
      }
    }
    ringi++;
  }
  //-- 2. find average height of the polygon
  double totalheight = 0.0;
  int heightcount = 0;
  oring = bg::exterior_ring(*(_p2));
  for (int i = 0; i < oring.size(); i++) {
    if (_p2z[0][i] != -9999) {
      totalheight += double(_p2z[0][i]);
      heightcount += 1;
    }
  }
  int avgheight;
  if (heightcount > 0)
    avgheight = int(totalheight / double(heightcount));
  else
    avgheight = 0;
  // std::clog << "avg height: " << avgheight << std::endl;
  // std::clog << "height count " << heightcount << std::endl;

  //-- 3. some vertices will have no values (no lidar point within tolerance thus)
  //--    assign them the avg
  ringi = 0;
  oring = bg::exterior_ring(*(_p2));
  for (int i = 0; i < oring.size(); i++) {
    if (_p2z[ringi][i] == -9999)
      _p2z[ringi][i] = avgheight;
  }
  ringi++;
  irings = bg::interior_rings(*(_p2));
  for (Ring2& iring : irings) {
    for (int i = 0; i < iring.size(); i++) {
      if (_p2z[ringi][i] == -9999)
        _p2z[ringi][i] = avgheight;
    }
    ringi++;
  }
}

//-------------------------------
//-------------------------------

Flat::Flat(char *wkt, std::string layername, std::vector<std::tuple<std::string, OGRFieldType, std::string>> attributes, std::string pid)
  : TopoFeature(wkt, layername, attributes, pid) {}

int Flat::get_number_vertices() {
  // return int(2 * _vertices.size());
  return (int(_vertices.size()) + int(_vertices_vw.size()));
}

bool Flat::add_elevation_point(Point2 &p, double z, float radius, LAS14Class lasclass, bool lastreturn) {
  if (within_range(p, *(_p2), radius)) {
    int zcm = int(z * 100);
    //-- 1. assign to polygon since within the threshold value (buffering of polygon)
    _zvaluesinside.push_back(zcm);
  }
  return true;
}

int Flat::get_height() {
  return get_vertex_elevation(0, 0);
}

bool Flat::lift_percentile(float percentile) {
  int z = 0;
  if (_zvaluesinside.empty() == false) {
    std::nth_element(_zvaluesinside.begin(), _zvaluesinside.begin() + (_zvaluesinside.size() * percentile), _zvaluesinside.end());
    z = _zvaluesinside[_zvaluesinside.size() * percentile];
  }
  this->lift_all_boundary_vertices_same_height(z);
  _zvaluesinside.clear();
  _zvaluesinside.shrink_to_fit();
  return true;
}

//-------------------------------
//-------------------------------

Boundary3D::Boundary3D(char *wkt, std::string layername, std::vector<std::tuple<std::string, OGRFieldType, std::string>> attributes, std::string pid)
  : TopoFeature(wkt, layername, attributes, pid) {}

int Boundary3D::get_number_vertices() {
  return (int(_vertices.size()) + int(_vertices_vw.size()));
}

bool Boundary3D::add_elevation_point(Point2 &p, double z, float radius, LAS14Class lasclass, bool lastreturn) {
  // no need for checking for point-in-polygon since only points in range of the vertices are added
  assign_elevation_to_vertex(p, z, radius);
  return true;
}

void Boundary3D::smooth_boundary(int passes) {
  std::vector<int> tmp;
  for (int p = 0; p < passes; p++) {
    for (auto& r : _p2z) {
      tmp.resize(r.size());
      tmp.front() = int((r[1] + r.back()) / 2);
      auto it = r.end();
      it -= 2;
      tmp.back() = int((r.front() + *it) / 2);
      for (int i = 1; i < (r.size() - 1); i++)
        tmp[i] = int((r[i - 1] + r[i + 1]) / 2);
    }
  }
}

// void Boundary3D::smooth_boundary(int passes) {
//   for (int p = 0; p < passes; p++) {
//     int ringi = 0;
//     Ring2 oring = bg::exterior_ring(*(_p2));
//     std::vector<int> elevs(bg::num_points(oring));
//     smooth_ring(_p2z[ringi], elevs);
//     for (int i = 0; i < oring.size(); i++) 
//       _p2z[ringi][i] = elevs[i];
//     ringi++;
//     auto irings = bg::interior_rings(*_p2);
//     for (Ring2& iring: irings) {
//       elevs.resize(bg::num_points(iring));
//       smooth_ring(_p2z[ringi], elevs);
//       for (int i = 0; i < iring.size(); i++) 
//         _p2z[ringi][i] = elevs[i];
//       ringi++;
//     }
//   }
// }

// void Boundary3D::smooth_ring(const std::vector<int> &r, std::vector<int> &elevs) {
//   elevs.front() = (bg::get<2>(r[1]) + bg::get<2>(r.back())) / 2;
//   auto it = r.end();
//   it -= 2;
//   elevs.back() = (bg::get<2>(r.front()) + bg::get<2>(*it)) / 2;
//   for (int i = 1; i < (r.size() - 1); i++) 
//     elevs[i] = (bg::get<2>(r[i - 1]) + bg::get<2>(r[i + 1])) / 2;
// }

//-------------------------------
//-------------------------------

TIN::TIN(char *wkt, std::string layername, std::vector<std::tuple<std::string, OGRFieldType, std::string>> attributes, std::string pid, int simplification, float innerbuffer)
  : TopoFeature(wkt, layername, attributes, pid) {
  _simplification = simplification;
  _innerbuffer = innerbuffer;
}

int TIN::get_number_vertices() {
  return (int(_vertices.size()) + int(_vertices_vw.size()));
}

bool TIN::add_elevation_point(Point2 &p, double z, float radius, LAS14Class lasclass, bool lastreturn) {
  bool toadd = false;
  // no need for checking for point-in-polygon since only points in range of the vertices are added
  assign_elevation_to_vertex(p, z, radius);
  if (_simplification <= 1)
    toadd = true;
  else {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(1, _simplification);
    if (dis(gen) == 1)
      toadd = true;
  }
  // Add the point to the lidar points if it is within the polygon and respecting the inner buffer size
  if (toadd && point_in_polygon(p, *(_p2)) && (_innerbuffer == 0.0 || (within_range(p, *(_p2), _innerbuffer) && this->get_distance_to_boundaries(p) > _innerbuffer))) {
    _lidarpts.push_back(Point3(p.x(), p.y(), z));
  }
  return toadd;
}

bool TIN::buildCDT() {
  getCDT(_p2, _p2z, _vertices, _triangles, _lidarpts);
  return true;
}
