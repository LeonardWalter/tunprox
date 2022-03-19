/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 2.0
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for more details governing rights and limitations under the License.
 *
 * The Original Code is part of the frlib.
 *
 * The Initial Developer of the Original Code is
 * Frank Reker <frank@reker.net>.
 * Portions created by the Initial Developer are Copyright (C) 2003-2014
 * by the Initial Developer. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _R__FRLIB_TOOLS_LIB_EDDI_H
#define _R__FRLIB_TOOLS_LIB_EDDI_H


#ifdef __cplusplus
extern "C" {
#endif


#define EDDITOOL_F_WAITANSWER	0x01
#define EDDITOOL_F_SENDANSWER	0x02
#define EDDITOOL_F_INSTCB		0x04
#define EDDITOOL_F_VERBOSE		0x08
#define EDDITOOL_F_SIGFATHER	0x10
#define EDDITOOL_F_TRIGFD		0x20



/* *********************
 * main functions
 * *********************/

int eddi_usage ();
int eddi_main (int argc, char **argv);


/* **********************
 * server functions
 * **********************/

int eddi_server (int flags);
void eddi_serverloop () __attribute((noreturn));
int eddi_serverinit ();
int eddi_serverpoll ();
int eddi_serverpollone ();



/* **************************
 * reader functions
 * **************************/

int eddi_readerinit (int flags);
int eddi_readerinstcb (int flags);
int eddi_readerpollloop (int flags);
int eddi_readerpoll ();
int eddi_readerpollone ();


/* **************************
 * writer function
 * **************************/

int eddi_writersend (char *ev, int flags);
int eddi_writersendvar (char *var, int type);


/* ********************************
 * direct writer function
 *   servers mainly for debugging
 * ********************************/

int eddi_dirsendev (const char *ev);


/* ********************************
 * direct reader functions
 *   servers mainly for debugging
 * ********************************/

int eddi_dirreadloop ();
int eddi_dirreadinit ();
int eddi_dirreadpoll ();
int eddi_dirreadpollone ();







#ifdef __cplusplus
}	/* extern "C" */
#endif







#endif	/* _R__FRLIB_TOOLS_LIB_EDDI_H */

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
