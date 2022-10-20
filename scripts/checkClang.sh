# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    checkClang.sh                                      :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2022/10/20 15:30:32 by vvaucoul          #+#    #+#              #
#    Updated: 2022/10/20 15:43:08 by vvaucoul         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/bin/sh

ls /bin/clang > /dev/null 2>&1
output=$?

if [ $output -ne 0 ]; then
    echo "false"
    exit 1
else
    echo "true"
    exit 0
fi