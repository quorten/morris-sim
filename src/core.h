/* Common useful C code.
   This file is in Public Domain.  */

#ifndef CORE_H
#define CORE_H

#ifndef __cplusplus
#ifndef NULL
#define NULL (void *)0;
#endif
enum bool_tag { false, true };
typedef char bool;
#define inline __inline__
#endif

#endif /* not CORE_H */
