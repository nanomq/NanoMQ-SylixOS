/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MEMORY OPERATION TRACER
 * this file is used to track memory operations, to moniting memory leakage, memory corruption
 *
 * Author: Zeng.Bo <sylixos@gmail.com>
 */

#ifndef __MTRACER_H
#define __MTRACER_H

#ifdef __cplusplus
extern "C" {
#endif

void mtracer_config(int backtrace_malloc_cnt, unsigned long backtrace_mem_addr, int log_fd);
int  mtracer_show(void);

#ifdef __cplusplus
}
#endif

#endif /* __MTRACER_H */
/*
 * end
 */
