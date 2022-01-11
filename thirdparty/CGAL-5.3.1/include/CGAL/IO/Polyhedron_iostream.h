// Copyright (c) 1997  ETH Zurich (Switzerland).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v5.3.1/Polyhedron/include/CGAL/IO/Polyhedron_iostream.h $
// $Id: Polyhedron_iostream.h 0149977 2020-10-23T09:10:53+02:00 Maxime Gimeno
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Lutz Kettner  <kettner@mpi-sb.mpg.de>

#ifndef CGAL_IO_POLYHEDRON_IOSTREAM_H
#define CGAL_IO_POLYHEDRON_IOSTREAM_H

#include <CGAL/license/Polyhedron.h>

#include <CGAL/IO/Polyhedron_OFF_iostream.h>
#include <CGAL/IO/Polyhedron_VRML_1_ostream.h>
#include <CGAL/IO/Polyhedron_VRML_2_ostream.h>
#ifdef CGAL_USE_GEOMVIEW
#include <CGAL/IO/Polyhedron_geomview_ostream.h>
#endif

#include <CGAL/IO/Polyhedron_inventor_ostream.h>

#endif // CGAL_IO_POLYHEDRON_IOSTREAM_H
