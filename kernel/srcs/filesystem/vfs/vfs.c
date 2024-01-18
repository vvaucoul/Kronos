/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vfs.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/27 12:50:04 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/01/19 00:12:31 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <filesystem/vfs/vfs.h>

#include <filesystem/ext2/ext2.h>
#include <filesystem/initrd.h>

#include <memory/memory.h>

Hashtable *vfs_mounts = NULL;

// ! ||--------------------------------------------------------------------------------||
// ! ||                                       VFS                                      ||
// ! ||--------------------------------------------------------------------------------||

int vfs_init(void) {
    vfs_mounts = hashtable_create();
    if (vfs_mounts == NULL) {
        __THROW("VFS: Failed to init VFS", 1);
    }
    return (0);
}

Vfs *vfs_get_current_fs(void) {
    // Todo: return current fs
    return (hashtable_get(vfs_mounts, "ext2"));
}

Vfs *vfs_get_fs(const char *fs_name) {
    return (hashtable_get(vfs_mounts, fs_name));
}

Vfs *vfs_create_fs(VfsInfo *fs_info, VfsFsOps *fsops, VfsFileOps *fops, VfsNodeOps *nops) {
    Vfs *vfs = kmalloc(sizeof(Vfs));

    if (vfs == NULL) {
        __THROW("VFS: Failed to create VFS", NULL);
    } else {
        memset(vfs, 0, sizeof(Vfs));

        vfs->fs_info = fs_info;
        vfs->fs_root = NULL;

        vfs->use_vfs_cache = 0;
        vfs->vfs_cache = NULL;

        vfs->fsops = fsops;
        vfs->fops = fops;
        vfs->nops = nops;

        return (vfs);
    }
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                    VFS NODES                                   ||
// ! ||--------------------------------------------------------------------------------||

VfsNode *vfs_create_node(Vfs *vfs, VfsNode *root_node, const char *node_name) {
    VfsNode *node = vfs->nops->create_node(root_node, node_name);

    if (vfs->use_vfs_cache == 1) {
        VfsNode *cache_node = vfs_create_cache_link(vfs->vfs_cache, node);
    
        if ((vfs_add_node(vfs->vfs_cache, root_node, cache_node)) == NULL) {
            __WARN("VFS: Failed to add node to VFS Cache", NULL);
        }
        return (node);
    }
    return (node);
}

int vfs_delete_node(Vfs *vfs, VfsNode *node) {
    if (vfs->use_vfs_cache == 1) {
        int st = 0;

        if ((st = vfs_remove_node(vfs->vfs_cache, node)) != 0) {
            __WARN("VFS: Failed to remove node from VFS", 1);
        }
        if ((st += vfs_destroy_cache_link(vfs->vfs_cache, node)) != 0) {
            __WARN("VFS: Failed to destroy node from VFS Cache", 1);
        }
        return (st);
    }
    return (vfs->nops->remove_node(node));
}

void *vfs_custom_nops(Vfs *vfs, VfsNode *node, uint8_t nops_index, ...) {
    if (vfs->use_vfs_cache == 1) {
        // VfsANode *a_node = (VfsANode *)node;

        // va_list args;
        // int i = 0;
        // va_start(args, i);

        // void *result = vfs->nops->custom_ops[nops_index](a_node->data, args);

        // va_end(args);
        // return (result);
    }
    return (NULL);
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                              VFS NODES OPERATIONS                              ||
// ! ||--------------------------------------------------------------------------------||

Dirent *vfs_readdir(Vfs *vfs, VfsNode *node, uint32_t index) {
    if (vfs == NULL || vfs->fops == NULL || vfs->fops->readdir == NULL) {
        return (NULL);
    } else {
        return (vfs->fops->readdir(node, index));
    }
    return (NULL);
}

VfsNode *vfs_finddir(Vfs *vfs, VfsNode *node, const char *name) {
    if (vfs == NULL || vfs->fops == NULL || vfs->fops->finddir == NULL) {
        return (NULL);
    } else {
        return (vfs->fops->finddir(node, name));
    }
    return (NULL);
}

int vfs_opendir(VfsNode *node) {
    __UNUSED(node);
    return (0);
}

int vfs_closedir(VfsNode *node) {
    __UNUSED(node);
    return (0);
}

int vfs_open(VfsNode *node, uint32_t flags) {
    __UNUSED(node);
    __UNUSED(flags);
    return (0);
}

int vfs_close(VfsNode *node) {
    __UNUSED(node);
    return (0);
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                               VFS MOUNT / UNMOUNT                              ||
// ! ||--------------------------------------------------------------------------------||

/**
 * @brief Mount a filesystem
 *
 * @param vfs
 * @return int
 */
int vfs_mount(Vfs *vfs) {
    if (vfs == NULL || vfs->fsops == NULL || vfs->fsops->mount == NULL) {
        return (-1);
    } else {
        vfs->fsops->mount(vfs);
        hashtable_insert(vfs_mounts, vfs->fs_info->name, vfs);
    }
    return (0);
}

/**
 * @brief Unmount a filesystem
 *
 * @param vfs
 * @return int
 */
int vfs_unmount(Vfs *vfs) {
    if (vfs == NULL || vfs->fsops == NULL || vfs->fsops->unmount == NULL) {
        return (-1);
    } else {
        vfs->fsops->mount(vfs);
        hashtable_remove(vfs_mounts, vfs->fs_info->name);
    }
    return (0);
}