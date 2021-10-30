/***********************************************************************
 *          C_WN_TTY_MIRROR.C
 *          Wn_tty_mirror GClass.
 *
 *          Tty (console)
 *
 *          https://stackoverflow.com/questions/65282503/ncurses-newterm-following-openpty
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
***********************************************************************/
#include <string.h>
#include <pty.h>
#include <unistd.h>
#include <ncurses/ncurses.h>
#include <ncurses/panel.h>
#include "c_wn_tty_mirror.h"

/***************************************************************************
 *              Constants
 ***************************************************************************/

/***************************************************************************
 *              Structures
 ***************************************************************************/

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
PRIVATE FILE *open_tty(char *path);
PRIVATE int clrscr(hgobj gobj);


/***************************************************************************
 *          Data: config, public data, private data
 ***************************************************************************/

/*---------------------------------------------*
 *      Attributes - order affect to oid's
 *---------------------------------------------*/
PRIVATE sdata_desc_t tattr_desc[] = {
/*-ATTR-type------------name----------------flag--------default-----description---------- */
SDATA (ASN_OCTET_STR,   "name",             0,          "",     "color"),

SDATA (ASN_OCTET_STR,   "layout_type",      0,          0,      "Layout inherit from parent"),
SDATA (ASN_INTEGER,     "w",                0,          0,      "logical witdh window size"),
SDATA (ASN_INTEGER,     "h",                0,          0,      "logical height window size"),
SDATA (ASN_INTEGER,     "x",                0,          0,      "x window coord"),
SDATA (ASN_INTEGER,     "y",                0,          0,      "y window coord"),
SDATA (ASN_INTEGER,     "cx",               0,          80,     "physical witdh window size"),
SDATA (ASN_INTEGER,     "cy",               0,          24,     "physical height window size"),
SDATA (ASN_INTEGER,     "scroll_size",      0,          1000000,"scroll size. 0 is unlimited (until out of memory)"),
SDATA (ASN_OCTET_STR,   "bg_color",         0,          "blue", "Background color"),
SDATA (ASN_OCTET_STR,   "fg_color",         0,          "white","Foreground color"),
SDATA (ASN_POINTER,     "user_data",        0,          0,      "user data"),
SDATA (ASN_POINTER,     "user_data2",       0,          0,      "more user data"),
SDATA (ASN_POINTER,     "subscriber",       0,          0,      "subscriber of output-events. If it's null then subscriber is the parent."),
SDATA_END()
};

/*---------------------------------------------*
 *      GClass trace levels
 *---------------------------------------------*/
enum {
    TRACE_MESSAGES = 0x0001,
};
PRIVATE const trace_level_t s_user_trace_level[16] = {
{"messages",        "Trace messages"},
{0, 0},
};


/*---------------------------------------------*
 *              Private data
 *---------------------------------------------*/
typedef struct _PRIVATE_DATA {
    const char *layout_type;        // inherit from parent glayout, don't touch.
    const char *fg_color;
    const char *bg_color;
    int32_t scroll_size;

    SCREEN *screen;     /* this screen - curses internal data */
    FILE *input;
    FILE *output;

    WINDOW *wn;     // ncurses window handler
    PANEL *panel;   // panel handler

    hgobj gobj_tty;

    int32_t cx;
    int32_t cy;
    int32_t base;
} PRIVATE_DATA;




            /******************************
             *      Framework Methods
             ******************************/




/***************************************************************************
 *      Framework Method create
 ***************************************************************************/
PRIVATE void mt_create(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*
     *  CHILD subscription model
     */
    hgobj subscriber = (hgobj)gobj_read_pointer_attr(gobj, "subscriber");
    if(!subscriber)
        subscriber = gobj_parent(gobj);
    gobj_subscribe_event(gobj, NULL, NULL, subscriber);

    /*
     *  Do copy of heavy used parameters, for quick access.
     *  HACK The writable attributes must be repeated in mt_writing method.
     */
    SET_PRIV(layout_type,               gobj_read_str_attr)
    SET_PRIV(fg_color,                  gobj_read_str_attr)
    SET_PRIV(bg_color,                  gobj_read_str_attr)
    SET_PRIV(cx,                        gobj_read_int32_attr)
    SET_PRIV(cy,                        gobj_read_int32_attr)
    SET_PRIV(scroll_size,               gobj_read_int32_attr)

    int x = gobj_read_int32_attr(gobj, "x");
    int y = gobj_read_int32_attr(gobj, "y");
    int cx = gobj_read_int32_attr(gobj, "cx");
    int cy = gobj_read_int32_attr(gobj, "cy");

    if(1) {
        priv->wn = newwin(cy, cx, y, x);
        if(!priv->wn) {
            log_error(0,
                "gobj",         "%s", gobj_full_name(gobj),
                "function",     "%s", __FUNCTION__,
                "msgset",       "%s", MSGSET_SYSTEM_ERROR,
                "msg",          "%s", "newwin() FAILED",
                NULL
            );
        }
        scrollok(priv->wn, true);
    }
}

/***************************************************************************
 *      Framework Method writing
 ***************************************************************************/
PRIVATE void mt_writing(hgobj gobj, const char *path)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*
     *
     */
    IF_EQ_SET_PRIV(bg_color,                gobj_read_str_attr)
    ELIF_EQ_SET_PRIV(fg_color,              gobj_read_str_attr)
    ELIF_EQ_SET_PRIV(cx,                    gobj_read_int32_attr)
    ELIF_EQ_SET_PRIV(cy,                    gobj_read_int32_attr)
    ELIF_EQ_SET_PRIV(scroll_size,           gobj_read_int32_attr)
    END_EQ_SET_PRIV()
}

/***************************************************************************
 *      Framework Method start
 ***************************************************************************/
PRIVATE int mt_start(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    /*
     *  Create pseudoterminal
     */
    if(0) {
        json_t *kw_pty = json_pack("{s:s, s:b}",
            "process", "screen",
            "no_output", 0
            // TODO get and pass rows,cols
        );
        priv->gobj_tty = gobj_create(gobj_name(gobj), GCLASS_PTY, kw_pty, gobj);
        if(priv->gobj_tty) {
            gobj_set_volatil(priv->gobj_tty, TRUE);
        }
        gobj_start(priv->gobj_tty);

    }

    if(0) {
        priv->input = priv->output = open_tty("/");

        cbreak();
        noecho();
        scrollok(stdscr, TRUE);
        box(stdscr, 0, 0);

        int x = gobj_read_int32_attr(gobj, "x");
        int y = gobj_read_int32_attr(gobj, "y");
        int cx = gobj_read_int32_attr(gobj, "cx");
        int cy = gobj_read_int32_attr(gobj, "cy");
        priv->wn = newwin(cy, cx, y, x);
    }


    return 0;
}

/***************************************************************************
 *      Framework Method stop
 ***************************************************************************/
PRIVATE int mt_stop(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->gobj_tty) {
        if(!gobj_is_destroying(priv->gobj_tty)) {
            gobj_stop(priv->gobj_tty);
        }
    }

    if(priv->wn) {
        delwin(priv->wn);
        priv->wn = 0;
    }
    if(priv->screen) {
        endwin();
        priv->screen = 0;
    }

    return 0;
}

/***************************************************************************
 *      Framework Method destroy
 ***************************************************************************/
PRIVATE void mt_destroy(hgobj gobj)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);
    if(priv->panel) {
        del_panel(priv->panel);
        priv->panel = 0;
        update_panels();
        doupdate();
    }
    if(priv->wn) {
        delwin(priv->wn);
        priv->wn = 0;
    }

    clrscr(gobj);
}




            /***************************
             *      Local Methods
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE FILE *open_tty(char *path)
{
    FILE *fp;
    int amaster;
    int aslave;
    char slave_name[1024];
    char s_option[sizeof(slave_name) + 80];
    const char *xterm_prog = 0;

    if ((xterm_prog = getenv("XTERM_PROG")) == 0)
        xterm_prog = "xterm";

    if (openpty(&amaster, &aslave, slave_name, 0, 0) != 0
        || strlen(slave_name) > sizeof(slave_name) - 1) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "openpty() FAILED",
            NULL
        );
    }
    if (strrchr(slave_name, '/') == 0) {
        errno = EISDIR;
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "slave_name FAILED",
            NULL
        );
    }
    snprintf(s_option, sizeof(s_option),
                "-S%s/%d", slave_name, aslave);
    if (fork()) {
        execlp(xterm_prog, xterm_prog, s_option, "-title", path, (char *) 0);
        _exit(0);
    }
    fp = fdopen(amaster, "r+");
    if (fp == 0) {
        log_error(0,
            "gobj",         "%s", __FILE__,
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "fdopen() FAILED",
            NULL
        );
    }
    //assert(fp != 0);
    return fp;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int clrscr(hgobj gobj)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    return 0;
}




            /***************************
             *      Actions
             ***************************/




/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_paint(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(!priv->wn) {
        // Debugging in kdevelop or batch mode has no wn
        KW_DECREF(kw);
        return 0;
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_keychar(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    const char *name = gobj_read_str_attr(gobj, "name");
    hgobj gobj_cmd = gobj_read_pointer_attr(gobj, "user_data");
    GBUFFER *gbuf = (GBUFFER *)(size_t)kw_get_int(kw, "gbuffer", 0, KW_REQUIRED);
    GBUF_INCREF(gbuf);
    GBUFFER *gbuf_content64 = gbuf_encodebase64(gbuf);
    char *content64 = gbuf_cur_rd_pointer(gbuf_content64);

    json_t *kw_command = json_pack("{s:s, s:s}",
        "name", name,
        "content64", content64
    );

    json_decref(gobj_command(gobj_cmd, "write-tty", kw_command, gobj));

    GBUF_DECREF(gbuf_content64);
    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_write_tty(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(src == priv->gobj_tty) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "What fck?",
            NULL
        );
        KW_DECREF(kw);
        return 0;
    }

    const char *content64 = kw_get_str(kw, "content64", 0, 0);
    if(empty_string(content64)) {
        log_error(0,
            "gobj",         "%s", gobj_full_name(gobj),
            "function",     "%s", __FUNCTION__,
            "msgset",       "%s", MSGSET_INTERNAL_ERROR,
            "msg",          "%s", "content64 empty",
            NULL
        );
        JSON_DECREF(kw);
        return -1;
    }

    GBUFFER *gbuf = gbuf_decodebase64string(content64);
    char *p = gbuf_cur_rd_pointer(gbuf);
    int len = gbuf_leftbytes(gbuf);

    if(gobj_trace_level(gobj) & TRACE_MESSAGES) {
        log_debug_dump(0, p, len,  "write_tty");
    }

    if(priv->output) {
        fwrite(p, len, 1, priv->output);
    } else

    if(priv->wn) {
        while(len-- > 0) {
            waddch(priv->wn, *p);
            wnoutrefresh(priv->wn);
        }
        doupdate();
    }

    if(priv->gobj_tty) {
        GBUF_INCREF(gbuf);
        json_t *kw_tty = json_pack("{s:I}",
            "gbuffer", (json_int_t)(size_t)gbuf
        );
        gobj_send_event(priv->gobj_tty, "EV_WRITE_TTY", kw_tty, gobj);
    }

    GBUF_DECREF(gbuf);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_scroll_line_up(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);


    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_scroll_line_down(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);


    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_scroll_page_up(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_scroll_page_down(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_scroll_top(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_scroll_bottom(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_clrscr(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
//     PRIVATE_DATA *priv = gobj_priv_data(gobj);

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_move(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    int x = kw_get_int(kw, "x", 0, KW_REQUIRED);
    int y = kw_get_int(kw, "y", 0, KW_REQUIRED);
    gobj_write_int32_attr(gobj, "x", x);
    gobj_write_int32_attr(gobj, "y", y);

    if(priv->panel) {
        //log_debug_printf(0, "move panel x %d y %d %s", x, y, gobj_name(gobj));
        move_panel(priv->panel, y, x);
        update_panels();
        doupdate();
    } else if(priv->wn) {
        //log_debug_printf(0, "move window x %d y %d %s", x, y, gobj_name(gobj));
        mvwin(priv->wn, y, x);
        wrefresh(priv->wn);
    }

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_size(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    int cx = kw_get_int(kw, "cx", 0, KW_REQUIRED);
    int cy = kw_get_int(kw, "cy", 0, KW_REQUIRED);
    gobj_write_int32_attr(gobj, "cx", cx);
    gobj_write_int32_attr(gobj, "cy", cy);

    if(priv->panel) {
        //log_debug_printf(0, "size panel cx %d cy %d %s", cx, cy, gobj_name(gobj));
        wresize(priv->wn, cy, cx);
        update_panels();
        doupdate();
    } else if(priv->wn) {
        //log_debug_printf(0, "size window cx %d cy %d %s", cx, cy, gobj_name(gobj));
        wresize(priv->wn, cy, cx);
        wrefresh(priv->wn);
    }
    gobj_send_event(gobj, "EV_PAINT", 0, gobj);  // repaint, ncurses doesn't do it

    KW_DECREF(kw);
    return 0;
}

/***************************************************************************
 *
 ***************************************************************************/
PRIVATE int ac_top(hgobj gobj, const char *event, json_t *kw, hgobj src)
{
    PRIVATE_DATA *priv = gobj_priv_data(gobj);

    if(priv->panel) {
        top_panel(priv->panel);
        update_panels(); // TODO is necessary this?
        doupdate();
    }

    KW_DECREF(kw);
    return 0;
}


/***************************************************************************
 *                          FSM
 ***************************************************************************/
PRIVATE const EVENT input_events[] = {
    {"EV_KEYCHAR",         0,  0,  0},
    {"EV_WRITE_TTY",        0,  0,  0},
    {"EV_PAINT",            0,  0,  0},
    {"EV_MOVE",             0,  0,  0},
    {"EV_SIZE",             0,  0,  0},
    {"EV_SET_TOP_WINDOW",   0,  0,  0},
    {"EV_SCROLL_LINE_UP",   0,  0,  0},
    {"EV_SCROLL_LINE_DOWN", 0,  0,  0},
    {"EV_SCROLL_PAGE_UP",   0,  0,  0},
    {"EV_SCROLL_PAGE_DOWN", 0,  0,  0},
    {"EV_SCROLL_TOP",       0,  0,  0},
    {"EV_SCROLL_BOTTOM",    0,  0,  0},
    {"EV_CLRSCR",           0,  0,  0},
    {NULL, 0, 0, 0}
};
PRIVATE const EVENT output_events[] = {
    {NULL, 0, 0, 0}
};
PRIVATE const char *state_names[] = {
    "ST_IDLE",
    "ST_DISABLED",
    NULL
};

PRIVATE EV_ACTION ST_IDLE[] = {
    {"EV_KEYCHAR",          ac_keychar,             0},
    {"EV_WRITE_TTY",        ac_write_tty,           0},
    {"EV_MOVE",             ac_move,                0},
    {"EV_SIZE",             ac_size,                0},
    {"EV_PAINT",            ac_paint,               0},
    {"EV_SET_TOP_WINDOW",   ac_top,                 0},
    {"EV_SCROLL_LINE_UP",   ac_scroll_line_up,      0},
    {"EV_SCROLL_LINE_DOWN", ac_scroll_line_down,    0},
    {"EV_SCROLL_PAGE_UP",   ac_scroll_page_up,      0},
    {"EV_SCROLL_PAGE_DOWN", ac_scroll_page_down,    0},
    {"EV_SCROLL_TOP",       ac_scroll_top,          0},
    {"EV_SCROLL_BOTTOM",    ac_scroll_bottom,       0},
    {"EV_CLRSCR",           ac_clrscr,              0},
    {0,0,0}
};

PRIVATE EV_ACTION ST_DISABLED[] = {
    {0,0,0}
};

PRIVATE EV_ACTION *states[] = {
    ST_IDLE,
    ST_DISABLED,
    NULL
};

PRIVATE FSM fsm = {
    input_events,
    output_events,
    state_names,
    states,
};

/***************************************************************************
 *              GClass
 ***************************************************************************/
/*---------------------------------------------*
 *              Local methods table
 *---------------------------------------------*/
PRIVATE LMETHOD lmt[] = {
    {0, 0, 0}
};

/*---------------------------------------------*
 *              GClass
 *---------------------------------------------*/
PRIVATE GCLASS _gclass = {
    0,  // base
    GCLASS_WN_TTY_MIRROR_NAME,
    &fsm,
    {
        mt_create,
        0, //mt_create2,
        mt_destroy,
        mt_start,
        mt_stop,
        0, //mt_play,
        0, //mt_pause,
        mt_writing,
        0, //mt_reading,
        0, //mt_subscription_added,
        0, //mt_subscription_deleted,
        0, //mt_child_added,
        0, //mt_child_removed,
        0, //mt_stats,
        0, //mt_command,
        0, //mt_inject_event,
        0, //mt_create_resource,
        0, //mt_list_resource,
        0, //mt_update_resource,
        0, //mt_delete_resource,
        0, //mt_add_child_resource_link
        0, //mt_delete_child_resource_link
        0, //mt_get_resource
        0, //mt_authorization_parser,
        0, //mt_authenticate,
        0, //mt_list_childs,
        0, //mt_stats_updated,
        0, //mt_disable,
        0, //mt_enable,
        0, //mt_trace_on,
        0, //mt_trace_off,
        0, //mt_gobj_created,
        0, //mt_future33,
        0, //mt_future34,
        0, //mt_publish_event,
        0, //mt_publication_pre_filter,
        0, //mt_publication_filter,
        0, //mt_authz_checker,
        0, //mt_future39,
        0, //mt_create_node,
        0, //mt_update_node,
        0, //mt_delete_node,
        0, //mt_link_nodes,
        0, //mt_future44,
        0, //mt_unlink_nodes,
        0, //mt_topic_jtree,
        0, //mt_get_node,
        0, //mt_list_nodes,
        0, //mt_shoot_snap,
        0, //mt_activate_snap,
        0, //mt_list_snaps,
        0, //mt_treedbs,
        0, //mt_treedb_topics,
        0, //mt_topic_desc,
        0, //mt_topic_links,
        0, //mt_topic_hooks,
        0, //mt_node_parents,
        0, //mt_node_childs,
        0, //mt_list_instances,
        0, //mt_node_tree,
        0, //mt_topic_size,
        0, //mt_future62,
        0, //mt_future63,
        0, //mt_future64
    },
    lmt,
    tattr_desc,
    sizeof(PRIVATE_DATA),
    0,  // acl
    s_user_trace_level,
    0, // cmds
    0, // gcflag
};

/***************************************************************************
 *              Public access
 ***************************************************************************/
PUBLIC GCLASS *gclass_wn_tty_mirror(void)
{
    return &_gclass;
}
