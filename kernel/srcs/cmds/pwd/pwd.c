/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pwd.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/01/09 10:48:53 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/01/09 17:25:09 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cmds/pwd.h>
#include <filesystem/ext2/ext2.h>
#include <multitasking/process.h>

int pwd(int argc, char **argv) {

    if (argc > 1) {
        __WARND("pwd: too many arguments");
        return (1);
    } else {
        task_t *task = get_task(getpid());

        printk("%s\n", task->env.current_directory);
        return (0);
    }
}