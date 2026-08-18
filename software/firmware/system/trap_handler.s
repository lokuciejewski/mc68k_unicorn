.global trap15_entry

.extern Trap15_Handler

trap15_entry:   /* Push C arguments onto stack in reverse order */
    move.l  %d3,-(%sp)                        /* arg3 */
    move.l  %d2,-(%sp)                        /* arg2 */
    move.l  %d1,-(%sp)                        /* arg1 */
    move.l  %d0,-(%sp)                        /* sys_id */

    jsr Trap15_Handler

    lea (16,%sp),%sp                      /* Clean up 16 bytes from stack */
    rte
