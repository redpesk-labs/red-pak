#include <stdio.h>
#include <libgen.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include <rpm/rpmlog.h>
#include <rpm/rpmts.h>
#include <rpm/rpmts.h>
#include <rpm-plugins/rpmplugin.h>

static rpmRC redpak_scriptlet_pre(rpmPlugin plugin, const char *s_name,
                                     int type)
{
    int rc = chroot(".");
    (void)plugin;
    (void)s_name;
    (void)type;
    return rc < 0 ? RPMRC_FAIL : RPMRC_OK;
}

struct rpmPluginHooks_s redpak_hooks = {
    .scriptlet_pre = redpak_scriptlet_pre,
};
