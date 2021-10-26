/****************************************************************************
 *          C_WN_TTY.H
 *          Wn_tty GClass.
 *
 *          Copyright (c) 2021 Niyamaka.
 *          All Rights Reserved.
 ****************************************************************************/
#pragma once

#include <yuneta.h>
#include "c_wn_stdscr.h"

/**rst**

.. _wn_tty-gclass:

**"Wn_tty"** :ref:`GClass`
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

``GCLASS_WN_TTY_NAME``
   Macro of the gclass string name, i.e **"Wn_tty"**.

``GCLASS_WN_TTY``
   Macro of the :func:`gclass_wn_tty()` function.


**rst**/

#ifdef __cplusplus
extern "C"{
#endif

/**rst**
   Return a pointer to the :ref:`GCLASS` struct defining the :ref:`wn_tty-gclass`.
**rst**/
PUBLIC GCLASS *gclass_wn_tty(void);

#define GCLASS_WN_TTY_NAME "Wn_tty"
#define GCLASS_WN_TTY gclass_wn_tty()


#ifdef __cplusplus
}
#endif
