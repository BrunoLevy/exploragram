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

#include <exploragram/hexdom/FF_visu.h>
#include <exploragram/hexdom/basic.h>
#include <exploragram/hexdom/mesh_utils.h>
#include <exploragram/hexdom/frame.h>
#include <exploragram/hexdom/spherical_harmonics_l4.h>
#include <exploragram/hexdom/sphere_model.h>  // a sphere for sh glyph

#include <cmath>

namespace GEO {

    void view_FF_with_gyphs(Mesh* m, Mesh* render, bool SH, double scale_in) {
        double scale = scale_in;

        if (!SH) {

            Attribute<double> orient(render->facets.attributes(), "orient");
            render->clear();
            Attribute<mat3> B;
            B.bind_if_is_defined(m->vertices.attributes(), "B");
            if (B.is_bound()) {
                FOR(v, m->vertices.nb()) {
                    vec3 dir[3];
                    FOR(i, 3)dir[i]= col(B[v], i);
                    for (int dim = 0; dim < 3; dim++) {
                        for (double sign = -1.; sign < 2.; sign += 2.) {
                            vec3 x = 0.5*scale * sign*dir[dim];
                            vec3 y = 0.5*scale * sign*dir[(dim + 1) % 3];
                            vec3 z = 0.5*scale * sign*dir[(dim + 2) % 3];
                            index_t off_v = render->vertices.create_vertices(4);
                            X(render)[off_v] = X(m)[v] + x - y - z;
                            X(render)[off_v + 1] = X(m)[v] + x + y - z;
                            X(render)[off_v + 2] = X(m)[v] + x + y + z;
                            X(render)[off_v + 3] = X(m)[v] + x - y + z;
                            index_t f = render->facets.create_quad(off_v + 0, off_v + 1, off_v + 2, off_v + 3);
                            orient[f] = dim * 2 + (sign + 1) / 2;  //sign * (dim + 1);
                        }
                    }
                }
            }
        }
        Attribute<SphericalHarmonicL4> sh;
        sh.bind_if_is_defined(m->vertices.attributes(), "sh");
        if (SH && sh.is_bound()) {
            if (m->cells.nb() > 0)
                scale *= get_cell_average_edge_size(m);
            else {
                double ave = 0;
                FOR(f, m->facets.nb())FOR(e, m->facets.nb_vertices(f))
                    ave += (X(m)[m->facets.vertex(f, e)] - X(m)[m->facets.vertex(f, (e + 1) % m->facets.nb_vertices(f))]).length();
                ave /= m->facet_corners.nb();
                scale *= ave;
            }
            plop(scale);
            Attribute<double> val(render->vertices.attributes(), "SH");

            FOR(v, m->vertices.nb()) {
                index_t off_v = render->vertices.create_vertices(SPHERE_MODEL_NB_PTS);
                FOR(vs, SPHERE_MODEL_NB_PTS) {
                    vec3 q = SPHERE_MODEL_PTS[vs];
                    val[off_v + vs] = .5 + (sh[v].value(q) - .5) / .64;
                    X(render)[off_v + vs] = X(m)[v] + scale * (3.+2. * (val[off_v + vs]))*q;
                }

                index_t off_f = render->facets.create_triangles(SPHERE_MODEL_NB_TRIANGLES);
                FOR(i, SPHERE_MODEL_NB_TRIANGLES)FOR(j, 3) {
                    render->facets.set_vertex(off_f + i, j, off_v + SPHERE_MODEL_TRIANGLES[i][j]);
                }
            }

        }
    }


    void view_U_locks(Mesh* m, Mesh* render, double scale_in,bool extractall) {
        double scale =scale_in;
        if (!m->vertices.attributes().is_defined("B")) return;
        if (!m->vertices.attributes().is_defined("lockU")) return;

        Attribute<mat3> B(m->vertices.attributes(), "B");
        Attribute<vec3> lockU(m->vertices.attributes(), "lockU");// how many dimensions are locked


        if (m->cells.nb() > 0)
            scale *= get_cell_average_edge_size(m);
        else {
            double ave = 0;
            FOR(f, m->facets.nb())FOR(e, m->facets.nb_vertices(f))
                ave += (X(m)[m->facets.vertex(f, e)] - X(m)[m->facets.vertex(f, (e + 1) % m->facets.nb_vertices(f))]).length();
            ave /= m->facet_corners.nb();
            scale *= ave;
        }

        Attribute<double> orient(render->edges.attributes(), "orient");
        render->clear();
        FOR(v, m->vertices.nb()) {
            vec3 dir[3];
            FOR(i, 3)dir[i] = col(B[v], i);

            int extracteddim = 0;
            for (index_t dim = 0; dim < 3; dim++) {
                if (lockU[v][dim] == 0 && !extractall) continue;
                extracteddim++;
                vec3 x = scale * dir[dim];
                index_t off_v = render->vertices.create_vertices(2);
                X(render)[off_v] = X(m)[v] + x ;
                X(render)[off_v + 1] = X(m)[v] - x ;
                index_t f = render->edges.create_edge(off_v + 0, off_v + 1);
                //orient[f] = dim;
                orient[f] = extracteddim;
            }
        }
    }


}
