///LAB4
#ifndef __MKIMG_H__
#define __MKIMG_H__

struct toc_entry {
  char name[24];
  u_int offset;  // in 4KB blocks
  u_int length;  // in 4KB blocks
};

#endif /* __MKIMG_H__ */
///END
