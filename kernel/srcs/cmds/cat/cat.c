/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cat.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/01/09 10:48:37 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/02/10 12:38:58 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <filesystem/vfs/vfs.h>

int cat(int argc, char **argv) {
    if (argc == 0) {
        __WARND("cat: no file specified");
        return (1);
    } else {
        //todo: print file content
    }
    return (0);
}