/**
 * library to read flattened device tree blobs
 * adapted from https://github.com/rems-project/system-litmus-harness/blob/master/lib/arch/device.c
 */

#include <macros.h>
#include <types.h>
#include <string.h>
#include <bcm2837/uart1.h>

/* there is a fancy trick you can do on Arm cores to swap endianness
  * but this works just fine ... */
u64 read_be64(char *p) {
  u8* pc = (u8*)p;

  u8 bs[] = {
    *(pc+7),
    *(pc+6),
    *(pc+5),
    *(pc+4),
    *(pc+3),
    *(pc+2),
    *(pc+1),
    *(pc+0)
  };

  u64 x = 0;
  for (int i = 7; i >= 0; i--) {
    x <<= 8;
    x += bs[i];
  }
  return x;
}

u32 read_be(u32 *p) {
  u8* pc = (u8*)p;
  u8 b0 = *pc;
  u8 b1 = *(pc+1);
  u8 b2 = *(pc+2);
  u8 b3 = *(pc+3);
  return ((u32)b0<<24) + ((u32)b1<<16) + ((u32)b2<<8) + (u32)b3;
}

#define FDT_BEGIN_NODE (0x00000001)
#define FDT_END_NODE (0x00000002)
#define FDT_PROP (0x00000003)
#define FDT_NOP (0x00000004)
#define FDT_END (0x00000009)

#define FDT_ALIGN(x) ((((u64)x) + 3) & (~3))

typedef struct  {
    u32 fdt_magic;
    u32 fdt_totalsize;
    u32 fdt_off_dt_struct;
    u32 fdt_off_dt_strings;
    u32 fdt_off_mem_rsvmap;
    u32 fdt_version;
    u32 fdt_last_comp_version;
    u32 fdt_boot_cpuid_phys;
    u32 fdt_size_dt_strings;
    u32 fdt_size_dt_struct;
} fdt_header;

typedef struct {
    u32 token;
    u32 nameoff;
} fdt_structure_piece_with_name;

typedef struct {
    char* current;
    u32 token;
    char* next;
} fdt_structure_piece;

typedef struct {
    u32 token;
    u32 len;
    u32 nameoff;
    char data[];
} fdt_structure_property_header;

typedef struct {
    u32 token;
    char name[];
} fdt_structure_begin_node_header;

char* fdt_read_str(char* fdt, u32 nameoff) {
    fdt_header* hd = (fdt_header*)fdt;
    char* name_start = fdt + read_be(&hd->fdt_off_dt_strings) + nameoff;
    return name_start;
}

fdt_structure_piece fdt_read_piece(char* p) {
  u32 token = read_be((u32*)p);
  fdt_structure_piece piece;
  piece.current = p;
  piece.token = token;
  char* next;
  char* ps;
  fdt_structure_property_header prop;
  switch (token) {
    case FDT_BEGIN_NODE:
      /* skip name */
      ps = p + sizeof(fdt_structure_begin_node_header);
      while (*ps != '\0')
          ps++;
      next = ps + 1;
      break;
    case FDT_END_NODE:
      next = p + 4;
      break;
    case FDT_PROP: {
        fdt_structure_property_header *hd = (fdt_structure_property_header*)p;
        prop.token = read_be(&hd->token);
        prop.len = read_be(&hd->len);
        prop.nameoff = read_be(&hd->nameoff);
        next = p + sizeof(fdt_structure_property_header) + prop.len;
        break;
    }
    case FDT_END:
      piece.next = NULL;
      return piece;
    default:
      piece.next = NULL;
      return piece;
  }
  piece.next = (char*)FDT_ALIGN((u64)next);
  return piece;
}

void dtb_print_bootargs(void *dtb) {
  fdt_header* hd = (fdt_header*)dtb;
  char *p = (char*)((u64)hd + read_be(&hd->fdt_off_dt_struct));
  char* current_node;
  while (p) {
    fdt_structure_piece piece = fdt_read_piece(p);
    switch (piece.token) {
      case FDT_BEGIN_NODE:
        current_node = (char*)&((fdt_structure_begin_node_header*)p)->name;
        break;
      case FDT_END_NODE:
        break;
      case FDT_PROP: {
          u32 nameoff = read_be(&((fdt_structure_property_header*)p)->nameoff);
          char* name = fdt_read_str(dtb, nameoff);
          if (strcmp(current_node, "chosen") == 0 && strcmp(name, "bootargs") == 0) {
            char* bootargs = (char*)&((fdt_structure_property_header*)p)->data;
            uart1_printf("cmdline = '%s'\n", bootargs);
            return;
          }
          break;
      }
      case FDT_END:
        return;
      default:
        return;
    }

    p = piece.next;
  }
}