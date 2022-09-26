
#ifndef __LINUX_VERSION_H
#define __LINUX_VERSION_H

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c)   (((a) << 16) + ((b) << 8) + (c))
#endif

#define LINUX_VERSION_CODE      KERNEL_VERSION(4, 4, 4)

#endif  /* __LINUX_VERSION_H */
