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

#include <exploragram/hexdom/mesh_inspector.h>
#include <exploragram/hexdom/basic.h>
#include <exploragram/hexdom/extra_connectivity.h>
#include <geogram/mesh/mesh_tetrahedralize.h>
#include <geogram/delaunay/delaunay.h>


namespace GEO {

    bool volume_boundary_is_manifold(Mesh* m, std::string& msg) {
        m->cells.compute_borders();
        if (!surface_is_manifold(m, msg)) {
            return false;
        }
        m->facets.clear();
        return true;
    }


    bool have_negative_tet_volume(Mesh*m) {
        FOR(c, m->cells.nb()) {
            vec3 A = X(m)[m->cells.vertex(c, 0)];
            vec3 B = X(m)[m->cells.vertex(c, 1)];
            vec3 C = X(m)[m->cells.vertex(c, 2)];
            vec3 D = X(m)[m->cells.vertex(c, 3)];
            double vol = dot(cross(B - A, C - A), D - A);
            if (vol < 0) {
                Attribute<double> signed_volume(m->cells.attributes(), "signed_volume");
                signed_volume[c] = vol;
                return true;
            }
        }
        return false;
    }

    bool surface_is_tetgenifiable(Mesh* m) {
        Mesh copy;
        copy.copy(*m);
        create_non_manifold_facet_adjacence(&copy);
        copy.facets.triangulate();
        try {
            mesh_tetrahedralize(copy, false, false, 1.);
        }
        catch (const GEO::Delaunay::InvalidInput& error_report) {
            FOR(i, error_report.invalid_facets.size()) {
                plop(error_report.invalid_facets[i]);
            }
            return false;
        }
        return true;
    }

    bool volume_is_tetgenifiable(Mesh* m) {
        Mesh copy;
        copy.copy(*m);
        copy.edges.clear();
        copy.cells.compute_borders();
        copy.cells.clear();
        return surface_is_tetgenifiable(&copy);
    }
    bool surface_is_manifold(Mesh* m, std::string& msg) {
        if (m->facets.nb() == 0) return true;
        {
            // check for duplicated corners around a face
            FOR(f, m->facets.nb()) FOR(fc, m->facets.nb_corners(f))
                if (m->facets.vertex(f, fc) == m->facets.vertex(f, next_mod(fc, m->facets.nb_corners(f)))) {
                    msg = "duplicated corner detected on  (face = " + String::to_string(f) + " , local corner = " + String::to_string(fc) + " , vertex = " +
                        String::to_string(m->facets.vertex(f, fc));
                    return false;
                }
            // output the type of surface
            index_t nb_edges_par_facets = m->facets.nb_corners(0);
            FOR(f, m->facets.nb()) if (m->facets.nb_corners(f) != nb_edges_par_facets) nb_edges_par_facets = index_t(-1);
            if (nb_edges_par_facets != index_t(-1)) plop(nb_edges_par_facets);

            // check if the mesh is manifold

            Attribute<int> nb_opp(m->facet_corners.attributes(), "nb_opp");
            Attribute<int> nb_occ(m->facet_corners.attributes(), "nb_occ");
            FOR(h, m->facet_corners.nb()) { nb_opp[h] = 0; nb_occ[h] = 0; }

            // edge connectivity
            FacetsExtraConnectivity fec(m);
            int nb_0_opp = 0;
            //int nb_1_opp = 0;
            int nb_multiple_opp = 0;
            int nb_duplicated_edge = 0;
            FOR(h, m->facet_corners.nb()) {
                index_t cir = h;
                index_t result = NOT_AN_ID; // not found
                do {
                    index_t candidate = fec.prev(cir);
                    if ((fec.org(candidate) == fec.dest(h)) && (fec.dest(candidate) == fec.org(h))) {
                        nb_opp[h]++;
                        if (result == NOT_AN_ID) result = candidate;
                        else nb_multiple_opp++;
                    }
                    if (cir != h && fec.dest(h) == fec.dest(cir)) {
                        nb_duplicated_edge++;
                        nb_occ[h]++;
                    }
                    cir = fec.c2c[cir];
                } while (cir != h);
                if (result == NOT_AN_ID)nb_0_opp++;
                //else nb_1_opp++;
            }


            if (nb_0_opp > 0) {
                msg = "surface have halfedges without opposite, nb= " + String::to_string(nb_0_opp);
                return false;
            }
            if (nb_multiple_opp > 0) {
                msg = "surface have halfedge with more than 2 opposites, nb= " + String::to_string(nb_multiple_opp);
                return false;
            }
            if (nb_duplicated_edge > 0) {
                msg = "halfedge appears in more than one facet, nb= " + String::to_string(nb_duplicated_edge);
                return false;
            }

            // check for non manifold vertices
            Attribute<bool> nonmanifold(m->vertices.attributes(), "nonmanifold");
            FOR(v, m->vertices.nb()) nonmanifold[v] = false;

            FOR(h, m->facet_corners.nb()) {
                if (nb_opp[h] != 1 || nb_occ[h] != 0)
                    nonmanifold[fec.org(h)] = true;
            }
            vector<int> val(m->vertices.nb(), 0);
            FOR(f, m->facets.nb()) FOR(lc, m->facets.nb_vertices(f)) val[m->facets.vertex(f, lc)]++;
            FOR(h, m->facet_corners.nb()) {
                int nb = 0;
                index_t cir = h;
                do {
                    nb++;
                    cir = fec.next_around_vertex(cir);// fec.c2c[cir];
                } while (cir != h);
                if (nb != val[fec.org(h)]) {
                    msg = "Vertex " + String::to_string(fec.org(h)) + " is non manifold ";
                    return false;
                }
            }
        }
        m->vertices.attributes().delete_attribute_store("nonmanifold");
        m->facet_corners.attributes().delete_attribute_store("nb_opp");
        m->facet_corners.attributes().delete_attribute_store("nb_occ");
        return true;
    }
    void get_facet_stats(Mesh* m, const char * msg, bool export_attribs) {
        geo_argused(export_attribs);
        GEO::Logger::out("HexDom")  << "-----------------------------------------" <<  std::endl;
        GEO::Logger::out("HexDom")  << "get_facet_stats  " << msg <<  std::endl;
        GEO::Logger::out("HexDom")  << "-----------------------------------------" <<  std::endl;

        {
            Attribute<int> nb_opp(m->facet_corners.attributes(), "nb_opp");
            Attribute<int> nb_occ(m->facet_corners.attributes(), "nb_occ");
            FOR(h, m->facet_corners.nb()) nb_opp[h] = 0;

            // edge connectivity
            FacetsExtraConnectivity fec(m);
            int nb_0_opp = 0;
            int nb_1_opp = 0;
            int nb_multiple_opp = 0;
            int nb_duplicated_edge = 0;
            FOR(h, m->facet_corners.nb()) {
                index_t cir = h;
                index_t result = NOT_AN_ID; // not found
                do {
                    index_t candidate = fec.prev(cir);
                    if ((fec.org(candidate) == fec.dest(h)) && (fec.dest(candidate) == fec.org(h))) {
                        nb_opp[h]++;
                        if (result == NOT_AN_ID) result = candidate;
                        else nb_multiple_opp++;
                    }
                    if (cir != h && fec.dest(h) == fec.dest(cir)) {
                        nb_duplicated_edge++;
                        nb_occ[h]++;
                    }
                    cir = fec.c2c[cir];
                } while (cir != h);
                if (result == NOT_AN_ID)nb_1_opp++;
                else nb_0_opp++;
            }



            FOR(f, m->facets.nb()) FOR(fc, m->facets.nb_corners(f))
                if (m->facets.vertex(f, fc) == m->facets.vertex(f, next_mod(fc, m->facets.nb_corners(f))))
                    GEO::Logger::out("HexDom")  << "Duplicated vertex found at facet #" << f << ", local corner= " << fc << " and vertex is " << m->facets.vertex(f, fc) <<  std::endl;

            plop(nb_0_opp);
            plop(nb_1_opp);
            plop(nb_multiple_opp);
            plop(nb_duplicated_edge);

            // check for non manifold vertices
            Attribute<bool> nonmanifold(m->vertices.attributes(), "nonmanifold");
            FOR(v, m->vertices.nb()) nonmanifold[v] = false;

            FOR(h, m->facet_corners.nb()) {
                if (nb_opp[h] != 1 || nb_occ[h] != 0)
                    nonmanifold[fec.org(h)] = true;

            }

            vector<int> val(m->vertices.nb(), 0);
            FOR(f, m->facets.nb()) FOR(lc, m->facets.nb_vertices(f)) val[m->facets.vertex(f, lc)]++;
            FOR(h, m->facet_corners.nb()) {
                int nb = 0;
                index_t cir = h;
                do {
                    nb++;
                    cir = fec.c2c[cir];
                } while (cir != h);
                if (nb != val[fec.org(h)]) {
                    GEO::Logger::out("HexDom")  << "Vertex " << fec.org(h) << " is non-manifold !!" <<  std::endl;
                    nonmanifold[fec.org(h)] = true;
                }
            }
        }
        m->vertices.attributes().delete_attribute_store("nonmanifold");
        m->facet_corners.attributes().delete_attribute_store("nb_opp");
        m->facet_corners.attributes().delete_attribute_store("nb_occ");
    }
    double tet_vol(vec3 A, vec3 B, vec3 C, vec3 D) {
        B = B - A;
        C = C - A;
        D = D - A;
        return (1. / 6.)*dot(D, cross(B, C));
    }
    void get_hex_proportion(Mesh*m, double &nb_hex_prop, double &vol_hex_prop) {
        int nb_tets = 0;
        int nb_hexs = 0;
        double vol_tets = 0;
        double vol_hexs = 0;
        FOR(c, m->cells.nb()) if (m->cells.nb_facets(c) == 4) {
            vol_tets += tet_vol(X(m)[m->cells.vertex(c, 0)], X(m)[m->cells.vertex(c, 1)], X(m)[m->cells.vertex(c, 2)], X(m)[m->cells.vertex(c, 3)]);
            nb_tets++;
        }
        FOR(c, m->cells.nb()) if (m->cells.nb_facets(c) == 6) {
            vector<vec3> P(8);
            FOR(cv, 8) P[cv] = X(m)[m->cells.vertex(c, cv)];

            vol_hexs += tet_vol(P[0], P[3], P[2], P[6]);
            vol_hexs += tet_vol(P[0], P[7], P[3], P[6]);
            vol_hexs += tet_vol(P[0], P[7], P[6], P[4]);

            vol_hexs += tet_vol(P[0], P[1], P[3], P[7]);
            vol_hexs += tet_vol(P[0], P[1], P[7], P[5]);
            vol_hexs += tet_vol(P[0], P[5], P[7], P[4]);
            nb_hexs++;
        }
        if (nb_hexs + nb_tets>0) nb_hex_prop = double(nb_hexs) / double(nb_hexs + nb_tets);
        if (nb_hexs + nb_tets>0) vol_hex_prop = double(vol_hexs) / double(vol_hexs + vol_tets);

    }
}
