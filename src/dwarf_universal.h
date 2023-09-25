

#ifndef Dwarf_Unsigned
#define Dwarf_Unsigned unsigned long long
#endif
#ifndef Dwarf_Signed
#define Dwarf_Signed unsigned long long
#endif
#ifndef Dwarf_Small
#define Dwarf_Small unsigned char
#endif

struct Dwarf_Universal_Head_s;
typedef struct Dwarf_Universal_Head_s *  Dwarf_Universal_Head;

int dwarf_object_detector_universal_head(
    char         *dw_path,
    Dwarf_Unsigned      dw_filesize,
    Dwarf_Unsigned     *dw_contentcount,
    Dwarf_Universal_Head * dw_head,
    int            *errcode);

int dwarf_object_detector_universal_instance(
    Dwarf_Universal_Head dw_head,
    Dwarf_Unsigned  dw_index_of,
    Dwarf_Unsigned *dw_cpu_type,
    Dwarf_Unsigned *dw_cpu_subtype,
    Dwarf_Unsigned *dw_offset,
    Dwarf_Unsigned *dw_size,
    Dwarf_Unsigned *dw_align,
    int         *errcode);
void dwarf_dealloc_universal_head(Dwarf_Universal_Head dw_head);
