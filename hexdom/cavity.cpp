/*
 *  Copyright (c) 2000-2022 Inria
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  * Neither the name of the ALICE Project-Team nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Contact: Bruno Levy
 *
 *     https://www.inria.fr/fr/bruno-levy
 *
 *     Inria,
 *     Domaine de Voluceau,
 *     78150 Le Chesnay - Rocquencourt
 *     FRANCE
 *
 */

#include <exploragram/hexdom/cavity.h>
#include <exploragram/hexdom/hex.h>
#include <exploragram/hexdom/PGP.h>
#include <exploragram/hexdom/basic.h>
#include <exploragram/hexdom/extra_connectivity.h>
#include <geogram/numerics/matrix_util.h>
#include <geogram/basic/permutation.h>
#include <algorithm>
#include <geogram/mesh/triangle_intersection.h>
#include <geogram/mesh/mesh_tetrahedralize.h>
#include <geogram/delaunay/delaunay.h>

#include <geogram/points/nn_search.h>
#include <geogram/points/colocate.h>
#include <queue>

#include <exploragram/hexdom/mesh_inspector.h>
#include <exploragram/hexdom/intersect_tools.h>
#include <exploragram/hexdom/polygon.h>

namespace GEO {






    void merge_hex_boundary_and_quadtri(Mesh* hex, Mesh* quad) {
        if (hex->cells.nb() == 0) return;
        double eps = 1e-3*get_cell_average_edge_size(hex);

        // add hex surface to quadt
        index_t off_v = quad->vertices.create_vertices(hex->vertices.nb());
        FOR(v, hex->vertices.nb()) X(quad)[off_v + v] = X(hex)[v];

        index_t off_f = quad->facets.create_facets(hex->facets.nb(), 4);
        FOR(f, hex->facets.nb()) FOR(fc, 4)
            quad->facets.set_vertex(off_f + f, (3 - fc), off_v + hex->facets.vertex(f, fc));


        // merge vertices from hex faces to quatri vertices

        {
            vector<index_t> to_kill(quad->vertices.nb(), 0);
            vector<index_t> old2new(quad->vertices.nb());
            FOR(v, quad->vertices.nb()) old2new[v] = v;

            NearestNeighborSearch_var NN = NearestNeighborSearch::create(3);
            NN->set_points(off_v, quad->vertices.point_ptr(0));

            for (index_t v = off_v; v < quad->vertices.nb(); v++) {
                index_t nearest = NN->get_nearest_neighbor(X(quad)[v].data());
                if ((X(quad)[v] - X(quad)[nearest]).length2() == 0) {
                    old2new[v] = nearest;
                }
            }

            FOR(f, quad->facets.nb()) FOR(fv, quad->facets.nb_vertices(f))
                quad->facets.set_vertex(f, fv, old2new[quad->facets.vertex(f, fv)]);
            FOR(v, quad->vertices.nb()) if (old2new[v] != v) to_kill[v] = NOT_AN_ID;
            quad->vertices.delete_elements(to_kill);
        }




        // remove useless quads
        vector<vec3> f_bary(quad->facets.nb(), vec3(0, 0, 0));

        FOR(f, quad->facets.nb()) FOR(fc, quad->facets.nb_vertices(f))
            f_bary[f] = f_bary[f] + (1. / quad->facets.nb_vertices(f))*quad->vertices.point(quad->facets.vertex(f, fc));

        vector<index_t> bary_old2new(quad->facets.nb());
        Geom::colocate((double*)(f_bary.data()), 3, quad->facets.nb(), bary_old2new, eps);

        vector<index_t> to_kill(quad->facets.nb(), 0);
        FOR(f, quad->facets.nb()) if (bary_old2new[f] != f) {
            if (quad->facets.nb_vertices(f) != 4) continue;
            if (quad->facets.nb_vertices(bary_old2new[f]) != 4) continue;

            bool have_same_vertices = true;
            FOR(fv1, quad->facets.nb_vertices(f)) {
                bool isin = false;
                FOR(fv2, quad->facets.nb_vertices(bary_old2new[f])) {
                    isin = isin || quad->facets.vertex(f, fv1) == quad->facets.vertex(bary_old2new[f], fv2);
                }
                have_same_vertices = have_same_vertices && isin;
            }
            if (!have_same_vertices) continue;
            to_kill[f] = true;
            to_kill[bary_old2new[f]] = true;
        }
        quad->facets.delete_elements(to_kill);
        create_non_manifold_facet_adjacence(quad);


    }


}
