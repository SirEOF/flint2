/*=============================================================================

    This file is part of FLINT.

    FLINT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLINT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLINT; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

=============================================================================*/
/******************************************************************************

    Copyright (C) 2011 William Hart
    Copyright (C) 2012 Sebastian Pancratz

******************************************************************************/

#include <stdlib.h>
#include "fmpz_vec.h"
#include "fmpz_mod_poly.h"

long _fmpz_mod_poly_xgcd_euclidean(fmpz *G, fmpz *S, fmpz *T, 
                                   const fmpz *A, long lenA, 
                                   const fmpz *B, long lenB, 
                                   const fmpz_t invB, const fmpz_t p)
{
    _fmpz_vec_zero(G, lenB);
    _fmpz_vec_zero(S, lenB-1);
    _fmpz_vec_zero(T, lenA-1);

    if (lenB == 1)
    {
        fmpz_set(G + 0, B + 0);
        fmpz_one(T + 0);
        return 1;
    }
    else
    {
        fmpz *Q, *R;
        long lenQ, lenR;

        Q = _fmpz_vec_init(2 * lenA);
        R = Q + lenA;

        _fmpz_mod_poly_divrem(Q, R, A, lenA, B, lenB, invB, p);
        lenR = lenB - 1;
        FMPZ_VEC_NORM(R, lenR);

        if (lenR == 0)
        {
            _fmpz_vec_set(G, B, lenB);
            fmpz_one(T + 0);

            _fmpz_vec_clear(Q, 2 * lenA);
            return lenB;
        }
        else
        {
            fmpz_t inv;
            fmpz *D, *U, *V1, *V3, *W;
            long lenD, lenU, lenV1, lenV3, lenW;

            fmpz_init(inv);
            W  = _fmpz_vec_init(FLINT_MAX(5 * lenB, lenA + lenB));
            D  = W  + lenB;
            U  = D  + lenB;
            V1 = U  + lenB;
            V3 = V1 + lenB;

            lenU = 0;
            _fmpz_vec_set(D, B, lenB);
            lenD = lenB;
            fmpz_one(V1 + 0);
            lenV1 = 1;
            lenV3 = 0;
            FMPZ_VEC_SWAP(V3, lenV3, R, lenR);

            do {
                fmpz_invmod(inv, V3 + (lenV3 - 1), p);
                _fmpz_mod_poly_divrem(Q, R, D, lenD, V3, lenV3, inv, p);
                lenQ = lenD - lenV3 + 1;
                lenR = lenV3 - 1;
                FMPZ_VEC_NORM(R, lenR);

                if (lenV1 >= lenQ)
                    _fmpz_mod_poly_mul(W, V1, lenV1, Q, lenQ, p);
                else
                    _fmpz_mod_poly_mul(W, Q, lenQ, V1, lenV1, p);
                lenW = lenQ + lenV1 - 1;

                _fmpz_mod_poly_sub(U, U, lenU, W, lenW, p);
                lenU = FLINT_MAX(lenU, lenW);
                FMPZ_VEC_NORM(U, lenU);

                FMPZ_VEC_SWAP(U, lenU, V1, lenV1);
                {
                    fmpz *__t;
                    long __tn;

                    __t = D;
                    D   = V3;
                    V3  = R;
                    R   = __t;
                    __tn  = lenD;
                    lenD  = lenV3;
                    lenV3 = lenR;
                    lenR  = __tn;
                }

            } while (lenV3 != 0);

            _fmpz_vec_set(G, D, lenD);
            _fmpz_vec_set(S, U, lenU);
            {
                lenQ = lenA + lenU - 1;

                _fmpz_mod_poly_mul(Q, A, lenA, S, lenU, p);
                _fmpz_mod_poly_neg(Q, Q, lenQ, p);
                _fmpz_mod_poly_add(Q, G, lenD, Q, lenQ, p);

                _fmpz_mod_poly_divrem(T, W, Q, lenQ, B, lenB, invB, p);
            }

            _fmpz_vec_clear(W, FLINT_MAX(5 * lenB, lenA + lenB));
            _fmpz_vec_clear(Q, 2 * lenA);
            fmpz_clear(inv);

            return lenD;
        }
    }
}

void 
fmpz_mod_poly_xgcd_euclidean(fmpz_mod_poly_t G, 
                             fmpz_mod_poly_t S, fmpz_mod_poly_t T,
                             const fmpz_mod_poly_t A, const fmpz_mod_poly_t B)
{
    const long lenA = A->length, lenB = B->length;
    fmpz_t inv;

    fmpz_init(inv);
    if (lenA == 0)
    {
        if (lenB == 0) 
        {
            fmpz_mod_poly_zero(G);
            fmpz_mod_poly_zero(S);
            fmpz_mod_poly_zero(T);
        }
        else 
        {
            fmpz_invmod(inv, fmpz_mod_poly_lead(B), &B->p);
            fmpz_mod_poly_scalar_mul_fmpz(G, B, inv);
            fmpz_mod_poly_zero(S);
            fmpz_mod_poly_set_fmpz(T, inv);
        }
    } 
    else if (lenB == 0)
    {
        fmpz_invmod(inv, fmpz_mod_poly_lead(A), &A->p);
        fmpz_mod_poly_scalar_mul_fmpz(G, A, inv);
        fmpz_mod_poly_zero(T);
        fmpz_mod_poly_set_fmpz(S, inv);
    }
    else
    {
        fmpz *g, *s, *t;
        long lenG;

        if (G == A || G == B)
        {
            g = _fmpz_vec_init(FLINT_MIN(lenA, lenB));
        }
        else
        {
            fmpz_mod_poly_fit_length(G, FLINT_MIN(lenA, lenB));
            g = G->coeffs;
        }
        if (S == A || S == B)
        {
            s = _fmpz_vec_init(lenB - 1);
        }
        else
        {
            fmpz_mod_poly_fit_length(S, lenB - 1);
            s = S->coeffs;
        }
        if (T == A || T == B)
        {
            t = _fmpz_vec_init(lenA - 1);
        }
        else
        {
            fmpz_mod_poly_fit_length(T, lenA - 1);
            t = T->coeffs;
        }

        if (lenA >= lenB)
        {
            fmpz_invmod(inv, fmpz_mod_poly_lead(B), &B->p);
            lenG = _fmpz_mod_poly_xgcd_euclidean(g, s, t, 
                A->coeffs, lenA, B->coeffs, lenB, inv, &B->p);
        }
        else
        {
            fmpz_invmod(inv, fmpz_mod_poly_lead(A), &A->p);
            lenG = _fmpz_mod_poly_xgcd_euclidean(g, t, s, 
                B->coeffs, lenB, A->coeffs, lenA, inv, &A->p);
        }

        if (G == A || G == B)
        {
            _fmpz_vec_clear(G->coeffs, G->alloc);
            G->coeffs = g;
            G->alloc  = FLINT_MIN(lenA, lenB);
        }
        if (S == A || S == B)
        {
            _fmpz_vec_clear(S->coeffs, S->alloc);
            S->coeffs = s;
            S->alloc  = lenB - 1;
        }
        if (T == A || T == B)
        {
            _fmpz_vec_clear(T->coeffs, T->alloc);
            T->coeffs = t;
            T->alloc  = lenA - 1;
        }

        _fmpz_mod_poly_set_length(G, lenG);
        _fmpz_mod_poly_set_length(S, lenB - lenG);
        _fmpz_mod_poly_set_length(T, lenA - lenG);
        _fmpz_mod_poly_normalise(S);
        _fmpz_mod_poly_normalise(T);

        if (!fmpz_is_one(fmpz_mod_poly_lead(G)))
        {
            fmpz_invmod(inv, fmpz_mod_poly_lead(G), &A->p);
            fmpz_mod_poly_scalar_mul_fmpz(G, G, inv);
            fmpz_mod_poly_scalar_mul_fmpz(S, S, inv);
            fmpz_mod_poly_scalar_mul_fmpz(T, T, inv);
        }
    }
    fmpz_clear(inv);
}

