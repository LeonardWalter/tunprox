/*
 * Copyright (C) 2015-2021 by Frank Reker, Deutsche Telekom AG
 *
 * LDT - Lightweight (MP-)DCCP Tunnel kernel module
 *
 * This is not Open Source software. 
 * This work is made available to you under a source-available license, as 
 * detailed below.
 *
 * Copyright 2022 Deutsche Telekom AG
 *
 * Permission is hereby granted, free of charge, subject to below Commons 
 * Clause, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 * “Commons Clause” License Condition v1.0
 *
 * The Software is provided to you by the Licensor under the License, as
 * defined below, subject to the following condition.
 *
 * Without limiting other conditions in the License, the grant of rights under
 * the License will not include, and the License does not grant to you, the
 * right to Sell the Software.
 *
 * For purposes of the foregoing, “Sell” means practicing any or all of the
 * rights granted to you under the License to provide to third parties, for a
 * fee or other consideration (including without limitation fees for hosting 
 * or consulting/ support services related to the Software), a product or 
 * service whose value derives, entirely or substantially, from the
 * functionality of the Software. Any license notice or attribution required
 * by the License must also include this Commons Clause License Condition
 * notice.
 *
 * Licensor: Deutsche Telekom AG
 */

#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <net/tcp_states.h>
#include <linux/skbuff.h>
#include <linux/version.h>


#include "ldt_kernel.h"
#include "ldt_dev.h"
#include "ldt_tun.h"
#include "ldt_cfg.h"
#include "ldt_netlink.h"
#include "ldt_event.h"




int
ldt_event_send (net, evtype, iarg, sarg)
	struct net						*net;
	enum ldt_event_type		evtype;
	u32								iarg;
	const char						*sarg;
{
	return ldt_nl_send_event (net, (u32)evtype, iarg, sarg);
}


int
ldt_event_pong (net, data)
	struct net	*net;
	u32			data;
{
	return ldt_event_send (net, LDT_EVTYPE_PONG, data, NULL);
}




int
ldt_event_crsend2 (tun, evtype, evstype, desc)
	struct ldt_tun	*tun;
	int						evtype;
	const char				*evstype, *desc;
{
	char			_buf[256], *buf;
	int			ret, len;
	const char	*name;

	if (!tun || !evstype || evtype < 0) return -EINVAL;
	if (!desc) desc="";
	name = ((tun->tdev && tun->tdev->ndev) ?  tun->tdev->ndev->name : "");
	if (ldt_cfg_enable_debug && printk_ratelimit())
		printk ("ldt_event_crsend: event >>%s<<: %s (%s:%d)\n",
					evstype, desc, (*name?name:"<unknown>"), tun->id);
	if (TUNIFFUNC(tun,tp_createvent)) {
		len = ret = ldt_tun_createvent (tun, _buf, sizeof (_buf),
														evstype, desc);
		if (ret < 0) goto simplevent;
		len++;
		if (len > sizeof(_buf)) {
			buf = kmalloc (len+1, GFP_ATOMIC);
			if (!buf) goto simplevent;
			ret = ldt_tun_createvent (tun, buf, len, evstype, desc);
			if (ret < 0) {
				kfree (buf);
				goto simplevent;
			}
		} else {
			buf = _buf;
		}
	} else {
simplevent:
		snprintf (_buf, sizeof (_buf)-1, "<event type=\"%s\">\n"
					"  <desc>%s</desc>\n"
					"  <iface>%s</iface><tunid>%d</tunid>\n"
					"</event>\n", evstype, desc, name, (int)tun->id);
		_buf[sizeof(_buf)-1]=0;
		buf = _buf;
	}
	if (ldt_cfg_enable_debug >= 3 && printk_ratelimit())
		printk ("ldt event: %s\n", buf);
	ret = ldt_event_send (TUN2NET(tun), evtype, 0, buf);
	if (ret < 0 && printk_ratelimit())
		printk ("ldt_event_crsend(): error sending event: %d\n", ret);
	if (buf && buf != _buf) kfree (buf);

	return ret;
}


#define TP_EVKIND_GLOBAL		1
#define TP_EVKIND_TDEV			2
#define TP_EVKIND_TUNCONNECT	3
#define TP_EVKIND_TUNFAIL		4
struct evinfo {
	int			evtype;
	const char	*evstr;
	const char	*desc;
	int			kind;
};

struct evinfo	evlist[] = {
	{ LDT_EVTYPE_UNSPEC, "unspec", "not specified", TP_EVKIND_GLOBAL },
	{ LDT_EVTYPE_PONG, "pong", "ping response", TP_EVKIND_GLOBAL },
	{ LDT_EVTYPE_IFDOWN, "ifdown", "interface brought down", TP_EVKIND_TDEV },
	{ LDT_EVTYPE_IFUP, "ifup", "new interface brought up", TP_EVKIND_TDEV },
	{ LDT_EVTYPE_LDTDOWN, "tpdown", "ldt module unloaded", TP_EVKIND_GLOBAL },
	{ LDT_EVTYPE_REBIND, "rebind", "tunnel was rebound", TP_EVKIND_TUNCONNECT },
	{ LDT_EVTYPE_SUBFLOW_UP, "subflowup", "new subflow added", TP_EVKIND_TUNCONNECT },
	{ LDT_EVTYPE_SUBFLOW_DOWN, "subflowdown", "subflow removed", TP_EVKIND_TUNCONNECT },
	{ LDT_EVTYPE_CONN_ESTAB, "connestab", "connection established", TP_EVKIND_TUNCONNECT },
	{ LDT_EVTYPE_CONN_ESTAB_FAIL, "connestabfail", "connection establishment failed", TP_EVKIND_TUNFAIL },
	{ LDT_EVTYPE_CONN_ACCEPT, "connaccept", "connection accepted", TP_EVKIND_TUNCONNECT },
	{ LDT_EVTYPE_CONN_ACCEPT_FAIL, "connacceptfail", "connection failed to accept", TP_EVKIND_TUNFAIL },
	{ LDT_EVTYPE_CONN_LISTEN, "connlisten", "server listening", TP_EVKIND_TUNCONNECT },
	{ LDT_EVTYPE_CONN_LISTEN_FAIL, "connlistenfail", "failed to listen", TP_EVKIND_TUNFAIL },
	{ -1, "unknown", "unknown event", TP_EVKIND_GLOBAL }};
#define TP_EVLIST_SZ	26

int
ldt_event_crsend (evtype, dat, reason)
	int	evtype;
	void	*dat;
	int	reason;
{
	int				kind, ret;
	struct evinfo	*p=NULL;
	struct ldt_tun	*tun;
	struct ldt_dev	*tdev;
	char				buf[256], *buf2 = buf;
	struct net		*net;

	if (evtype < 0 || evtype > LDT_EVTYPE_MAX) return -ERANGE;
	if (evtype <= TP_EVLIST_SZ && evlist[evtype].evtype == evtype) {
		p = evlist + evtype;
	} else {
		for (p=evlist; p->evtype >= 0; p++) {
			if (p->evtype == evtype) break;
		}
	}
	kind = dat ? p->kind : TP_EVKIND_GLOBAL;
	switch (kind) {
	case TP_EVKIND_GLOBAL:
	default:
		snprintf (buf, sizeof (buf)-1, "<event type=\"%s\">\n"
					"  <desc>%s</desc>\n"
					"</event>\n", p->evstr, p->desc);
		buf[sizeof(buf)-1]=0;
		net=0;
		break;
	case TP_EVKIND_TDEV:
		tdev = (struct ldt_dev*)dat;
		snprintf (buf, sizeof (buf)-1, "<event type=\"%s\">\n"
					"  <desc>%s</desc>\n"
					"  <iface>%s</iface>\n"
					"</event>\n",
					p->evstr, p->desc, tdev->ndev->name);
		buf[sizeof(buf)-1]=0;
		net = TDEV2NET(tdev);
		break;
	case TP_EVKIND_TUNCONNECT:
		tun = (struct ldt_tun*)dat;
		net = TUN2NET(tun);
		return ldt_event_crsend2 (tun, evtype, p->evstr, p->desc);
	}
	if (ldt_cfg_enable_debug >= 3 && printk_ratelimit())
		printk ("ldt event: %s\n", buf2);
	ret = ldt_event_send (net, evtype, 0, buf2);
	if (ret < 0 && printk_ratelimit())
		printk ("ldt_event_crsend(): error sending event: %d\n", ret);
	if (buf2 != buf) kfree (buf2);
	return 0;
}








/*
 * Overrides for XEmacs and vim so that we get a uniform tabbing style.
 * XEmacs/vim will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-indent-level: 3
 * c-basic-offset: 3
 * tab-width: 3
 * End:
 * vim:tw=0:ts=3:wm=0:
 */
