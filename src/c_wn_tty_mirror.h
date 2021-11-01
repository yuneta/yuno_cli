/****************************************************************************
 *          C_WN_TTY_MIRROR.H
 *          Wn_tty_mirror GClass.
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <yuneta.h>
#include "c_wn_stdscr.h"

/**rst**

.. _wn_tty_mirror-gclass:

**"Wn_tty_mirror"** :ref:`GClass`
===========================

Description
===========

UI List

Events
======

Input Events
------------

Order
^^^^^

Request
^^^^^^^

Output Events
-------------

Response
^^^^^^^^

Unsolicited
^^^^^^^^^^^

Macros
======

``GCLASS_WN_TTY_MIRROR_NAME``
   Macro of the gclass string name, i.e **"Wn_tty_mirror"**.

``GCLASS_WN_TTY_MIRROR``
   Macro of the :func:`gclass_wn_tty_mirror()` function.


**rst**/

#ifdef __cplusplus
extern "C"{
#endif

/**rst**
   Return a pointer to the :ref:`GCLASS` struct defining the :ref:`wn_tty_mirror-gclass`.
**rst**/
PUBLIC GCLASS *gclass_wn_tty_mirror(void);

#define GCLASS_WN_TTY_MIRROR_NAME "Wn_tty_mirror"
#define GCLASS_WN_TTY_MIRROR gclass_wn_tty_mirror()


#ifdef __cplusplus
}
#endif
