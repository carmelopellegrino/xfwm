/*      $Id$
 
        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.
 
        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.
 
        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
        xfwm4    - (c) 2002-2005 Olivier Fourdan
 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <libxfce4util/libxfce4util.h> 
#include "mypixmap.h"
#include "mywindow.h"
#include "screen.h"

#ifdef HAVE_RENDER
#include <X11/extensions/Xrender.h>
#endif

static void
xfwmWindowSetVisual (xfwmWindow * win, Visual *visual, gint depth)
{
    g_return_if_fail (win->screen_info != NULL);
    
    if (visual)
    {
        win->visual = visual;
    }
    else
    {
        win->visual = win->screen_info->visual;
    }
    
    if (depth)
    {
        win->depth = depth;
    }
    else
    {
        win->depth = win->screen_info->depth;
    }
}

void
xfwmWindowInit (xfwmWindow * win)
{
    g_return_if_fail (win != NULL);
    
    win->window = None;
    win->map = FALSE;
    win->screen_info = NULL;
    win->depth = 0;
    win->x = 0;
    win->y = 0;
    win->width = 1;
    win->height = 1;
}

void
xfwmWindowCreate (ScreenInfo * screen_info, Visual *visual, gint depth, Window parent,  
                  xfwmWindow * win, Cursor cursor)
{
    TRACE ("entering xfwmWindowCreate");

    g_return_if_fail (screen_info != NULL);
    
    win->window = XCreateSimpleWindow (myScreenGetXDisplay (screen_info), 
                                       parent, 0, 0, 1, 1, 0, 0, 0);
    TRACE ("Created XID 0x%lx", win->window);
    if (cursor != None)
    {
        XDefineCursor (myScreenGetXDisplay (screen_info), 
                       win->window, cursor);
    }
    win->map = FALSE;
    win->screen_info = screen_info;
    win->x = 0;
    win->y = 0;
    win->width = 1;
    win->height = 1;
    xfwmWindowSetVisual (win, visual, depth);
}

void
xfwmWindowDelete (xfwmWindow * win)
{
    TRACE ("entering xfwmWindowDelete");

    if (win->window != None)
    {
        XDestroyWindow (myScreenGetXDisplay (win->screen_info), 
                        win->window);
        win->window = None;
    }
    win->map = FALSE;
}

void
xfwmWindowShow (xfwmWindow * win, int x, int y, int width, int height,
    gboolean refresh)
{
    TRACE ("entering xfwmWindowShow");

    if (!(win->window))
    {
        return;
    }
    if ((width < 1) || (height < 1))
    {
        xfwmWindowHide (win);
        return;
    }
    if (!(win->map))
    {
        XMapWindow (myScreenGetXDisplay (win->screen_info), 
                    win->window);
        win->map = TRUE;
    }
    TRACE ("Showing XID 0x%lx", win->window);
    if (((x != win->x) || (y != win->y)) && ((width != win->width)
            || (height != win->height)))
    {
        XMoveResizeWindow (myScreenGetXDisplay (win->screen_info), 
                           win->window, x, y, 
                           (unsigned int) width,
                           (unsigned int) height);
        win->x = x;
        win->y = y;
        win->width = width;
        win->height = height;
    }
    else if ((x != win->x) || (y != win->y))
    {
        XMoveWindow (myScreenGetXDisplay (win->screen_info), 
                     win->window, 
                     x, y);
        if (refresh)
        {
            XClearWindow (myScreenGetXDisplay (win->screen_info), 
                          win->window);
        }
        win->x = x;
        win->y = y;
    }
    else if ((width != win->width) || (height != win->height))
    {
        XResizeWindow (myScreenGetXDisplay (win->screen_info), 
                       win->window, 
                       (unsigned int) width,
                       (unsigned int) height);
        win->width = width;
        win->height = height;
    }
    else if (refresh)
    {
        XClearWindow (myScreenGetXDisplay (win->screen_info), 
                      win->window);
    }
}

void
xfwmWindowHide (xfwmWindow * win)
{
    TRACE ("entering xfwmWindowHide");

    if (win->map)
    {
        g_assert (win->window);
        XUnmapWindow (myScreenGetXDisplay (win->screen_info), win->window);
        win->map = FALSE;
    }
}

gboolean
xfwmWindowVisible (xfwmWindow *win)
{
    g_return_val_if_fail (win, FALSE);
    
    return win->map;
}        

gboolean
xfwmWindowDeleted (xfwmWindow *win)
{
    g_return_val_if_fail (win, TRUE);

    return (win->window == None);
}        

void 
xfwmWindowTemp (ScreenInfo *screen_info, Visual *visual, 
                gint depth, Window parent,
                xfwmWindow * win, 
                int x, int y, int width, int height, 
                long eventmask)
{
    XSetWindowAttributes attributes;

    attributes.event_mask = eventmask;
    attributes.override_redirect = TRUE;
    win->window = XCreateWindow (myScreenGetXDisplay (screen_info), 
                                 parent, x, y, width, height, 0, 0, 
                                 InputOnly, CopyFromParent,
                                 CWEventMask | CWOverrideRedirect, 
                                 &attributes);
    XMapRaised (myScreenGetXDisplay (screen_info), win->window);
    XFlush (myScreenGetXDisplay (screen_info));

    win->map = TRUE;
    win->screen_info = screen_info;
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    xfwmWindowSetVisual (win, visual, depth);
}

void
xfwmWindowSetBG (xfwmWindow * win, xfwmPixmap * pix)
{
    if ((win->width < 1) || (win->height < 1) || (pix->width < 1) || (pix->height < 1))
    {
        return;
    }

    if ((win->visual == win->screen_info->visual) && (win->depth == win->screen_info->depth))
    {
        XSetWindowBackgroundPixmap (myScreenGetXDisplay (win->screen_info), win->window, pix->pixmap);
    }
#ifdef HAVE_RENDER
    else if (myDisplayHaveRender (win->screen_info->display_info))
    {
        Picture psrc, pdst;
        XRenderPictFormat *format_src, *format_dst;
        Pixmap temp;

        format_src = XRenderFindVisualFormat (myScreenGetXDisplay (pix->screen_info), 
                                              pix->screen_info->visual);
        format_dst = XRenderFindVisualFormat (myScreenGetXDisplay (win->screen_info), 
                                              win->visual);

        if (!format_src || !format_dst)
        {
            TRACE ("xfwmWindowSetBG: Cannot get XRender picture format");
            return;
        }

        temp = XCreatePixmap (myScreenGetXDisplay (win->screen_info), 
                              win->window, 
                              pix->width, pix->height, win->depth);

        psrc = XRenderCreatePicture (myScreenGetXDisplay (pix->screen_info), 
                                     pix->pixmap, format_src, 0, NULL);
        pdst = XRenderCreatePicture (myScreenGetXDisplay (win->screen_info), 
                                     temp, format_dst, 0, NULL);

        XRenderComposite (myScreenGetXDisplay (win->screen_info), PictOpSrc, psrc, None, pdst, 0, 0, 0, 0, 0, 0, pix->width, pix->height);

        XRenderFreePicture (myScreenGetXDisplay (pix->screen_info), psrc);
        XRenderFreePicture (myScreenGetXDisplay (win->screen_info), pdst);

        XSetWindowBackgroundPixmap (myScreenGetXDisplay (win->screen_info), win->window, temp);

        XFreePixmap (myScreenGetXDisplay (win->screen_info), temp);
    }
#endif
}
