/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   macros.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/04 02:56:41 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/10/17 16:45:17 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _MACROS_H
#define _MACROS_H

/*******************************************************************************
 *                                 IDENTIFIERS                                 *
 ******************************************************************************/

#define AND &&
#define OR ||
#define NOT !
#define NOTEQUALS !=
#define EQUALS ==
#define IS =

/*******************************************************************************
 *                                    LOOPS                                    *
 ******************************************************************************/

#define FOREVER for (;;)
#define RANGE(i, y, x) for (i = (y); (((x) >= (y)) ? (i <= (x)) : (i >= x)); \
                            (((x) >= (y)) ? ((i)++) : ((i)--)))
#define FOREACH(i, A)                         \
    for (int _keep = 1,                       \
             _count = 0,                      \
             _size = sizeof(A) / sizeof *(A); \
         _keep && _count != _size;            \
         _keep = !_keep, _count++)            \
        for (i = (A) + _count; _keep; _keep = !_keep)

/*******************************************************************************
 *                                   ARRAYS                                    *
 ******************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define SET(d, n, v)                               \
    do                                             \
    {                                              \
        size_t i_, n_;                             \
        for (n_ = (n), i_ = 0; n_ > 0; --n_, ++i_) \
            (d)[i_] = (v);                         \
    } while (0)
#define ZERO(d, n) SET(d, n, 0)
#define COLUMNS(S, E) ((E) - (S) + 1)
#define IS_ARRAY(a) ((void *)&a == (void *)a)

#endif /* _MACROS_H */