///BEGIN 4
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <strings.h>

#include "mkimg.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

// 2 MB
#define FILE_SIZE (2*(1 << 20))

extern char *optarg;
extern int optind;



void
set_toc_pos (FILE *fp, u_int entryno)
{
  int ret;
  ret = fseek (fp, sizeof (struct toc_entry) * entryno, SEEK_SET);
  if (ret < 0) {
    perror ("fseek");
    exit (1);
  }
}

void
write_toc (FILE *fp, u_int entryno, char *name, u_int off, u_int len)
{
  int ret;
  struct toc_entry ent;
  bzero (&ent, sizeof (ent));
  bcopy (name, ent.name, strlen (name));
  ent.length = htonl(len);
  ent.offset = htonl(off);

  set_toc_pos (fp, entryno);
  if ((ret = fwrite (&ent, sizeof (ent), 1, fp)) != 1) {
    fprintf (stderr, "ret = %d\n", ret);
    perror ("write_toc: fwrite");
    exit (1);
  }
}


void
read_toc (FILE *fp, u_int entryno, struct toc_entry *ent)
{
  int ret;
  set_toc_pos (fp, entryno);
  if ((ret = fread (ent, sizeof (*ent), 1, fp)) != 1) {
    fprintf (stderr, "ret = %d\n", ret);
    perror ("read_toc: fread");
    exit (1);
  }
  ent->length = ntohl(ent->length);
  ent->offset = ntohl(ent->offset);
}


void
create(char *imgname)
{
  FILE *fp;
  int i;
  
  if (!(fp = fopen (imgname, "w"))) {
    perror ("fopen");
    exit (1);
  }
  
  // create a blank disk img
  for (i = 0; i < FILE_SIZE; i++) {
    char c = '\000';
    if (fwrite (&c, 1, 1, fp) != 1) {
      perror ("fwrite");
      exit (1);
    }
  }
  fclose (fp);
}


void
add(char *imgname, char *binname)
{
  char binfn[500];
  FILE *fp, *bfp;
  // add 'binary' to the existing disk iamge
  u_int i, nextfree, nblocks, nbytes;
  
  if (!(fp = fopen (imgname, "r+"))) {
    perror ("fopen");
    exit (1);
  }
  
  i = snprintf (binfn, sizeof (binfn), "%s/%s", binname, binname);
  assert (i < sizeof (binfn));
  
  if (!(bfp = fopen (binfn, "r"))) {
    perror ("fopen");
    exit (1);
  }
  
  // run through the TOC
  nextfree = 1;
  for (i = 0; i < 4096 / sizeof (struct toc_entry); i++) {
    struct toc_entry ent;
    read_toc (fp, i, &ent);
    // detect last TOC entry
    if (ent.offset == 0)
      break;
    nextfree = ent.offset + ent.length;
  }
  
  // seek disk img to free data
  if (fseek (fp, nextfree * 4096, 0) < 0) {
    perror ("fseek");
    exit (1);
  }
  
  // append the binary to the disk img
  nbytes = 0;
  while (1) {
    char c;
    int nread;
    nread = fread (&c, 1, 1, bfp);
    if (nread < 0) {
      perror ("main: fread");
      exit (1);
    } else if (nread == 0)
      break; // eof
    
    if (fwrite (&c, 1, 1, fp) != 1) {
      perror ("fwrite");
      exit (1);
    }
    nbytes += 1;
  }
  
  nblocks = (nbytes + 4095)/4096;
  write_toc (fp, i, binname, nextfree, nblocks);

  fclose (fp);
  fclose (bfp);
}



static void
usage (void)
{
  fprintf (stderr, "usage: mkimg <disk image> [-c] [-a <binary>]\n");
  exit (1);
}

int
main (int argc, char **argv)
{
  char *imgname = NULL;
  int fl_create = 0;
  int fl_add = 0;
  int opt;


  setvbuf(stdout, NULL, _IOFBF,0); 

  if (argc <= 1)
    usage ();

  imgname = argv[1];
  argc -= 1;
  argv += 1;

  while ((opt = getopt (argc, argv, "ca")) != -1) {
    switch (opt) {
    case 'c':
      fl_create = 1;
      break;
    case 'a':
      fl_add = 1;
      break;
    default:
      usage ();
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (!fl_create && !fl_add)
    usage ();

  if (fl_create)
    create (imgname);
  else
    for ( ; argc > 0; argc--, argv++) 
      add (imgname, argv[0]);

  return (0);
}
///END
