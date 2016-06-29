/*
    Authors:
        Pavel Březina <pbrezina@redhat.com>

    Copyright (C) 2016 Red Hat

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <popt.h>
#include <stdio.h>

#include "util/util.h"
#include "tools/common/sss_tools.h"
#include "tools/sssctl/sssctl.h"
#include "sbus/sssd_dbus.h"
#include "responder/ifp/ifp_iface.h"

errno_t sssctl_domain_list(struct sss_cmdline *cmdline,
                           struct sss_tool_ctx *tool_ctx,
                           void *pvt)
{
    sss_sifp_ctx *sifp;
    sss_sifp_error error;
    char **domains;
    int start = 0;
    errno_t ret;
    int i;

    /* Parse command line. */
    struct poptOption options[] = {
        {"start", 's', POPT_ARG_NONE, &start, 0, _("Start SSSD if it is not running"), NULL },
        POPT_TABLEEND
    };

    ret = sss_tool_popt(cmdline, options, SSS_TOOL_OPT_OPTIONAL, NULL, NULL);
    if (ret != EOK) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Unable to parse command arguments\n");
        return ret;
    }

    if (!sssctl_start_sssd(start)) {
        return ERR_SSSD_NOT_RUNNING;
    }

    error = sssctl_sifp_init(tool_ctx, &sifp);
    if (error != SSS_SIFP_OK) {
        sssctl_sifp_error(sifp, error, "Unable to connect to the InfoPipe");
        return EFAULT;
    }

    error = sss_sifp_list_domains(sifp, &domains);
    if (error != SSS_SIFP_OK) {
        sssctl_sifp_error(sifp, error, "Unable to get domains list");
        return EIO;
    }

    for (i = 0; domains[i] != NULL; i++) {
        puts(domains[i]);
    }

    return EOK;
}

static errno_t sssctl_domain_status_online(struct sss_tool_ctx *tool_ctx,
                                           sss_sifp_ctx *sifp,
                                           const char *domain_path)
{
    TALLOC_CTX *tmp_ctx;
    sss_sifp_error error;
    DBusMessage *reply;
    bool is_online;
    errno_t ret;

    tmp_ctx = talloc_new(NULL);
    if (tmp_ctx == NULL) {
        DEBUG(SSSDBG_CRIT_FAILURE, "talloc_new() failed\n");
        return ENOMEM;
    }

    error = sssctl_sifp_send(tmp_ctx, sifp, &reply, domain_path,
                             IFACE_IFP_DOMAINS_DOMAIN,
                             IFACE_IFP_DOMAINS_DOMAIN_ISONLINE);
    if (error != SSS_SIFP_OK) {
        sssctl_sifp_error(sifp, error, "Unable to get online status");
        ret = EIO;
        goto done;
    }

    ret = sbus_parse_reply(reply, DBUS_TYPE_BOOLEAN, &is_online);
    if (ret != EOK) {
        goto done;
    }

    printf(_("Online status: %s\n"), is_online ? _("Online") : _("Offline"));

    ret = EOK;

done:
    talloc_free(tmp_ctx);
    return ret;
}

struct sssctl_domain_status_opts {
    const char *domain;
    int online;
    int last;
    int active;
    int servers;
    int force_start;
};

errno_t sssctl_domain_status(struct sss_cmdline *cmdline,
                             struct sss_tool_ctx *tool_ctx,
                             void *pvt)
{
    struct sssctl_domain_status_opts opts = {0};
    sss_sifp_ctx *sifp;
    sss_sifp_error error;
    const char *path;
    bool opt_set;
    errno_t ret;

    /* Parse command line. */
    struct poptOption options[] = {
        {"online", 'o', POPT_ARG_NONE , &opts.online, 0, _("Show online status"), NULL },
        /*
        {"last-requests", 'l', POPT_ARG_NONE, &opts.last, 0, _("Show last requests that went to data provider"), NULL },
        {"active-server", 'a', POPT_ARG_NONE, &opts.active, 0, _("Show information about active server"), NULL },
        {"servers", 'r', POPT_ARG_NONE, &opts.servers, 0, _("Show list of discovered servers"), NULL },
        */
        {"start", 's', POPT_ARG_NONE, &opts.force_start, 0, _("Start SSSD if it is not running"), NULL },
        POPT_TABLEEND
    };

    ret = sss_tool_popt_ex(cmdline, options, SSS_TOOL_OPT_OPTIONAL,
                           NULL, NULL, "DOMAIN", _("Specify domain name."),
                           &opts.domain, &opt_set);
    if (ret != EOK) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Unable to parse command arguments\n");
        return ret;
    }

    if (opt_set == false) {
        opts.online = true;
        opts.last = true;
        opts.active = true;
        opts.servers = true;
    }

    path = sbus_opath_compose(tool_ctx, IFP_PATH_DOMAINS, opts.domain);
    if (path == NULL) {
        printf(_("Out of memory!\n"));
        return ENOMEM;
    }

    if (!sssctl_start_sssd(opts.force_start)) {
        return ERR_SSSD_NOT_RUNNING;
    }

    error = sssctl_sifp_init(tool_ctx, &sifp);
    if (error != SSS_SIFP_OK) {
        sssctl_sifp_error(sifp, error, "Unable to connect to the InfoPipe");
        return EFAULT;
    }

    ret = sssctl_domain_status_online(tool_ctx, sifp, path);
    if (ret != EOK) {
        fprintf(stderr, _("Unable to get online status\n"));
        return ret;
    }

    return EOK;
}
