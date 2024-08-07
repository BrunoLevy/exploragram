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

#ifndef H_HEXDOM_ALGO_FRAME_H
#define H_HEXDOM_ALGO_FRAME_H

#include <exploragram/basic/common.h>
#include <exploragram/hexdom/basic.h>
#include <exploragram/hexdom/spherical_harmonics_l4.h> // to compute average_frame(vector<mat3>& data) and representative_frame
#include <cmath>


namespace GEO {

    /***
     *               ___     __        __     __
     *     |\/|  /\   |     |__)  /\  /__` | /  `
     *     |  | /~~\  |     |__) /~~\ .__/ | \__,
     *
     */

    inline bool is_identity_auvp(const mat3& m, double eps = 1e-15) {
        // [BL] TODO: test extra-diagonal elements ? (can be a big cow without that !!!)
        return std::abs(m(0, 0) - 1.) < eps && std::abs(m(1, 1) - 1.) < eps && std::abs(m(2, 2) - 1.) < eps;
    }

    mat3 EXPLORAGRAM_API normalize_columns(const mat3& B);
    mat3 EXPLORAGRAM_API invert_columns_norm(const mat3& B);

    /***
     *               ___     ___            ___  __      __   __  ___
     *     |\/|  /\   |     |__  |  | |    |__  |__)    |__) /  \  |
     *     |  | /~~\  |     |___ \__/ |___ |___ |  \    |  \ \__/  |
     *
     */

    mat3 EXPLORAGRAM_API rotx(double angle);
    mat3 EXPLORAGRAM_API roty(double angle);
    mat3 EXPLORAGRAM_API rotz(double angle);

    // non optimized version is  "return rotz(xyz[2]) *roty(xyz[1]) *rotx(xyz[0]);"
    mat3 euler_to_mat3(vec3 xyz);

    //http://www.staff.city.ac.uk/~sbbh653/publications/euler.pdf
    vec3 mat3_to_euler(const mat3& r);

    /***
     *                __      __   ___  __             ___      ___    __
     *     /\  \_/ | /__`    |__) |__  |__)  |\/| |  |  |   /\   |  | /  \ |\ |
     *    /~~\ / \ | .__/    |    |___ |  \  |  | \__/  |  /~~\  |  | \__/ | \|
     *
     */


    struct EXPLORAGRAM_API AxisPermutation {
        AxisPermutation(index_t id=0) {  mid = id; }
        void aligns_B_wrt_ref(mat3 ref, mat3 B);
        void make_col2_equal_to_z(mat3 B, vec3 z);
        const mat3& get_mat() const;
        bool is_identity() { return mid == 0; }
        double operator()(index_t i, index_t j) {return get_mat()(i, j); }
        AxisPermutation inverse();

        index_t mid;
    };

    inline vec3i operator*(const AxisPermutation& p, const vec3i& v) { return p.get_mat()*v; }
    inline vec3 operator*(const AxisPermutation& p, const vec3& v) { return p.get_mat()*v; }

    /***
     *     ___  __              ___
     *    |__  |__)  /\   |\/| |__
     *    |    |  \ /~~\  |  | |___
     *
     */

    struct EXPLORAGRAM_API Frame {
        mat3& r;
        Frame(mat3& M) :r(M) {}

        inline mat3 apply_permutation(AxisPermutation& ap) {return r*ap.get_mat();}

        void make_z_equal_to(vec3 z);

        static mat3 average_frame(vector<mat3>& data);
        static mat3 representative_frame(vector<vec3>& bunch_of_vectors, vector<double>& w);
        static mat3 representative_frame(vector<vec3>& bunch_of_vectors);
    };

    /***
     *     ___  ___    ___  __   __        __
     *    |__  |__      |  /  \ /  \ |    /__`
     *    |    |        |  \__/ \__/ |___ .__/
     *
     */

    AxisPermutation EXPLORAGRAM_API Rij(Mesh* m, Attribute<mat3>& B, index_t i, index_t j);

    bool EXPLORAGRAM_API triangle_is_frame_singular(Mesh* m, Attribute<mat3>& B, index_t c, index_t cf);

    bool triangle_is_frame_singular___give_stable_direction(Mesh* m, int& stable_dir_index, Attribute<mat3>& B, index_t c, index_t cf, index_t cfv=0);
    bool triangle_is_frame_singular___give_stable_direction(Mesh* m, vec3& stable_dir_geom, Attribute<mat3>& B, index_t c, index_t cf, index_t cfv=0);
}

#endif
